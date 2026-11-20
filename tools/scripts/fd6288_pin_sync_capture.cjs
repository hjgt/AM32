// PWM-triggered diagnostic only. All timestamps below are software readout.
const fs=require('node:fs'),assert=require('node:assert/strict'),{execFileSync}=require('node:child_process');
const {connect}=require('./fd6288_startup_capture.cjs');
function decode(b){
 assert.equal(b.length,12320);assert.equal(b.readUInt32LE(0),0x46505331);assert.equal(b.readUInt32LE(4),1);
 const count=b.readUInt32LE(8);assert(count<=512);assert.equal(b.readUInt32LE(12),1,'Not frozen/full yet');
 const records=Array.from({length:count},(_,i)=>{
  const o=32+24*i,r={cycles:b.readUInt32LE(o)};
  ['a','b','c','pa3','pa7','cnt','arr','duty','trigger'].forEach((n,j)=>r[n]=b.readUInt16LE(o+4+2*j));
  for(const n of ['a','b','c','pa3','pa7'])assert(r[n]<=4095);
  r.step=b[o+22];r.flags=b[o+23];assert(r.step>=1&&r.step<=6);assert(!(r.flags&~123));
  r.eligible=(r.flags&123)===123;r.neutral=(r.a+r.b+r.c)/3;
  return r;
 });
 const eligible=records.filter(r=>r.eligible);
 const stats=Object.fromEntries(['a','b','c','pa3','pa7','neutral'].map(n=>{const a=eligible.map(r=>r[n]);
  return [n,a.length?{min:Math.min(...a),mean:a.reduce((x,y)=>x+y)/a.length,max:Math.max(...a)}:null]}));
 return {closed_capture:b.readUInt32LE(16),count,eligible:eligible.length,
  span_us:count>1?((records.at(-1).cycles-records[0].cycles)>>>0)/160:0,stats,records,
  flags:{1:'conservative complete ON window',2:'ADC2 JEOS fresh since last serviced ADC1',8:'no ADC2 completion during read',16:'readout cadence near one PWM period',32:'past commutation blanking',64:'three-phase drive validity passed'},
  caveats:['Common trigger, independent converters; not simultaneous ADC apertures. PA3 is ADC1 rank4, PA7 ADC2 rank1.',
   'No hardware sequence tag: freshness/cadence/window are filters, not proof of exact pairing after an unobserved stall.',
   'cycles/cnt are readout timestamps. ADC regular arbitration and unknown input RC can affect values.',
   'Neutral is the uncalibrated three-phase average. Do not identify wiring from mean/ratio alone.']};
}
async function dump(elf,out,stopped){
 assert(stopped,'Fresh stop confirmation required');assert(out&&!fs.existsSync(out)&&!fs.existsSync(out+'.json'));
 const nm=execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'});
 const m=nm.match(/^([\da-f]+) ([\da-f]+) \w fd_pin_sync$/m);assert(m,'Matching pin-sync ELF required');
 const addr=parseInt(m[1],16);assert.equal(parseInt(m[2],16),12320);assert(addr>=0x20000000&&addr+12320<=0x20008000);
 const c=await connect();try{
  async function read(a,n){const v=(await c.rpc(`read_memory 0x${a.toString(16)} 8 ${n}`)).trim().split(/\s+/).map(Number);
   assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=255));return Buffer.from(v);}
  const word=async a=>(await read(a,4)).readUInt32LE();
  async function gate(){assert.equal(await word(0xe000ed08),0x08001000);
   assert.equal((await word(0x48000010))&0x1f00,0);assert.equal((await word(0x48001410))&1,0);
   for(let i=0;i<4;i++)assert.equal(await word(0x40012c34+i*4),0);}
  await gate();const raw=await read(addr,12320),result=decode(raw);await gate();
  assert.deepEqual(await read(addr,12320),raw,'Capture changed during read');
  fs.writeFileSync(out,raw,{flag:'wx'});fs.writeFileSync(out+'.json',JSON.stringify(result,null,2)+'\n',{flag:'wx'});
  console.log(JSON.stringify({closed_capture:result.closed_capture,count:result.count,eligible:result.eligible,span_us:result.span_us,stats:result.stats},null,2));
 }finally{c.close()}
}
module.exports={decode};
if(require.main===module)(async()=>{const a=process.argv.slice(2);
 if(a[0]==='decode')console.log(JSON.stringify(decode(fs.readFileSync(a[1])),null,2));
 else if(a[0]==='dump')await dump(a[1],a[2],a.includes('--stopped'));
 else throw Error('decode FILE | dump MATCHING.elf NEW.bin --stopped');
})().catch(e=>{console.error(e.message);process.exitCode=1});
