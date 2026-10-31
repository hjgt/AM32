const assert=require('node:assert/strict');
const reasons=['invalid','blank','pre','hysteresis','early','unarmed','confirmation','accepted','reported','coast'];
const events=['','start','commutate','zc','timeout','desync','stop','fault','bench_stop','rev_step','rev_edge','rev_handoff','rev_decision'];
function decode(b) {
 assert.equal(b.length,21568);const u=o=>b.readUInt32LE(o),v=o=>b.readUInt16LE(o);
 const names=['magic','abi','variant','clock_hz','size','command','active_bank','no_lock_bank','lost_bank','success_bank','trials_seen','rejected_commands','max_finish_cycles'];
 const header=Object.fromEntries(names.map((n,i)=>[n,u(i*4)]));
 assert(header.magic===0x46535731 && header.abi===3 && header.variant===27 && header.clock_hz===160000000 && header.size===21568);
 assert.equal(header.active_bank,0xffffffff,'Recording active');
 assert(u(52)===1280 && u(56)===16 && u(60)===48);
 const fields=['state','trial_id','started','ended','reason','final_stage','handoff_at','samples_seen','samples_stored','events','no_fresh','accepted','timeouts','max_sample_cycles','max_event_cycles','last_sample_at'];
 const h=Object.fromEntries(fields.map((n,i)=>[n,u(64+4*i)]));
 assert([0,2,3,4,5].includes(h.state));assert(h.samples_stored<=1280 && h.samples_stored<=h.samples_seen);
 const samples=[],ev=[];
 if(h.state) {
 assert.equal(h.trial_id,header.trials_seen);assert(h.final_stage<=4);
 for(let i=0;i<h.samples_stored;i++) {
  const o=128+i*16,meta=b[o+14],state=b[o+15],step=meta&7,reason=(meta>>3)&15;
  assert(step>=1 && step<=6 && reason<10 && ((state>>3)&7)<=4);
  const adc=[v(o+4),v(o+6),v(o+8)];assert(adc.every(x=>x<=4095));
  const floating=step===1||step===4?2:step===2||step===5?0:1;
  const margin=adc[floating]-Math.floor(adc.reduce((a,x)=>a+x,0)/3),at=u(o);
  assert(at<=h.ended && (!i || at>=samples[i-1].at_cycles));
  samples.push({sequence:i+1,at_cycles:at,at_us:at/160,adc,step,reason:reasons[reason],pre_seen:!!(meta&128),confirmations:state&7,stage:(state>>3)&7,rising:!!(state&64),forward:!!(state&128),margin,directed_margin:state&64?margin:-margin,pwm:v(o+10),trigger:v(o+12)});
 }
 for(let i=0;i<Math.min(h.events,48);i++) {
  const o=20608+i*20,at=u(o),kind=b[o+18];assert(kind>0 && kind<events.length && at<=h.ended);
  assert(b[o+16]>=1 && b[o+16]<=6);
  ev.push({sequence:i+1,at_us:at/160,kind:events[kind],elapsed_ticks:v(o+4),interval_ticks:v(o+6),aux:v(o+8),wait_ticks:v(o+10),input:v(o+12),duty:v(o+14),step:b[o+16],flags:b[o+17],stage:b[o+19]});
 }
 }
 return {header,banks:h.state?[{bank:0,pinned:false,classification:['empty','active','aborted','no_lock','post_handoff_loss','success'][h.state],header:h,samples,events:{records:ev,omitted_suffix:Math.max(0,h.events-48)}}]:[],sampling_profile:{name:'S4K-DENSE'},recording_profile:{mode:'latest_trial_full_rate_prefix',capacity:1280,sample_bytes:16},caveats:['Only the latest trial is retained; stop and read before starting again. First1280 software-consumed ADC triplets, not full300ms.','Times are logger processing times; PWM/CCR4 are current preload register snapshots, not latched hardware sample timestamps. No IRQ-missed or discarded-at-boundary samples can be reconstructed.','Existing bench metrics freeze after observation; final reason can update later. No drive algorithm changes; logger overhead must be measured on hardware.']};
}
module.exports={decode};
