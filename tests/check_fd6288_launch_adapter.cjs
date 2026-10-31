// Structural integration guard in addition to C policy/vector tests.
const fs=require('node:fs'),assert=require('node:assert/strict');
const s=fs.readFileSync(require('node:path').join(__dirname,'../Src/main.c'),'utf8');
const start=s.indexOf('if (action == FD_REV_LAUNCH) {');assert(start>0);
const end=s.indexOf('\n#endif',start),b=s.slice(start,end);
assert(b.indexOf('allOff();')>=0);
assert(b.indexOf('allOff();')<b.indexOf('uint32_t off_at'));
assert(b.indexOf('while ((uint32_t)(DWT->CYCCNT - off_at) < 5u * FD_REV_US) {}')<b.indexOf('step = fdDoubleAlignLaunchSeed();'));
assert(!b.includes('comStep(')&&!b.includes('commutate('));
const branch=s.slice(end,s.indexOf('\nstatic void fdRevObserveEdge',end));
assert.equal((branch.match(/commutate\(\);/g)||[]).length,1);
assert(branch.includes('if (action == FD_REV_RETRY)'));
console.log('PASS launch adapter structure: all-off gap before seed, one shared commutation, retry-only timeout accounting');
