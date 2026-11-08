const assert=require('assert/strict');
const {SIZE,decodePins}=require('../tools/scripts/fd6288_sine_pin_decode.cjs');
const pwm={trial:3,psc:0,cr1:0x81,bdtr:0x200a0ce,ccer:0x1555,arr:6665,ccr1:3603,ccr2:3063,ccr3:3597,gpioa_moder:0xaaaaffff,gpioa_afrh:0x66666,gpiof_moder:2,gpiof_afrl:6};
function buffer(rows){const b=Buffer.alloc(SIZE);[0x46504931,1,3,rows.length,1,1234].forEach((v,i)=>b.writeUInt32LE(v,i*4));rows.forEach(([before,after,a,f],i)=>{const p=24+i*12;b.writeUInt16LE(before,p);b.writeUInt16LE(after,p+2);b.writeUInt16LE(a,p+4);b.writeUInt16LE(f,p+6);b.writeUInt32LE(160000+i*100,p+8);});return b;}
const b=buffer([[100,110,0,0],[1000,1010,0x700,0],[3100,3110,0x500,0],[3500,3510,0x1500,0],[3800,3810,0x1000,0],[4000,4010,0x1800,1],[3060,3070,0,0],[6660,5,0,0]]);
const ok=decodePins(b,pwm);assert.equal(ok.analysis.valid,6);assert.equal(ok.analysis.excluded,2);assert.equal(ok.analysis.matched,6);assert.equal(ok.analysis.expected_ab_different,2);assert.equal(ok.analysis.verdict,'matches_at_sampled_times');
const bad=Buffer.from(b);bad.writeUInt16LE(0x700,24+2*12+4);assert.equal(decodePins(bad,pwm).analysis.mismatched,1);
assert.equal(decodePins(buffer([[1000,1010,0x700,0]]),pwm).analysis.verdict,'insufficient_mixed_channel_coverage');
assert.equal(decodePins(b,{...pwm,bdtr:70}).analysis.verdict,'unsupported_timer_or_pin_configuration');
assert.throws(()=>decodePins(b.subarray(0,SIZE-1),pwm));assert.throws(()=>decodePins(b,{...pwm,trial:4}));
for(const [off,val] of [[0,0],[4,2],[12,257],[16,0]]){const x=Buffer.from(b);x.writeUInt32LE(val,off);assert.throws(()=>decodePins(x,pwm));}
console.log('PASS pin decoder: manually specified high/low levels, phase differences, mismatch, boundary/wrap exclusions, insufficient coverage and ABI guards');
const pwm12={...pwm,arr:13330,ccr1:7205,ccr2:6125,ccr3:7193};
const ok12=decodePins(buffer([[1000,1010,0x700,0],[6300,6310,0x500,0],[6700,6710,0x1500,0],[7400,7410,0x1000,0],[7600,7610,0x1800,1],[6120,6130,0,0]]),pwm12);
assert.equal(ok12.analysis.valid,5);assert.equal(ok12.analysis.excluded,1);assert.equal(ok12.analysis.matched,5);assert.equal(ok12.analysis.expected_ab_different,2);
const {decodeSensors}=require('../tools/scripts/fd6288_native_sine_capture.cjs');
const sensor=Buffer.alloc(40);[0x46535331,1,3,95,0,53,820,805,10,30].forEach((v,i)=>sensor.writeUInt32LE(v,i*4));sensor.writeInt32LE(-3,16);
const decoded=decodeSensors(sensor);assert.equal(decoded.updates,95);assert.equal(decoded.baseline_current,-3);assert.equal(decoded.peak_current_increase,56);assert.equal(decoded.voltage_drop,15);assert(decoded.sampled_only);
assert.throws(()=>decodeSensors(sensor.subarray(0,39)));const wrong=Buffer.from(sensor);wrong.writeUInt32LE(2,4);assert.throws(()=>decodeSensors(wrong));
console.log('PASS 12k pin timing and sensor summary: signed units, peak delta, voltage drop and ABI guards');
let alignModes=0;
for(let pin=8;pin<=12;pin++)alignModes|=((pin===8||pin===11)?2:1)<<(2*pin);
const align={...pwm,ccr1:1200,ccr2:1200,ccr3:1200,ccr4:0,gpioa_moder:alignModes,gpiof_moder:1};
const alignData=buffer([[100,110,0,1],[500,510,0x100,1],[1300,1310,0,1],[2000,2010,0x800,1],[1195,1205,0,1]]);
const alignOk=decodePins(alignData,align);assert.equal(alignOk.analysis.drive_mode,'sixstep_alignment_step6');assert.equal(alignOk.analysis.valid,4);assert.equal(alignOk.analysis.matched,4);assert.equal(alignOk.analysis.excluded,1);
const lowMissing=Buffer.from(alignData);lowMissing.writeUInt16LE(0,24+12+6);assert.equal(decodePins(lowMissing,align).analysis.mismatched,1);
assert.equal(decodePins(alignData,{...align,ccr4:600}).analysis.configured,false);
console.log('PASS six-step pad decoder: A complementary PWM, B commands off, C low on; detects missing low command and excludes transitions');
