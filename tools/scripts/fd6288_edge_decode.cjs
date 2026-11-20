const fs=require('node:fs'),assert=require('node:assert/strict');
function decode(b,afterDrive=false){
 if(b.length===12404 && [7,11].includes(b.readUInt32LE(4)))return decodeHiZ(b,afterDrive);
 if(b.length===12404)return [4,5,6,8,9,10].includes(b.readUInt32LE(4))?decodeLaunch(b):b.readUInt32LE(4)===3?decodePairs(b):decodeRaw(b);
 assert.equal(b.length,592);assert.equal(b.readUInt32LE(0),0x46454f31);assert.equal(b.readUInt32LE(4),1);
 const names=['magic','abi','trial','started','frozen','sectors','active','first_samples','seen','count','max_cycles','calls','start0','end0','start1','end1'];
 const header=Object.fromEntries(names.map((k,i)=>[k,b.readUInt32LE(i*4)]));
 assert.equal(header.frozen,1);assert.equal(header.active,0xffffffff);assert(header.sectors<=2&&header.count<=22);
 const events=['sample1','sample2','sample3','sample4','first_pre','first_hysteresis','first_post','first_accepted','first_invalid','first_confirmation'];
 const records=[];
 for(let i=0;i<22;i++){
  const o=64+i*24;if(!b[o+21])continue;
  assert.equal(b[o+21],1);assert(b.readUInt32LE(o)<=64000000);assert(b[o+19]>=1&&b[o+19]<=6);
  const alignment=i<2;
  assert(alignment?b[o+18]===255:b[o+18]<10);
  records.push({slot:i,sector:alignment?'alignment':Math.floor((i-2)/10)+1,
   event:alignment?(i?'alignment_59_8ms':'alignment_59ms'):events[(i-2)%10],at_us:b.readUInt32LE(o)/160,
   a:b.readUInt16LE(o+4),b:b.readUInt16LE(o+6),c:b.readUInt16LE(o+8),
   directed_margin:alignment?null:b.readInt16LE(o+10),cnt:b.readUInt16LE(o+12),duty:b.readUInt16LE(o+14),trigger:b.readUInt16LE(o+16),
   reason:b[o+18],step:b[o+19],flags:b[o+20]});
 }
 assert.equal(records.length,header.count);
 return {header,hook_max_us:header.max_cycles/160,records,caveats:[
  'Sparse milestones, not a waveform. Missing event slot means not observed in retained scope, not proof of no physical crossing.',
  'Timestamps are software observation times; CNT/duty/trigger are readout-time values, not hardware sample timestamps.',
  'Drive flags bit0=software-valid non-blank; alignment bit0=existing alignment ON-window screen. Bit1=expected rising; bit2=alignment.',
  'max_cycles covers selected snapshot work, not the whole ISR or the skipped-sample eligibility check. Requires hardware measurement.',
  'First2 drive sectors only. Freezes on second boundary or stop. Alignment checkpoint ages are relative to second positioning vector.']};
}
function decodeStep4Pair(b,holdMs=5){
 assert([5,11].includes(holdMs),'Unexpected step 4 capture duration');
 const capacity=holdMs===11?128:64, stride=24, head=16;
 assert.equal(b.length,head+capacity*stride);
 const [trial,started,count,maxCycles]=[0,4,8,12].map(i=>b.readUInt32LE(i));
 assert(trial>0&&count<=capacity);
 const lateTrigger=count>1?b.readUInt16LE(head+stride+16):null;
 if(lateTrigger!==null)assert([780,900].includes(lateTrigger),'Unexpected late step 4 trigger');
 const records=[];
 for(let i=0;i<count;i++){
  const o=head+i*stride, at_us=b.readUInt32LE(o)/160, trigger=b.readUInt16LE(o+16);
  const flags=b[o+20];
  assert(at_us<holdMs*1000&&trigger===(i%2?lateTrigger:600)&&b[o+19]===4&&b[o+21]===1);
  if(i)assert(at_us>=records[i-1].at_us);
  const a=b.readUInt16LE(o+4), z=b.readUInt16LE(o+6), c=b.readUInt16LE(o+8);
  const margin=c-Math.floor((a+z+c)/3);
  assert.equal(b.readInt16LE(o+10),(flags&2)?margin:0-margin);
  records.push({at_us,a,b:z,c,directed_margin:b.readInt16LE(o+10),
   cnt:b.readUInt16LE(o+12),duty:b.readUInt16LE(o+14),trigger,valid:Boolean(flags&1)});
 }
 const pairs=[];
 for(let i=0;i+1<count;i+=2){
  const early=records[i],late=records[i+1],dt=late.at_us-early.at_us;
  if(early.valid&&late.valid&&dt>0&&dt<200&&early.duty===late.duty)
   pairs.push({early:i,late:i+1,dt_us:dt,delta_margin:late.directed_margin-early.directed_margin});
 }
 return {trial,started_cycles:started,hook_max_us:maxCycles/160,records,pairs,
  caveat:'Paired ADC groups are from nearby PWM cycles. Validity is a software ON-window and drive-span screen.'};
}
function decodePoweredCapture(b){
 const headerBytes=32,sectorBytes=16,sampleBytes=24,maxSectors=6,maxSamples=256;
 const sampleStart=headerBytes+sectorBytes*maxSectors;
 assert(b.length>=sampleStart);
 const [trial,started,end,count,seen,sectorCount,active,maxCycles]=
  Array.from({length:8},(_,i)=>b.readUInt32LE(i*4));
 assert(trial>0&&!active&&count<=maxSamples&&count<=seen&&sectorCount<=maxSectors);
 assert(b.length>=sampleStart+count*sampleBytes);
 const sectors=Array.from({length:sectorCount},(_,i)=>{
  const o=headerBytes+i*sectorBytes;
  const from_resume_us=b.readUInt32LE(o)/160,step=b.readUInt16LE(o+4);
  assert(step>=1&&step<=6&&from_resume_us<=end/160);
  return {from_resume_us,step,compare:b.readUInt16LE(o+6),
   current_ca:b.readInt16LE(o+8),peak_filtered_current_ca:b.readInt16LE(o+10),
   voltage_cv:b.readUInt16LE(o+12),min_filtered_voltage_cv:b.readUInt16LE(o+14)};
 });
 const records=Array.from({length:count},(_,i)=>{
  const o=sampleStart+i*sampleBytes;
  const at=b.readUInt32LE(o),sector=b.readUInt16LE(o+22),step=b[o+19];
  assert(b[o+21]===1&&sector>=1&&sector<=sectors.length&&step===sectors[sector-1].step);
  assert(at<=end&&at<6400000);
  if(i)assert(at>=b.readUInt32LE(o-sampleBytes));
  return {from_resume_us:at/160,sector,step,a:b.readUInt16LE(o+4),b:b.readUInt16LE(o+6),
   c:b.readUInt16LE(o+8),directed_margin:b.readInt16LE(o+10),cnt:b.readUInt16LE(o+12),
   duty:b.readUInt16LE(o+14),trigger:b.readUInt16LE(o+16),reason:b[o+18],flags:b[o+20]};
 });
 return {trial,started_cycles:started,duration_us:end/160,seen,hook_max_us:maxCycles/160,sectors,records,
  caveats:['ADC readout only. Reason 7 means accepted by the ADC detector.',
   'Sector current and voltage are filtered telemetry. Compare values are timer register snapshots.']};
}
function decodeOnAdcPair(b) {
 const trial=b.readUInt32LE(0), count=b.readUInt32LE(4);
 assert(count<=96&&b.length===12+24*count);
 const records=Array.from({length:count},(_,index)=>{
  const offset=12+24*index, step=b[offset+19], flags=b[offset+20];
  assert([3,5,6].includes(step)&&b[offset+21]===1);
  return {index,step,at_us:b.readUInt32LE(offset)/160,
   a:b.readUInt16LE(offset+4),b:b.readUInt16LE(offset+6),c:b.readUInt16LE(offset+8),
   directed_margin:b.readInt16LE(offset+10),cnt:b.readUInt16LE(offset+12),
   duty:b.readUInt16LE(offset+14),trigger:b.readUInt16LE(offset+16),
   valid:!!(flags&1),late:!!(flags&8)};
 });
 const pairs=[];
 for(let i=1;i<records.length;i++){
  const early=records[i-1], late=records[i], dt=late.at_us-early.at_us;
  if(early.valid&&late.valid&&!early.late&&late.late&&early.step===late.step&&dt>0&&dt<250)
   pairs.push({step:early.step,early:i-1,late:i,dt_us:dt,
    delta_margin:late.directed_margin-early.directed_margin});
 }
 return {trial,hook_max_us:b.readUInt32LE(8)/160,records,pairs};
}
function decodeHandoffCapture(b) {
 const capacity=96, headerBytes=60, recordBytes=24;
 const [trial, started, active, next, count, , switches, feedbacks, seed, lastStartup,
  stopped, stopReason, stopStep, stopTimer, maxCycles]=Array.from({length:15},(_,i)=>b.readUInt32LE(i*4));
 assert(trial>0&&!active&&count<=capacity&&next<capacity);
 assert.equal(b.length,headerBytes+count*recordBytes);
 assert(count===capacity||next===count);
 assert(stopReason ? stopStep>=1&&stopStep<=6 : count===0);
 const records=Array.from({length:count},(_,i)=>{
  const slot=(i+next+capacity-count)%capacity;
  const o=headerBytes+slot*recordBytes, step=b[o+19], reason=b[o+18];
  assert(b[o+21]===1&&step>=1&&step<=6);
  const at=b.readUInt32LE(o);
  const base={from_handoff_us:((at-started)>>>0)/160,
   before_stop_us:((stopped-at)>>>0)/160,step,sector:b.readUInt16LE(o+22)};
  const a=b.readUInt16LE(o+4),bPhase=b.readUInt16LE(o+6),c=b.readUInt16LE(o+8);
  const cnt=b.readUInt16LE(o+12),duty=b.readUInt16LE(o+14),trigger=b.readUInt16LE(o+16);
  if(reason===240)return {...base,kind:'switch',interval_timer_us:cnt/2,duty,trigger};
  if(reason===241)return {...base,kind:'feedback',measured_interval_us:a/2,smoothed_interval_us:bPhase/2,requested_delay_us:c/2};
  assert(reason<10);
  return {...base,kind:'adc',a,b:bPhase,c,directed_margin:b.readInt16LE(o+10),
   cnt,duty,trigger,reason,flags:b[o+20]};
 });
 return {trial,started_cycles:stopReason?started:null,duration_us:stopReason?((stopped-started)>>>0)/160:null,
  switches,feedbacks,seed_interval_us:stopReason?seed/2:null,last_startup_interval_us:stopReason?lastStartup/2:null,
  stop_reason:stopReason||null,stop_step:stopStep||null,stop_timer_us:stopReason?stopTimer/2:null,
  hook_max_us:maxCycles/160,records,
  caveats:['Rolling buffer. Software time and sector counts may wrap.',
   'ADC reason 7 is detector acceptance. Feedback events are controller inputs with requested delays, not proof of a resulting switch.']};
}
function decodeRunupAdc(b) {
 assert.equal(b.length,1320);
 const trial=b.readUInt32LE(0);
 assert(trial>0&&b.readUInt32LE(8)===0);
 const sectors=Array.from({length:3},(_,i)=>{
  const step=i+4, count=b.readUInt16LE(16+i*2);
  assert(count<=18);
  const samples=Array.from({length:count},(_,j)=>{
   const o=24+(i*18+j)*24;
   const reason=b[o+18], flags=b[o+20];
   assert(b[o+19]===step&&b[o+21]===1&&(reason<10||(step===4&&reason===254)));
   return {from_step_us:b.readUInt32LE(o)/160,a:b.readUInt16LE(o+4),b:b.readUInt16LE(o+6),
    c:b.readUInt16LE(o+8),directed_margin:b.readInt16LE(o+10),cnt:b.readUInt16LE(o+12),
    duty:b.readUInt16LE(o+14),trigger:b.readUInt16LE(o+16),reason,flags};
  });
  return {step,samples};
 });
 const samples=sectors[0].samples;
 const pairs=[];
 for(let i=1;i<samples.length;i++){
  const early=samples[i-1],late=samples[i],dt=late.from_step_us-early.from_step_us;
  if(early.reason===254&&late.reason===254&&(early.flags&9)===1&&(late.flags&9)===9&&dt>0&&dt<250)
   pairs.push({early:i-1,late:i,dt_us:dt,delta_margin:late.directed_margin-early.directed_margin});
 }
 return {trial,sectors,step4_pairs:pairs};
}
module.exports={decode,decodeStep4Pair,decodePoweredCapture,decodeOnAdcPair,decodeHandoffCapture,decodeRunupAdc};
if(require.main===module)console.log(JSON.stringify(decode(fs.readFileSync(process.argv[2])),null,2));

