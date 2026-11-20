// Read-only event summary. Run only after the operator has returned throttle
// to zero and confirmed the motor stopped. Never halts, resets or writes RAM.
const fs=require('fs'),path=require('path'),cp=require('child_process'),assert=require('assert/strict');
const {connect,parseHex}=require('./fd6288_startup_capture.cjs');
const {SIZE:PIN_SIZE,decodePins}=require('./fd6288_sine_pin_decode.cjs');
const fields=['magic','abi','trial','started','phase','reason','sine_steps','sixstep_at','first_edge_at','sixth_edge_at','stopped_at','edges','input_at_start','input_at_handoff','last_step'];
const pwmFields=['magic','abi','trial','steps','cr1','psc','arr','ccer','bdtr','ccr1','ccr2','ccr3','ccr4','gpioa_moder','gpiof_moder','gpioa_afrh','gpiof_afrl','phase_a','phase_b','phase_c'];
function decodePwm(b){assert.equal(b.length,80);const d=Object.fromEntries(pwmFields.map((n,i)=>[n,b.readUInt32LE(i*4)]));assert.equal(d.magic,0x46535031);assert.equal(d.abi,1);return d;}
function decodeSensors(b){assert.equal(b.length,40);const names=['magic','abi','trial','updates','baseline_current','peak_current','baseline_voltage','minimum_voltage','baseline_raw_current','peak_raw_current'];const d=Object.fromEntries(names.map((n,i)=>[n,(i===4||i===5)?b.readInt32LE(i*4):b.readUInt32LE(i*4)]));assert.equal(d.magic,0x46535331);assert.equal(d.abi,1);d.units={current:'0.01A filtered',voltage:'0.01V filtered',raw_current:'ADC counts'};d.peak_current_increase=d.peak_current-d.baseline_current;d.voltage_drop=d.baseline_voltage-d.minimum_voltage;d.sampled_only=true;return d;}
function decode(b){assert.equal(b.length,60);const d=Object.fromEntries(fields.map((n,i)=>[n,b.readUInt32LE(i*4)]));assert.equal(d.magic,0x46534e31);assert.equal(d.abi,1);d.phase_name=['idle','sine','sixstep','six_real_edges','stopped'][d.phase];for(const n of ['sixstep_at','first_edge_at','sixth_edge_at','stopped_at'])d[n+'_ms']=d[n]===0xffffffff?null:d[n]/160000;return d;}
function decodeM2Coast(b) {
  const names = ['magic','abi','trial','off_at','rate','angle','frames','clamp_frames',
    'max_gap_cycles','max_span_ticks','seen','count'];
  assert.equal(b.length, 48 + 512 * 12);
  const header = Object.fromEntries(names.map((name, i) => [name, b.readUInt32LE(i * 4)]));
  assert.equal(header.magic, 0x464d5331);
  assert.equal(header.abi, 1);
  assert(header.count <= 512 && header.count <= header.seen);
  const samples = Array.from({length: header.count}, (_, i) => {
    const at = 48 + i * 12;
    const a = b.readUInt16LE(at + 4), bPhase = b.readUInt16LE(at + 6), c = b.readUInt16LE(at + 8);
    return {us: b.readUInt32LE(at) / 160, a, b: bPhase, c,
      ab: a - bPhase, bc: bPhase - c, ca: c - a, at_floor: a === 0 || bPhase === 0 || c === 0};
  });
  return {...header, off_ms: header.off_at === 0xffffffff ? null : header.off_at / 160000,
    commanded_mechanical_rpm: header.rate / 72,
    max_update_gap_us: header.max_gap_cycles / 160, samples};
}
function decodeM2Handoff(b) {
  assert(b.length>=32,'Incomplete M2 handoff header');
  assert.equal(b.readUInt32LE(0),0x464d4831);
  const abi=b.readUInt32LE(4);
  assert(abi===1||abi===2,'Unsupported M2 handoff ABI');
  const capacity=abi===1?12:24;
  assert.equal(b.length,32+capacity*16);
  const values=Array.from({length:b.length/4},(_,i)=>b.readUInt32LE(i*4));
  const [,,trial,pickup,coastPeriod,closed,closedPeriod,count]=values;
  assert(count<=capacity);
  const ms=t=>t===0xffffffff?null:t/160000;
  const sectors=Array.from({length:count},(_,i)=>{
    const [at,edge,step,streak]=values.slice(8+i*4,12+i*4);
    return {step,at_ms:ms(at),edge_ms:ms(edge),streak};
  });
  return {trial,pickup_ms:ms(pickup),coast_period_us:coastPeriod/160,
    closed_ms:ms(closed),closed_period_us:closedPeriod/160,sectors};
}
function decodeM2Powered(b) {
  assert(b.length>=12,'Incomplete M2 powered header');
  assert.equal(b.readUInt32LE(0),0x464d5031);
  const abi=b.readUInt32LE(4);
  assert(abi===1||abi===2,'Unsupported M2 powered ABI');
  const capacity=abi===1?12:24;
  const headerSize=12+capacity*2;
  assert.equal(b.length,headerSize+capacity*24*12);
  const sectors=Array.from({length:capacity},(_,i)=>{
    const count=b.readUInt16LE(12+i*2);
    assert(count<=24);
    return Array.from({length:count},(_,j)=>{
      const p=headerSize+(i*24+j)*12, flags=b.readUInt16LE(p+10);
      return {us:b.readUInt32LE(p)/160,a:b.readUInt16LE(p+4),b:b.readUInt16LE(p+6),c:b.readUInt16LE(p+8),
        edge:!!(flags&1),qualified:!!(flags&2),rising:!!(flags&4),trigger_ticks:(flags>>>3)||null};
    });
  });
  return {trial:b.readUInt32LE(8),sectors};
}
function decodeM2Adaptation(b) {
  assert.equal(b.length,400);
  assert.equal(b.readUInt32LE(0),0x464d4131);
  assert.equal(b.readUInt32LE(4),1);
  const trial=b.readUInt32LE(8),count=b.readUInt32LE(12);
  assert(count<=24);
  const records=Array.from({length:count},(_,i)=>{
    const at=16+i*16;
    const due=b.readUInt32LE(at+8);
    return {period_us:b.readUInt32LE(at)/160,
      cycle_period_us:b.readUInt32LE(at+4)/160,
      due_us:due===0xffffffff?null:due/160,
      trigger_ticks:b.readUInt16LE(at+12),
      decision:b.readUInt8(at+14),front_samples:b.readUInt8(at+15)};
  });
  return {trial,count,records};
}
function decodeM2Step4Coast(b) {
  assert.equal(b.length,6176);
  const [magic,abi,trial,off,sector,prior,seen,count]=Array.from({length:8},(_,i)=>b.readUInt32LE(i*4));
  assert.equal(magic,0x464d3431);
  assert.equal(abi,1);
  assert(count<=512&&count<=seen);
  const ms=t=>t===0xffffffff?null:t/160000;
  const samples=Array.from({length:count},(_,i)=>{
    const p=32+i*12, a=b.readUInt16LE(p+4), bPhase=b.readUInt16LE(p+6), c=b.readUInt16LE(p+8);
    return {us:b.readUInt32LE(p)/160,a,b:bPhase,c,ab:a-bPhase,bc:bPhase-c,ca:c-a,
      at_floor:a===0||bPhase===0||c===0};
  });
  return {trial,off_ms:ms(off),step4_ms:ms(sector),step3_edge_ms:ms(prior),
    drive_us:off===0xffffffff?null:((off-sector)>>>0)/160,seen,count,samples};
}
function decodeM2Recovery(b) {
  assert.equal(b.length,3120);
  const names=['magic','abi','trial','attempts','off_at','resumed_at',
    'reference_period','measured_period','first_sector','step','seen','count'];
  const d=Object.fromEntries(names.map((name,i)=>[name,b.readUInt32LE(i*4)]));
  assert.equal(d.magic,0x464d5231);assert.equal(d.abi,1);
  assert(d.attempts<=1&&d.count<=256&&d.count<=d.seen&&d.first_sector<=24&&d.step<=6);
  assert(d.attempts||!d.count);
  const ms=value=>value===0xffffffff?null:value/160000;
  const samples=Array.from({length:d.count},(_,i)=>{
    const at=48+i*12,a=b.readUInt16LE(at+4),bp=b.readUInt16LE(at+6),c=b.readUInt16LE(at+8);
    return {us:b.readUInt32LE(at)/160,a,b:bp,c,ab:a-bp,bc:bp-c,ca:c-a,at_floor:a===0||bp===0||c===0};
  });
  return {...d,off_ms:ms(d.off_at),resumed_ms:ms(d.resumed_at),
    reference_period_us:d.reference_period/160,measured_period_us:d.measured_period/160,samples};
}

