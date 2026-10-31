// Read-only, stopped sector-statistics board. No halt/reset/flash/RAM writes.
const fs=require('node:fs'),path=require('node:path'),assert=require('node:assert/strict');
const {execFileSync}=require('node:child_process');
const {connect,parseHex,decodeBench}=require('./fd6288_startup_capture.cjs');
const {decode:decodeLight}=require('./fd6288_light_decode.cjs');
function decode(b){
 assert.equal(b.length,1472);assert.equal(b.readUInt32LE(0),0x46534331);assert.equal(b.readUInt32LE(4),1);
 const keys=['magic','abi','trial','started','active','sectors','frozen','finish_reason'];
 const h=Object.fromEntries(keys.map((k,i)=>[k,b.readUInt32LE(i*4)]));
 assert.equal(h.frozen,1);assert.equal(h.active,0xffffffff);assert(h.sectors<10000);
 const records=[];
 for(let i=0;i<Math.min(h.sectors,24);i++){
  const o=32+60*i,u=n=>b.readUInt32LE(o+n),s=n=>b.readUInt16LE(o+n);
  const reasons=Array.from({length:10},(_,j)=>s(16+j*2));
  assert.equal(reasons.reduce((a,v)=>a+v,0),s(36));assert(s(38)<=s(36));
  // S4T may acquire until820ms. This is a decode plausibility bound,
  // not a firmware timeout; allow service latency below1s.
  assert(u(4)>=u(0)&&u(4)<=160000000);assert(b[o+52]>=1&&b[o+52]<=6);
  records.push({sector:i+1,start_us:u(0)/160,end_us:u(4)/160,
   first_accept_us:u(8)===0xffffffff?null:u(8)/160,last_accept_us:u(12)===0xffffffff?null:u(12)/160,
   reasons:Object.fromEntries(['invalid','blank','pre','hysteresis','early','unarmed','confirmation','accepted','reported','coast'].map((k,j)=>[k,reasons[j]])),
   samples:s(36),usable:s(38),minimum:s(38)?b.readInt16LE(o+40):null,maximum:s(38)?b.readInt16LE(o+42):null,
   first:s(38)?b.readInt16LE(o+44):null,last:s(38)?b.readInt16LE(o+46):null,pre_seen:s(48),max_confirmations:s(50),
   step:b[o+52],stage:b[o+53],rising:b[o+54],forward:b[o+55],exit_kind:u(56)});
 }
 return {header:h,records,omitted_sectors:Math.max(0,h.sectors-24),caveats:['Latest trial only; extrema are directed margins of valid post-blank consumed samples, not a waveform or measured rotor direction.','Positioning sectors have no ADC samples by design. Max/min do not describe temporal crossing order.','Exit kinds:1 positioning dwell,2 feedback step,3 timeout retry,4 handoff,5 stopped,6 planned acquisition boundary,7 bridge release,8 re-alignment boundary. S4R alignment bins describe the first alignment only; release has no energized sector record.']};
}
async function dump(elf,out,stopped){
 assert(stopped,'Require fresh operator stop confirmation');assert(!fs.existsSync(out)&&!fs.existsSync(out+'.json'));
 const syms={};for(const l of execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'}).split('\n')){
  const m=l.match(/^([\da-f]+) ([\da-f]+) \w (\w+)$/i);if(m)syms[m[3]]={a:parseInt(m[1],16),n:parseInt(m[2],16)};
 }
 assert.equal(syms.fd_sector_diag?.n,1472);
 const light=syms.fd_power_bench?.n===40;
 assert(light||syms.fd_power_bench?.n===552);
 if(light)assert.equal(syms.fd_light_log?.n,1600);
 const image=parseHex(execFileSync('arm-none-eabi-objcopy',['-O','ihex',elf,'/dev/stdout'],{encoding:'utf8'}));
 const c=await connect();try{
  const read=async(a,n)=>{if(n>512){const chunks=[];for(let i=0;i<n;i+=512)chunks.push(await read(a+i,Math.min(512,n-i)));return Buffer.concat(chunks);}const v=(await c.rpc(`read_memory 0x${a.toString(16)} 8 ${n}`)).trim().split(/\s+/).map(Number);assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=255));return Buffer.from(v)};
  const ram=async n=>{const s=syms[n];assert(s&&s.a>=0x20000000&&s.a+s.n<=0x20008000);return read(s.a,s.n)};
  const word=async a=>(await read(a,4)).readUInt32LE();
  async function gate(){assert.equal(await word(0xe000ed08),0x08001000);for(const n of ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active']){const b=await ram(n);assert.equal(b.readUIntLE(0,b.length),0,n)}assert.equal((await word(0x48000010))&0x1f00,0);assert.equal((await word(0x48001410))&1,0);for(let i=0;i<4;i++)assert.equal(await word(0x40012c34+4*i),0);}
  await gate();
  for(const a of [0x08001000,syms.fdTraceInit.a&~1,syms.fdRevTick.a&~1]){
   const b=await read(a,16);for(let i=0;i<16;i++)assert.equal(b[i],image.get(a+i),'Selected identity mismatch');
  }
  const raw=await ram('fd_sector_diag'),benchRaw=await ram(light?'fd_light_log':'fd_power_bench');
  const result=decode(raw),bench=light?decodeLight(benchRaw):decodeBench(benchRaw,30);
  if(light){
   assert.equal(benchRaw.readUInt32LE(16),0,'Active light trial');
   assert.equal(bench.completed,bench.trials);
   assert.equal(result.header.trial,bench.trials);
  }else{
   delete bench.startup_profile;
   bench.caveats=['60ms positioning included in360ms hard acquisition deadline; no one-second motor stop after handoff.'];
   assert.equal(result.header.trial,bench.header.trials);
  }
  const settings=await ram('eepromBuffer'),async=await ram('fd_async_diag');
  let alignment=null,alignRaw=null;
  if(syms.fd_align_observe){
   assert.equal(syms.fd_align_observe.n,808);alignRaw=await ram('fd_align_observe');
   alignment=require('./fd6288_align_decode.cjs').decode(alignRaw);
   assert.equal(alignment.trial,result.header.trial);
  }
  await gate();assert.deepEqual(await ram('fd_sector_diag'),raw);assert.deepEqual(await ram(light?'fd_light_log':'fd_power_bench'),benchRaw);
  if(alignRaw)assert.deepEqual(await ram('fd_align_observe'),alignRaw);
  let reposition=null;
  if(syms.fd_reposition_diag){
   assert.equal(syms.fd_reposition_diag.n,32);
   const b=await ram('fd_reposition_diag');
   assert.equal(b.readUInt32LE(0),0x46525031);assert.equal(b.readUInt32LE(4),result.header.trial);
   reposition={trial:b.readUInt32LE(4),reason:b.readUInt32LE(24)};
   for(const [name,offset] of [['release_us',8],['realign_us',12],['retry_first_edge_us',16],['handoff_us',20],['finish_us',28]])
    reposition[name]=b.readUInt32LE(offset)===0xffffffff?null:b.readUInt32LE(offset)/160;
   assert.deepEqual(await ram('fd_reposition_diag'),b);
  }
  let edge_observer=null;
  if(syms.fd_edge_observe){
   assert([592,12404].includes(syms.fd_edge_observe.n));const b=await ram('fd_edge_observe');
   edge_observer=require('./fd6288_edge_decode.cjs').decode(b);
   assert.equal(edge_observer.header.trial,result.header.trial);
   assert.deepEqual(await ram('fd_edge_observe'),b);
  }
  const output={capturedAt:new Date().toISOString(),elf:path.resolve(elf),identity:'Known flashed session and selected code bytes, not full flash',...result,bench,alignment,reposition,edge_observer,settings_hex:settings.toString('hex'),async_hex:async.toString('hex')};
  fs.writeFileSync(out,raw,{flag:'wx'});fs.writeFileSync(out+'.json',JSON.stringify(output,null,2),{flag:'wx'});
  console.log(JSON.stringify(output,null,2));
 }finally{c.close()}
}
module.exports={decode};
if(require.main===module)(async()=>{const a=process.argv.slice(2);if(a[0]==='decode')console.log(JSON.stringify(decode(fs.readFileSync(a[1])),null,2));else if(a[0]==='dump')await dump(a[1],a[2],a.includes('--stopped'));else throw Error('decode FILE | dump MATCHING.elf NEW.bin --stopped');})().catch(e=>{console.error(e.message);process.exitCode=1});