function decodeRaw(b){
 assert.equal(b.readUInt32LE(0),0x46454f31);assert.equal(b.readUInt32LE(4),2);
 const names=['magic','abi','trial','started','frozen','sectors','active','first_samples','seen','count','max_cycles','calls','start0','end0','start1','end1','post_samples'];
 const h=Object.fromEntries(names.map((k,i)=>[k,b.readUInt32LE(i*4)]));
 assert(h.frozen===1 && h.active===0xffffffff && h.first_samples<=256 && h.post_samples<=192);
 const slots=[];for(let i=0;i<2;i++)if(b[68+i*24+21])slots.push([i,'alignment']);
 for(let i=0;i<h.first_samples;i++)slots.push([2+i,'first_drive']);
 const n=Math.min(h.seen,64),begin=h.seen-n;
 for(let i=0;i<n;i++)slots.push([258+((begin+i)%64),'before_accept_or_tail']);
 for(let i=0;i<h.post_samples;i++)slots.push([322+i,'after_accept']);
 const records=slots.map(([slot,window])=>{const o=68+slot*24;assert(b[o+21]===1);assert(b[o+19]>=1&&b[o+19]<=6);assert(b[o+18]<10||window==='alignment');return {slot,window,sector:b.readUInt16LE(o+22),at_us:b.readUInt32LE(o)/160,a:b.readUInt16LE(o+4),b:b.readUInt16LE(o+6),c:b.readUInt16LE(o+8),directed_margin:b.readInt16LE(o+10),cnt:b.readUInt16LE(o+12),duty:b.readUInt16LE(o+14),trigger:b.readUInt16LE(o+16),reason:b[o+18],step:b[o+19],flags:b[o+20]};});
 return {header:h,hook_max_us:h.max_cycles/160,records,caveats:['Software observation timestamps, not hardware sampling times. CCR may be preload values.','First drive <=256 groups; last <=64 groups through first acceptance (or tail if absent); next <=192 groups. Windows may overlap; gaps are not recorded.','Blank and invalid samples retain ABC; flags bit0 means valid nonblank, not physical validity.','Hook maximum excludes detector, eligibility and rest of ISR. No hardware cost validation yet.']};
}

