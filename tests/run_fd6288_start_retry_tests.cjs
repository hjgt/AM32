const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
function fn(signature){
 const start=main.indexOf(signature),open=main.indexOf('{',start);assert(start>=0&&open>start);
 if(signature==='static void fdBenchService(void)')return main.slice(start,main.indexOf('static uint8_t fdBenchGuardInput(void)',start));
 let depth=0;for(let i=open;i<main.length;i++){if(main[i]==='{')depth++;if(main[i]==='}'&&!--depth)return main.slice(start,i+1);}throw Error(signature);
}
const ph=fs.readFileSync(path.join(root,'Inc/fd6288_startup_power.h'),'utf8');
const reasons=ph.slice(ph.indexOf('enum {'),ph.indexOf('};',ph.indexOf('enum {'))+2);
const source=fs.readFileSync(path.join(__dirname,'fd6288_start_retry_adapter_tests.c'),'utf8').replace('/* ACTUAL_REASONS */',reasons)
  .replace('/* ACTUAL_RETRY_RECORD */',[
  'static void fdStartRetryRecord(uint32_t now, uint8_t action)',
  'static void fdStartRetryBegin(uint32_t now)',
  'static void fdStartRetryEdgeRecord(uint32_t now)',
  'static void fdStartRetryFinish(uint32_t now, uint8_t reason)'
 ].map(fn).join('\n'))
 .replace('/* ACTUAL_SERVICE */',fn('static void fdBenchService(void)'))
 .replace('/* ACTUAL_ADAPTER */',[
  'static void fdBenchStop(uint8_t reason)','static uint8_t fdBenchGuardInput(void)',
  'static void fdRevTick(void)','static void fdRevObserveEdge(void)','static void fdBenchWritePwm(uint16_t ticks)', 'static uint8_t fdAsyncAllowed(void)'
 ].map(fn).join('\n')).replace('while ((uint32_t)(DWT->CYCCNT - off_at) < 5u * FD_REV_US) {}',
 'while ((uint32_t)(DWT->CYCCNT - off_at) < 5u * FD_REV_US) {DWT->CYCCNT++;}');
// Peripheral simulation only: DWT advances while actual code polls5us bridge gap.

const shortFive=process.argv.includes('--short-five'),longFive=process.argv.includes('--long-five'),maxAttempts=(shortFive||longFive)?6:3,size=48+40*maxAttempts;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-launch-hold-')),exe=path.join(dir,'test');
for(const [cmd,args,input] of [[process.env.CC||'cc',[...((shortFive||longFive)?['-DFD6288_SECOND_ALIGN_MS='+(shortFive?100:300),'-DFD6288_START_RETRIES=5']:[]),'-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),'-x','c','-','-o',exe],source],[exe,[],undefined]]){
 const r=spawnSync(cmd,args,{input,env:{...process.env,FD_SR_TEST_RECORD:path.join(dir,'records.bin')},encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}

const {decode}=require('../tools/scripts/fd6288_start_retry_decode.cjs');
const SIZE=size;
const records=fs.readFileSync(path.join(dir,'records.bin'));assert.equal(records.length,SIZE*3);
const decoded=[0,1,2].map(i=>decode(records.subarray(i*SIZE,(i+1)*SIZE)));
assert.deepEqual(decoded.map(d=>d.successful_attempt),[1,maxAttempts,null]);
assert.deepEqual(decoded.map(d=>d.failed_attempts),[0,maxAttempts-1,maxAttempts]);
assert.deepEqual(decoded.map(d=>d.attempts_started),[1,maxAttempts,maxAttempts]);
for(const d of decoded)for(let i=1;i<d.attempts.length;i++){
 assert(d.attempts[i].begin_ms-d.attempts[i-1].off_ms>=100);
 assert(d.attempts[i].launch_ms-d.attempts[i].begin_ms>=(shortFive?130:330));
}
for(const offset of [0,4,12,16,32,40,44,48,64,80]){
 const bad=Buffer.from(records.subarray(0,SIZE));bad.writeUInt32LE(0xffffffff,offset);assert.throws(()=>decode(bad));
}
assert.throws(()=>decode(Buffer.alloc(SIZE-1)));
console.log('PASS actual C summary -> JS: first/last attempt success and maximum failures, repeated positioning/off times, corrupt ABI/counters/timestamps rejected');
