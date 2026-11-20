const assert=require('node:assert/strict');
const {decodeM2Wire}=require('../tools/scripts/fd6288_native_sine_capture.cjs');
const b=Buffer.alloc(1056);[0x464d5731,1,7,130,0,0,6400,100].forEach((v,i)=>b.writeUInt32LE(v,i*4));
for(let i=2;i<130;i++){
 const at=32+(i%128)*8;b.writeUInt32LE(i*160000,at);b.writeUInt16LE(1250,at+4);b.writeUInt16LE((2<<9)|312,at+6);
}
let d=decodeM2Wire(b,24);assert.equal(d.samples.length,128);assert.equal(d.samples[0].at_ms,2);assert.equal(d.samples[127].at_ms,129);
assert.equal(d.samples[0].wire_period_us,1248);assert(Math.abs(d.samples[0].wire_mechanical_rpm-4006.4103)<0.001);
assert.equal(d.max_correction_us,40);assert.throws(()=>decodeM2Wire(b,0));
b.writeUInt16LE(0xfff,32+(129%128)*8+6);assert.equal(decodeM2Wire(b).samples[127].wire_mechanical_rpm,0);
for(let i=2;i<130;i++)b.writeUInt32LE((0xffff0000+(i-2)*160000)>>>0,32+(i%128)*8);
assert.equal(decodeM2Wire(b).samples[127].window_us,127000);
console.log('PASS actual DShot period/payload units, 24-pole RPM, ring order and stopped sentinel');