function decodePairs(b){
 assert.equal(b.readUInt32LE(0),0x46454f31);assert.equal(b.readUInt32LE(4),3);assert.equal(b.readUInt32LE(16),1);
 const count=b.readUInt32LE(28);assert(count<=512);
 const records=[];
 for(let i=0;i<count;i++) {const o=68+(2+i)*24;assert(b[o+21]===1&&b[o+18]===254);const late=!!(b[o+20]&8);assert.equal(b.readUInt16LE(o+14),1200);assert.equal(b.readUInt16LE(o+16),late?900:600);
 records.push({index:i,at_us:b.readUInt32LE(o)/160,a:b.readUInt16LE(o+4),b:b.readUInt16LE(o+6),c:b.readUInt16LE(o+8),mid_error:b.readInt16LE(o+10),late,cnt:b.readUInt16LE(o+12),trigger:b.readUInt16LE(o+16),step:b[o+19]});}
 const pairs=[];
 for(let i=1;i<records.length;i++){const a=records[i-1],z=records[i];if(!a.late&&z.late&&a.step===z.step&&z.at_us>a.at_us&&z.at_us-a.at_us<250) pairs.push({early:i-1,late:i,dt_us:z.at_us-a.at_us,delta_mid:z.mid_error-a.mid_error});}
 return {hook_max_us:b.readUInt32LE(40)/160,header:{abi:3,trial:b.readUInt32LE(8),started:b.readUInt32LE(12),count},records,pairs,caveats:['Second alignment only; early CCR4=600, late=900 at compare1200. One group discarded after each preload switch.','Pair eligibility uses same step and <250us software interval, not independent rotor-position or hardware sample-time evidence.','No drive windows in this diagnostic mode. Unpaired records remain visible.']};
}

