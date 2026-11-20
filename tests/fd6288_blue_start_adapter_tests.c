#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_SEQUENCE
#define TARGET_STARTUP_SEQUENCE_V2
#define TARGET_STARTUP_FEEDBACK
#define TARGET_STARTUP_S4
#define TARGET_STARTUP_S4E
#define TARGET_STARTUP_HANDOFF_REAL
#define TARGET_STARTUP_RESCUE_RETAIN
#define TARGET_STARTUP_RESCUE_BRIDGE
#define TARGET_STARTUP_PHASE_DELAY
#define TARGET_STARTUP_SEED_DELAY
#define TARGET_STARTUP_RESCUE_RAMP
#define TARGET_STARTUP_FIRST_WAIT
#define TARGET_FD_DOUBLE_ALIGN
#define TARGET_FD_ALIGN_HOLD
#define TARGET_FD_ALIGN_SETTLE
#define TARGET_FD_ALIGN_BUDGET
#define TARGET_FD_LAUNCH_ADVANCE
#define TARGET_FD_REPOSITION
#define TARGET_FD_FULL_RETRY
#define TARGET_FD_RETRY_ADJACENT
#define TARGET_FD_STALE_RETRY
#define TARGET_FD_ASYNC_COMM
#define TARGET_FD_NO_DIAGNOSTICS
#define TARGET_FD_BLUE_START
#define TARGET_FD_NATIVE_EARLY
#define TARGET_FD_FIRST_EDGE_500US
#define FD6288_SECOND_ALIGN_MS 300
#include "fd6288_startup_sequence.h"
/* ACTUAL_REASONS */
static struct { uint32_t CYCCNT; } dwt;
#define DWT (&dwt)
static FdRevPolicy fd_rev;
static struct { uint32_t inhibit, active_id, floor; } fd_power_bench;
static struct { uint32_t started; } fd_power_policy;
static struct { uint32_t frozen; } fd_align_observe;
static struct { uint8_t auto_advance; } eepromBuffer;
static uint32_t mask,outputs,commutations,scheduled,last_reason,failure_pin;
#ifdef TARGET_FD_LAUNCH_TWO
static uint32_t schedule_at,schedule_deadline;
#endif
static uint32_t armed,running,adjusted_input,input,forward,step;
volatile uint8_t fd_rev_acquiring;
static uint32_t stuck_rotor_inhibit,drive_safety_limit_inhibit,heli_coast_active;
static uint32_t heli_coast_restart_inhibit,fd_power_starting,prop_brake_active;
static uint32_t duty_cycle_setpoint,duty_cycle,adjusted_duty_cycle,last_duty_cycle;
static uint32_t zero_crosses,commutation_interval,thiszctime,commutation_intervals[6];
static uint32_t average_interval,last_average_interval,desync_check,bad_count,bemf_timeout_happened;
static uint32_t fd_reposition_diag[8];
static uint32_t fd_blue_diag[32];
static void fdBlueRecord(uint32_t now,uint8_t action){(void)now;(void)action;}
#define fdAlignBegin(...) ((void)0)
#define fdEdgeBegin(...) ((void)0)

