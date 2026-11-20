const assert = require('node:assert/strict');
const {execFileSync} = require('node:child_process');
const fs = require('node:fs');
const path = require('node:path');
const {connect, parseHex} = require('./fd6288_startup_capture.cjs');

const headerSize = 20, sectorSize = 528, sampleSize = 16;
const symbolName = 'fd_release_closed_capture';

function decodeReleaseCapture(bytes) {
  assert.equal(bytes.readUInt32LE(0), 0x46524331);
  assert.equal(bytes.readUInt32LE(4), 1);
  const trial = bytes.readUInt32LE(8);
  const handoffCycles = bytes.readUInt32LE(12);
  const count = bytes.readUInt32LE(16);
  assert(count <= 12);
  const sectors = Array.from({length: count}, (_, i) => {
    const start = headerSize + i * sectorSize;
    const at = bytes.readUInt32LE(start);
    const edge = bytes.readUInt32LE(start + 4);
    const period = bytes.readUInt16LE(start + 8);
    const edgePeriod = bytes.readUInt16LE(start + 10);
    const step = bytes.readUInt8(start + 12);
    const rising = bytes.readUInt8(start + 13) !== 0;
    const samples = bytes.readUInt8(start + 14);
    assert(step >= 1 && step <= 6 && samples <= 32);
    return {step, rising, at_us: at / 160,
      edge_from_step_us: edge === 0xffffffff ? null : ((edge - at) >>> 0) / 160,
      interval_us: period / 2, edge_interval_us: edgePeriod ? edgePeriod / 2 : null,
      estimated_mechanical_rpm: period ? 20000000 / (period * 12) : null,
      samples: Array.from({length: samples}, (_, j) => {
        const p = start + 16 + j * sampleSize;
        const a = bytes.readUInt16LE(p + 4), b = bytes.readUInt16LE(p + 6), c = bytes.readUInt16LE(p + 8);
        const floating = [c, a, b, c, a, b][step - 1];
        const neutral = Math.floor((a + b + c) / 3);
        return {us: bytes.readUInt32LE(p) / 160, a, b, c,
          margin: rising ? floating - neutral : neutral - floating,
          duty_ticks: bytes.readUInt16LE(p + 10), trigger_ticks: bytes.readUInt16LE(p + 12),
          read_counter: bytes.readUInt16LE(p + 14)};
      })};
  });
  return {trial, handoff_cycles: handoffCycles, sectors};
}

async function dump(elf, out) {
  const symbols = Object.create(null);
  const rows = execFileSync('arm-none-eabi-nm', ['-S', '--defined-only', elf], {encoding: 'utf8'});
  for (const line of rows.split('\n')) {
    const match = line.match(/^([0-9a-f]+) ([0-9a-f]+) \w (.+)$/);
    if (match) symbols[match[3]] = {address: parseInt(match[1], 16), size: parseInt(match[2], 16)};
  }
  assert.equal(symbols[symbolName]?.size, 6356, 'Matching Release capture ELF required');
  const hex = parseHex(fs.readFileSync(elf.replace(/\.elf$/, '.hex'), 'utf8'));
  const link = await connect();
  try {
    async function mem(addr, length) {
      const raw = await link.rpc(`read_memory 0x${addr.toString(16)} 8 ${length}`);
      const values = raw.trim().split(/\s+/).map(Number);
      assert.equal(values.length, length);
      assert(values.every(x => Number.isInteger(x) && x >= 0 && x <= 255));
      return Buffer.from(values);
    }
    async function stopped() {
      assert.equal((await link.rpc('[target current] curstate')).trim(), 'running');
      assert.equal((await mem(0xe000ed08, 4)).readUInt32LE(), 0x08001000);
      for (const name of ['input', 'newinput', 'adjusted_input', 'running', 'duty_cycle', 'heli_coast_active']) {
        const s = symbols[name];
        assert.equal((await mem(s.address, s.size)).readUIntLE(0, s.size), 0, name + ' active');
      }
    }
    await stopped();
    const code = symbols.fdAdcJeosService;
    const actual = await mem(code.address, code.size);
    for (let i = 0; i < actual.length; i++)
      assert.equal(actual[i], hex.get(code.address + i), 'Capture code identity');
    const recorder = symbols[symbolName];
    const header = await mem(recorder.address, headerSize);
    const count = header.readUInt32LE(16);
    assert(count <= 12);
    const chunks = [header];
    for (let i = 0; i < count; i++) chunks.push(await mem(recorder.address + headerSize + i * sectorSize, sectorSize));
    assert.deepEqual(await mem(recorder.address, headerSize), header, 'Capture changed during read');
    await stopped();
    const settings = await mem(symbols.eepromBuffer.address, 48);
    const record = {read_at: new Date().toISOString(), elf: path.resolve(elf),
      settings: {pwm_khz: settings[24], startup_power: settings[25], motor_kv: settings[26]*40+20,
        motor_poles: settings[27], timing_raw: settings[23], auto_advance: settings[47]},
      ...decodeReleaseCapture(Buffer.concat(chunks))};
    fs.writeFileSync(out, JSON.stringify(record, null, 2) + '\n', {flag: 'wx'});
    console.log(JSON.stringify({...record, sectors: record.sectors.map(({samples, ...s}) => ({...s, samples: samples.length}))}, null, 2));
  } finally {
    link.close();
  }
}

if (require.main === module) {
  const [elf, out, ...args] = process.argv.slice(2);
  assert(elf && out && args.includes('--stopped'), 'Usage: node fd6288_release_closed_capture.cjs MATCHING.elf NEW.json --stopped');
  dump(elf, out).catch(err => { console.error(err); process.exitCode = 1; });
}
module.exports = {decodeReleaseCapture};