function decodeLaunch(b){
 assert.equal(b.readUInt32LE(0),0x46454f31);assert([4,5,6,8,9,10].includes(b.readUInt32LE(4)));
 const names=['magic','abi','trial','started','frozen','sectors','active','first_samples','seen','count','max_cycles','calls','start0','end0','start1','end1','post_samples'];
 const h=Object.fromEntries(names.map((k,i)=>[k,b.readUInt32LE(i*4)]));
 assert(h.frozen===1&&h.active===0xffffffff&&h.first_samples<=512&&h.count===h.first_samples);
 assert(h.end1>=h.start0&&h.seen===h.count+h.post_samples);
 const records=[];
 for(let i=0;i<h.first_samples;i++){
  const o=68+(2+i)*24,at=b.readUInt32LE(o);
  assert(b[o+21]===1&&b[o+19]>=1&&b[o+19]<=6&&b[o+18]<10);
  assert(at>=h.start0&&at-h.start0<(h.abi===10?80000:h.abi===9?124000:(h.abi===6||h.abi===8)?61000:40000)*160);
  if(i)assert(at>=records[i-1].at_us*160);
  records.push({index:i,sector:b.readUInt16LE(o+22),at_us:at/160,from_launch_us:(at-h.start0)/160,
   a:b.readUInt16LE(o+4),b:b.readUInt16LE(o+6),c:b.readUInt16LE(o+8),directed_margin:b.readInt16LE(o+10),
   cnt:b.readUInt16LE(o+12),duty:b.readUInt16LE(o+14),trigger:b.readUInt16LE(o+16),reason:b[o+18],step:b[o+19],flags:b[o+20]});
 }
 const tail=[];
 if(h.abi>=5)for(let i=0;i<2;i++){
  const o=68+i*24;if(!b[o+21])continue;
  assert(b[o+21]===1&&b[o+18]===255&&b[o+19]>=1&&b[o+19]<=6);
  const at=b.readUInt32LE(o);assert(at<=h.start0);
  tail.push({slot:i,at_us:at/160,before_launch_us:(h.start0-at)/160,
   a:b.readUInt16LE(o+4),b:b.readUInt16LE(o+6),c:b.readUInt16LE(o+8),mid_error:b.readInt16LE(o+10),
   cnt:b.readUInt16LE(o+12),duty:b.readUInt16LE(o+14),trigger:b.readUInt16LE(o+16),flags:b[o+20]});
 }
 return {header:h,hook_max_us:h.max_cycles/160,records,
  ...(h.abi>=5?{alignment_tail:tail,alignment_hook_max_us:tail.some(x=>x.slot===0)?b.readUInt16LE(68+22)/160:null}:{}),caveats:[
  ...(h.abi===9?['ABI9: initial pulse and re-kicks share one124ms/512-record window. start1 means latest vector boundary, not first ACCEPT. Every vector first2ms dense, otherwise1in4; all ACCEPT retained. Gaps are all-off releases; metadata identifies ignored/usable events.']:[]),
  ...(h.abi===8?['ABI8: latest BLUE_START attempt only. First vector fixed30ms, first event observed but ignored for control. Second vector feedback-led.']:[]),
  'ABI4/5/6/8/9 reuse alignment-pair storage at first launch; alignment CCR4 pairing is retained, but its paired raw records are replaced.',
  ...(h.abi>=5?['ABI5/6 preserve two checkpoints near second-alignment end: B minus (A+C)/2, not drive directed margin. Bit0 uses the alignment readout-window screen.','alignment_hook_max_us includes selected alignment/pair work; stored with a65535-cycle ceiling. Neither maximum is a whole-ISR measurement.']:[]),
  h.abi===10?'ABI10: first attempt only. Every consumed triplet for first 2 ms, then one in eight plus all ACCEPT samples, up to 80 ms or 512 records.':h.abi===9?'ABI9: first2ms of every energized vector full consumed rate, otherwise1in4, ACCEPT retained, up to512 records or124ms including release gaps.':(h.abi===6||h.abi===8)?'ABI6/8: first2ms of each of the two drive vectors at full consumed rate, otherwise one in four groups; ACCEPT always retained.512 records or61ms, skipped count explicit.':'First2ms: every consumed triplet. Then alternate consumed groups, always keeping ACCEPT. At most512 retained records or40ms from launch; skipped count is explicit.',
  'Software ADC-readout timestamps and current timer register snapshots, not hardware sample times or an oscilloscope waveform.',
  'Blank/invalid groups remain visible. Flags bit0 is software-valid nonblank, not independent physical validity.',
  'Hook maximum covers selected recorder work only; excludes eligibility, detector, IRQ and launch header reset. No guarantee of unchanged timing.']};
}