function decodeM2Closed(b) {
  assert.equal(b.length,528);assert.equal(b.readUInt32LE(0),0x464d4c31);assert.equal(b.readUInt32LE(4),1);
  const trial=b.readUInt32LE(8),count=b.readUInt32LE(12),length=Math.min(count,32);
  const first=count>32?count%32:0;
  const events=Array.from({length},(_,i)=>{
    const at=16+((first+i)%32)*16,step=b.readUInt8(at+12),kind=b.readUInt8(at+13);
    assert(step>=1&&step<=6&&kind<=2);
    return {at_ms:b.readUInt32LE(at)/160000,elapsed_us:b.readUInt32LE(at+4)/2,
      duty_ticks:b.readUInt16LE(at+8),trigger_ticks:b.readUInt16LE(at+10),
      step,kind,event:['commutation','edge','stop'][kind]};
  });
  return {trial,count,events};
}

function decodeM2AdcCapture(b) {
  assert.equal(b.length,3152,'M2 ADC recorder size');
  const names=['magic','abi','trial','count','state','trigger_at','trigger_step',
    'trigger_elapsed','trigger_period','post_remaining','max_stage_cycles',
    'max_commit_cycles','max_irq_cycles','irq_count','first_at','budget_trips',
    'reserved0','reserved1','reserved2','reserved3'];
  const header=Object.fromEntries(names.map((name,i)=>[name,b.readUInt32LE(i*4)]));
  assert.equal(header.magic,0x464d4431);
  assert.equal(header.abi,1);
  assert(header.state<=4,'M2 ADC recorder state');
  assert(header.trigger_step<=6,'M2 ADC trigger step');
  const length=Math.min(header.count,128);
  const first=header.count>128?header.count%128:0;
  const oldestCycles=length?b.readUInt32LE(80+first*24):0;
  const samples=Array.from({length},(_,i)=>{
    const at=80+((first+i)%128)*24;
    const cycles=b.readUInt32LE(at);
    const step=b.readUInt8(at+20),decision=b.readUInt8(at+21);
    const flags=b.readUInt8(at+22),qualification=b.readUInt8(at+23);
    assert(step>=1&&step<=6,'M2 ADC sample step');
    assert(decision<10,'M2 ADC sample decision');
    assert((flags&0xc0)===0,'M2 ADC sample flags');
    return {at_ms:cycles/160000,window_us:((cycles-oldestCycles)>>>0)/160,
      a:b.readUInt16LE(at+4),b:b.readUInt16LE(at+6),c:b.readUInt16LE(at+8),
      elapsed_us:b.readUInt16LE(at+10)/2,period_us:b.readUInt16LE(at+12)/2,
      duty_ticks:b.readUInt16LE(at+14),trigger_ticks:b.readUInt16LE(at+16),
      read_counter:b.readUInt16LE(at+18),step,decision,flags,
      rising:!!(flags&1),sample_valid:!!(flags&2),before_seen:!!(flags&4),
      reported:!!(flags&8),accepted:!!(flags&16),collision:!!(flags&32),
      blank_before:qualification&15,confirm_after:qualification>>>4};
  });
  const ms=value=>value===0xffffffff?null:value/160000;
  return {...header,state_name:['recording','post_trigger','fault_frozen',
      'stopped_frozen','budget_stopped'][header.state],
    trigger_at_ms:ms(header.trigger_at),first_at_ms:ms(header.first_at),
    trigger_elapsed_us:header.trigger_elapsed/2,trigger_period_us:header.trigger_period/2,
    max_stage_us:header.max_stage_cycles/160,max_commit_us:header.max_commit_cycles/160,
    max_irq_us:header.max_irq_cycles/160,
    window_span_us:samples.length?samples[samples.length-1].window_us:0,samples};
}

