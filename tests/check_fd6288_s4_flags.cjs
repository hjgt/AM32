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
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1,FD6288_STARTUP_NOEDGE_LOG:1,FD6288_STARTUP_RESCUE_RAMP:1,FD6288_STARTUP_SPLIT_LOG:1,FD6288_STARTUP_FIRST_WAIT:1},24],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1,FD6288_STARTUP_NOEDGE_LOG:1,FD6288_STARTUP_RESCUE_RAMP:1,FD6288_STARTUP_SPLIT_LOG:1,FD6288_STARTUP_FIRST_WAIT:1,FD6288_ASYNC_COMM:1},25],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1,FD6288_STARTUP_NOEDGE_LOG:1,FD6288_STARTUP_RESCUE_RAMP:1,FD6288_STARTUP_SPLIT_LOG:1,FD6288_STARTUP_FIRST_WAIT:1,FD6288_ASYNC_COMM:1,FD6288_ADC_JEOS:1},26],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1,FD6288_STARTUP_NOEDGE_LOG:1,FD6288_STARTUP_RESCUE_RAMP:1,FD6288_STARTUP_SPLIT_LOG:1,FD6288_STARTUP_FIRST_WAIT:1,FD6288_ASYNC_COMM:1,FD6288_ADC_JEOS:1,FD6288_DENSE_LOG:1},27],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1,FD6288_STARTUP_NOEDGE_LOG:1,FD6288_STARTUP_RESCUE_RAMP:1,FD6288_STARTUP_SPLIT_LOG:1},23],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1,FD6288_STARTUP_NOEDGE_LOG:1,FD6288_STARTUP_RESCUE_RAMP:1},22],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1,FD6288_STARTUP_NOEDGE_LOG:1},21],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1,FD6288_STARTUP_SEED_DELAY:1},20],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1,FD6288_STARTUP_PHASE_DELAY:1},19],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1,FD6288_STARTUP_LOSS_LOG:1},18],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2,FD6288_STARTUP_S4E:1},17],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:2},16],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1,FD6288_STARTUP_ACQ_LOG:1},15],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1,FD6288_STARTUP_RESCUE_BRIDGE:1},14],
  [{FD6288_STARTUP_HANDOFF_REAL:1,FD6288_STARTUP_RESCUE_RETAIN:1},13],
  [{},11], [{FD6288_STARTUP_HANDOFF_REAL:1},12], [{FD6288_STARTUP_S4:0},10],
  [{FD6288_STARTUP_S4:0,FD6288_ZCD_ACTUAL_DEADTIME:0},9],
  [{FD6288_STARTUP_S4:0,FD6288_ZCD_ACTUAL_DEADTIME:0,FD6288_STARTUP_WAVE_TRACE:0},7],
]) {
  const r=preprocess(overrides); assert.equal(r.status,0,r.stderr);
  assert(new RegExp(`^#define FD6288_TRACE_VARIANT ${variant}u$`,'m').test(r.stdout));
  assert.equal(/^#define TARGET_STARTUP_S4[ \t]*$/m.test(r.stdout),variant>=11);
  assert.equal(/^#define TARGET_STARTUP_HANDOFF_REAL[ \t]*$/m.test(r.stdout),variant>=12);
  assert.equal(/^#define TARGET_STARTUP_RESCUE_RETAIN[ \t]*$/m.test(r.stdout),variant>=13);
  assert.equal(/^#define TARGET_STARTUP_RESCUE_BRIDGE[ \t]*$/m.test(r.stdout),variant>=14);
  assert.equal(/^#define TARGET_STARTUP_ACQ_LOG[ \t]*$/m.test(r.stdout),variant===15);
  assert.equal(/^#define TARGET_STARTUP_EDGE_LOG[ \t]*$/m.test(r.stdout),variant>=16);
  assert.equal(/^#define TARGET_STARTUP_S4E[ \t]*$/m.test(r.stdout),variant>=17);
  assert.equal(/^#define TARGET_STARTUP_LOSS_LOG[ \t]*$/m.test(r.stdout),variant>=18);
  assert.equal(/^#define TARGET_STARTUP_PHASE_DELAY[ \t]*$/m.test(r.stdout),variant>=19);
  assert.equal(/^#define TARGET_STARTUP_SEED_DELAY[ \t]*$/m.test(r.stdout),variant>=20);
  assert.equal(/^#define TARGET_STARTUP_NOEDGE_LOG[ \t]*$/m.test(r.stdout),variant>=21);
  assert.equal(/^#define TARGET_STARTUP_RESCUE_RAMP[ \t]*$/m.test(r.stdout),variant>=22);
  assert.equal(/^#define TARGET_STARTUP_SPLIT_LOG[ \t]*$/m.test(r.stdout),variant>=23);
  assert.equal(/^#define TARGET_STARTUP_FIRST_WAIT[ \t]*$/m.test(r.stdout),variant>=24);
}
for (const overrides of [
  {FD6288_DENSE_LOG:1}, {FD6288_DENSE_LOG:2},
  {FD6288_ADC_JEOS:1}, {FD6288_ADC_JEOS:2}, {FD6288_ASYNC_COMM:1}, {FD6288_ASYNC_COMM:2},
  {FD6288_STARTUP_FIRST_WAIT:1}, {FD6288_STARTUP_FIRST_WAIT:2}, {FD6288_STARTUP_FIRST_WAIT:-1},
  {FD6288_STARTUP_SPLIT_LOG:1}, {FD6288_STARTUP_SPLIT_LOG:2}, {FD6288_STARTUP_SPLIT_LOG:-1},
  {FD6288_STARTUP_NOEDGE_LOG:1}, {FD6288_STARTUP_NOEDGE_LOG:2}, {FD6288_STARTUP_NOEDGE_LOG:-1},
  {FD6288_STARTUP_SEED_DELAY:1}, {FD6288_STARTUP_SEED_DELAY:2}, {FD6288_STARTUP_SEED_DELAY:-1},
  {FD6288_STARTUP_PHASE_DELAY:1}, {FD6288_STARTUP_PHASE_DELAY:2}, {FD6288_STARTUP_PHASE_DELAY:-1},
  {FD6288_STARTUP_LOSS_LOG:1}, {FD6288_STARTUP_LOSS_LOG:2}, {FD6288_STARTUP_LOSS_LOG:-1},
  {FD6288_STARTUP_S4E:1}, {FD6288_STARTUP_S4E:2}, {FD6288_STARTUP_S4E:-1},
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
  {FD6288_STARTUP_RESCUE_BRIDGE:1}, {FD6288_STARTUP_RESCUE_BRIDGE:2},
  {FD6288_STARTUP_RESCUE_BRIDGE:-1},
  {FD6288_STARTUP_ACQ_LOG:1}, {FD6288_STARTUP_ACQ_LOG:2}, {FD6288_STARTUP_ACQ_LOG:3}, {FD6288_STARTUP_ACQ_LOG:-1},
]) assert.notEqual(preprocess(overrides).status,0,JSON.stringify(overrides));
const alignment = Object.fromEntries(['STARTUP_HANDOFF_REAL','STARTUP_RESCUE_RETAIN',
  'STARTUP_RESCUE_BRIDGE','STARTUP_S4E','STARTUP_LOSS_LOG','STARTUP_PHASE_DELAY',
  'STARTUP_SEED_DELAY','STARTUP_NOEDGE_LOG','STARTUP_RESCUE_RAMP','STARTUP_SPLIT_LOG',
  'STARTUP_FIRST_WAIT','ASYNC_COMM','ADC_JEOS','QUIET_LOG','DOUBLE_ALIGN']
  .map(k=>['FD6288_'+k,1]));
alignment.FD6288_STARTUP_ACQ_LOG=2;
const aligned=preprocess(alignment);
assert.equal(aligned.status,0,aligned.stderr);
assert.match(aligned.stdout,/#define FD6288_TRACE_VARIANT 28u/);
assert.match(aligned.stdout,/#define TARGET_FD_DOUBLE_ALIGN/);
assert.match(aligned.stdout,/#define TARGET_FD_QUIET_LOG/);
const budget=preprocess({...alignment,FD6288_ALIGN_BUDGET:1});
assert.equal(budget.status,0,budget.stderr);
assert.match(budget.stdout,/#define FD6288_TRACE_VARIANT 29u/);
const stats=preprocess({...alignment,FD6288_ALIGN_BUDGET:1,FD6288_SECTOR_DIAG:1});
assert.equal(stats.status,0,stats.stderr);
assert.match(stats.stdout,/#define FD6288_TRACE_VARIANT 30u/);
assert.notEqual(preprocess({...alignment,FD6288_SECTOR_DIAG:1}).status,0);
assert.notEqual(preprocess({...alignment,FD6288_ALIGN_BUDGET:1,FD6288_SECTOR_DIAG:2}).status,0);
assert.notEqual(preprocess({...alignment,FD6288_ALIGN_BUDGET:2}).status,0);
assert.notEqual(preprocess({FD6288_ALIGN_BUDGET:1}).status,0);
for(const bad of [{FD6288_QUIET_LOG:0},{FD6288_DENSE_LOG:1},{FD6288_ADC_JEOS:0},
  {FD6288_DOUBLE_ALIGN:2},{FD6288_QUIET_LOG:2}])
  assert.notEqual(preprocess({...alignment,...bad}).status,0);
const cleanFlags={...alignment,FD6288_ALIGN_BUDGET:1,FD6288_NO_DIAGNOSTICS:1,FD6288_NATIVE_EARLY:1};
const clean=preprocess(cleanFlags);
assert.equal(clean.status,0,clean.stderr);
assert.match(clean.stdout,/#define FD6288_TRACE_VARIANT 31u/);
assert.match(clean.stdout,/#define TARGET_FD_NO_DIAGNOSTICS/);
for(const bad of [{FD6288_ALIGN_BUDGET:0},{FD6288_QUIET_LOG:0},
 {FD6288_SECTOR_DIAG:1},{FD6288_NO_DIAGNOSTICS:2},{FD6288_NATIVE_EARLY:2}])
 assert.notEqual(preprocess({...cleanFlags,...bad}).status,0);
const scan=preprocess({...cleanFlags,FD6288_PIN_SCAN:1,FD6288_NATIVE_EARLY:0});
assert.equal(scan.status,0,scan.stderr);assert.match(scan.stdout,/#define FD6288_TRACE_VARIANT 32u/);
assert.match(scan.stdout,/#define TARGET_FD_PIN_SCAN/);
assert.notEqual(preprocess({...cleanFlags,FD6288_PIN_SCAN:2}).status,0);
assert.notEqual(preprocess({...cleanFlags,FD6288_PIN_SCAN:1,FD6288_NO_DIAGNOSTICS:0}).status,0);
const sync=preprocess({...cleanFlags,FD6288_PIN_SYNC:1,FD6288_NATIVE_EARLY:0});
assert.equal(sync.status,0,sync.stderr);assert.match(sync.stdout,/#define FD6288_TRACE_VARIANT 33u/);
assert.match(sync.stdout,/#define TARGET_FD_PIN_SYNC/);
for(const bad of [{FD6288_PIN_SYNC:2},{FD6288_PIN_SYNC:1,FD6288_PIN_SCAN:1},
 {FD6288_PIN_SYNC:1,FD6288_NO_DIAGNOSTICS:0}])assert.notEqual(preprocess({...cleanFlags,...bad}).status,0);
const hold=preprocess({...cleanFlags,FD6288_ALIGN_HOLD:1});
assert.equal(hold.status,0,hold.stderr);
assert.match(hold.stdout,/#define FD6288_TRACE_VARIANT 34u/);
assert.match(hold.stdout,/#define TARGET_FD_ALIGN_HOLD/);
for(const bad of [{FD6288_ALIGN_HOLD:2},{FD6288_NATIVE_EARLY:0},
 {FD6288_DOUBLE_ALIGN:0},{FD6288_PIN_SCAN:1},{FD6288_PIN_SYNC:1}])
 assert.notEqual(preprocess({...cleanFlags,FD6288_ALIGN_HOLD:1,...bad}).status,0);
const acq=preprocess({...cleanFlags,FD6288_ALIGN_HOLD:1,FD6288_ACQ_STATS:1});
assert.equal(acq.status,0,acq.stderr);assert.match(acq.stdout,/#define FD6288_TRACE_VARIANT 35u/);
assert.match(acq.stdout,/#define TARGET_FD_SECTOR_DIAG/);
assert.match(acq.stdout,/#define TARGET_FD_NO_DIAGNOSTICS/);
for(const bad of [{FD6288_ACQ_STATS:2},{FD6288_NATIVE_EARLY:0},{FD6288_NO_DIAGNOSTICS:0},
 {FD6288_SECTOR_DIAG:1},{FD6288_PIN_SCAN:1},{FD6288_PIN_SYNC:1}])
 assert.notEqual(preprocess({...cleanFlags,FD6288_ACQ_STATS:1,...bad}).status,0);
const launchFlags={...cleanFlags,FD6288_ALIGN_HOLD:1,FD6288_ACQ_STATS:1,FD6288_LAUNCH_ADVANCE:1};
const launch=preprocess(launchFlags);
assert.equal(launch.status,0,launch.stderr);
assert.match(launch.stdout,/#define FD6288_TRACE_VARIANT 36u/);
assert.match(launch.stdout,/#define TARGET_FD_LAUNCH_ADVANCE/);
for(const bad of [{FD6288_LAUNCH_ADVANCE:2},{FD6288_ACQ_STATS:0},{FD6288_ALIGN_HOLD:0},
 {FD6288_NATIVE_EARLY:0},{FD6288_PIN_SCAN:1}])
 assert.notEqual(preprocess({...launchFlags,...bad}).status,0);
const settle=preprocess({...launchFlags,FD6288_ALIGN_SETTLE:1});
assert.equal(settle.status,0,settle.stderr);assert.match(settle.stdout,/#define FD6288_TRACE_VARIANT 37u/);
for(const bad of [{FD6288_ALIGN_SETTLE:2},{FD6288_LAUNCH_ADVANCE:0}])
 assert.notEqual(preprocess({...launchFlags,FD6288_ALIGN_SETTLE:1,...bad}).status,0);
const repositionFlags={...launchFlags,FD6288_ALIGN_SETTLE:1,FD6288_REPOSITION:1};
const reposition=preprocess(repositionFlags);
assert.equal(reposition.status,0,reposition.stderr);
assert.match(reposition.stdout,/#define FD6288_TRACE_VARIANT 40u/);
for(const bad of [{FD6288_REPOSITION:2},{FD6288_ALIGN_SETTLE:0},{FD6288_PLANNED_ACQ:1}])
 assert.notEqual(preprocess({...repositionFlags,...bad}).status,0);
const edgeFlags={...launchFlags,FD6288_ALIGN_SETTLE:1,FD6288_EDGE_OBSERVE:1};
const edge=preprocess(edgeFlags);assert.equal(edge.status,0,edge.stderr);
assert.match(edge.stdout,/#define FD6288_TRACE_VARIANT 41u/);
assert.match(edge.stdout,/#define TARGET_FD_EDGE_OBSERVE/);
assert.doesNotMatch(edge.stdout,/#define TARGET_FD_REPOSITION\b|#define TARGET_FD_PLANNED_ACQ\b/);
for(const bad of [{FD6288_EDGE_OBSERVE:3},{FD6288_ALIGN_SETTLE:0},{FD6288_REPOSITION:1},{FD6288_PLANNED_ACQ:1}])
 assert.notEqual(preprocess({...edgeFlags,...bad}).status,0);
const fullRetryFlags={...repositionFlags,FD6288_FULL_RETRY:1};
const fullRetry=preprocess(fullRetryFlags);
assert.equal(fullRetry.status,0,fullRetry.stderr);
assert.match(fullRetry.stdout,/#define FD6288_TRACE_VARIANT 42u/);
for(const bad of [{FD6288_FULL_RETRY:2},{FD6288_REPOSITION:0},{FD6288_EDGE_OBSERVE:1}])
 assert.notEqual(preprocess({...fullRetryFlags,...bad}).status,0);
console.log('PASS actual target flags through S4T finite full retry and incompatible builds rejected');
const adjacentFlags={...fullRetryFlags,FD6288_RETRY_ADJACENT:1};
const adjacent=preprocess(adjacentFlags);
assert.equal(adjacent.status,0,adjacent.stderr);
assert.match(adjacent.stdout,/#define FD6288_TRACE_VARIANT 43u/);
for(const bad of [{FD6288_RETRY_ADJACENT:2},{FD6288_FULL_RETRY:0}])
 assert.notEqual(preprocess({...adjacentFlags,...bad}).status,0);
console.log('PASS S4U adjacent retry flags');
const staleFlags={...adjacentFlags,FD6288_STALE_RETRY:1};
const stale=preprocess(staleFlags);assert.equal(stale.status,0,stale.stderr);
assert.match(stale.stdout,/#define FD6288_TRACE_VARIANT 45u/);
for(const bad of [{FD6288_STALE_RETRY:2},{FD6288_RETRY_ADJACENT:0}])
 assert.notEqual(preprocess({...staleFlags,...bad}).status,0);
console.log('PASS S4V stale feedback retry flags');
