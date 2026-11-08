// Read-only event summary. Run only after the operator has returned throttle
// to zero and confirmed the motor stopped. Never halts, resets or writes RAM.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');
const {connect,parseHex}=require('./fd6288_startup_capture.cjs');
const fields=['magic','abi','trial','started','phase','reason','sine_steps','sixstep_at','first_edge_at','sixth_edge_at','stopped_at','edges','input_at_start','input_at_handoff','last_step'];
const pwmFields=['magic','abi','trial','steps','cr1','psc','arr','ccer','bdtr','ccr1','ccr2','ccr3','ccr4','gpioa_moder','gpiof_moder','gpioa_afrh','gpiof_afrl','phase_a','phase_b','phase_c'];
function decodePwm(b){assert.equal(b.length,80);const d=Object.fromEntries(pwmFields.map((n,i)=>[n,b.readUInt32LE(i*4)]));assert.equal(d.magic,0x46535031);assert.equal(d.abi,1);return d;}
function decode(b){assert.equal(b.length,60);const d=Object.fromEntries(fields.map((n,i)=>[n,b.readUInt32LE(i*4)]));assert.equal(d.magic,0x46534e31);assert.equal(d.abi,1);d.phase_name=['idle','sine','sixstep','six_real_edges','stopped'][d.phase];for(const n of ['sixstep_at','first_edge_at','sixth_edge_at','stopped_at'])d[n+'_ms']=d[n]===0xffffffff?null:d[n]/160000;return d;}
async function dump(elf,out,stopped){
 assert(stopped,'Require --stopped after operator confirmation');assert(!fs.existsSync(out),'Output already exists');
 const nm=cp.execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'}),syms={};
 for(const row of nm.split('\n')){const m=row.match(/^([0-9a-f]+) ([0-9a-f]+) \w (.+)$/);if(m)syms[m[3]]={address:parseInt(m[1],16),size:parseInt(m[2],16)};}
 assert.equal(syms.fd_native_sine?.size,60,'Matching native-sine ELF required');
 const hex=parseHex(fs.readFileSync(elf.replace(/\.elf$/,'.hex'),'utf8'));
 const c=await connect();
 try{
  async function mem(address,n){const r=await c.rpc(`read_memory 0x${address.toString(16)} 8 ${n}`),v=r.trim().split(/\s+/).map(Number);assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=255));return Buffer.from(v);}
  async function scalar(name){assert(syms[name],name);const s=syms[name];return(await mem(s.address,s.size)).readUIntLE(0,s.size);}
  async function check(){assert.equal((await c.rpc('[target current] curstate')).trim(),'running');assert.equal((await mem(0xe000ed08,4)).readUInt32LE(),0x08001000);for(const n of ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active'])assert.equal(await scalar(n),0,n);assert.equal((await mem(0x48000010,4)).readUInt32LE()&0x1f00,0);assert.equal((await mem(0x48001410,4)).readUInt32LE()&1,0);for(let i=0;i<4;i++)assert.equal((await mem(0x40012c34+4*i,4)).readUInt32LE(),0);}
  await check();
  // Selected code identity, not a full-flash readback.
  for(const name of ['advanceincrement','loadEEpromSettings','fdBenchService','fdBenchStop','fdNativeSineBridgeAllowed','main']){const s=syms[name];assert(s&&s.size>0&&s.size<8192,name);const b=await mem(s.address,s.size);for(let i=0;i<b.length;i++)assert.equal(b[i],hex.get(s.address+i),'Code identity: '+name);}
  const address=syms.fd_native_sine.address,a=await mem(address,60),settings=await mem(syms.eepromBuffer.address,48);
  await check();assert.deepEqual(await mem(address,60),a,'Summary changed during stopped read');
  let pwm_snapshot=null;
  if(syms.fd_sine_pwm_snapshot){const s=syms.fd_sine_pwm_snapshot;assert.equal(s.size,80);const b=await mem(s.address,80);assert.deepEqual(await mem(s.address,80),b,'PWM snapshot changed during stopped read');pwm_snapshot=decodePwm(b);assert.equal(pwm_snapshot.trial,decode(a).trial,'PWM snapshot trial mismatch');}
  async function words(address,n){const v=(await c.rpc(`read_memory 0x${address.toString(16)} 32 ${n}`)).trim().split(/\s+/).map(Number);assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=0xffffffff));return v;}
  const clockA=await words(0xe0001000,2),demcr=(await words(0xe000edfc,1))[0],clockB=await words(0xe0001000,2);
  const clock={ctrl:clockB[0],demcr,cyccnt_first:clockA[1],cyccnt_second:clockB[1],delta:(clockB[1]-clockA[1])>>>0,selftest_ok:syms.fd_native_clock_ok?await scalar('fd_native_clock_ok'):null};
  clock.valid=Boolean((clock.ctrl&1)&&(demcr&0x1000000)&&clock.delta>0&&clock.selftest_ok!==0);
  const summary=decode(a);summary.timing_valid=clock.valid;
  if(!clock.valid)for(const n of ['sixstep_at','first_edge_at','sixth_edge_at','stopped_at'])summary[n+'_ms']=null;
  const result={read_at:new Date().toISOString(),elf:path.resolve(elf),summary,clock,pwm_snapshot,ram_settings:{sine:settings[19],startup_power:settings[25],motor_kv:settings[26]*40+20,motor_poles:settings[27],advance_raw:settings[23],auto_advance:settings[47],sine_range:settings[40],sine_power:settings[45]},raw_hex:a.toString('hex')};
  fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify(result,null,2));return result;
 }finally{c.close();}
}
if(require.main===module){const [elf,out,...flags]=process.argv.slice(2);assert(elf&&out,'Usage: node fd6288_native_sine_capture.cjs MATCHING.elf NEW.json --stopped');dump(elf,out,flags.includes('--stopped')).catch(e=>{console.error(e);process.exitCode=1;});}
module.exports={decode,decodePwm,dump};