function decodeM2Wire(b,poles=24) {
  assert.equal(b.length,1056);assert.equal(b.readUInt32LE(0),0x464d5731);assert.equal(b.readUInt32LE(4),1);
  assert(poles>=2&&poles%2===0);
  const names=['magic','abi','trial','count','origin','last_at','max_correction','corrected_edges'];
  const d=Object.fromEntries(names.map((n,i)=>[n,b.readUInt32LE(i*4)]));
  const length=Math.min(d.count,128),first=d.count>128?d.count%128:0;
  const firstCycles=length?b.readUInt32LE(32+first*8):0;
  const samples=Array.from({length},(_,i)=>{
    const at=32+((first+i)%128)*8,period=b.readUInt16LE(at+4),payload=b.readUInt16LE(at+6);
    assert(payload<=0xfff);
    const decoded=(payload&511)<<(payload>>>9);
    return {at_ms:b.readUInt32LE(at)/160000,
      window_us:((b.readUInt32LE(at)-firstCycles)>>>0)/160,raw_period_us:period,payload,
      wire_period_us:decoded,wire_mechanical_rpm:payload===0xfff?0:
        decoded?60e6/decoded/(poles/2):null};
  });
  return {...d,poles,max_correction_us:d.max_correction/160,samples};
}

function decodeM2Heli(b,origin=0) {
  assert.equal(b.length,96);assert.equal(b.readUInt32LE(0),0x46485331);
  const names=['magic','abi','trial','rise_ms','at','last_edge','period','turn_sum',
    'turn_edges','last_step','good_turns','bad_turns','healthy','output','requested',
    'credit','reason','holds','first_hold_at','first_hold_duty','first_hold_age',
    'first_hold_period','first_hold_reason','active'];
  const d=Object.fromEntries(names.map((n,i)=>[n,b.readUInt32LE(i*4)]));
  assert.equal(d.abi,1);assert(d.reason<=4&&d.first_hold_reason<=4);
  const reason=['rising','at_target','bad_feedback','overdue_feedback','stopped'];
  return {...d,reason_name:reason[d.reason],trusted_sector_us:d.period/160,
    output_percent:d.output/20,requested_percent:d.requested/20,
    first_hold:d.holds?{at_ms:((d.first_hold_at-origin)>>>0)/160000,
      reason:reason[d.first_hold_reason],duty_percent:d.first_hold_duty/20,
      feedback_age_us:d.first_hold_age/160,trusted_sector_us:d.first_hold_period/160}:null};
}