function decodeHiZ(b,afterDrive){
 assert.equal(b.readUInt32LE(0),0x46454f31);
 const longCoast=b.readUInt32LE(4)===11;
 const observationMs=longCoast?40:5;
 const names=['magic','abi','trial','started','frozen','sectors','active','first_samples','seen','count','max_cycles','calls','start0','end0','start1','end1','post_samples'];
 const h=Object.fromEntries(names.map((k,i)=>[k,b.readUInt32LE(i*4)]));
 assert(h.frozen===1&&h.active===0xffffffff&&h.first_samples<=512&&h.count===h.first_samples);
 assert(h.seen===h.count+h.post_samples&&(longCoast||h.post_samples===0));
 assert(h.sectors===0&&h.start1>=h.start0&&h.end1>=h.start0);
 const read=(slot)=>{const o=68+slot*24;assert.equal(b[o+21],1);assert(b[o+19]>=1&&b[o+19]<=6);
  const a=b.readUInt16LE(o+4),z=b.readUInt16LE(o+6),c=b.readUInt16LE(o+8),at=b.readUInt32LE(o);
  return {slot,at_us:at/160,from_bridge_off_us:(at-h.start0)/160,a,b:z,c,ab:a-z,bc:z-c,ca:c-a,
   mid_error:slot<2?b.readInt16LE(o+10):null,cnt:b.readUInt16LE(o+12),duty:b.readUInt16LE(o+14),trigger:b.readUInt16LE(o+16),
   reason:b[o+18],[(afterDrive||longCoast)&&slot>=2?'last_driven_step':'last_alignment_step']:b[o+19],flags:b[o+20],boundary_group:!!(b[o+20]&16)};};
 const records=[];for(let i=0;i<h.count;i++){
  const slot=2+i,v=read(slot);
  assert(v.reason===253&&(v.flags&32)&&v.from_bridge_off_us>=0&&v.from_bridge_off_us<observationMs*1000);
  if(longCoast){
   v.conversion_index=b.readUInt16LE(68+slot*24+22);
   assert(v.conversion_index<h.seen);
   if(i)assert(v.conversion_index>records[i-1].conversion_index);
   v.adc_zero_phases=['a','b','c'].filter(phase=>v[phase]===0);
  }
  if(i)assert(v.at_us>=records[i-1].at_us);records.push(v);
 }
 const alignment_tail=[];for(let i=0;i<2;i++)if(b[68+i*24+21]){
  const v=read(i);assert(v.reason===255&&v.from_bridge_off_us<=0);alignment_tail.push(v);
 }
 return {header:h,observation_ms:observationMs,hook_max_us:h.max_cycles/160,alignment_tail,records,caveats:[
  afterDrive || longCoast ? 'High-Z samples after the timed drive sectors. start0=software time after all six gate commands off; start1=CH4 high-Z sampling compare write complete.' :
   'No launch, zero drive sectors. start0=software time after all six gate commands off; start1=CH4 high-Z sampling compare write complete.',
  longCoast ? 'Up to 40 ms or 512 retained groups. All groups in the first 500 us, then alternate groups.' :
   'Every consumed group for <5 ms, up to 512.',
  'Software readout timestamps and live/preloaded CCR snapshots, not hardware trigger timestamps.',
  'Hi-Z flag32. Boundary flag16 marks first2 groups OR first150us; possible queued driven conversion/preload transition. These groups are retained.',
  'Bit0 on Hi-Z groups means no second JEOS seen during reads, not analog validity. Tail bit0 retains powered alignment readout-window screen.',
  'No group is automatically classified as pure BEMF. Later groups can still include demagnetization/clamping and release-induced motion.',
  'Compare line differences AB/BC/CA. Releasing drive torque can change rotor motion; post-off voltage alone does not prove pre-off motion.',
  'Hook maximum is selected record work, not complete IRQ cost.']};
}
