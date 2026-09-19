// Verify a MAIN-FIRMWARE pair only. Never accepts BL/settings images.
const fs = require('node:fs');
const assert = require('node:assert/strict');
const crypto = require('node:crypto');
const [binPath, hexPath] = process.argv.slice(2);
if (!binPath || !hexPath) throw new Error('Usage: node tests/check_fd6288_image.cjs MAIN.bin MAIN.hex');
const bin = fs.readFileSync(binPath);
const hex = fs.readFileSync(hexPath);
const start = 0x08001000, end = 0x0800f800;
assert.equal(bin.length, end - start, 'main image must span exactly 58 KiB');
const sp = bin.readUInt32LE(0), reset = bin.readUInt32LE(4);
assert(sp > 0x20000000 && sp <= 0x20008000 && sp % 8 === 0, 'invalid stack');
assert((reset & 1) && reset >= start && reset < end, 'invalid Thumb reset vector');
let base = 0, eof = false, min = Infinity, max = 0, count = 0;
const addresses = new Set();
for (const line of hex.toString('ascii').trim().split(/\r?\n/)) {
  assert(!eof, 'record after EOF');
  assert(/^:[0-9a-fA-F]+$/.test(line) && line.length % 2 === 1, 'invalid HEX record');
  const record = Buffer.from(line.slice(1), 'hex');
  assert.equal(record.length, record[0] + 5, 'length mismatch');
  assert.equal(record.reduce((sum, n) => sum + n, 0) & 255, 0, 'checksum mismatch');
  const offset = record.readUInt16BE(1), type = record[3];
  if (type === 0) {
    for (let i = 0; i < record[0]; ++i) {
      const addr = base + offset + i;
      assert(addr >= start && addr < end, 'HEX would write outside main region');
      assert(!addresses.has(addr), 'duplicate HEX byte');
      addresses.add(addr);
      assert.equal(record[4 + i], bin[addr - start], 'BIN/HEX data mismatch');
      min = Math.min(min, addr); max = Math.max(max, addr); count++;
    }
  } else if (type === 4 || type === 2) {
    assert.equal(record[0], 2);
    base = record.readUInt16BE(4) * (type === 4 ? 65536 : 16);
  } else if (type === 1) {
    assert.equal(record[0], 0); eof = true;
  } else if (type === 5) {
    assert.equal(record[0], 4);
    assert.equal(record.readUInt32BE(4), reset, 'HEX entry/reset mismatch');
  } else {
    throw new Error(`Unexpected HEX type ${type}`);
  }
}
assert(eof && min === start && max === end - 1);
for (let i = 0; i < 8; ++i) assert(addresses.has(start + i));
for (let i = end - 32; i < end; ++i) assert(addresses.has(i), 'missing .file_name byte');
for (let i = 0; i < bin.length; ++i) {
  if (!addresses.has(start + i)) assert.equal(bin[i], 0, 'nonzero BIN data absent from HEX');
}
console.log(`PASS: ${count} HEX data bytes, all inside 0x${min.toString(16)}..0x${max.toString(16)}.`);
console.log(`BIN ${bin.length} bytes; initial SP 0x${sp.toString(16)}, reset 0x${reset.toString(16)}.`);
console.log('Bootloader and EEPROM have no HEX data records. BIN/HEX bytes agree.');
for (const [name, data] of [[binPath, bin], [hexPath, hex]]) {
  console.log(`${crypto.createHash('sha256').update(data).digest('hex')}  ${name}`);
}
