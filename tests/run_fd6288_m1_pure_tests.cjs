// Actual M1 adapter and alignment-pair control, with simulated peripheral/time.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
function fn(signature){
 const start=main.indexOf(signature+'\n{'),open=main.indexOf('{',start);assert(start>=0&&open>start,signature);
 if(signature==='static void fdBenchService(void)')return main.slice(start,main.indexOf('static uint8_t fdBenchGuardInput(void)',start));
 let depth=0;for(let i=open;i<main.length;i++){if(main[i]==='{')depth++;if(main[i]==='}'&&!--depth)return main.slice(start,i+1);}throw Error(signature);
}
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'m1-pure-tests-'));
function run(name,source,pure){
 const exe=path.join(dir,name+(pure?'-pure':'-diagnostic'));
 const r=cp.spawnSync(process.env.CC||'cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-DFD6288_START_RETRIES=5','-DFD6288_SECOND_ALIGN_MS=300',...(pure?['-DTARGET_FD_M1_PURE']:[]),'-I'+path.join(root,'Inc'),'-x','c','-','-o',exe],{input:source,encoding:'utf8'});
 if(r.status!==0){fs.writeFileSync(exe+'.c',source);process.stderr.write(r.stderr);throw Error('Compile '+exe);}
 const result=cp.spawnSync(exe,[],{encoding:'utf8'});if(result.stderr)process.stderr.write(result.stderr);assert.equal(result.status,0,exe);return result.stdout;
}
const ph=fs.readFileSync(path.join(root,'Inc/fd6288_startup_power.h'),'utf8');
const reasons=ph.slice(ph.indexOf('enum {'),ph.indexOf('};',ph.indexOf('enum {'))+2);
let source=fs.readFileSync(path.join(__dirname,'fd6288_start_retry_adapter_tests.c'),'utf8').replace('/* ACTUAL_REASONS */',reasons);
source=source.replace('static struct { uint32_t frozen; } fd_align_observe;','#ifndef TARGET_FD_M1_PURE\nstatic struct { uint32_t frozen; } fd_align_observe;\n#endif')
.replace('static uint32_t fd_reposition_diag[8];','#ifndef TARGET_FD_M1_PURE\nstatic uint32_t fd_reposition_diag[8];\n#endif')
.replace('static volatile FdStartRetryDiagnostics fd_start_retry_diag;','#ifndef TARGET_FD_M1_PURE\nstatic volatile FdStartRetryDiagnostics fd_start_retry_diag;\n#endif');
const helperNames=['fdStartRetryRecord','fdStartRetryBegin','fdStartRetryEdgeRecord','fdStartRetryFinish'];
const helpers=[
 'static void fdStartRetryRecord(uint32_t now, uint8_t action)',
 'static void fdStartRetryBegin(uint32_t now)',
 'static void fdStartRetryEdgeRecord(uint32_t now)',
 'static void fdStartRetryFinish(uint32_t now, uint8_t reason)'
].map(fn).join('\n');
source=source.replace('/* ACTUAL_RETRY_RECORD */','#ifndef TARGET_FD_M1_PURE\n'+helpers+'\n#else\n'+helperNames.map(n=>'#define '+n+'(...) ((void)0)').join('\n')+'\n#endif');
source=source.replaceAll('FD6288_FIRST_EDGE_DELAY_US','500u');
source=source.replace('/* ACTUAL_SERVICE */',fn('static void fdBenchService(void)'));
const adapters=[
 'static void fdBenchStop(uint8_t reason)','static uint8_t fdBenchGuardInput(void)',
 'static void fdRevTick(void)','static void fdRevObserveEdge(void)','static void fdBenchWritePwm(uint16_t ticks)','static uint8_t fdAsyncAllowed(void)'
].map(fn).join('\n').replace('while ((uint32_t)(DWT->CYCCNT - off_at) < 5u * FD_REV_US) {}','while ((uint32_t)(DWT->CYCCNT - off_at) < 5u * FD_REV_US) {DWT->CYCCNT++;}');
source=source.replace('/* ACTUAL_ADAPTER */',`
#define fdRevTick fdRevTickActual
#define fdRevObserveEdge fdRevObserveEdgeActual
${adapters}
#undef fdRevTick
#undef fdRevObserveEdge
static uint32_t control_hash=2166136261u;
static void hash_control(void){
 const uint32_t v[]={fd_rev.stage,fd_rev.confidence,fd_rev.align_left,fd_rev.sr_attempts,fd_rev.sr_failures,fd_rev.period,fd_rev.measured,fd_rev.last_real_period,fd_rev.edge_at,fd_rev.delay,running,outputs,step,commutations,scheduled,last_reason,zero_crosses,fd_rev_acquiring,fd_power_bench.floor,fd_power_bench.inhibit,bemf_timeout_happened};
 for(unsigned i=0;i<sizeof(v)/sizeof(v[0]);i++)control_hash=(control_hash^v[i])*16777619u;
}
static void fdRevTick(void){fdRevTickActual();hash_control();}
static void fdRevObserveEdge(void){fdRevObserveEdgeActual();hash_control();}
`);
source=source.replace(/assert\([^;]*fd_start_retry_diag[^;]*\);/g,m=>'\n#ifndef TARGET_FD_M1_PURE\n'+m+'\n#endif\n');
source=source.replace('    const char *record_path = getenv("FD_SR_TEST_RECORD");','\n#ifndef TARGET_FD_M1_PURE\n    const char *record_path = getenv("FD_SR_TEST_RECORD");');
source=source.replace('    puts("PASS ALIGN_RETRY','\n#endif\n    printf("control-checksum %u\\n",control_hash);\n    puts("PASS ALIGN_RETRY');
const outputs=[false,true].map(p=>run('adapter',source,p));assert.equal(outputs[0],outputs[1],'Control trajectories differ');process.stdout.write(outputs[1]);
// The pair probe changes CCR4; preserve its entire schedule after removing data writes.
const pair=`
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define TARGET_FD_EDGE_RAW
#define TARGET_FD_ALIGN_SAMPLE_PAIR
#include "fd6288_edge_observe.h"
#ifdef TARGET_FD_M1_PURE
volatile FdM1PairControl fd_m1_pair_control;
#else
volatile FdEdgeObserve fd_edge_observe;
#endif
static struct {uint32_t CCR1,CCR4;} tim;
#define TIM1 (&tim)
static struct {uint32_t sector_at;uint8_t align_left;} fd_rev;
#ifndef TARGET_FD_M1_PURE
static unsigned step=1;
#endif
static uint16_t zcd_ccr4_from_duty(uint16_t duty){return duty/2u;}
volatile uint8_t fd_align_sample_late;
static uint8_t fd_pair_started,fd_pair_discard;
static uint32_t fd_pair_trial;
${fn('static void fdAlignmentPairReset(void)')}
${fn('static void fdAlignmentPairSample(const uint16_t v[3], uint32_t at, uint32_t cnt, uint32_t duty, uint32_t trigger)')}
int main(void){
 uint32_t h=2166136261u;
 for(unsigned trial=1;trial<=4;trial++){
  uint32_t start=trial==2?0xfffffff0u:trial*1000u;
  fd_rev.sector_at=start;fd_rev.align_left=1;TIM1->CCR1=1200;TIM1->CCR4=123;
  fdEdgeBegin(trial,start);
  for(unsigned us=0;us<100000;us+=42){
   uint16_t v[3]={(uint16_t)(us%4000),900,1100};uint32_t at=start+us*160u;
   fdAlignmentPairSample(v,at,1000,1200,TIM1->CCR4);
   const uint32_t values[]={TIM1->CCR4,fd_align_sample_late,fd_pair_started,fd_pair_discard,FD_ALIGN_PAIR_SAMPLES};
   for(unsigned i=0;i<5;i++)h=(h^values[i])*16777619u;
  }
  assert(FD_ALIGN_PAIR_SAMPLES==512u);assert(TIM1->CCR4==600);
  fdAlignmentPairReset();assert(!fd_pair_started&&!fd_align_sample_late);
 }
 printf("pair-schedule-checksum %u\\n",h);return 0;
}
`;
const pairOut=[false,true].map(p=>run('pair',pair,p));assert.equal(pairOut[0],pairOut[1],'Alignment trigger schedules differ');process.stdout.write(pairOut[1]);
console.log('PASS M1 diagnostic/PURE control equivalence: six-attempt retry, handoff, feedback loss, native faults, zero/disarm, both directions, DWT wrap; alignment CCR4 schedule unchanged. ASan/UBSan enabled.');
