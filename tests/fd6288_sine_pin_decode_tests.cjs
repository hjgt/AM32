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
