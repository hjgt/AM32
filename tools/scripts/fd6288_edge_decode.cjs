const fs=require('node:fs'),assert=require('node:assert/strict');
function decode(b){
 if(b.length===12404)return b.readUInt32LE(4)===3?decodePairs(b):decodeRaw(b);
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
module.exports={decode};
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