static uint8_t auto_advance_level,temp_advance;
static uint32_t __get_PRIMASK(void){return mask;}
static void __disable_irq(void){mask=1;}
static void __set_PRIMASK(uint32_t v){mask=v;}
#define ADC_ZCD_RESET_COUNT 0
#define ADC_ZCD_RESET_TIMEOUT 1
#define FD_POWER_WINDOW_END 1
#define SET_DUTY_CYCLE_ALL(v) (outputs=(v))
#define SET_INTERVAL_TIMER_COUNT(v) ((void)(v))
#define SET_HELI_COAST_HIGH_Z_SAMPLE(v) (outputs=0)
#define fdTraceEvent(...) ((void)0)
#define fdTraceFreeze(...) ((void)0)
#define fdTracePinFailure() (failure_pin++)
#define fdEdgeClose(...) ((void)0)
#define fdLightHandoff(...) ((void)0)
#define fdLightRetry(...) ((void)0)
#define fdLightEdge(...) ((void)0)
static void adcZcdSeedHandoff(uint32_t v){(void)v;}
static void adcZcdResetAcquisition(uint32_t v){(void)v;}
static unsigned stuckRotorGuardInput(void){return stuck_rotor_inhibit;}
static void allOff(void){outputs=0;scheduled=0;}
static void commutate(void){step=forward?(step==6?1:step+1):(step==1?6:step-1);commutations++;
#ifdef TARGET_FD_LAUNCH_TWO
scheduled=0; /* actual commutate cancels the pending timer */
#endif
}
static void fdAsyncScheduleDeadline(uint32_t a,uint32_t b){
#ifdef TARGET_FD_LAUNCH_TWO
schedule_at=a;schedule_deadline=b;
#else
(void)a;(void)b;
#endif
scheduled++;}
static void fdBenchFinish(uint8_t reason){last_reason=reason;fd_power_bench.active_id=0;}
static void fdPowerRequestStop(void *p,uint32_t reason){(void)p;(void)reason;}
static void fdPowerOperatorZero(void *p){(void)p;}
static unsigned fdBenchConfigOk(void){return 1;}
static void heliCoastTrackingLost(uint8_t inhibit){assert(fd_power_bench.inhibit);allOff();running=0;heli_coast_restart_inhibit=inhibit;}
static void fdBenchStop(uint8_t reason);
#define fdBenchObserve(...) ((void)0)
#define fdTraceTrialFinish(...) ((void)0)
#define FD_POWER_FADE_TICKS (20000u*160u)
static unsigned min_startup_duty=360,minimum_duty_cycle=210;
static unsigned driveEffectiveDutyLimit(void){return 610;}
/* ACTUAL_SERVICE */
/* ACTUAL_ADAPTER */

