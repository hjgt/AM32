// Synthetic RAM/RPC only: this test never connects to OpenOCD or hardware.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {decode, decodeBench, parseHex, symbols, hardware, SIZE, HEADER, MAGIC} = require('../tools/scripts/fd6288_startup_capture.cjs');
const elf = path.resolve(process.argv[2] || 'obj-startup-timing-B/AM32_CUSTOM_FD6288_G431_2.20.elf');
const syms = symbols(elf);
const variant = process.argv[3] ? Number(process.argv[3]) : syms.fd_rev_acquiring ? 4 : syms.fd_power_bench ? (elf.includes('power-P0') ? 2 : 3) : 1;
assert([1, 2, 3, 4, 5].includes(variant), 'Explicit variant must be 1..5');
const flash = parseHex(fs.readFileSync(elf.replace(/\.elf$/, '.hex'), 'utf8'));
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-capture-tests-'));
function fixture(samples = 700, events = 200) {
  const b = Buffer.alloc(SIZE);
  [MAGIC, 1, variant, 160000000, 3, 0, 4, 0xfffffff0, 160, 1, samples, events].forEach((v, i) => b.writeUInt32LE(v, i * 4));
  function put(offset, n, kind) {
    b.writeUInt32LE(n, offset); b.writeUInt32LE(144, offset + 4);
    b.writeUInt16LE(1000, offset + 8); b.writeInt16LE(-20, offset + 18);
    b[offset + 28] = 1;
    if (kind === 'sample') b[offset + 29] = 7;
    else { b[offset + 30] = 4; b.writeUInt16LE(10, offset + 26); }
  }
  for (let n = 1; n <= Math.min(samples, 64); n++) put(HEADER + (n - 1) * 32, n, 'sample');
  for (let n = Math.max(1, samples - 511); n <= samples; n++) put(HEADER + 64 * 32 + ((n - 1) % 512) * 32, n, 'sample');
  for (let n = 1; n <= Math.min(events, 32); n++) put(HEADER + 576 * 32 + (n - 1) * 32, n, 'event');
  for (let n = Math.max(1, events - 127); n <= events; n++) put(HEADER + 608 * 32 + ((n - 1) % 128) * 32, n, 'event');
  return b;
}
function benchFixture(count = 18) {
  const b = Buffer.alloc(552);
  [0x46504231, 1, variant === 2 ? 0 : 40, count, count, count, 0, 1, 0, 0]
    .forEach((v, i) => b.writeUInt32LE(v, i * 4));
  for (let id = Math.max(1, count - 15); id <= count; id++) {
    const o = 40 + ((id - 1) % 16) * 32;
    [id, 48000000, 5, 2, 8000000, 16000000].forEach((v, i) => b.writeUInt32LE(v, o + i * 4));
    b.writeInt16LE(123, o + 24); b.writeUInt16LE(760, o + 26);
    b.writeUInt16LE(400, o + 28); b[o + 30] = 1;
    if (variant === 5) b[o + 31] = 0x64;
  }
  return b;
}
function mock({moving = false, cpu = 'running', wrongFlash = false, badABI = false, badBench = false, data = fixture()} = {}) {
  const ram = Buffer.alloc(32768);
  data.copy(ram, syms.fd_startup_trace.address - 0x20000000);
  if (syms.fd_power_bench) {
    benchFixture().copy(ram, syms.fd_power_bench.address - 0x20000000);
    if (badBench) ram.writeUInt32LE(99, syms.fd_power_bench.address - 0x20000000 + 4);
  }
  if (badABI) ram.writeUInt32LE(99, syms.fd_startup_trace.address - 0x20000000 + 4);
  if (moving) ram[syms.running.address - 0x20000000] = 1;
  let writes = 0;
  const api = {close() {}, async rpc(cmd) {
    if (cmd === '[target current] curstate') return cpu;
    let m = cmd.match(/^read_memory 0x([\da-f]+) (8|32) (\d+)$/);
    if (m) {
      const addr = parseInt(m[1], 16), stride = Number(m[2]) / 8, n = Number(m[3]) * stride;
      if (stride === 4) assert.equal(addr % 4, 0);
      const bytes = Buffer.from(Array.from({length: n}, (_, i) => {
        const a = addr + i;
        if (a >= 0x20000000 && a < 0x20008000) return ram[a - 0x20000000];
        if (a >= 0x08001000 && a < 0x0800f800) return (flash.get(a) ?? 255) ^ ((wrongFlash && a === 0x08001000) ? 1 : 0);
        if (a >= 0xe000ed08 && a < 0xe000ed0c) return [0, 16, 0, 8][a - 0xe000ed08];
        assert((a >= 0x48000010 && a < 0x48000014) || (a >= 0x48001410 && a < 0x48001414) || (a >= 0x40012c34 && a < 0x40012c44), `Unexpected read ${cmd}`);
        return 0;
      }));
      return Array.from({length: n / stride}, (_, i) => `0x${bytes.readUIntLE(i * stride, stride).toString(16)}`).join(' ');
    }
    assert.equal(cmd, `write_memory 0x${(syms.fd_startup_trace.address + 20).toString(16)} 32 {0x41524d31}`, 'Unexpected write or hardware operation');
    writes++;
    const offset = syms.fd_startup_trace.address - 0x20000000;
    ram.writeUInt32LE(1, offset + 16); ram.writeUInt32LE(5, offset + 24);
    return '';
  }};
  return {connect: async () => api, writes: () => writes};
}
async function main() {
  const bench = decodeBench(benchFixture(), variant);
  assert.equal(bench.records.length, 16); assert.equal(bench.records[0].id, 3);
  assert.equal(bench.records[15].id, 18); assert.equal(bench.omitted_earlier_trials, 2);
  assert.equal(bench.records[0].elapsed_us, 300000);
  assert.equal(bench.records[0].peak_filtered_current_A, 1.23);
  assert.equal(bench.records[0].reason, variant === 5 ? 'closed_observation_1s' : 'window_300ms');
  const revSummary = benchFixture(1);
  revSummary[70] = 8; revSummary[71] = 2;
  assert.equal(decodeBench(revSummary, 4).records[0].reason, 'rev_no_lock');
  assert.equal(decodeBench(revSummary, 4).records[0].rev_stage, 'open_loop');
  revSummary[70] = 9; revSummary[71] = 4;
  assert.equal(decodeBench(revSummary, 4).records[0].reason, 'rev_lost_feedback');
  revSummary[71] = 5;
  assert.throws(() => decodeBench(revSummary, 4), /Unknown rev-up stage/);
  revSummary[71] = 0x22;
  assert.equal(decodeBench(revSummary, 5).records[0].max_qualified_streak, 2);
  assert.equal(decodeBench(revSummary, 5).records[0].rev_stage, 'open_loop');
  revSummary[71] = 0x74;
  assert.throws(() => decodeBench(revSummary, 5), /Invalid qualified streak/);
  const revTrace = fixture(0, 1);
  revTrace.writeUInt32LE(4, 8);
  for (const offset of [HEADER + 576 * 32, HEADER + 608 * 32]) {
    revTrace[offset + 30] = 10; revTrace.writeUInt16LE(0x0306, offset + 26);
  }
  assert.equal(decode(revTrace).events.records[0].rev_stage, 'pending_handoff');
  assert.equal(decode(revTrace).events.records[0].confidence, 6);
  revTrace.writeUInt32LE(5, 8);
  for (const offset of [HEADER + 576 * 32, HEADER + 608 * 32]) {
    revTrace[offset + 30] = 12; revTrace.writeUInt16LE(0x5c02, offset + 26);
  }
  assert.equal(decode(revTrace).events.records[0].decision, 'too_late');
  assert.equal(decode(revTrace).events.records[0].sector_percent, 92);
  assert.equal(decodeBench(benchFixture(0)).records.length, 0);
  const badBench = benchFixture(); badBench.writeUInt32LE(19, 32);
  assert.throws(() => decodeBench(badBench), /Unfinished/);
  const mismatch = benchFixture(); mismatch.writeUInt32LE(0, 40 + 2 * 32);
  assert.throws(() => decodeBench(mismatch), /sequence mismatch/);
  const b = fixture(), d = decode(b);
  assert.equal(d.samples.retained, 576); assert.deepEqual(d.samples.gaps, [[65, 188]]);
  assert.equal(d.events.retained, 160); assert.deepEqual(d.events.gaps, [[33, 72]]);
  assert.equal(d.samples.records[0].us, 1); assert.equal(d.samples.records[0].margin, -20);
  assert.equal(d.events.records[0].current, -20); assert.equal(d.events.records[0].timeout_count_before, 10);
  assert.equal(decode(fixture(3, 2)).samples.retained, 3);
  assert.equal(decode(fixture(0, 0)).events.retained, 0);
  assert.throws(() => decode(b.subarray(0, SIZE - 1)));
  const bad = Buffer.from(b); bad.writeUInt32LE(44, HEADER);
  assert.throws(() => decode(bad), /sequence mismatch/);
  const good = mock(); await hardware('arm', elf, undefined, true, good.connect); assert.equal(good.writes(), 1);
  for (const opts of [{moving: true}, {cpu: 'halted'}, {wrongFlash: true}, {badABI: true}]) {
    const m = mock(opts); await assert.rejects(hardware('arm', elf, undefined, true, m.connect)); assert.equal(m.writes(), 0);
  }
  let connected = false;
  await assert.rejects(hardware('arm', elf, undefined, false, async () => { connected = true; }));
  assert.equal(connected, false);
  const m = mock(), out = path.join(temp, 'failure.bin');
  await hardware('dump', elf, out, true, m.connect);
  assert.deepEqual(fs.readFileSync(out), b);
  assert.equal(m.writes(), 0);
  const saved = JSON.parse(fs.readFileSync(`${out}.json`));
  assert.equal(saved.header.variant, variant);
  if (variant >= 2) {
    assert.deepEqual(saved.bench, bench);
    const quickOut = path.join(temp, 'quick.bin'), quick = mock();
    await hardware('dump', elf, quickOut, true, quick.connect, true);
    assert.equal(quick.writes(), 0);
    assert.deepEqual(JSON.parse(fs.readFileSync(`${quickOut}.json`)).bench, bench);
    assert.equal(JSON.parse(fs.readFileSync(`${quickOut}.json`)).metadata.identity_check, 'known_session_vectors_and_selected_functions_not_full_flash');
    await assert.rejects(hardware('arm', elf, undefined, true, quick.connect, true));
    const wrong = mock({wrongFlash: true});
    await assert.rejects(hardware('dump', elf, path.join(temp, 'wrong-quick.bin'), true, wrong.connect, true));
    assert.equal(wrong.writes(), 0);
    const m = mock({badBench: true});
    const badOut = path.join(temp, 'bad-bench.bin');
    await assert.rejects(hardware('dump', elf, badOut, true, m.connect));
    assert(!fs.existsSync(badOut)); assert.equal(m.writes(), 0);
  }
  await assert.rejects(hardware('dump', elf, out, true, m.connect));
  console.log('PASS: offline decoder + mock RPC identity/stopped gates/arm/dump/overwrite guards. No hardware accessed.');
}
main().catch(e => { console.error(e); process.exitCode = 1; });
