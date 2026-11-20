const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
function fn(signature,source=main){
 const start=source.indexOf(signature),open=source.indexOf('{',start);assert(start>=0&&open>start);
 if(signature==='static void fdBenchService(void)')return main.slice(start,main.indexOf('static uint8_t fdBenchGuardInput(void)',start));
 let depth=0;for(let i=open;i<source.length;i++){if(source[i]==='{')depth++;if(source[i]==='}'&&!--depth)return source.slice(start,i+1);}throw Error(signature);
}
const ph=fs.readFileSync(path.join(root,'Inc/fd6288_startup_power.h'),'utf8');
const reasons=ph.slice(ph.indexOf('enum {'),ph.indexOf('};',ph.indexOf('enum {'))+2);
const pairStart=main.indexOf('enum { FD_STEP4_PAIR_CAPACITY');
assert(pairStart>=0);
const pairEnd=main.indexOf('\n#endif',pairStart);
const step35Pair=process.argv.includes('--m2-step35-adc-pair');
const runupStep4Pair=process.argv.includes('--m2-runup-step4-pair');
const earlyStep5Dwell=process.argv.includes('--m2-early-step5-dwell');
const runupStep4Anchor=earlyStep5Dwell||process.argv.includes('--m2-runup-step4-anchor');
const runupAdc=runupStep4Pair||runupStep4Anchor||process.argv.includes('--m2-runup-adc-capture');
const timedRunup=runupAdc||process.argv.includes('--m2-timed-runup');
const step4Dense=timedRunup||process.argv.includes('--m2-closed-step4-dense');
const step6Late=step4Dense||process.argv.includes('--m2-step6-late-adc');
const handoffCapture=step6Late||process.argv.includes('--m2-handoff-capture');
const step6Pair=process.argv.includes('--m2-step6-adc-pair');
const onPair=step35Pair||step6Pair||runupStep4Pair;
const step5Late=step6Pair||handoffCapture||process.argv.includes('--m2-step5-late-adc');
const compareMode=step35Pair||process.argv.includes('--m2-step5-coast-pulse-9000')?3:
 process.argv.includes('--m2-step5-coast-pulse')?2:
 process.argv.includes('--m2-step5-coast-off')?1:0;
