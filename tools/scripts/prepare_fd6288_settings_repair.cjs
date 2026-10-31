// Offline, one-incident settings repair. Does NOT connect to or flash hardware.
// Input is a fresh full 64 KiB backup. Only the ten explicitly approved bytes
// are restored; direction, stuck-rotor switch and the rest of the page stay put.
const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const assert = require('node:assert/strict');

const [beforePath, outputPrefix] = process.argv.slice(2);
assert(beforePath && outputPrefix,
    'Usage: node tools/scripts/prepare_fd6288_settings_repair.cjs BEFORE_64K.bin OUTPUT_PREFIX');
const root = path.resolve(__dirname, '../..');
const releaseDir = path.join(root, 'release-assets/fd6288-g431-final-2026-09-15');
const baseline = fs.readFileSync(path.join(releaseDir, 'AM32_FD6288_G431_SETTINGS_FINAL.bin'));
const baselineHex = fs.readFileSync(path.join(releaseDir, 'AM32_FD6288_G431_SETTINGS_FINAL.hex'));
const hash = data => crypto.createHash('sha256').update(data).digest('hex');
assert.equal(hash(baseline), '3d2b86e38d037c7979719831ea29ba65b7dc6519bce717c03a695c38da37e4cb');
assert.equal(hash(baselineHex), '7beb53f5d2ccb71b6ba4c8424a18113bd827d02f78d11d833d5f15a2730454a9');

function decodeSettingsHex(hex) {
    const out = Buffer.alloc(2048), seen = new Set();
    let base = 0, eof = false;
    for (const line of hex.toString('ascii').trim().split(/\r?\n/)) {
        assert(!eof && /^:[0-9a-fA-F]+$/.test(line) && line.length % 2 === 1);
        const b = Buffer.from(line.slice(1), 'hex');
        assert.equal(b.length, b[0] + 5);
        assert.equal(b.reduce((s, v) => s + v, 0) & 255, 0);
        const type = b[3];
        if (type === 0) {
            for (let i = 0; i < b[0]; i++) {
                const offset = base + b.readUInt16BE(1) + i - 0x0800f800;
                assert(offset >= 0 && offset < 2048, 'HEX escapes settings page');
                assert(!seen.has(offset), 'overlapping HEX data');
                seen.add(offset); out[offset] = b[4 + i];
            }
        } else if (type === 4) {
            assert.equal(b[0], 2); base = b.readUInt16BE(4) * 65536;
        } else if (type === 1) {
            assert.equal(b[0], 0); eof = true;
        } else if (type === 5) {
            // Historical objcopy output carries a start-address record.
            // It writes no bytes and must NOT be used as an execution entry.
            assert.equal(b[0], 4);
            assert.equal(b.readUInt32BE(4), 0x0800f800);
        } else {
            throw new Error(`Unexpected HEX record type ${type}`);
        }
    }
    assert(eof && seen.size === 2048, 'incomplete settings page');
    return out;
}
assert.deepEqual(decodeSettingsHex(baselineHex), baseline, 'published HEX/BIN mismatch');

const before = fs.readFileSync(beforePath);
assert.equal(before.length, 65536, 'need a complete fresh 64 KiB backup');
const original = Buffer.from(before.subarray(0xf800));
assert.deepEqual(Array.from(original.subarray(0, 5)), [1, 3, 17, 2, 20]);
assert.equal(original.subarray(5, 13).toString('ascii'), 'Unknown ', 'unexpected starting settings');
assert.equal(original[17], 0, 'direction changed: review before repairing');
assert.equal(original[22], 0, 'stuck-rotor switch changed: review before repairing');
assert.equal(original[24], 48);
assert.equal(original[47], 255);
const fields = new Map([
    [5, 'max_ramp'], [6, 'minimum_duty_cycle'], [7, 'disable_stick_calibration'],
    [8, 'absolute_voltage_cutoff'], [9, 'current_P'], [10, 'current_I'],
    [11, 'current_D'], [12, 'active_brake_power'], [24, 'pwm_frequency'],
    [47, 'auto_advance'],
]);
const repaired = Buffer.from(original);
const changes = [];
for (const [offset, field] of fields) {
    repaired[offset] = baseline[offset];
    changes.push({ offset, address: `0x${(0x0800f800 + offset).toString(16)}`,
        field, before: original[offset], after: repaired[offset] });
}
for (let i = 0; i < 2048; i++) {
    if (!fields.has(i)) assert.equal(repaired[i], original[i], `unapproved change at ${i}`);
}
function record(type, address, data) {
    const bytes = [data.length, address >> 8, address & 255, type, ...data];
    bytes.push((-bytes.reduce((s, v) => s + v, 0)) & 255);
    return ':' + Buffer.from(bytes).toString('hex').toUpperCase();
}
const records = [record(4, 0, [8, 0])];
for (let i = 0; i < 2048; i += 16) records.push(record(0, 0xf800 + i, repaired.subarray(i, i + 16)));
records.push(record(1, 0, []));
const repairedHex = Buffer.from(records.join('\n') + '\n');
assert.deepEqual(decodeSettingsHex(repairedHex), repaired);
const manifest = {
    purpose: 'SWD settings-only repair; not an am32.ca main-firmware upload',
    address: '0x0800F800', length: 2048, preserved: 'all bytes except the ten listed changes',
    beforeFlashSHA256: hash(before), beforeSettingsSHA256: hash(original),
    repairedSettingsSHA256: hash(repaired), repairedHexSHA256: hash(repairedHex), changes,
};
// Exclusive creation protects earlier backups and generated artifacts.
for (const suffix of ['.bin', '.hex', '.json']) assert(!fs.existsSync(outputPrefix + suffix));
fs.writeFileSync(outputPrefix + '.bin', repaired, { flag: 'wx' });
fs.writeFileSync(outputPrefix + '.hex', repairedHex, { flag: 'wx' });
fs.writeFileSync(outputPrefix + '.json', JSON.stringify(manifest, null, 2) + '\n', { flag: 'wx' });
console.log(JSON.stringify(manifest, null, 2));
