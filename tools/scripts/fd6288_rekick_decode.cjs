const assert=require('node:assert/strict');
function decode(b){
 assert.equal(b.length,160);assert.equal(b.readUInt32LE(0),0x46524b31);assert.equal(b.readUInt32LE(4),1);
 const k=b.readUInt32LE(12);assert(k<=3);const pulses=[];
 const names=['launch','first_event','event_age','classification','release','finished','usable_events','reserved'];
 for(let i=0;i<=k;i++){const p={pulse:i+1};names.forEach((n,j)=>{let v=b.readUInt32LE(32+32*i+j*4);p[n+(j===3||j===6||j===7?'':'_us')]=v===0xffffffff?null:j===3||j===6||j===7?v:v/160;});assert(p.classification===null||[1,2,3].includes(p.classification));pulses.push(p);}
 return {trial:b.readUInt32LE(8),rekicks_applied:k,stage:b.readUInt32LE(16),confidence:b.readUInt32LE(20),stop_reason:b.readUInt32LE(24),pulses,caveats:['Classification1=early ignored,2=normal accepted,3=early after3 re-kicks ->stop. Threshold1ms inclusive at software acceptance; release20ms, no repeated alignment. All times software observations. Initial+3 maximum; ordinary timeout/fault/zero stops, no other retry.']};
}module.exports={decode};