const step5Delay=process.argv.includes('--m2-step5-delay-4900');
const secondDelay=compareMode || step5Delay || step5Late || process.argv.includes('--m2-pickup-second-delay-2000');
const firstDwell=secondDelay || process.argv.includes('--m2-pickup-first-dwell-4000');
const step4Delay=firstDwell || process.argv.includes('--m2-step4-delay-2200');
const powered=step4Delay || process.argv.includes('--m2-powered-capture');
const pickup=powered || process.argv.includes('--m2-low-speed-pickup');
const pickupStart=main.indexOf('static FdM2Pickup fd_pickup;');
assert(pickupStart>=0);
const pickupEnd=main.indexOf('\n#endif\n#ifdef TARGET_FD_M2_STEP4_ADC_PAIR',pickupStart);
assert(pickupEnd>pickupStart);
const peripherals=fs.readFileSync(path.join(root,'Mcu/g431/Inc/peripherals.h'),'utf8');
const source=fs.readFileSync(path.join(__dirname,'fd6288_hiz_adapter_tests.c'),'utf8').replace('/* ACTUAL_REASONS */',reasons)
 .replace('/* ACTUAL_SERVICE */',fn('static void fdBenchService(void)'))
 .replace('/* ACTUAL_PICKUP_DATA */',main.slice(pickupStart,pickupEnd))
 .replace('/* ACTUAL_PICKUP_ENTRY */',fn('static void fdM2PickupStart(uint32_t now, uint8_t sector)'))
 .replace('/* ACTUAL_PAIR */',[(onPair||step5Late) ? peripherals.match(/enum \{ FD_M2_LATE_ADC_TICKS[^\n]+/)[0] : '',
  fn('static inline uint16_t zcd_ccr4_from_duty',peripherals),
  '#ifdef TARGET_FD_M2_STEP4_ADC_PAIR',main.slice(pairStart,pairEnd),
  fn('static void fdObserveStep4PairAdc(void)'),'#endif'].join('\n'))
 .replace('/* ACTUAL_ADAPTER */',[
  'static void fdBenchStop(uint8_t reason)','static uint8_t fdBenchGuardInput(void)',
  ...(step5Late?['static void fdM2SetStartupAperture(uint8_t late)']:[]),
  'static void fdRevEnterHiZ(void)','static void fdRevTick(void)','static void fdRevObserveEdge(void)','static void fdBenchWritePwm(uint16_t ticks)', 'static void fdObserveHiZAdc(void)',
  ...(onPair?['static uint8_t fdObserveOnAdcPair(void)']:[]), 'void fdAdcJeosService(void)'
 ].map(signature=>fn(signature)).join('\n')).replace('while ((uint32_t)(DWT->CYCCNT - off_at) < 5u * FD_REV_US) {}',
 'while ((uint32_t)(DWT->CYCCNT - off_at) < 5u * FD_REV_US) {DWT->CYCCNT++;}');
const earlyStep4=process.argv.includes('--m2-early-step4');
const fullStep4=earlyStep4 || process.argv.includes('--m2-step4-adc-pair-11ms');
const pair=fullStep4 || process.argv.includes('--m2-step4-adc-pair');
const step4Ms=fullStep4?11:5;
const step4=pair || process.argv.includes('--m2-step4');
const longCoast=pickup || process.argv.includes('--m2-coast-40ms');
const m2=longCoast || step4 || process.argv.includes('--m2-after-two');
const m2Flags=m2 ? ['-DTARGET_FD_M2_HIZ_AFTER_TWO','-DTARGET_FD_M2_START_CAPTURE','-DTARGET_FD_M2_START_RAMP',
 '-DTARGET_FD_START_RETRY','-DFD6288_START_RETRIES=0','-DTARGET_FD_NATIVE_EARLY','-DTARGET_FD_FIRST_EDGE_500US','-DFD6288_FIRST_EDGE_DELAY_US=2000'] : [];
if(compareMode)m2Flags.push('-DTARGET_FD_M2_STEP5_COAST_COMPARE',`-DFD6288_M2_STEP5_COAST_COMPARE=${compareMode}`);
if(onPair)m2Flags.push('-DTARGET_FD_M2_ON_ADC_PAIR');
if(step35Pair)m2Flags.push('-DTARGET_FD_M2_STEP35_ADC_PAIR');
if(step6Pair)m2Flags.push('-DTARGET_FD_M2_STEP6_ADC_PAIR');
if(step5Late)m2Flags.push('-DTARGET_FD_M2_STEP5_LATE_ADC');
if(step6Late)m2Flags.push('-DTARGET_FD_M2_STEP6_LATE_ADC');
if(handoffCapture)m2Flags.push('-DTARGET_FD_M2_HANDOFF_CAPTURE');
if(step4Dense)m2Flags.push('-DTARGET_FD_M2_CLOSED_STEP4_DENSE');
if(timedRunup)m2Flags.push('-DTARGET_FD_M2_TIMED_RUNUP');
if(runupAdc)m2Flags.push('-DTARGET_FD_M2_RUNUP_ADC_CAPTURE');
if(runupStep4Pair)m2Flags.push('-DTARGET_FD_M2_RUNUP_STEP4_PAIR');
if(runupStep4Anchor)m2Flags.push('-DTARGET_FD_M2_RUNUP_STEP4_ANCHOR');
if(earlyStep5Dwell)m2Flags.push('-DTARGET_FD_M2_EARLY_STEP5_DWELL');
if(step5Delay)m2Flags.push('-DTARGET_FD_M2_STEP5_DELAY_4900');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-launch-hold-')),exe=path.join(dir,'test');
const runtimeArgs=[path.join(dir,'capture.bin'),path.join(dir,'pair.bin')];
if(pickup){
 const record=require('../docs/m2-startup/capture-m2-hiz-40ms-trial4.json');
 const rows=record.startup_wave.records.filter(r=>r.flags&1);
 const fixture=path.join(dir,'coast.txt');
 fs.writeFileSync(fixture,rows.map(r=>[Math.round(r.from_bridge_off_us*160),r.a,r.b,r.c].join(' ')).join('\n')+'\n');
 runtimeArgs.push(fixture);
 if(powered)runtimeArgs.push(path.join(dir,'powered.bin'));
 if(step35Pair||step6Pair)runtimeArgs.push(path.join(dir,'on-pair.bin'));
 if(handoffCapture)runtimeArgs.push(path.join(dir,'handoff.bin'));
 if(runupAdc)runtimeArgs.push(path.join(dir,'runup-adc.bin'));
}
for(const [cmd,args,input] of [[process.env.CC||'cc',[...m2Flags,...(secondDelay?['-DTARGET_FD_M2_PICKUP_SECOND_DELAY_2000']:[]),...(firstDwell?['-DTARGET_FD_M2_PICKUP_FIRST_DWELL_4000']:[]),...(step4Delay?['-DTARGET_FD_M2_STEP4_DELAY_2200']:[]),...(powered?['-DTARGET_FD_M2_POWERED_CAPTURE']:[]),...(pickup?['-DTARGET_FD_M2_LOW_SPEED_PICKUP']:[]),...(longCoast?['-DTARGET_FD_M2_HIZ_40MS']:[]),...(earlyStep4?['-DFD6288_M2_HIZ_STEP3_US=10300']:[]),...(pair?['-DTARGET_FD_M2_STEP4_ADC_PAIR']:[]),...(step4?['-DFD6288_M2_HIZ_STEP4_MS='+step4Ms]:[]),...(process.argv.includes('--two')?['-DTARGET_FD_LAUNCH_TWO']:[]),'-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),'-x','c','-','-o',exe],source],[exe,runtimeArgs,undefined]]){
 const r=spawnSync(cmd,args,{input,encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}

const decoded=require('../tools/scripts/fd6288_edge_decode.cjs').decode(fs.readFileSync(path.join(dir,'capture.bin')),m2);
assert.equal(decoded.header.abi,longCoast?11:7);assert.equal(decoded.records.length,longCoast?486:120);assert.equal(decoded.alignment_tail.length,2);
assert(decoded.records.every(x=>x.ab===20&&x.bc===20&&x.ca===-40));
assert.equal(decoded.records[0][m2?'last_driven_step':'last_alignment_step'],step4?4:m2?3:6);
assert(decoded.records[0].boundary_group&&!decoded.records[10].boundary_group);
if(longCoast){
 assert.equal(decoded.observation_ms,40);
 assert.equal(decoded.header.seen,960);
 assert.equal(decoded.header.post_samples,474);
 assert(decoded.records.at(-1).from_bridge_off_us>39800);
 assert(decoded.records.every(x=>x.adc_zero_phases.join(',')==='c'));
 assert.deepEqual(decoded.records.slice(10,15).map(x=>x.conversion_index),[10,11,12,14,16]);
}
console.log('PASS Hi-Z C logger → JS decoder, tail preservation, line differences');
if(pair){
 const capture=require('../tools/scripts/fd6288_edge_decode.cjs').decodeStep4Pair(fs.readFileSync(path.join(dir,'pair.bin')),step4Ms);
 assert.equal(capture.records.length,fullStep4?87:2);
 assert.deepEqual(capture.records.slice(0,2).map(record=>record.trigger),[600,780]);
 assert.equal(capture.pairs.length,fullStep4?43:1);
 if(fullStep4)assert(capture.records.at(-1).at_us>10900,'Capture reaches the end of step 4');
 assert.equal(capture.pairs[0].dt_us,126);
 assert.equal(capture.pairs[0].delta_margin,20);
 console.log('PASS step 4 interleaved ADC groups and decoded margin delta');
}

if(powered){
 const raw=fs.readFileSync(path.join(dir,'powered.bin'));
 const capture=require('../tools/scripts/fd6288_edge_decode.cjs').decodePoweredCapture(raw);
 assert.deepEqual(capture.sectors.map(s=>s.step),[1,2,3,4]);
 const edges=capture.records.filter(x=>x.reason===7);
 assert.equal(edges.length,3);
 if(step4Delay)assert.equal(capture.sectors[3].from_resume_us-edges[2].from_resume_us,2200);
 if(secondDelay)assert.equal(capture.sectors[2].from_resume_us-edges[1].from_resume_us,2000);
 if(firstDwell)assert.equal(capture.sectors[1].from_resume_us,4000);
 assert.equal(capture.records[0].from_resume_us,40);
 assert.equal(capture.sectors[3].peak_filtered_current_ca,220);
 assert.equal(capture.sectors[3].min_filtered_voltage_cv,1110);
 assert(capture.records.some(x=>x.step===4&&x.from_resume_us>30000));
 console.log('PASS M2 pickup powered capture, sector snapshots and ADC record decoder');
}
if(step35Pair){
 const raw=fs.readFileSync(path.join(dir,'on-pair.bin'));
 const capture=require('../tools/scripts/fd6288_edge_decode.cjs').decodeOnAdcPair(raw);
 assert.deepEqual(capture.pairs.map(p=>p.step),[3,5]);
 assert(capture.pairs.every(p=>p.dt_us===126&&p.delta_margin<0));
 assert.deepEqual(capture.records.map(r=>r.trigger),[667,960,667,960]);
 console.log('PASS step 3/5 C logger → JS decoder');
}
if(step6Pair){
 const raw=fs.readFileSync(path.join(dir,'on-pair.bin'));
 const capture=require('../tools/scripts/fd6288_edge_decode.cjs').decodeOnAdcPair(raw);
 assert.equal(capture.pairs.length,45);
 assert(capture.pairs.every(p=>p.step===6&&p.dt_us===126&&p.delta_margin<0));
 assert(capture.records.every((r,i)=>r.trigger===(i%2?960:667)));
 console.log('PASS full step 6 C logger → JS decoder');
}
if(handoffCapture){
 const raw=fs.readFileSync(path.join(dir,'handoff.bin'));
 const capture=require('../tools/scripts/fd6288_edge_decode.cjs').decodeHandoffCapture(raw);
 assert.equal(capture.records.length,96);
 assert.equal(capture.stop_timer_us,22500);
 assert.equal(capture.stop_step,2);
 assert.deepEqual(capture.records.slice(-3).map(r=>r.kind),['adc','feedback','switch']);
 assert(capture.records.every((r,i,a)=>!i||r.before_stop_us<=a[i-1].before_stop_us));
 console.log('PASS handoff C rolling capture → JS decoder');
}
if(runupAdc){
 const raw=fs.readFileSync(path.join(dir,'runup-adc.bin'));
 const capture=require('../tools/scripts/fd6288_edge_decode.cjs').decodeRunupAdc(raw);
 assert.deepEqual(capture.sectors.map(s=>[s.step,s.samples.length]),[[4,runupStep4Pair?10:15],[5,15],[6,15]]);
 if(runupStep4Pair){
  assert.equal(capture.step4_pairs.length,5);
  assert(capture.step4_pairs.every(p=>p.dt_us===126&&p.delta_margin<0));
  assert(capture.sectors[0].samples.every((r,i)=>r.trigger===(i%2?960:667)));
 }
 console.log('PASS timed run-up ADC logger → JS decoder');
}
