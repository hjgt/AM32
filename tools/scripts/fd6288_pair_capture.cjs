// S2D read-only capture. No write-memory, arm, halt, reset or flash commands.
const fs = require('node:fs'), path = require('node:path'), assert = require('node:assert/strict');
const crypto = require('node:crypto');
const {spawnSync} = require('node:child_process');
const {parseHex, symbols, connect, decodeBench} = require('./fd6288_startup_capture.cjs');
const SIZE = 18624, BANK = 9280, NONE = 0xffffffff, MAGIC = 0x46535031;
const reasons = ['invalid', 'blank', 'pre', 'hysteresis', 'early', 'unarmed', 'confirmation', 'accepted', 'reported', 'coast'];
const states = ['empty', 'active', 'aborted', 'failure', 'success'];
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
function decode(b) {
  assert.equal(b.length, SIZE, 'Wrong paired trace size');
  const u32 = a => b.readUInt32LE(a), u16 = a => b.readUInt16LE(a), i16 = a => b.readInt16LE(a);
  const names = ['magic', 'abi', 'variant', 'clock_hz', 'size', 'command', 'active_bank', 'failure_bank', 'success_bank', 'trials_seen', 'rejected_commands', 'max_finish_cycles'];
  const header = Object.fromEntries(names.map((n, i) => [n, u32(i * 4)]));
  assert(header.magic === MAGIC && header.abi === 1 && header.variant === 6 && header.clock_hz === 160000000 && header.size === SIZE, 'Wrong paired ABI');
  assert.equal(header.active_bank, NONE, 'Pair capture active');
  assert([NONE, 0, 1].includes(header.failure_bank) && [NONE, 0, 1].includes(header.success_bank));
  assert(header.failure_bank === NONE || header.failure_bank !== header.success_bank, 'Same bank pinned twice');
  const banks = [];
  for (let k = 0; k < 2; k++) {
    const base = 64 + k * BANK;
    const names = ['state', 'trial_id', 'started', 'ended', 'reason', 'final_stage', 'sectors', 'overflow', 'handoff_at', 'accepted_total', 'closed_zc', 'timeouts', 'max_sample_cycles', 'max_event_cycles', 'max_boundary_cycles', 'freeze_reason'];
    const h = Object.fromEntries(names.map((n, i) => [n, u32(base + i * 4)]));
    assert([0, 2, 3, 4].includes(h.state), 'Active or invalid bank');
    if (!h.state) { assert(k !== header.failure_bank && k !== header.success_bank); continue; }
    assert(h.trial_id > 0 && h.trial_id <= header.trials_seen && h.sectors <= 72 && h.final_stage <= 4);
    if (k === header.failure_bank) assert.equal(h.state, 3);
    if (k === header.success_bank || h.state === 4) {
      assert(h.state === 4 && h.reason === 1 && h.final_stage === 4 && h.handoff_at !== NONE);
      assert(h.ended >= h.handoff_at + 160000000, 'Incomplete successful observation');
    }
    const records = [];
    for (let n = 0; n < h.sectors; n++) {
      const o = base + 64 + n * 128;
      const r = {index: n};
      ['began', 'ended', 'last_sample', 'first_pre', 'first_post', 'forward_at', 'reverse_at', 'accepted_at'].forEach((f, i) => {const v = u32(o + i * 4); r[`${f}_us`] = v === NONE ? null : v / 160;});
      for (const [i, f] of ['first_adc', 'last_adc', 'min_adc', 'max_adc'].entries()) r[f] = [0, 1, 2].map(j => u16(o + 32 + i * 6 + j * 2));
      ['directed_min', 'directed_max', 'directed_first', 'directed_last'].forEach((f, i) => r[f] = i16(o + 56 + i * 2));
      r.reasons = Object.fromEntries(reasons.map((f, i) => [f, u16(o + 64 + i * 2)]));
      ['samples', 'no_fresh', 'forward_crossings', 'reverse_crossings', 'duty_first', 'duty_last', 'pwm_first', 'pwm_last', 'trigger_first', 'trigger_last', 'input_first', 'input_last'].forEach((f, i) => r[f] = u16(o + 84 + i * 2));
      r.current_max_A = i16(o + 108) / 100; r.voltage_min_V = u16(o + 110) / 100;
      r.commanded_us = u16(o + 112);
      ['step', 'flags', 'stage', 'decision', 'sector_percent', 'confidence'].forEach((f, i) => r[f] = b[o + 114 + i]);
      r.valid_samples = u16(o + 120); r.issues = u16(o + 122);
      assert(r.step >= 1 && r.step <= 6 && r.stage >= 1 && r.stage <= 3 && r.confidence <= 6);
      assert(r.decision === 255 || r.decision <= 6);
      assert(r.sector_percent === 255 || r.sector_percent <= 100);
      assert(r.ended_us !== null && r.began_us !== null && r.ended_us >= r.began_us && r.ended_us <= h.ended / 160);
      if (n) assert.equal(records[n - 1].ended_us, r.began_us, 'Acquisition sector gap');
      assert(r.valid_samples <= r.samples);
      if (!r.valid_samples) for (const f of ['first_adc', 'last_adc', 'min_adc', 'max_adc', 'directed_min', 'directed_max', 'directed_first', 'directed_last']) r[f] = null;
      records.push(r);
    }
    banks.push({bank: k, pinned: k === header.failure_bank || k === header.success_bank, classification: states[h.state], header: h, records});
  }
  for (const index of [header.failure_bank, header.success_bank]) if (index !== NONE) assert(banks.some(b => b.bank === index));
  return {header, banks, caveats: ['Sector summaries span alignment/acquisition, not continuous ADC waveforms; closed-loop run has aggregate counts and the separate 16-trial summary.', 'Forward/reverse crossings refer to observed voltage crossing the expected Schmitt band, NOT measured rotor direction.', 'ADC ranks are sequential software-consumed samples. Filtered current is not peak phase current. Duty_first is boundary value; duty_last is last sampled output.', 'Record/counter overflow and phase mismatch are explicit flags. Actual logger overhead needs on-board cycle maxima; offline tests do not validate timing.', 'Both banks pin first completed failure and first full 1-second closed-loop observation. Operator early-zero does not pin. No motor actions originate in the logger.']};
}
async function dump(elf, out, stopped, connection = connect) {
  assert(stopped, 'Require --stopped after operator zero/full-stop confirmation');
  assert(out && !fs.existsSync(out) && !fs.existsSync(out + '.json'), 'New output path required');
  elf = path.resolve(elf);
  const syms = symbols(elf, true), hex = fs.readFileSync(elf.replace(/\.elf$/, '.hex'));
  const expected = parseHex(hex.toString());
  const p = spawnSync('arm-none-eabi-objcopy', ['-O', 'ihex', elf, '/dev/stdout'], {encoding: 'utf8'});
  assert.equal(p.status, 0, p.stderr); assert.deepEqual(parseHex(p.stdout), expected);
  const c = await connection();
  try {
    async function mem(a, count) {
      const b = Buffer.alloc(count), stride = a % 4 || count % 4 ? 1 : 4;
      for (let i = 0; i < count; i += 1024) {
        const n = Math.min(1024, count - i);
        const v = (await c.rpc(`read_memory 0x${(a + i).toString(16)} ${stride * 8} ${n / stride}`)).trim().split(/\s+/).map(Number);
        assert(v.length === n / stride && v.every(x => Number.isInteger(x) && x >= 0 && x <= (stride === 4 ? NONE : 255)), 'Bad memory reply');
        v.forEach((x, j) => b.writeUIntLE(x, i + j * stride, stride));
      }
      return b;
    }
    const word = async a => (await mem(a, 4)).readUInt32LE();
    const scalar = async n => { const s = syms[n]; assert(s && s.size <= 4); return (await mem(s.address, s.size)).readUIntLE(0, s.size); };
    async function stop() {
      assert.equal((await c.rpc('[target current] curstate')).trim(), 'running');
      assert.equal(await word(0xe000ed08), 0x08001000);
      for (const n of ['input', 'adjusted_input', 'newinput', 'running', 'heli_coast_active', 'duty_cycle']) assert.equal(await scalar(n), 0, n);
      assert.equal((await word(0x48000010)) & 0x1f00, 0); assert.equal((await word(0x48001410)) & 1, 0);
      for (let i = 0; i < 4; i++) assert.equal(await word(0x40012c34 + i * 4), 0);
    }
    await stop();
    // Narrow identity for a previously verified S2D flash, not a full backup.
    const ranges = [[0x08001000, 8], [0x0800f7e0, 32]];
    for (const n of ['fdTraceInit', 'fdTraceTrialFinish', 'fdBenchConfigOk', 'fdBenchService']) {
      const s = syms[n]; assert(s && s.address >= 0x08001000 && s.address + s.size <= 0x0800f800 && s.size > 0 && s.size < 4096);
      ranges.push([s.address, s.size]);
    }
    for (const [a, n] of ranges) {
      const b = await mem(a, n);
      for (let i = 0; i < n; i++) if (expected.has(a + i)) assert.equal(b[i], expected.get(a + i), 'Wrong board image');
    }
    await stop();
    const pair = await mem(syms.fd_startup_pair.address, SIZE);
    assert.deepEqual(await mem(syms.fd_startup_pair.address, SIZE), pair, 'Pair data changed');
    const benchRaw = await mem(syms.fd_power_bench.address, 552);
    assert.deepEqual(await mem(syms.fd_power_bench.address, 552), benchRaw, 'Trials changed');
    const d = decode(pair), bench = decodeBench(benchRaw, 6);
    assert.equal(d.header.trials_seen, bench.header.trials);
    const protection = {};
    for (const n of ['stuck_rotor_inhibit', 'stuck_rotor_trip_count', 'bemf_timeout', 'bemf_timeout_happened']) protection[n] = await scalar(n);
    const settings = await mem(syms.eepromBuffer.address, syms.eepromBuffer.size);
    await stop();
    assert.deepEqual(await mem(syms.fd_startup_pair.address, 64), pair.subarray(0, 64), 'Trial changed during capture');
    const data = {...d, bench, metadata: {capturedAt: new Date().toISOString(), elf, hex_sha256: sha(hex), trace_sha256: sha(pair), identity: 'known_session_selected_functions_not_full_flash', settings_ram_hex: settings.toString('hex'), protection_after_zero: protection, bench_raw_hex: benchRaw.toString('hex')}};
    fs.mkdirSync(path.dirname(path.resolve(out)), {recursive: true});
    fs.writeFileSync(out, pair, {flag: 'wx'}); fs.writeFileSync(out + '.json', JSON.stringify(data, null, 2) + '\n', {flag: 'wx'});
    console.log(JSON.stringify({saved: path.resolve(out), trials: bench.header.trials, banks: d.banks.map(b => ({trial: b.header.trial_id, result: b.classification, sectors: b.header.sectors, overflow: b.header.overflow, max_sample_cycles: b.header.max_sample_cycles, max_boundary_cycles: b.header.max_boundary_cycles}))}, null, 2));
    return data;
  } finally {c.close();}
}
module.exports = {decode, dump, SIZE, BANK, MAGIC, NONE};
if (require.main === module) {
  const args = process.argv.slice(2);
  (async () => {
    if (args[0] === 'decode') console.log(JSON.stringify(decode(fs.readFileSync(args[1])), null, 2));
    else if (args[0] === 'dump') await dump(args[1], args[2], args.includes('--stopped'));
    else throw Error('decode PAIR.bin | dump MATCHING.elf NEW.bin --stopped (read only; known verified S2D session)');
  })().catch(e => {console.error(e.message); process.exitCode = 1;});
}