async function dump(elf,out,stopped){
 assert(stopped,'Require --stopped after operator confirmation');assert(!fs.existsSync(out),'Output already exists');
 const nm=cp.execFileSync('arm-none-eabi-nm',['-S','--defined-only',elf],{encoding:'utf8'}),syms={};
 for(const row of nm.split('\n')){const m=row.match(/^([0-9a-f]+) ([0-9a-f]+) \w (.+)$/);if(m)syms[m[3]]={address:parseInt(m[1],16),size:parseInt(m[2],16)};}
 assert.equal(syms.fd_native_sine?.size,60,'Matching native-sine ELF required');
 const hex=parseHex(fs.readFileSync(elf.replace(/\.elf$/,'.hex'),'utf8'));
 const c=await connect();
 try{
  async function mem(address,n){
    if(n>512){
      const chunks=[];
      for(let offset=0;offset<n;offset+=512)
        chunks.push(await mem(address+offset,Math.min(512,n-offset)));
      return Buffer.concat(chunks);
    }
    const r=await c.rpc(`read_memory 0x${address.toString(16)} 8 ${n}`);
    const values=r.trim().split(/\s+/).map(Number);
    assert.equal(values.length,n);
    assert(values.every(x=>Number.isInteger(x)&&x>=0&&x<=255));
    return Buffer.from(values);
  }
  async function scalar(name){assert(syms[name],name);const s=syms[name];return(await mem(s.address,s.size)).readUIntLE(0,s.size);}
  async function check(){assert.equal((await c.rpc('[target current] curstate')).trim(),'running');assert.equal((await mem(0xe000ed08,4)).readUInt32LE(),0x08001000);for(const n of ['input','adjusted_input','newinput','running','duty_cycle','heli_coast_active'])assert.equal(await scalar(n),0,n);assert.equal((await mem(0x48000010,4)).readUInt32LE()&0x1f00,0);assert.equal((await mem(0x48001410,4)).readUInt32LE()&1,0);for(let i=0;i<4;i++)assert.equal((await mem(0x40012c34+4*i,4)).readUInt32LE(),0);}
  await check();
  // Selected code identity, not a full-flash readback.
  const outputIdentity=syms.fd_m2_sine_coast?['fdAdcJeosService','fdNativeSineBridgeAllowed']:syms.fdSineClampApply?['fdSineClampApply','phaseAPWM','phaseBLOW','phaseCPWM']:syms.fdSixStepAlignApply?['fdSixStepAlignApply','comStep']:['advanceincrement','fdNativeSineBridgeAllowed'];
  for(const name of [...outputIdentity,'loadEEpromSettings','fdBenchService','fdBenchStop','main']){const s=syms[name];assert(s&&s.size>0&&s.size<8192,name);const b=await mem(s.address,s.size);for(let i=0;i<b.length;i++)assert.equal(b[i],hex.get(s.address+i),'Code identity: '+name);}
  const address=syms.fd_native_sine.address,a=await mem(address,60),settings=await mem(syms.eepromBuffer.address,48);
  await check();assert.deepEqual(await mem(address,60),a,'Summary changed during stopped read');
  let pwm_snapshot=null;
  if(syms.fd_sine_pwm_snapshot){const s=syms.fd_sine_pwm_snapshot;assert.equal(s.size,80);const b=await mem(s.address,80);assert.deepEqual(await mem(s.address,80),b,'PWM snapshot changed during stopped read');pwm_snapshot=decodePwm(b);assert.equal(pwm_snapshot.trial,decode(a).trial,'PWM snapshot trial mismatch');}
  let pin_trace=null;
  if(syms.fd_sine_pins){const s=syms.fd_sine_pins;assert.equal(s.size,PIN_SIZE);const b=await mem(s.address,PIN_SIZE);assert.deepEqual(await mem(s.address,PIN_SIZE),b,'Pin trace changed during stopped read');assert(pwm_snapshot);pin_trace=decodePins(b,pwm_snapshot,decode(a).last_step);await check();}
  let sensors=null;
  if(syms.fd_sine_sensors){const s=syms.fd_sine_sensors;assert.equal(s.size,40);const b=await mem(s.address,40);assert.deepEqual(await mem(s.address,40),b,'Sensor summary changed during stopped read');sensors=decodeSensors(b);assert.equal(sensors.trial,decode(a).trial,'Sensor summary trial mismatch');await check();}
  let coast=null;
  if (syms.fd_m2_sine_coast) {
    const s=syms.fd_m2_sine_coast;
    assert.equal(s.size,6192);
    const chunks=[];
    for (let offset=0; offset<s.size; offset+=512) {
      const size=Math.min(512,s.size-offset);
      const chunk=await mem(s.address+offset,size);
      chunks.push(chunk);
    }
    coast=decodeM2Coast(Buffer.concat(chunks));
    assert.equal(coast.trial,decode(a).trial,'Coast trial mismatch');
    assert.deepEqual(await mem(s.address,48),Buffer.concat(chunks).subarray(0,48),'Coast header changed during stopped read');
    assert.deepEqual(await mem(address,60),a,'Trial changed during coast read');
    await check();
  }
  async function words(address,n){const v=(await c.rpc(`read_memory 0x${address.toString(16)} 32 ${n}`)).trim().split(/\s+/).map(Number);assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=0xffffffff));return v;}
  const clockA=await words(0xe0001000,2),demcr=(await words(0xe000edfc,1))[0],clockB=await words(0xe0001000,2);
  const clock={ctrl:clockB[0],demcr,cyccnt_first:clockA[1],cyccnt_second:clockB[1],delta:(clockB[1]-clockA[1])>>>0,selftest_ok:syms.fd_native_clock_ok?await scalar('fd_native_clock_ok'):null};
  clock.valid=Boolean((clock.ctrl&1)&&(demcr&0x1000000)&&clock.delta>0&&clock.selftest_ok!==0);
  const summary=decode(a);summary.timing_valid=clock.valid;
  if(pin_trace)summary.drive_mode=pin_trace.analysis.drive_mode;
  if(!clock.valid)for(const n of ['sixstep_at','first_edge_at','sixth_edge_at','stopped_at'])summary[n+'_ms']=null;
  const result={read_at:new Date().toISOString(),elf:path.resolve(elf),summary,clock,pwm_snapshot,pin_trace,sensors,ram_settings:{sine:settings[19],pwm_frequency_khz:settings[24],startup_power:settings[25],motor_kv:settings[26]*40+20,motor_poles:settings[27],advance_raw:settings[23],auto_advance:settings[47],sine_range:settings[40],sine_power:settings[45]},raw_hex:a.toString('hex')};
  if (coast) result.coast=coast;
  if (syms.fd_m2_sine_handoff) {
    const s=syms.fd_m2_sine_handoff;
    assert(s.size===224||s.size===416,'Unsupported handoff recorder size');
    const raw=await mem(s.address,s.size);
    result.handoff=decodeM2Handoff(raw);
    assert.equal(result.handoff.trial,summary.trial,'Handoff trial mismatch');
    assert.deepEqual(await mem(s.address,s.size),raw,'Handoff changed during stopped read');
    await check();
  }
  if (syms.fd_m2_sine_powered) {
    const s=syms.fd_m2_sine_powered;
    assert(s.size===3492||s.size===6972,'Unsupported powered recorder size');
    const chunks=[];
    for (let offset=0;offset<s.size;offset+=512)
      chunks.push(await mem(s.address+offset,Math.min(512,s.size-offset)));
    const raw=Buffer.concat(chunks);
    const powered=decodeM2Powered(raw);
    assert.equal(powered.trial,summary.trial,'Powered capture trial mismatch');
    const headerSize=12+powered.sectors.length*2;
    assert.deepEqual(await mem(s.address,headerSize),raw.subarray(0,headerSize),'Powered capture changed');
    assert.deepEqual(await mem(address,60),a,'Trial changed during powered read');
    await check();
    for (const [i,sector] of result.handoff.sectors.entries()) sector.samples=powered.sectors[i];
  }
  if (syms.fd_m2_sine_adaptation) {
    const s=syms.fd_m2_sine_adaptation;
    assert.equal(s.size,400);
    const raw=await mem(s.address,s.size);
    result.adaptation=decodeM2Adaptation(raw);
    assert.equal(result.adaptation.trial,summary.trial,'Adaptation trial mismatch');
    assert.deepEqual(await mem(s.address,16),raw.subarray(0,16),'Adaptation header changed');
    assert.deepEqual(await mem(address,60),a,'Trial changed during adaptation read');
    await check();
  }
  for(const [name,key,decode,size] of [
    ['fd_m2_sine_recovery','recovery',decodeM2Recovery,3120],
    ['fd_m2_sine_closed','closed',decodeM2Closed,528]]) {
    if(!syms[name])continue;
    const s=syms[name];assert.equal(s.size,size);
    const raw=await mem(s.address,s.size);result[key]=decode(raw);
    assert.equal(result[key].trial,summary.trial,key+' trial mismatch');
    assert.deepEqual(await mem(s.address,48),raw.subarray(0,48),key+' header changed');
    assert.deepEqual(await mem(address,60),a,'Trial changed during '+key+' read');
    await check();
  }
  if(syms.fd_m2_heli) {
    const s=syms.fd_m2_heli;assert.equal(s.size,96);
    const raw=await mem(s.address,s.size);
    result.heli=decodeM2Heli(raw,summary.started);
    assert.equal(result.heli.trial,summary.trial);
    assert.deepEqual(await mem(s.address,s.size),raw,'Heli state changed');
    await check();
  }
  if(syms.fd_m2_wire) {
    const s=syms.fd_m2_wire;assert.equal(s.size,1056);
    const raw=await mem(s.address,s.size);
    result.wire=decodeM2Wire(raw,result.ram_settings.motor_poles);
    assert.equal(result.wire.trial,summary.trial);
    assert.deepEqual(await mem(s.address,32),raw.subarray(0,32),'Wire header changed');
    await check();
  }
  if(syms.fd_m2_sine_adc_capture) {
    const s=syms.fd_m2_sine_adc_capture;
    assert.equal(s.size,3152);
    const raw=await mem(s.address,s.size);
    result.adc_capture=decodeM2AdcCapture(raw);
    assert.equal(result.adc_capture.trial,summary.trial,'ADC capture trial mismatch');
    assert.deepEqual(await mem(s.address,80),raw.subarray(0,80),'ADC capture header changed');
    assert.deepEqual(await mem(address,60),a,'Trial changed during ADC capture read');
    await check();
  }
  if (syms.fd_m2_sine_step4_coast) {
    const s=syms.fd_m2_sine_step4_coast;
    assert.equal(s.size,6176);
    const header=await mem(s.address,32);
    const count=header.readUInt32LE(28);
    assert(count<=512);
    const raw=Buffer.alloc(s.size);
    header.copy(raw);
    for (let offset=0;offset<count*12;offset+=512) {
      const chunk=await mem(s.address+32+offset,Math.min(512,count*12-offset));
      chunk.copy(raw,32+offset);
    }
    result.step4_coast=decodeM2Step4Coast(raw);
    assert.equal(result.step4_coast.trial,summary.trial,'Step 4 coast trial mismatch');
    assert.deepEqual(await mem(s.address,32),header,'Step 4 coast header changed');
    assert.deepEqual(await mem(address,60),a,'Trial changed during step 4 coast read');
    await check();
  }
  const summaryResult={...result,pin_trace:pin_trace?{header:pin_trace.header,analysis:pin_trace.analysis}:null};
  if(coast) summaryResult.coast={...coast,samples:undefined};
  if(result.handoff) summaryResult.handoff={...result.handoff,
    sectors:result.handoff.sectors.map(({samples,...sector})=>sector)};
  if(result.step4_coast) summaryResult.step4_coast={...result.step4_coast,samples:undefined};
  if(result.recovery) summaryResult.recovery={...result.recovery,samples:undefined};
  if(result.adc_capture) summaryResult.adc_capture={...result.adc_capture,samples:undefined};
  fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n',{flag:'wx'});
  console.log(JSON.stringify(summaryResult,null,2));
  return result;
 }finally{c.close();}
}
if(require.main===module){const [elf,out,...flags]=process.argv.slice(2);assert(elf&&out,'Usage: node fd6288_native_sine_capture.cjs MATCHING.elf NEW.json --stopped');dump(elf,out,flags.includes('--stopped')).catch(e=>{console.error(e);process.exitCode=1;});}
module.exports={decode,decodePwm,decodeSensors,decodeM2Coast,decodeM2Handoff,decodeM2Powered,decodeM2Adaptation,decodeM2Step4Coast,decodeM2Recovery,decodeM2Closed,decodeM2AdcCapture,decodeM2Wire,decodeM2Heli,dump};
