// Preprocess the actual target definition; no firmware or hardware writes.
const fs = require('node:fs'), path = require('node:path');
const assert = require('node:assert/strict');
const {spawnSync} = require('node:child_process');
const header = fs.readFileSync(path.join(__dirname, '../Inc/targets.h'), 'utf8');
const start = header.indexOf('#ifdef CUSTOM_FD6288_G431');
const input = header.slice(start, header.indexOf('#ifndef FIRMWARE_NAME', start));
assert(start >= 0 && input.length > 1000);
const base = {CUSTOM_FD6288_G431:1, FD6288_STARTUP_TRACE:1,
  FD6288_STARTUP_TIMING_FIX:1, FD6288_STARTUP_SEQUENCE:3,
  FD6288_STARTUP_PAIR_TRACE:1, FD6288_STARTUP_S3_POWER:0,
  FD6288_STARTUP_POWER_BENCH:0, FD6288_STARTUP_POWER_EXTRA:40,
  FD6288_STARTUP_WAVE_TRACE:1, FD6288_ZCD_ACTUAL_DEADTIME:1,
  FD6288_STARTUP_S4:1};
function preprocess(overrides) {
  return spawnSync(process.env.CC || 'cc', ['-E','-dM','-x','c',
    ...Object.entries({...base,...overrides}).map(([k,v])=>`-D${k}=${v}`), '-'],
    {input,encoding:'utf8'});
}
for (const [overrides,variant] of [
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1},13],
  [{},11], [{FD6288_STARTUP_HANDOFF_REAL:1},12], [{FD6288_STARTUP_S4:0},10],
  [{FD6288_STARTUP_S4:0,FD6288_ZCD_ACTUAL_DEADTIME:0},9],
  [{FD6288_STARTUP_S4:0,FD6288_ZCD_ACTUAL_DEADTIME:0,FD6288_STARTUP_WAVE_TRACE:0},7],
]) {
  const r=preprocess(overrides); assert.equal(r.status,0,r.stderr);
  assert(new RegExp(`^#define FD6288_TRACE_VARIANT ${variant}u$`,'m').test(r.stdout));
  assert.equal(/^#define TARGET_STARTUP_S4[ \t]*$/m.test(r.stdout),variant>=11);
  assert.equal(/^#define TARGET_STARTUP_HANDOFF_REAL[ \t]*$/m.test(r.stdout),variant>=12);
  assert.equal(/^#define TARGET_STARTUP_RESCUE_RETAIN[ \t]*$/m.test(r.stdout),variant===13);
}
for (const overrides of [
  {FD6288_STARTUP_S4:2}, {FD6288_STARTUP_S4:-1},
  {FD6288_STARTUP_SEQUENCE:0}, {FD6288_STARTUP_SEQUENCE:2},
  {FD6288_STARTUP_TRACE:0}, {FD6288_STARTUP_TIMING_FIX:0},
  {FD6288_STARTUP_PAIR_TRACE:0}, {FD6288_STARTUP_WAVE_TRACE:0},
  {FD6288_ZCD_ACTUAL_DEADTIME:0}, {FD6288_STARTUP_S3_POWER:1},
  {FD6288_STARTUP_POWER_BENCH:1}, {FD6288_STARTUP_POWER_EXTRA:80},
  {FD6288_STARTUP_HANDOFF_REAL:2}, {FD6288_STARTUP_HANDOFF_REAL:-1},
  {FD6288_STARTUP_S4:0,FD6288_STARTUP_HANDOFF_REAL:1},
  {FD6288_STARTUP_RESCUE_RETAIN:1}, {FD6288_STARTUP_RESCUE_RETAIN:2},
  {FD6288_STARTUP_RESCUE_RETAIN:-1},
]) assert.notEqual(preprocess(overrides).status,0,JSON.stringify(overrides));
console.log('PASS actual target flags: S3/S3-D/S3-DT/S4 identities; incompatible S4 builds rejected');
