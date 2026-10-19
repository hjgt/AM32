// Offline decode + explicitly stopped-board capture. Never halt/reset/flash.
const fs = require('node:fs');
const path = require('node:path');
const net = require('node:net');
const crypto = require('node:crypto');
const { spawnSync } = require('node:child_process');
const assert = require('node:assert/strict');
const SIZE = 23656, HEADER = 104, MAGIC = 0x46535431;
const reasons = ['invalid', 'blank', 'pre', 'hysteresis', 'early', 'unarmed', 'confirmation', 'accepted', 'reported', 'coast'];
const events = ['', 'start', 'commutate', 'zc', 'timeout', 'desync', 'stop', 'fault', 'bench_stop', 'rev_step', 'rev_edge', 'rev_handoff', 'rev_decision'];
const revDecisions = ['ignored_stage_or_duplicate', 'too_early', 'too_late', 'first_qualified_edge', 'inconsistent_period', 'qualified_pair', 'ready_for_handoff'];
const benchReasons = ['', 'window_300ms', 'native_stuck', 'safety_cap', 'not_running', 'bad_config', 'no_zero_or_start_not_allowed', 'operator_zero', 'rev_no_lock', 'rev_lost_feedback'];
const revStages = ['off', 'align', 'open_loop', 'pending_handoff', 'closed_loop'];
const sha = b => crypto.createHash('sha256').update(b).digest('hex');
function header(b) {
  assert(b.length >= HEADER, 'Truncated trace header');
  const names = ['magic', 'abi', 'variant', 'clock_hz', 'state', 'command', 'capture_id', 'start_cycles', 'end_cycles', 'end_reason', 'sample_sequence', 'event_sequence', 'no_fresh'];
  const h = Object.fromEntries(names.map((n, i) => [n, b.readUInt32LE(i * 4)]));
  assert(h.magic === MAGIC && h.abi === 1 && h.clock_hz === 160000000 && h.variant <= 5, 'Wrong trace ABI/firmware');
  h.decisions = Object.fromEntries(reasons.map((r, i) => [r, b.readUInt32LE(52 + 4 * i)]));
  h.max_sample_cycles = b.readUInt32LE(92);
  h.max_event_cycles = b.readUInt32LE(96);
  h.rejected_commands = b.readUInt32LE(100);
  return h;
}
function decode(b) {
  assert.equal(b.length, SIZE, 'Wrong trace size');
  const h = header(b);
  function record(offset, kind) {
    const r = {sequence: b.readUInt32LE(offset), cycles: b.readUInt32LE(offset + 4)};
    r.us = ((r.cycles - h.start_cycles) >>> 0) / (h.clock_hz / 1e6);
    const fields = kind === 'sample' ? ['elapsed', 'interval', 'va', 'vb', 'vc', 'margin', 'duty', 'pwm', 'trigger', 'flags'] : ['elapsed', 'interval', 'wait', 'duty', 'input', 'current', 'voltage', 'pwm', 'zero_crosses', 'aux'];
    fields.forEach((f, i) => { r[f] = ['margin', 'current'].includes(f) ? b.readInt16LE(offset + 8 + 2 * i) : b.readUInt16LE(offset + 8 + 2 * i); });
    r.step = b[offset + 28];
    if (kind === 'sample') {
      r.reason = reasons[b[offset + 29]] || 'unknown';
      r.pre_seen = b[offset + 30]; r.confirmations = b[offset + 31];
    } else {
      r.flags = b[offset + 29]; r.kind = events[b[offset + 30]] || 'unknown';
      if (r.kind === 'zc') r.wait_skipped = !!r.aux;
      if (r.kind === 'timeout') r.timeout_count_before = r.aux;
      if (r.kind === 'rev_step') r.commanded_sector_us = r.aux;
      if (r.kind === 'rev_edge') {
        r.rev_stage = revStages[r.aux >> 8]; r.confidence = r.aux & 255;
      }
      if (r.kind === 'rev_handoff') r.confidence = r.aux;
      if (r.kind === 'rev_decision') {
        r.decision = revDecisions[r.aux & 255] || 'unknown';
        r.sector_percent = (r.aux >> 8) === 255 ? null : r.aux >> 8;
      }
    }
    return r;
  }
  function collect(kind, firstAt, firstN, ringAt, ringN, total) {
    const out = new Map();
    for (let n = 1; n <= Math.min(firstN, total); n++) {
      const r = record(firstAt + (n - 1) * 32, kind);
      assert.equal(r.sequence, n, 'First-record sequence mismatch'); out.set(n, r);
    }
    for (let n = Math.max(1, total - ringN + 1); n <= total; n++) {
      const r = record(ringAt + ((n - 1) % ringN) * 32, kind);
      assert.equal(r.sequence, n, 'Ring-record sequence mismatch'); out.set(n, r);
    }
    const records = [...out.values()].sort((a, c) => a.sequence - c.sequence);
    const gaps = [];
    for (let i = 1; i < records.length; i++) if (records[i].sequence !== records[i - 1].sequence + 1) gaps.push([records[i - 1].sequence + 1, records[i].sequence - 1]);
    return {total, retained: records.length, gaps, records};
  }
  return {
    header: h, end: ['none', 'operator_zero', h.variant === 5 ? 'two_second_capture_cap' : 'one_second_capture_cap', 'fault', 'request', 'bench_stop'][h.end_reason],
    samples: collect('sample', HEADER, 64, HEADER + 64 * 32, 512, h.sample_sequence),
    events: collect('event', HEADER + 576 * 32, 32, HEADER + 608 * 32, 128, h.event_sequence),
    caveats: ['Software-consumed sequential ADC results, not an oscilloscope waveform.', 'zc means zcfoundroutine entry: variant A also calls it after forced timeout; correlate with accepted ADC samples.', 'elapsed/interval/wait use 0.5 us ticks; current is filtered centiamps, voltage centivolts.', 'flags indicate commanded direction, NOT measured rotor direction.', 'First records plus ring tail; gaps are omitted/overwritten history, not missing ADC interrupts.', 'cycle maxima measure only inner logger work, not full ISR or total overhead.', `The ${h.variant === 5 ? 2 : 1} s capture cap stops logging only, never the motor. No hardware-validation claim.`],
  };
}
function decodeBench(b, variant = 3) {
  assert.equal(b.length, 552, 'Wrong bench summary size');
  const names = ['magic', 'abi', 'extra', 'trials', 'completed', 'stops', 'inhibit', 'last_reason', 'active_id', 'floor'];
  const h = Object.fromEntries(names.map((n, i) => [n, b.readUInt32LE(i * 4)]));
  assert(h.magic === 0x46504231 && h.abi === 1 && [0, 40].includes(h.extra), 'Wrong bench summary ABI');
  assert(h.completed === h.trials && h.active_id === 0, 'Unfinished bench trial');
  assert(h.last_reason < benchReasons.length, 'Unknown bench stop reason');
  const records = [];
  for (let id = Math.max(1, h.trials - 15); id <= h.trials; id++) {
    const o = 40 + ((id - 1) % 16) * 32;
    assert.equal(b.readUInt32LE(o), id, 'Bench ring sequence mismatch');
    const r = {id};
    ['elapsed_cycles', 'accepted', 'timeouts', 'first_observed_cycles', 'fifth_observed_cycles']
      .forEach((n, i) => { r[n] = b.readUInt32LE(o + 4 + 4 * i); });
    for (const n of ['elapsed', 'first_observed', 'fifth_observed'])
      r[`${n}_us`] = r[`${n}_cycles`] === 0xffffffff ? null : r[`${n}_cycles`] / 160;
    r.peak_filtered_current_A = b.readInt16LE(o + 24) / 100;
    r.min_voltage_V = b.readUInt16LE(o + 26) / 100;
    r.max_duty = b.readUInt16LE(o + 28);
    assert(b[o + 30] > 0 && b[o + 30] < benchReasons.length, 'Unknown trial reason');
    r.reason = benchReasons[b[o + 30]];
    if (variant >= 4) {
      const stage = variant >= 5 ? b[o + 31] & 15 : b[o + 31];
      assert(stage < revStages.length, 'Unknown rev-up stage');
      r.rev_stage = revStages[stage];
      if (variant >= 5) {
        r.max_qualified_streak = b[o + 31] >> 4;
        assert(r.max_qualified_streak <= 6, 'Invalid qualified streak');
        if (b[o + 30] === 1) r.reason = 'closed_observation_1s';
      }
    }
    records.push(r);
  }
  return {header: h, records, omitted_earlier_trials: Math.max(0, h.trials - 16),
    ...(variant >= 11 ? {startup_profile: {name:variant===21?'S4G-NOEDGE':variant===20?'S4G-SEED':variant===19?'S4F-PHASE':variant>=17?'S4E':variant>=14?'S4D':variant===13?'S4C':variant===12?'S4B':'S4', initial_request:360, acquisition_request:400,
      feedback_wait_us:variant>=17?22500:null, motor_stops_at_observation_end:variant<17,
      latest_real_interval_at_handoff:variant>=12, retains_unused_rescue:variant>=13, bridges_solitary_rescue_edge:variant>=14,
      rescue_after_empty_retries:2, rescue_sector_ms:8, rescue_sectors:6,
      closed_floor_hold_ms:150, closed_floor_fade_ms:300, closed_floor_end_ms:450,
      all_retries_count_as_native_timeouts:true}} : {}),
    ...(variant === 8 ? {power_profile: {name: 'S3-P',initial_request: 540,acquisition_request: 600,startup_cap: 610,post_handoff_base: 360,release_ms: 20,legacy_extra_is_not_profile_amplitude: true}} : {}),
    caveats: [variant >= 9 ? 'Last 16 summaries. S3-D retains separate first no-lock, first post-handoff loss and first complete observation tails. Operator zero and other stops are not pinned.' : variant >= 6 ? 'Last 16 summaries. Paired acquisition banks separately retain first completed failure and first full closed-loop observation. Early operator zero does not pin a bank.' : variant >= 4 ? 'Last 16 summaries. Trace capture_id is the trial id; each start replaces a non-failure trace, first failed trial remains pinned until explicit ARM. Closed-loop stage alone is not proof of normal loaded operation.' : 'Last 16 trial summaries, not continuous waveforms. Main trace remains first-shot until explicitly rearmed.',
      'Observed times are service timestamps, not physical zero-cross timestamps. Current is a filtered software maximum, not peak phase current.',
      variant>=17 ? 'S4E: 300 ms acquisition STOP remains. After handoff, motor has NO one-second stop. Metrics freeze at one-second observation; elapsed/current/counters are NOT full-run totals. Summary final reason reflects eventual stop, possibly later than the stored metrics. First runtime timeout still stops drive.' : 'Motor stops at the bounded observation deadline.',
      variant >= 7 ? 'S3 family (including S4): event-led acquisition <= 300 ms; streak counts consecutive ADC-qualified edges, NOT S2 phase-window qualification. Six edges earn <= 1 s observation; first runtime timeout aborts. Not a native stuck-trip proof or flight mode.' : variant >= 5 ? 'S2: acquisition <= 300 ms; only six-edge handoff earns <= 1 s observation; first runtime timeout aborts. Not a hardware current limiter, native stuck-trip proof or flight mode.' : 'The 300 ms bench window is a software stop request, not a hardware current limiter or a flight mode.']};
}
function parseHex(text) {
  let base = 0, eof = false;
  const data = new Map();
  for (const line of text.trim().split(/\r?\n/)) {
    assert(/^:[0-9a-f]+$/i.test(line) && !eof, 'Invalid HEX');
    const r = Buffer.from(line.slice(1), 'hex');
    assert(r.length === r[0] + 5 && (r.reduce((a, v) => a + v, 0) & 255) === 0, 'HEX checksum/length');
    if (r[3] === 0) for (let i = 0; i < r[0]; i++) {
      const a = base + r.readUInt16BE(1) + i;
      assert(a >= 0x08001000 && a < 0x0800f800 && !data.has(a), 'Not main-only HEX'); data.set(a, r[4 + i]);
    }
    else if (r[3] === 4 || r[3] === 2) { assert.equal(r[0], 2); base = r.readUInt16BE(4) * (r[3] === 4 ? 65536 : 16); }
    else if (r[3] === 1) eof = true;
    else assert.equal(r[3], 5, 'Unsupported HEX record');
  }
  assert(eof && data.has(0x08001000) && data.has(0x0800f7ff), 'Incomplete image');
  return data;
}
function symbols(elf, paired = false) {
  const p = spawnSync('arm-none-eabi-nm', ['-S', '--defined-only', elf], {encoding: 'utf8'});
  assert.equal(p.status, 0, p.stderr);
  const out = {};
  for (const line of p.stdout.split('\n')) {
    const m = line.match(/^([\da-f]+) ([\da-f]+) \w (\w+)$/i);
    if (m) out[m[3]] = {address: parseInt(m[1], 16), size: parseInt(m[2], 16)};
  }
  const traceSymbol = paired === 'wave' ? 'fd_startup_wave' : paired ? 'fd_startup_pair' : 'fd_startup_trace';
  assert(out[traceSymbol]?.size === (paired === 'wave' ? 21568 : paired ? 18624 : SIZE), 'ELF has no supported bench trace');
  for (const n of [traceSymbol, 'input', 'adjusted_input', 'newinput', 'running', 'heli_coast_active', 'duty_cycle', 'stuck_rotor_inhibit', 'stuck_rotor_trip_count', 'bemf_timeout', 'bemf_timeout_happened', 'eepromBuffer']) {
    assert(out[n] && out[n].address >= 0x20000000 && out[n].address + out[n].size <= 0x20008000, `Bad/missing RAM symbol ${n}`);
  }
  if (out.fd_power_bench) assert(out.fd_power_bench.size === 552 &&
    out.fd_power_bench.address >= 0x20000000 && out.fd_power_bench.address + 552 <= 0x20008000,
    'Bad bench summary symbol');
  return out;
}
async function connect() {
  const s = net.createConnection({host: '127.0.0.1', port: 6666});
  s.setTimeout(10000, () => s.destroy(new Error('OpenOCD timeout')));
  await new Promise((resolve, reject) => { s.once('connect', resolve); s.once('error', reject); });
  let pending, buffer = '';
  s.on('data', chunk => {
    buffer += chunk.toString();
    const i = buffer.indexOf('\x1a');
    if (i >= 0 && pending) { const p = pending; pending = null; const reply = buffer.slice(0, i); buffer = buffer.slice(i + 1); p.resolve(reply); }
  });
  s.on('error', e => { if (pending) { pending.reject(e); pending = null; } });
  s.on('close', () => { if (pending) { pending.reject(new Error('OpenOCD closed')); pending = null; } });
  return {
    close: () => s.destroy(),
    rpc: command => new Promise((resolve, reject) => {
      assert(!pending, 'Concurrent RPC forbidden'); pending = {resolve, reject};
      s.write(`set _fd_result {}; if {[catch {${command}} _fd_result]} {format "ERR:%s" $_fd_result} else {format "OK:%s" $_fd_result}\x1a`);
    }).then(reply => { assert(reply.startsWith('OK:'), reply); return reply.slice(3); }),
  };
}
async function hardware(command, elf, out, stopped, connection = connect, quickDump = false) {
  assert(['arm', 'dump'].includes(command) && stopped, 'Require arm/dump + --stopped after operator confirmation');
  assert(!quickDump || command === 'dump', 'Quick identity is read-only dump only');
  elf = path.resolve(elf);
  const syms = symbols(elf), hexPath = elf.replace(/\.elf$/, '.hex');
  assert.notEqual(hexPath, elf);
  const expected = parseHex(fs.readFileSync(hexPath, 'utf8'));
  const p = spawnSync('arm-none-eabi-objcopy', ['-O', 'ihex', elf, '/dev/stdout'], {encoding: 'utf8'});
  assert.equal(p.status, 0, p.stderr);
  assert.deepEqual(expected, parseHex(p.stdout), 'ELF and HEX do not match');
  const c = await connection();
  try {
    async function memory(address, count) {
      const b = Buffer.alloc(count);
      // Aligned SRAM/flash blocks can use word transfers: much faster on
      // ST-Link, with identical little-endian output and all stop/identity
      // checks intact. Small/unaligned symbols retain byte reads.
      const stride = address % 4 === 0 && count % 4 === 0 ? 4 : 1;
      for (let i = 0; i < count; i += 1024) {
        const n = Math.min(1024, count - i);
        const r = await c.rpc(`read_memory 0x${(address + i).toString(16)} ${stride * 8} ${n / stride}`);
        const values = r.trim().split(/\s+/).map(Number);
        assert(values.length === n / stride && values.every(v => Number.isInteger(v) && v >= 0 && v <= (stride === 4 ? 0xffffffff : 255)), 'Malformed memory reply');
        if (stride === 4) values.forEach((v, j) => b.writeUInt32LE(v, i + 4 * j));
        else b.set(values, i);
      }
      return b;
    }
    const u32 = async a => (await memory(a, 4)).readUInt32LE();
    async function stoppedCheck() {
      assert.equal((await c.rpc('[target current] curstate')).trim(), 'running', 'CPU must be running, never halted');
      assert.equal(await u32(0xe000ed08), 0x08001000, 'Not in main firmware');
      const state = {};
      for (const name of ['input', 'adjusted_input', 'newinput', 'running', 'heli_coast_active', 'duty_cycle']) {
        const s = syms[name]; state[name] = (await memory(s.address, s.size)).readUIntLE(0, s.size);
        assert.equal(state[name], 0, `Not safely stopped: ${name}`);
      }
      assert.equal((await u32(0x48000010)) & 0x1f00, 0, 'PA8..PA12 not low');
      assert.equal((await u32(0x48001410)) & 1, 0, 'PF0 not low');
      for (let i = 0; i < 4; i++) assert.equal(await u32(0x40012c34 + 4 * i), 0, `TIM1 CCR${i + 1} not zero`);
      return state;
    }
    await stoppedCheck();
    // A just-flashed, already verified P0/P1 session can use a small identity
    // check for repeated read-only dumps. This is NOT full flash verification.
    let ranges;
    if (quickDump) {
      assert(syms.fd_power_bench, 'Quick dump supports known bench builds only');
      ranges = [[0x08001000, 8], [0x0800f7e0, 32]];
      for (const name of ['fdTraceInit', 'fdBenchConfigOk', 'fdBenchStop', 'fdBenchService']) {
        const s = syms[name];
        assert(s && s.address >= 0x08001000 && s.address + s.size <= 0x0800f800 && s.size > 0 && s.size <= 1024);
        ranges.push([s.address, s.size]);
      }
    } else {
      ranges = [];
      for (let a = 0x08001000; a < 0x0800f800; a += 1024) ranges.push([a, Math.min(1024, 0x0800f800 - a)]);
    }
    for (const [a, n] of ranges) {
      if (![...Array(n).keys()].some(i => expected.has(a + i))) continue;
      const b = await memory(a, n);
      for (let i = 0; i < n; i++) if (expected.has(a + i)) assert.equal(b[i], expected.get(a + i), `Firmware mismatch at 0x${(a + i).toString(16)}`);
    }
    await stoppedCheck();
    const addr = syms.fd_startup_trace.address;
    const before = header(await memory(addr, HEADER));
    if (command === 'arm') {
      assert(before.state !== 2, 'Active capture must finish before rearming');
      await c.rpc(`write_memory 0x${(addr + 20).toString(16)} 32 {0x41524d31}`);
      const after = header(await memory(addr, HEADER));
      assert(after.state === 1 && after.capture_id === before.capture_id + 1 && after.command === 0, 'Firmware did not accept arm command');
      console.log(JSON.stringify({armed: true, variant: after.variant, capture_id: after.capture_id,
        note: after.variant === 5 ? 'No throttle change. S2: 300 ms acquisition, then <= 1 s observation only after handoff. First runtime timeout aborts. Bench only.' : after.variant >= 2 ? 'No throttle change. P0/P1/S1 separately impose a 300 ms drive window, even on success. Bench only; wait for full stop between trials.' :
          'No throttle change. Start only on operator decision. Logging cap is NOT motor shutdown.'}, null, 2));
    } else {
      assert(before.state === 3, 'Trace must be frozen; return to zero first');
      assert(out && !fs.existsSync(out) && !fs.existsSync(`${out}.json`), 'Provide a new output BIN path');
      const b = await memory(addr, SIZE), after = header(await memory(addr, HEADER));
      assert.deepEqual(after, before, 'Trace changed during dump');
      assert.deepEqual(header(b), before, 'Inconsistent trace header');
      const stoppedState = await stoppedCheck();
      const protection = {};
      for (const name of ['stuck_rotor_inhibit', 'stuck_rotor_trip_count', 'bemf_timeout', 'bemf_timeout_happened']) {
        const s = syms[name]; protection[name] = (await memory(s.address, s.size)).readUIntLE(0, s.size);
      }
      const settings = await memory(syms.eepromBuffer.address, syms.eepromBuffer.size);
      let bench, benchBytes;
      if (before.variant >= 2) {
        assert(syms.fd_power_bench, 'Bench image missing summary symbol');
        benchBytes = await memory(syms.fd_power_bench.address, 552);
        assert.deepEqual(await memory(syms.fd_power_bench.address, 552), benchBytes, 'Bench summaries changed during dump');
        bench = decodeBench(benchBytes, before.variant);
        assert.equal(bench.header.extra, before.variant === 2 ? 0 : 40);
        await stoppedCheck();
      }
      const d = {...decode(b), bench, metadata: {capturedAt: new Date().toISOString(), elf, identity_check: quickDump ? 'known_session_vectors_and_selected_functions_not_full_flash' : 'all_defined_hex_bytes', hex_sha256: sha(fs.readFileSync(hexPath)), trace_sha256: sha(b), stoppedState, protection_after_zero: protection, settings_ram_hex: settings.toString('hex'), bench_raw_hex: benchBytes?.toString('hex')}};
      fs.mkdirSync(path.dirname(path.resolve(out)), {recursive: true});
      fs.writeFileSync(out, b, {flag: 'wx'});
      fs.writeFileSync(`${out}.json`, JSON.stringify(d, null, 2) + '\n', {flag: 'wx'});
      console.log(JSON.stringify({saved: path.resolve(out), end: d.end, variant: before.variant, samples: d.samples.retained, sample_gaps: d.samples.gaps, events: d.events.retained}, null, 2));
    }
  } finally { c.close(); }
}
async function main(args) {
  if (args[0] === 'decode') { console.log(JSON.stringify(decode(fs.readFileSync(args[1])), null, 2)); return; }
  if (['arm', 'dump'].includes(args[0])) { await hardware(args[0], args[1], args[2] === '--stopped' ? undefined : args[2], args.includes('--stopped'), connect, args.includes('--quick-dump')); return; }
  console.log('Offline: node tools/scripts/fd6288_startup_capture.cjs decode TRACE.bin\nStopped board only (OpenOCD already running):\n  ... arm MAIN.elf --stopped\n  ... dump MAIN.elf NEW-TRACE.bin --stopped [--quick-dump]\nQuick dump: known, already verified P0/P1 session only; selected identity bytes, NOT a full flash check. Never available for arm.\nNever starts OpenOCD, halts/resets CPU, changes throttle, or writes flash/settings.\nARM writes only a trace command in RAM after flash identity + stopped checks.');
}
module.exports = {decode, decodeBench, header, parseHex, symbols, hardware, connect, SIZE, HEADER, MAGIC};
if (require.main === module) main(process.argv.slice(2)).catch(e => { console.error(e.message); process.exitCode = 1; });
