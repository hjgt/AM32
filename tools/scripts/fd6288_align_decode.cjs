const assert=require('node:assert/strict');
function decode(b){
 assert.equal(b.length,808);assert.equal(b.readUInt32LE(0),0x46414c31);assert.equal(b.readUInt32LE(4),1);assert.equal(b.readUInt32LE(12),1);
 const bins=Array.from({length:18},(_,i)=>{const o=16+i*44,total=b.readUInt32LE(o),valid=b.readUInt32LE(o+4);assert(valid<=total&&total<1000);
 const phases=Array.from({length:3},(_,j)=>{const sum=b.readUInt32LE(o+8+j*4),minimum=b.readUInt16LE(o+20+j*2),maximum=b.readUInt16LE(o+26+j*2),first=b.readUInt16LE(o+32+j*2),last=b.readUInt16LE(o+38+j*2);if(valid)assert(minimum<=first&&first<=maximum&&minimum<=last&&last<=maximum&&maximum<=4095&&sum>=minimum*valid&&sum<=maximum*valid);return valid?{minimum,maximum,first,last,mean:sum/valid}:null;});
 return {vector:i<6?1:2,start_ms:i*5,total,valid,phases};});
 return {trial:b.readUInt32LE(8),bins,caveat:'Consumed sequential ADC samples in conservative ON-window; not mechanical rotor position or proof of rest.'};
}
module.exports={decode};
if(require.main===module)console.log(JSON.stringify(decode(require('node:fs').readFileSync(process.argv[2])),null,2));
