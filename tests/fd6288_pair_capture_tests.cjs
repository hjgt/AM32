// No real connection: exercise the actual read-only capture against fake RAM.
const assert = require('node:assert/strict'), fs = require('node:fs'), os = require('node:os'), path = require('node:path');
const wave = process.argv[4] === 'wave';
const {decode, dump, SIZE, NONE} = require(wave ? '../tools/scripts/fd6288_wave_capture.cjs' : '../tools/scripts/fd6288_pair_capture.cjs');
const {symbols, parseHex} = require('../tools/scripts/fd6288_startup_capture.cjs');
const elf = path.resolve(process.argv[2]), fixture = fs.readFileSync(process.argv[3]);
const syms = symbols(elf, wave ? 'wave' : true), flash = parseHex(fs.readFileSync(elf.replace(/\.elf$/, '.hex'), 'utf8'));
const traceSymbol = wave ? 'fd_startup_wave' : 'fd_startup_pair';
const temp = fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-pair-rpc-'));
const decoded = decode(fixture);
function mock(opts = {}) {
  const ram = Buffer.alloc(32768), traceAt = syms[traceSymbol].address - 0x20000000;
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
      if (a === syms[traceSymbol].address && count === 1024) fullTraceReads++;
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
  if (d.header.variant >= 11) {
    assert.equal(d.sampling_profile.name, d.header.variant === 14 ? 'S4D' : d.header.variant === 13 ? 'S4C' : d.header.variant === 12 ? 'S4B' : 'S4');
    assert.equal(d.bench.startup_profile.latest_real_interval_at_handoff, d.header.variant >= 12);
    assert.equal(d.bench.startup_profile.retains_unused_rescue, d.header.variant >= 13);
    assert.equal(d.bench.startup_profile.bridges_solitary_rescue_edge, d.header.variant === 14);
    assert.equal(d.bench.startup_profile.rescue_sector_ms, 8);
    assert.equal(d.bench.startup_profile.closed_floor_end_ms, 450);
    assert.equal(d.bench.startup_profile.all_retries_count_as_native_timeouts, true);
  }
  if (d.header.variant === 8) {
    assert.deepEqual(d.bench.power_profile, {name:'S3-P', initial_request:540,
      acquisition_request:600, startup_cap:610, post_handoff_base:360,
      release_ms:20, legacy_extra_is_not_profile_amplitude:true});
    assert.equal(d.banks[0].records[0].commanded_us, null);
    assert.equal(d.banks[0].records[0].retry_deadline_us, 22500);
  }
  await assert.rejects(dump(elf, out, true, good.connect));
  for (const name of ['moving', 'halted', 'gates', 'pwm', 'wrongVTOR', 'wrongFlash', 'badABI', 'active', 'benchActive', 'changing', 'badReply']) {
    const api = mock({[name]: true}), target = path.join(temp, name + '.bin');
    await assert.rejects(dump(elf, target, true, api.connect), name);
    assert(api.closed && !fs.existsSync(target), name);
  }
  const corrupt = Buffer.from(fixture); corrupt.writeUInt32LE(wave ? 1000001 : 73, 64 + (wave ? 32 : 24));
  assert.throws(() => decode(corrupt)); assert.throws(() => decode(fixture.subarray(0, SIZE - 1)));
  console.log('PASS paired capture: real C ABI, identity, zero/gates/PWM, active/changing/corrupt buffers, overwrite refusal; read-only mock RPC only');
})().catch(e => {console.error(e); process.exitCode = 1;});
