const fs=require('node:fs'),assert=require('node:assert/strict');
function decode(b){
 assert.equal(b.length,1600);assert.equal(b.readUInt32LE(0),0x464c4731);assert.equal(b.readUInt32LE(4),1);
 const names=['id','started','first_edge','handoff','edges','retries','max_streak','reason','stage','input_at_stop','interval_at_stop','current_at_stop'];
 const records=[];
 for(let i=0;i<32;i++){
  const o=64+i*48,r={};names.forEach((n,j)=>r[n]=j===11?b.readInt32LE(o+j*4):b.readUInt32LE(o+j*4));
  if(!r.id)continue;
  for(const n of ['first_edge','handoff'])r[n+'_us']=r[n]===0xffffffff?null:r[n]/160;
  r.reason_name=({1:'window',2:'native_stuck',3:'safety_cap',4:'not_running',5:'bad_config',6:'no_zero',7:'operator_zero',8:'no_lock',9:'closed_feedback_timeout',10:'commutation_late',11:'commutation_invalid'})[r.reason]||'unknown';
  records.push(r);
 }
 return {trials:b.readUInt32LE(8),completed:b.readUInt32LE(12),records:records.sort((a,b)=>a.id-b.id)};
}
module.exports={decode};
if(require.main===module)console.log(JSON.stringify(decode(fs.readFileSync(process.argv[2])),null,2));
