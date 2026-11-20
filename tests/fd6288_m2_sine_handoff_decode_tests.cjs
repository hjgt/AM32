const assert=require('node:assert/strict');
const {decodeM2Recovery,decodeM2Closed}=require('../tools/scripts/fd6288_native_sine_capture.cjs');

const recovery=Buffer.alloc(3120);
[0x464d5231,1,9,1,320000000,324800000,266640,270000,6,4,960,256]
  .forEach((v,i)=>recovery.writeUInt32LE(v,i*4));
for(let i=0;i<256;i++){
  const at=48+i*12;
  recovery.writeUInt32LE(i*20000,at);
  recovery.writeUInt16LE(100,at+4);recovery.writeUInt16LE(200,at+6);recovery.writeUInt16LE(50,at+8);
}
let d=decodeM2Recovery(recovery);
assert.equal(d.off_ms,2000);assert.equal(d.resumed_ms,2030);
assert.equal(d.first_sector,6);assert.equal(d.step,4);
assert.equal(d.reference_period_us,1666.5);assert.equal(d.samples.length,256);
assert.equal(d.samples[255].us,31875);assert.equal(d.samples[0].ab,-100);
recovery.writeUInt32LE(0xffffffff,20);assert.equal(decodeM2Recovery(recovery).resumed_ms,null);
recovery.writeUInt32LE(257,44);assert.throws(()=>decodeM2Recovery(recovery));
assert.throws(()=>decodeM2Recovery(recovery.subarray(0,3119)));

const closed=Buffer.alloc(528);
[0x464d4c31,1,9,40].forEach((v,i)=>closed.writeUInt32LE(v,i*4));
for(let i=8;i<40;i++){
  const at=16+(i%32)*16;
  closed.writeUInt32LE(i*160000,at);closed.writeUInt32LE(3200,at+4);
  closed.writeUInt16LE(1334,at+8);closed.writeUInt16LE(960,at+10);
  closed.writeUInt8(i%6+1,at+12);closed.writeUInt8(i===39?2:i%2,at+13);
}
d=decodeM2Closed(closed);
assert.equal(d.events.length,32);assert.equal(d.events[0].at_ms,8);assert.equal(d.events[31].at_ms,39);
assert.equal(d.events[31].event,'stop');assert.equal(d.events[31].elapsed_us,1600);
assert.equal(d.events[31].trigger_ticks,960);
closed.writeUInt32LE(0,12);assert.deepEqual(decodeM2Closed(closed).events,[]);
closed.writeUInt32LE(1,12);closed.writeUInt8(7,28);assert.throws(()=>decodeM2Closed(closed));
assert.throws(()=>decodeM2Closed(Buffer.alloc(528)));
console.log('PASS recovery coast units, capacity, pending resumption and closed-loop ring order, stop state and ABI checks');
