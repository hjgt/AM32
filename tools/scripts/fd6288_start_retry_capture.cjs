// Read-only latest startup/retry episode. No halt/reset/RAM writes/flash/throttle.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');
const {connect,parseHex}=require('./fd6288_startup_capture.cjs');
const {decode}=require('./fd6288_start_retry_decode.cjs');
async function dump(elf,out,stopped){
 assert(stopped,'Require --stopped after operator confirmation');assert(!fs.existsSync(out),'Output already exists');
 const nm=cp.execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'}),syms={};
 for(const row of nm.split('\n')){const m=row.match(/^([0-9a-f]+) ([0-9a-f]+) \w (.+)$/);if(m)syms[m[3]]={address:parseInt(m[1],16),size:parseInt(m[2],16)};}
 const rampUs=syms.fd_m2_ramp_us;
 const ramp=rampUs??syms.fd_m2_ramp_ms;
 const SIZE=syms.fd_start_retry_diag?.size;
 const highZ=Boolean(syms.fd_hiz_active && SIZE===88);
 assert((highZ?[88]:[168,288]).includes(SIZE),'Matching M2 or ALIGN_RETRY ELF required');
 const hex=parseHex(fs.readFileSync(elf.replace(/\.elf$/,'.hex'),'utf8'));
 const c=await connect();
 try{
  async function mem(address,n){
   const chunks=[];
   for(let i=0;i<n;i+=128){
    const length=Math.min(128,n-i),command=`read_memory 0x${(address+i).toString(16)} 8 ${length}`;
    let reply;
    for(let attempt=0;attempt<3;attempt++){
     try{reply=await c.rpc(command);break;}catch(error){if(attempt===2)throw error;}
    }
    const bytes=reply.trim().split(/\s+/).map(Number);
    assert.equal(bytes.length,length);
    assert(bytes.every(x=>Number.isInteger(x)&&x>=0&&x<=255));
    chunks.push(Buffer.from(bytes));
   }
   return Buffer.concat(chunks);
  }
  async function scalar(name){assert(syms[name],name);const s=syms[name];return(await mem(s.address,s.size)).readUIntLE(0,s.size);}
  async function check(){assert.equal((await c.rpc('[target current] curstate')).trim(),'running');assert.equal((await mem(0xe000ed08,4)).readUInt32LE(),0x08001000);for(const n of ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active'])assert.equal(await scalar(n),0,n);assert.equal((await mem(0x48000010,4)).readUInt32LE()&0x1f00,0);assert.equal((await mem(0x48001410,4)).readUInt32LE()&1,0);for(let i=0;i<4;i++)assert.equal((await mem(0x40012c34+4*i,4)).readUInt32LE(),0);}
  await check();
  // Selected code identity, not a full-flash readback.
  for(const name of ['fdRevTick','zcfoundroutine','fdBenchService','fdBenchStop','main','loadEEpromSettings']){
   const s=syms[name];assert(s&&s.size>0&&s.size<8192,name);
   const b=await mem(s.address,s.size);for(let i=0;i<b.length;i++)assert.equal(b[i],hex.get(s.address+i),'Code identity: '+name);
  }
  const address=syms.fd_start_retry_diag.address,a=await mem(address,SIZE);
  let ramp_sector_ms;
  if(ramp){
   const bytes=await mem(ramp.address,ramp.size);
   for(let i=0;i<bytes.length;i++)assert.equal(bytes[i],hex.get(ramp.address+i),'Ramp cadence identity');
   if(rampUs){
    assert((highZ?[4,6]:[12]).includes(bytes.length),'Expected M2 microsecond cadence');
    ramp_sector_ms=Array.from({length:bytes.length/2},(_,i)=>bytes.readUInt16LE(i*2)/1000);
   }else{
    // The original two-step firmware kept the full six-sector table in its ELF.
    if(highZ)assert([2,3,6].includes(bytes.length),'Expected M2 high-Z cadence');
    ramp_sector_ms=Array.from(highZ && bytes.length===6 ? bytes.subarray(0,2) : bytes);
   }
  }
  const wave=syms.fd_edge_observe;
  const waveAbi=highZ?(await mem(wave.address+4,4)).readUInt32LE():10;
  if(highZ&&!ramp){
   // In the two-vector 40 ms profile the compiler folds both dwells into fdRevTick.
   // Its code identity was checked above. ABI11 is exclusive to that profile.
   assert.equal(waveAbi,11,'Missing cadence table for unrecognized high-Z profile');
   ramp_sector_ms=[12,12];
  }
  const summary=decode(a,ramp_sector_ms,highZ,waveAbi===11?40:5),settings=await mem(syms.eepromBuffer.address,48);
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
  let startup_wave, step4_adc_pair, step35_adc_pair, step6_adc_pair;
  let low_speed_pickup, powered_capture, handoff_capture, timed_runup, runup_adc;
  const pickup=syms.fd_m2_pickup_capture;
  if(pickup){
   assert(highZ && pickup.size===24);
   const [trial,coastAt,resumeAt,period,sector,duty]=await words(pickup.address,6);
   assert.equal(trial,summary.trial);
   assert(sector<=6&&duty<=400);
   if(sector)assert(coastAt>0&&resumeAt>coastAt&&resumeAt-coastAt<6400000&&period>=640000&&period<=1600000);
   else assert(!resumeAt&&!period&&!duty);
   low_speed_pickup={coast_ms:coastAt?coastAt/160000:null,resume_ms:resumeAt?resumeAt/160000:null,
    sector:sector||null,sector_period_ms:period?period/160000:null,requested_duty:duty,drive_limit_ms:100};
   summary.total_limit_ms+=100;
  }
  let step5_coast;
  const comparison=syms.fd_m2_step5_coast;
  if(comparison){
   assert(pickup&&comparison.size===20);
   const [trial,pulseUs,edgeAt,step5At,offAt]=await words(comparison.address,5);
   assert.equal(trial,summary.trial);
   assert([0,4000,9000].includes(pulseUs));
   assert(!step5At||(pulseUs&&edgeAt&&step5At>=edgeAt));
   assert(!offAt||(edgeAt&&offAt>=edgeAt));
   const ms=cycles=>cycles?cycles/160000:null;
   step5_coast={pulse_requested_us:pulseUs,trigger_ms:ms(edgeAt),
    step5_ms:ms(step5At),bridge_off_ms:ms(offAt),
    pulse_actual_us:step5At&&offAt?(offAt-step5At)/160:null};
  }
  const runup=syms.fd_m2_runup;
  if(runup){
   assert.equal(runup.size,104);
   const raw=await mem(runup.address,runup.size);
   assert.equal(raw.readUInt32LE(0),summary.trial);
   const count=raw.readUInt32LE(4);
   assert(count<=6);
   const sectors=Array.from({length:count},(_,i)=>{
    const o=8+i*16, step=raw[o+12], edge=raw.readUInt32LE(o+8);
    assert.equal(step,i+1);
    return {step,start_ms:raw.readUInt32LE(o)/160000,dwell_ms:raw.readUInt32LE(o+4)/160000,
     adc_event_ms:edge?edge/160000:null,qualified:!!raw[o+13],streak:raw[o+14],
     phase_anchored:!!raw[o+15]};
   });
   timed_runup={sectors,handoff_ms:count?summary.attempts[0].handoff_ms:null};
  }
  if(process.argv.includes('--wave')){
   assert.equal(wave?.size,12404,'Missing startup waveform buffer');
   const bytes=await mem(wave.address,wave.size);
   assert((highZ?[7,11]:[10]).includes(waveAbi),'Expected matching M2 waveform mode');
   assert.equal(bytes.readUInt32LE(4),waveAbi,'Waveform mode changed');
   startup_wave=require('./fd6288_edge_decode.cjs').decode(bytes,highZ);
   assert.equal(startup_wave.header.trial,summary.trial);
   if(step5_coast){
    startup_wave.window=step5_coast.bridge_off_ms?'after_comparison_release':'before_pickup';
    if(step5_coast.bridge_off_ms)
     assert(Math.abs(startup_wave.header.start0/160000-step5_coast.bridge_off_ms)<0.01);
   }
   const pair=syms.fd_step4_pair;
   if(pair){
    assert(highZ && ramp_sector_ms.length===3);
    assert.equal(await scalar('fd_step4_pair_compare'),0);
    step4_adc_pair=require('./fd6288_edge_decode.cjs').decodeStep4Pair(
     await mem(pair.address,pair.size),ramp_sector_ms[2]);
    assert.equal(step4_adc_pair.trial,summary.trial);
   }
   const onPair=syms.fd_m2_on_adc_pair||syms.fd_m2_step35_pair;
   if(onPair){
    assert.equal(onPair.size,2316);
    const head=await mem(onPair.address,12), count=head.readUInt32LE(4);
    assert(count<=96);
    const records=count?await mem(onPair.address+12,count*24):Buffer.alloc(0);
    const capture=require('./fd6288_edge_decode.cjs').decodeOnAdcPair(Buffer.concat([head,records]));
    assert.equal(capture.trial,summary.trial);
    if(syms.fd_m2_step5_coast)step35_adc_pair=capture;
    else step6_adc_pair=capture;
    assert.deepEqual(await mem(onPair.address,12),head,'PWM-on capture changed');
   }
   const powered=syms.fd_m2_powered_capture;
   if(powered){
    assert(pickup&&powered.size===6272);
    const head=await mem(powered.address,128),stored=head.readUInt32LE(12);
    assert(stored<=256);
    const samples=await mem(powered.address+128,stored*24);
    powered_capture=require('./fd6288_edge_decode.cjs').decodePoweredCapture(Buffer.concat([head,samples]));
    assert.equal(powered_capture.trial,summary.trial);
    assert.deepEqual(await mem(powered.address,128),head,'Powered capture changed');
   }
   const handoff=syms.fd_m2_handoff;
   if(handoff){
    assert.equal(handoff.size,2364);
    const head=await mem(handoff.address,60), count=head.readUInt32LE(16);
    assert(count<=96);
    const records=count?await mem(handoff.address+60,count*24):Buffer.alloc(0);
    handoff_capture=require('./fd6288_edge_decode.cjs').decodeHandoffCapture(Buffer.concat([head,records]));
    assert.equal(handoff_capture.trial,summary.trial);
    assert.equal(Boolean(handoff_capture.stop_reason),Boolean(summary.successful_attempt));
    if(handoff_capture.stop_reason)assert.equal(handoff_capture.stop_reason,summary.stop_reason);
    assert.deepEqual(await mem(handoff.address,60),head,'Handoff capture changed');
   }
   const runupAdc=syms.fd_m2_runup_adc;
   if(runupAdc){
    assert(timed_runup&&runupAdc.size===1320);
    const raw=await mem(runupAdc.address,runupAdc.size);
    runup_adc=require('./fd6288_edge_decode.cjs').decodeRunupAdc(raw);
    assert.equal(runup_adc.trial,summary.trial);
    for(const sector of runup_adc.sectors)
     if(sector.samples.length)assert(timed_runup.sectors.some(s=>s.step===sector.step));
   }
   await check();
   assert.deepEqual(await mem(address,SIZE),a,'Trial changed during waveform read');
  }
  const result={startup_wave,step4_adc_pair,step35_adc_pair,step6_adc_pair,low_speed_pickup,step5_coast,powered_capture,timed_runup,runup_adc,handoff_capture,read_at:new Date().toISOString(),elf:path.resolve(elf),summary,clock,
    ram_settings:{sine:settings[19],pwm_frequency_khz:settings[24],startup_power:settings[25],motor_kv:settings[26]*40+20,motor_poles:settings[27],advance_raw:settings[23],auto_advance:settings[47]},
    bench_raw_hex:benchRaw.toString('hex'),raw_hex:a.toString('hex')};
  const pairSummary=p=>p&&{records:p.records.length,pairs:p.pairs.length};
  const runupAdcSummary=runup_adc?.sectors.map(s=>({step:s.step,samples:s.samples.length}));
  fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n',{flag:'wx'});console.log(JSON.stringify({...result,startup_wave:startup_wave?.header,step4_adc_pair:pairSummary(step4_adc_pair),step35_adc_pair:pairSummary(step35_adc_pair),step6_adc_pair:pairSummary(step6_adc_pair),powered_capture:powered_capture&&{records:powered_capture.records.length,sectors:powered_capture.sectors},runup_adc:runupAdcSummary,handoff_capture:handoff_capture&&{...handoff_capture,records:handoff_capture.records.length}},null,2));return result;
 }finally{c.close();}
}
if(require.main===module){const [elf,out,...flags]=process.argv.slice(2);assert(elf&&out,'Usage: node fd6288_start_retry_capture.cjs MATCHING.elf NEW.json --stopped [--wave]');dump(elf,out,flags.includes('--stopped')).catch(e=>{console.error(e);process.exitCode=1;});}
module.exports={dump};