static void begin(uint32_t start,unsigned dir){
 fd_power_policy.started=start;fdRevBeginPolicy(&fd_rev,start);
 fd_power_bench.inhibit=0;fd_power_bench.active_id=1;fd_power_bench.floor=360;
 armed=running=adjusted_input=input=fd_rev_acquiring=1;forward=dir;step=dir?5:3;
 mask=stuck_rotor_inhibit=drive_safety_limit_inhibit=heli_coast_active=heli_coast_restart_inhibit=0;
 outputs=123;commutations=scheduled=last_reason=0;
 DWT->CYCCNT=start+30000u*160u;fdRevTick();assert(step==(dir?6u:2u));
 DWT->CYCCNT=start+330000u*160u;
 /* Policy LAUNCH handled separately: existing5us while loop is hardware,
  * not simulated by a constant DWT register in this host harness. */
 assert(fdRevAdvancePolicy(&fd_rev,DWT->CYCCNT)==FD_REV_LAUNCH);
 step=dir?2:6;fd_rev.sector_at=DWT->CYCCNT;commutations=scheduled=0;
 for(unsigned i=8;i<32;i++)fd_blue_diag[i]=UINT32_MAX;
}
static void force(void){uint32_t at=fd_rev.sector_at;
 DWT->CYCCNT=at+500u*160u;fdRevObserveEdge();
 assert(fd_rev.blue_first&&!fd_rev.seen&&!fd_rev.confidence&&!scheduled&&!commutations);
 DWT->CYCCNT=at+30000u*160u-1;fdRevTick();assert(!commutations);
 DWT->CYCCNT++;fdRevTick();
 assert(!fd_rev.blue_first&&fd_rev.stage==FD_REV_OPEN&&commutations==1&&!zero_crosses);
 assert(step==(forward?3u:5u));
}
int main(void){
 assert(FD_BLUE_TOTAL_TICKS==2090000u*160u);
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned dir=0;dir<2;dir++){
  uint32_t start=wrap?UINT32_MAX-1000:1000;begin(start,dir);force();
  uint32_t second=fd_rev.sector_at;DWT->CYCCNT=second+22500u*160u-1;fdRevTick();assert(outputs);
  DWT->CYCCNT++;fdRevTick();assert(fd_rev.stage==FD_REV_RELAX&&!outputs&&!scheduled);
  uint32_t off=fd_rev.sector_at;
  fdBenchWritePwm(999);assert(!outputs);
  DWT->CYCCNT=off+100000u*160u-1;fdRevTick();assert(fd_rev.blue_attempts==1&&!outputs);
  DWT->CYCCNT++;fdRevTick();assert(fd_rev.blue_attempts==2&&fd_rev.stage==FD_REV_ALIGN&&step==(dir?5u:3u));
  assert(!fd_rev.reposition_used&&fd_rev.align_left==2&&!fd_rev.confidence);
  /* Exhaust next attempts without repeating the hardware launch gap. */
  fd_rev.stage=FD_REV_OPEN;fd_rev.blue_first=0;fd_rev.period=FD_REV_RETRY_TICKS;fd_rev.sector_at=DWT->CYCCNT;
  DWT->CYCCNT+=FD_REV_RETRY_TICKS;fdRevTick();assert(fd_rev.stage==FD_REV_RELAX&&!outputs);
  DWT->CYCCNT+=FD_BLUE_RELAX_TICKS;fdRevTick();assert(fd_rev.blue_attempts==3);
  fd_rev.stage=FD_REV_OPEN;fd_rev.blue_first=0;fd_rev.period=FD_REV_RETRY_TICKS;fd_rev.sector_at=DWT->CYCCNT;
  DWT->CYCCNT+=FD_REV_RETRY_TICKS;fdRevTick();
  assert(last_reason==FD_BENCH_NO_LOCK&&fd_power_bench.inhibit&&!outputs&&!running);
  fdBenchWritePwm(999);assert(!outputs);assert(fdBenchGuardInput());
  adjusted_input=0;assert(!fdBenchGuardInput());
 }
 /* First ignored event cannot seed measured period or six-edge handoff. */
 begin(0,1);force();
 for(unsigned i=0;i<6;i++){
  DWT->CYCCNT=fd_rev.sector_at+8000u*160u;fdRevObserveEdge();
  assert(fd_rev.confidence==i+1);
  if(i==0){assert(fd_rev.measured==0&&fd_rev.delay==500u*160u);}
  if(fd_rev.stage==FD_REV_PENDING){DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();}
 }
 assert(fd_rev.stage==FD_REV_CLOSED&&!fd_rev_acquiring&&zero_crosses==6);
 assert(!fdRevDeadlinePolicy(&fd_rev,UINT32_MAX,0));
 for(unsigned fault=0;fault<3;fault++){
  begin(0,1);force();DWT->CYCCNT+=FD_REV_RETRY_TICKS;fdRevTick();assert(!outputs);
  if(!fault)stuck_rotor_inhibit=1;else if(fault==1)drive_safety_limit_inhibit=1;else DWT->CYCCNT=FD_BLUE_TOTAL_TICKS;
  fdRevTick();assert(fd_power_bench.inhibit&&!outputs&&!running);
 }
 /* Operator zero cancels every acquisition stage, including pending timer and100ms release. */
 for(unsigned stage=0;stage<4;stage++){
  begin(0,1);
  if(stage==1)force();
  if(stage==2){force();DWT->CYCCNT+=8000u*160u;fdRevObserveEdge();assert(fd_rev.stage==FD_REV_PENDING);}
  if(stage==3){force();DWT->CYCCNT+=FD_REV_RETRY_TICKS;fdRevTick();assert(fd_rev.stage==FD_REV_RELAX);}
  adjusted_input=0;fdBenchService();unsigned before=commutations;
  assert(!fd_rev_acquiring&&!fd_power_bench.active_id);
  DWT->CYCCNT+=500000u*160u;fdRevTick();assert(commutations==before);
 }
 begin(0,1);force();DWT->CYCCNT+=FD_REV_RETRY_TICKS;fdRevObserveEdge();
 assert(fd_rev.stage==FD_REV_RELAX&&!fd_rev.confidence&&!outputs);
 /* Per-attempt deadline cannot be renewed by late service or events. */
 FdRevPolicy p;fdRevBeginPolicy(&p,0);assert(fdRevAdvancePolicy(&p,FD_BLUE_ATTEMPT_TICKS)==FD_REV_RELEASE);
 uint32_t off=p.sector_at;assert(fdRevAdvancePolicy(&p,off+FD_BLUE_RELAX_TICKS-1)==FD_REV_NONE);
 assert(fdRevAdvancePolicy(&p,off+FD_BLUE_RELAX_TICKS)==FD_REV_BLUE_RESTART);
 assert(p.blue_attempt_at==off+FD_BLUE_RELAX_TICKS&&p.blue_attempts==2);
 puts("PASS BLUE_START actual tick/event/stop/PWM adapter: fixed30ms despite early event, forced step not ZC,100ms high-Z, original seeds,3-attempt stop, six usable edges, per-attempt and2.09s caps, faults/zero rearm, directions/wrap");
}
