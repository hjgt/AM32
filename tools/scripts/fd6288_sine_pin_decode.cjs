const assert = require('assert/strict');
const SIZE = 3096;
function decodePins(b, pwm) {
  assert.equal(b.length, SIZE);
  const header = Object.fromEntries(['magic','abi','trial','count','frozen','driven_at'].map((n,i)=>[n,b.readUInt32LE(i*4)]));
  assert.equal(header.magic, 0x46504931); assert.equal(header.abi, 1);
  assert(header.count <= 256); assert.equal(header.frozen, 1);
  assert.equal(header.trial, pwm.trial);
  const samples = [];
  for(let i=0;i<header.count;i++) {
    const p=24+i*12;
    samples.push({before:b.readUInt16LE(p),after:b.readUInt16LE(p+2),gpioa:b.readUInt16LE(p+4),gpiof:b.readUInt16LE(p+6),at:b.readUInt32LE(p+8)});
  }
  const ccr=[pwm.ccr1,pwm.ccr2,pwm.ccr3], period=pwm.arr+1, dt=368;
  let configured = pwm.psc===0 && !(pwm.cr1&0x302) && (pwm.bdtr&255)===0xce &&
    (pwm.bdtr&0x8000)!==0 && (pwm.ccer&0xfff)===0x555 &&
    period>0 && period<65536 && ccr.every(v=>v>dt && v+dt<period);
  for(let pin=8;pin<=12;pin++)configured &&= ((pwm.gpioa_moder>>>(pin*2))&3)===2 && ((pwm.gpioa_afrh>>>((pin-8)*4))&15)===6;
  configured &&= (pwm.gpiof_moder&3)===2 && (pwm.gpiof_afrl&15)===6;
  const analysis={configured,model:'PWM1 edge aligned, active-high main/complement, DT368; GPIO digital pad samples, not winding voltage/current',valid:0,excluded:0,matched:0,mismatched:0,expected_mixed_high:0,actual_mixed_high:0,expected_ab_different:0,actual_ab_different:0,mismatch_examples:[]};
  if(configured)for(let i=0;i<samples.length;i++) {
    const s=samples[i],lo=s.before-16,hi=s.after+16;
    // Reject rollover, wide reads and any sample crossing/near a transition.
    if(s.after<s.before || s.after-s.before>128 || lo<0 || hi>=period ||
       [dt,...ccr,...ccr.map(v=>v+dt)].some(t=>t>=lo&&t<=hi)) {analysis.excluded++;continue;}
    let expected=0;
    for(let c=0;c<3;c++) {
      if(s.before>=dt && s.before<ccr[c])expected|=1<<c;
      if(s.before>=ccr[c]+dt)expected|=1<<(c+3);
    }
    const actual=((s.gpioa>>>8)&31)|((s.gpiof&1)<<5);
    analysis.valid++;
    if((expected&7)!==0&&(expected&7)!==7)analysis.expected_mixed_high++;
    if((actual&7)!==0&&(actual&7)!==7)analysis.actual_mixed_high++;
    if(Boolean(expected&1)!==Boolean(expected&2))analysis.expected_ab_different++;
    if(Boolean(actual&1)!==Boolean(actual&2))analysis.actual_ab_different++;
    if(actual===expected)analysis.matched++;
    else {analysis.mismatched++;if(analysis.mismatch_examples.length<8)analysis.mismatch_examples.push({index:i,...s,expected,actual});}
  }
  analysis.verdict=!configured?'unsupported_timer_or_pin_configuration':analysis.mismatched?'pad_mismatch_observed':analysis.expected_mixed_high?'matches_at_sampled_times':'insufficient_mixed_channel_coverage';
  return {header,analysis,samples};
}
module.exports={SIZE,decodePins};
