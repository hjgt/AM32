// Synthetic RAM/RPC only: this test never connects to OpenOCD or hardware.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {decode, parseHex, symbols, hardware, SIZE, HEADER, MAGIC} = require('../tools/scripts/fd6288_startup_capture.cjs');
const elf = path.resolve(process.argv[2] || 'obj-startup-timing-B/AM32_CUSTOM_FD6288_G431_2.20.elf');
const syms = symbols(elf);
const flash = parseHex(fs.readFileSync(elf.replace(/\.elf$/, '.hex'), 'utf8'));
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-capture-tests-'));
function fixture(samples = 700, events = 200) {
  const b = Buffer.alloc(SIZE);
  [MAGIC, 1, 1, 160000000, 3, 0, 4, 0xfffffff0, 160, 1, samples, events].forEach((v, i) => b.writeUInt32LE(v, i * 4));
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
function mock({moving = false, cpu = 'running', wrongFlash = false, badABI = false, data = fixture()} = {}) {
  const ram = Buffer.alloc(32768);
  data.copy(ram, syms.fd_startup_trace.address - 0x20000000);
  if (badABI) ram.writeUInt32LE(99, syms.fd_startup_trace.address - 0x20000000 + 4);
  if (moving) ram[syms.running.address - 0x20000000] = 1;
  let writes = 0;
  const api = {close() {}, async rpc(cmd) {
    if (cmd === '[target current] curstate') return cpu;
    let m = cmd.match(/^read_memory 0x([\da-f]+) 8 (\d+)$/);
    if (m) {
      const addr = parseInt(m[1], 16), n = Number(m[2]);
      return Array.from({length: n}, (_, i) => {
        const a = addr + i;
        if (a >= 0x20000000 && a < 0x20008000) return ram[a - 0x20000000];
        if (a >= 0x08001000 && a < 0x0800f800) return (flash.get(a) ?? 255) ^ ((wrongFlash && a === 0x08001000) ? 1 : 0);
        if (a >= 0xe000ed08 && a < 0xe000ed0c) return [0, 16, 0, 8][a - 0xe000ed08];
        assert((a >= 0x48000010 && a < 0x48000014) || (a >= 0x48001410 && a < 0x48001414) || (a >= 0x40012c34 && a < 0x40012c44), `Unexpected read ${cmd}`);
        return 0;
      }).map(v => `0x${v.toString(16)}`).join(' ');
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
  assert.equal(JSON.parse(fs.readFileSync(`${out}.json`)).header.variant, 1);
  await assert.rejects(hardware('dump', elf, out, true, m.connect));
  console.log('PASS: offline decoder + mock RPC identity/stopped gates/arm/dump/overwrite guards. No hardware accessed.');
}
main().catch(e => { console.error(e); process.exitCode = 1; });
