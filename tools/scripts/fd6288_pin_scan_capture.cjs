const fs=require('node:fs'),assert=require('node:assert/strict');
const {execFileSync}=require('node:child_process');
const {connect}=require('./fd6288_startup_capture.cjs');
function decode(b){
 assert.equal(b.length,12328);assert.equal(b.readUInt32LE(0),0x46504931);assert.equal(b.readUInt32LE(4),1);
 const count=b.readUInt32LE(8),frozen=b.readUInt32LE(12);assert(count<=512);assert.equal(frozen,1);
 const names=['PA3','PA4','PA7','PB0','PA5_voltage','PA6_current'];
 const records=Array.from({length:count},(_,i)=>{const o=40+24*i;
  return {cycles:b.readUInt32LE(o),values:Object.fromEntries(names.map((n,j)=>[n,b.readUInt16LE(o+4+2*j)])),
   gpio_raw:[b.readUInt16LE(o+16),b.readUInt16LE(o+18),b.readUInt16LE(o+20)],step:b[o+22],running:b[o+23]};
 });
 for(const r of records)assert(Object.values(r.values).every(v=>v<=4095));
 return {trial:b.readUInt32LE(16),count,frozen,skipped:b.readUInt32LE(24),
  gpio_boot_moder:[b.readUInt32LE(28),b.readUInt32LE(32),b.readUInt32LE(36)],
  summary:Object.fromEntries(names.map(n=>{const v=records.map(r=>r.values[n]);return [n,v.length?
   {min:Math.min(...v),mean:v.reduce((a,x)=>a+x,0)/v.length,max:Math.max(...v)}:null]})),records,
  caveats:['Slow asynchronous regular ADC groups; cycles/step/GPIO are consumption time, not ADC sample time.',
   'No neutral/zero-cross identification from mean alone. No simultaneous three-phase reference.',
   'GPIO analog modes have unusable digital levels; no SWD pins sampled. Aliasing prevents reliable edge frequency inference.']};
}
async function dump(elf,out,stopped){
 assert(stopped,'Fresh stop confirmation required');assert(out&&!fs.existsSync(out)&&!fs.existsSync(out+'.json'));
 const nm=execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'});
 const m=nm.match(/^([\da-f]+) ([\da-f]+) \w fd_pin_scan$/m);assert(m,'Matching pin-scan ELF required');
 const addr=parseInt(m[1],16);assert.equal(parseInt(m[2],16),12328);assert(addr>=0x20000000&&addr+12328<=0x20008000);
 const c=await connect();try{
  async function read(a,n){const v=(await c.rpc(`read_memory 0x${a.toString(16)} 8 ${n}`)).trim().split(/\s+/).map(Number);
   assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=255));return Buffer.from(v);}
  const word=async a=>(await read(a,4)).readUInt32LE();
  async function gate(){assert.equal(await word(0xe000ed08),0x08001000);
   assert.equal((await word(0x48000010))&0x1f00,0);assert.equal((await word(0x48001410))&1,0);
   for(let i=0;i<4;i++)assert.equal(await word(0x40012c34+i*4),0);}
  await gate();
  const raw=await read(addr,12328),result=decode(raw);
  await gate();assert.deepEqual(await read(addr,12328),raw,'Capture changed during read');
  fs.writeFileSync(out,raw,{flag:'wx'});fs.writeFileSync(out+'.json',JSON.stringify(result,null,2)+'\n',{flag:'wx'});
  console.log(JSON.stringify({trial:result.trial,count:result.count,summary:result.summary},null,2));
 }finally{c.close()}
}
module.exports={decode};
if(require.main===module)(async()=>{const a=process.argv.slice(2);
 if(a[0]==='decode')console.log(JSON.stringify(decode(fs.readFileSync(a[1])),null,2));
 else if(a[0]==='dump')await dump(a[1],a[2],a.includes('--stopped'));
 else throw Error('decode FILE | dump MATCHING.elf NEW.bin --stopped');
})().catch(e=>{console.error(e.message);process.exitCode=1});
