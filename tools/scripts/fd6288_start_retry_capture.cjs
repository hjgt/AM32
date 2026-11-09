// Read-only latest startup/retry episode. No halt/reset/RAM writes/flash/throttle.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');
const {connect,parseHex}=require('./fd6288_startup_capture.cjs');
const {decode}=require('./fd6288_start_retry_decode.cjs');
async function dump(elf,out,stopped){
 assert(stopped,'Require --stopped after operator confirmation');assert(!fs.existsSync(out),'Output already exists');
 const nm=cp.execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'}),syms={};
 for(const row of nm.split('\n')){const m=row.match(/^([0-9a-f]+) ([0-9a-f]+) \w (.+)$/);if(m)syms[m[3]]={address:parseInt(m[1],16),size:parseInt(m[2],16)};}
 const SIZE=syms.fd_start_retry_diag?.size;assert([168,288].includes(SIZE),'Matching ALIGN_RETRY ELF required');
 const hex=parseHex(fs.readFileSync(elf.replace(/\.elf$/,'.hex'),'utf8'));
 const c=await connect();
 try{
  async function mem(address,n){const r=await c.rpc(`read_memory 0x${address.toString(16)} 8 ${n}`),v=r.trim().split(/\s+/).map(Number);assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=255));return Buffer.from(v);}
  async function scalar(name){assert(syms[name],name);const s=syms[name];return(await mem(s.address,s.size)).readUIntLE(0,s.size);}
  async function check(){assert.equal((await c.rpc('[target current] curstate')).trim(),'running');assert.equal((await mem(0xe000ed08,4)).readUInt32LE(),0x08001000);for(const n of ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active'])assert.equal(await scalar(n),0,n);assert.equal((await mem(0x48000010,4)).readUInt32LE()&0x1f00,0);assert.equal((await mem(0x48001410,4)).readUInt32LE()&1,0);for(let i=0;i<4;i++)assert.equal((await mem(0x40012c34+4*i,4)).readUInt32LE(),0);}
  await check();
  // Selected code identity, not a full-flash readback.
  for(const name of ['fdRevTick','zcfoundroutine','fdBenchService','fdBenchStop','main','loadEEpromSettings']){
   const s=syms[name];assert(s&&s.size>0&&s.size<8192,name);
   const b=await mem(s.address,s.size);for(let i=0;i<b.length;i++)assert.equal(b[i],hex.get(s.address+i),'Code identity: '+name);
  }
  const address=syms.fd_start_retry_diag.address,a=await mem(address,SIZE);
  const summary=decode(a),settings=await mem(syms.eepromBuffer.address,48);
  assert.equal(settings[19],0,'Sine startup must be off');assert.equal(settings[24],24,'Expected original24k mainline parameters');
  assert.equal(await scalar('fd_rev_acquiring'),0,'Still acquiring');
  assert.equal(syms.fd_power_bench.size,40);
  const benchRaw=await mem(syms.fd_power_bench.address,40);
  assert.equal(benchRaw.readUInt32LE(32),0,'Active trial');
  assert.equal(benchRaw.readUInt32LE(12),summary.trial,'Trial mismatch');
  await check();assert.deepEqual(await mem(address,SIZE),a,'Summary changed during stopped read');
  async function words(address,n){const v=(await c.rpc(`read_memory 0x${address.toString(16)} 32 ${n}`)).trim().split(/\s+/).map(Number);assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=0xffffffff));return v;}
  const clockA=await words(0xe0001000,2),demcr=(await words(0xe000edfc,1))[0],clockB=await words(0xe0001000,2);
  const clock={ctrl:clockB[0],demcr,cyccnt_first:clockA[1],cyccnt_second:clockB[1],delta:(clockB[1]-clockA[1])>>>0};
  clock.valid=Boolean((clock.ctrl&1)&&(demcr&0x1000000)&&clock.delta>0);
  assert(clock.valid,'Control clock invalid: do not interpret startup timings');
  await check();assert.deepEqual(await mem(syms.fd_power_bench.address,40),benchRaw,'Bench changed');
  const result={read_at:new Date().toISOString(),elf:path.resolve(elf),summary,clock,
    ram_settings:{sine:settings[19],pwm_frequency_khz:settings[24],startup_power:settings[25],motor_kv:settings[26]*40+20,motor_poles:settings[27],advance_raw:settings[23],auto_advance:settings[47]},
    bench_raw_hex:benchRaw.toString('hex'),raw_hex:a.toString('hex')};
  fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify(result,null,2));return result;
 }finally{c.close();}
}
if(require.main===module){const [elf,out,...flags]=process.argv.slice(2);assert(elf&&out,'Usage: node fd6288_start_retry_capture.cjs MATCHING.elf NEW.json --stopped');dump(elf,out,flags.includes('--stopped')).catch(e=>{console.error(e);process.exitCode=1;});}
module.exports={dump};
