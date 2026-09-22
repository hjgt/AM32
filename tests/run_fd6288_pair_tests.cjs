const fs = require('node:fs'), path = require('node:path'), os = require('node:os');
const {spawnSync} = require('node:child_process');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const read = p => fs.readFileSync(path.join(root, p), 'utf8');
const strip = s => s.replace(/^#include[^\n]*$/gm, '').replace(/^#pragma once$/gm, '');
const impl = ['Inc/fd6288_startup_pair.h', 'Inc/fd6288_startup_trace.h', 'Src/fd6288_startup_pair.c'].map(p => strip(read(p))).join('\n');
const source = read('tests/fd6288_pair_tests.c').replace('/* PAIR_IMPLEMENTATION */', impl);
for (const variant of [6, 7, 8]) {
const binary = path.join(fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-pair-')), 'test');
const fixture = binary + '.bin';
for (const [cmd, args, opts] of [
  [process.env.CC || 'cc', [`-DFD6288_TRACE_VARIANT=${variant}`, ...(variant >= 7 ? ['-DTARGET_STARTUP_FEEDBACK'] : []), '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror', '-fsanitize=address,undefined', '-x', 'c', '-', '-o', binary], {input: source}],
  [binary, [fixture], {}],
]) {
  const p = spawnSync(cmd, args, {encoding: 'utf8', ...opts});
  process.stdout.write(p.stdout || ''); process.stderr.write(p.stderr || '');
  if (p.status !== 0) process.exit(p.status || 1);
}
const {decode} = require('../tools/scripts/fd6288_pair_capture.cjs');
const d = decode(fs.readFileSync(fixture));
assert.equal(d.header.variant, variant);
assert.deepEqual(d.banks.map(b => [b.header.trial_id, b.classification]), [[1, 'failure'], [6, 'success']]);
assert.equal(d.banks[0].records[0].reverse_crossings, 2);
assert.equal(d.banks[0].records[1].sector_percent, 80);
assert.equal(d.banks[0].records[0].first_adc[2], 780);
assert.equal(d.banks[0].records[0].accepted_at_us, 200);
assert.equal(d.banks[1].header.closed_zc, 1);
console.log('PASS actual C binary layout -> JavaScript decoder');
if (process.argv[2]) {
  const p = spawnSync(process.execPath, [path.join(__dirname, 'fd6288_pair_capture_tests.cjs'), process.argv[2], fixture], {encoding: 'utf8'});
  process.stdout.write(p.stdout || ''); process.stderr.write(p.stderr || '');
  if (p.status !== 0) process.exit(p.status || 1);
}
}
