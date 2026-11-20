const assert=require('node:assert/strict');
function decode(b){
 assert.equal(b.length,128);assert.equal(b.readUInt32LE(0),0x46424c31);assert.equal(b.readUInt32LE(4),1);
 const n=b.readUInt32LE(12);assert(n>=1&&n<=3);
 const names=['started','launch','ignored_first_event','forced_step','first_usable_event','release','finished','accepted_count'];
 const attempts=[];for(let i=0;i<n;i++){
  const row={attempt:i+1};names.forEach((k,j)=>{const v=b.readUInt32LE(32+32*i+4*j);row[k+(j===7?'':'_us')]=v===0xffffffff?null:j===7?v:v/160;});attempts.push(row);
 }
 return {trial:b.readUInt32LE(8),attempts_started:n,stage:b.readUInt32LE(16),confidence:b.readUInt32LE(20),first_vector_fixed:!!b.readUInt32LE(24),stop_reason:b.readUInt32LE(28),attempts,caveats:['Times relative to whole trial; software observations. Latest attempt only in raw/alignment buffers. Six usable feedback edges needed after forced first step; ignored first-vector event not credited.']};
}module.exports={decode};
