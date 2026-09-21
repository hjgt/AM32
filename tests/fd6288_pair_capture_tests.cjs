// No real connection: exercise the actual read-only capture against fake RAM.
const assert = require('node:assert/strict'), fs = require('node:fs'), os = require('node:os'), path = require('node:path');
const {decode, dump, SIZE, NONE} = require('../tools/scripts/fd6288_pair_capture.cjs');
const {symbols, parseHex} = require('../tools/scripts/fd6288_startup_capture.cjs');
const elf = path.resolve(process.argv[2]), fixture = fs.readFileSync(process.argv[3]);
const syms = symbols(elf, true), flash = parseHex(fs.readFileSync(elf.replace(/\.elf$/, '.hex'), 'utf8'));
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-pair-rpc-'));
const decoded = decode(fixture);
function mock(opts = {}) {
  const ram = Buffer.alloc(32768), traceAt = syms.fd_startup_pair.address - 0x20000000;
  fixture.copy(ram, traceAt);
  if (opts.active) ram.writeUInt32LE(0, traceAt + 24);
  if (opts.badABI) ram.writeUInt32LE(9, traceAt + 4);
  if (opts.moving) ram[syms.adjusted_input.address - 0x20000000] = 100;
  const bench = Buffer.alloc(552);
  [0x46504231, 1, 40, 7, opts.benchActive ? 6 : 7, 7, 0, 8, opts.benchActive ? 7 : 0, 0].forEach((v, i) => bench.writeUInt32LE(v, 4 * i));
  for (let n = 1; n <= 7; n++) {
    const o = 40 + (n - 1) * 32;
    bench.writeUInt32LE(n, o); bench.writeUInt32LE(48000000, o + 4);
    bench[o + 30] = n === 6 ? 1 : 8; bench[o + 31] = n === 6 ? 0x64 : 0x12;
  }
  bench.copy(ram, syms.fd_power_bench.address - 0x20000000);
  let reads = 0, closed = false, fullTraceReads = 0;
  return {get reads() {return reads;}, get closed() {return closed;}, connect: async () => ({
    close() {closed = true;},
    async rpc(cmd) {
      if (cmd === '[target current] curstate') return opts.halted ? 'halted' : 'running';
      const m = cmd.match(/^read_memory 0x([a-f0-9]+) (8|32) (\d+)$/);
      assert(m, `Forbidden non-read RPC: ${cmd}`); reads++;
      const a = parseInt(m[1], 16), stride = Number(m[2]) / 8, count = Number(m[3]) * stride;
      if (a === syms.fd_startup_pair.address && count === 1024) fullTraceReads++;
      if (opts.changing && fullTraceReads === 2) ram[traceAt + 44] ^= 1;
      if (opts.badReply) return 'ERR unreadable';
      const bytes = Buffer.from(Array.from({length: count}, (_, i) => {
        const p = a + i;
        if (p >= 0x20000000 && p < 0x20008000) return ram[p - 0x20000000];
        if (p >= 0x08001000 && p < 0x0800f800) return (flash.get(p) ?? 255) ^ (opts.wrongFlash && p === 0x08001000 ? 1 : 0);
        if (p >= 0xe000ed08 && p < 0xe000ed0c) return [0, opts.wrongVTOR ? 0 : 16, 0, 8][p - 0xe000ed08];
        if (p >= 0x48000010 && p < 0x48000014) return opts.gates && p === 0x48000011 ? 1 : 0;
        assert((p >= 0x48001410 && p < 0x48001414) || (p >= 0x40012c34 && p < 0x40012c44));
        return opts.pwm && p === 0x40012c34 ? 1 : 0;
      }));
      return Array.from({length: count / stride}, (_, i) => '0x' + bytes.readUIntLE(i * stride, stride).toString(16)).join(' ');
    },
  })};
}
(async () => {
  let connected = false;
  await assert.rejects(dump(elf, path.join(temp, 'no-stop.bin'), false, async () => {connected = true;}));
  assert(!connected);
  const good = mock(), out = path.join(temp, 'pair.bin');
  const d = await dump(elf, out, true, good.connect);
  assert(good.closed && good.reads > 0); assert.deepEqual(fs.readFileSync(out), fixture);
  assert.deepEqual(d.banks, decoded.banks);
  assert.equal(d.header.active_bank, NONE); assert.equal(d.bench.header.trials, 7);
  await assert.rejects(dump(elf, out, true, good.connect));
  for (const name of ['moving', 'halted', 'gates', 'pwm', 'wrongVTOR', 'wrongFlash', 'badABI', 'active', 'benchActive', 'changing', 'badReply']) {
    const api = mock({[name]: true}), target = path.join(temp, name + '.bin');
    await assert.rejects(dump(elf, target, true, api.connect), name);
    assert(api.closed && !fs.existsSync(target), name);
  }
  const corrupt = Buffer.from(fixture); corrupt.writeUInt32LE(73, 64 + 24);
  assert.throws(() => decode(corrupt)); assert.throws(() => decode(fixture.subarray(0, SIZE - 1)));
  console.log('PASS paired capture: real C ABI, identity, zero/gates/PWM, active/changing/corrupt buffers, overwrite refusal; read-only mock RPC only');
})().catch(e => {console.error(e); process.exitCode = 1;});
