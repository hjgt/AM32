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
#define TARGET_FD_EARLY_REKICK
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
static uint32_t fd_rk_diag[40];
/* ACTUAL_RK_RECORD */
#define fdAlignBegin(...) ((void)0)
#define fdEdgeBegin(...) ((void)0)
static unsigned stepper_sine;

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
static void commutate(void){step=forward?(step==6?1:step+1):(step==1?6:step-1);commutations++;scheduled=0;
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
 armed=running=fd_rev_acquiring=1;adjusted_input=input=215;forward=dir;step=dir?5:3;
 mask=stuck_rotor_inhibit=drive_safety_limit_inhibit=heli_coast_active=heli_coast_restart_inhibit=0;
 outputs=123;commutations=scheduled=last_reason=0;
 for(unsigned i=0;i<40;i++)fd_rk_diag[i]=i<8?0:UINT32_MAX;
 for(unsigned i=0;i<4;i++)fd_rk_diag[8+8*i+6]=0;
 DWT->CYCCNT=start+30000u*160u;fdRevTick();assert(step==(dir?6u:2u));
 DWT->CYCCNT=start+330000u*160u;
 assert(fdRevAdvancePolicy(&fd_rev,DWT->CYCCNT)==FD_REV_LAUNCH);
 step=dir?2:6;fd_rev.sector_at=DWT->CYCCNT;commutations=scheduled=0;
 fdRkRecord(DWT->CYCCNT,FD_REV_LAUNCH);
}
static void early(unsigned us){
 DWT->CYCCNT=fd_rev.sector_at+us*160u;fdRevObserveEdge();
}
static void wait_release(void){
 assert(fd_rev.stage==FD_REV_RELAX&&!outputs&&!scheduled&&!fdAsyncAllowed());
 uint32_t off=fd_rev.sector_at;
 fdBenchWritePwm(999);assert(!outputs);
 DWT->CYCCNT=off+FD_RK_RELAX_TICKS-1u;fdRevTick();assert(fd_rev.stage==FD_REV_RELAX);
 unsigned before=commutations;
 DWT->CYCCNT++;fdRevTick();
 assert(fd_rev.stage==FD_REV_OPEN&&fd_rev.rk_first&&commutations==before+1u);
 assert(step==(forward?2u:6u)&&!fd_rev.confidence&&!zero_crosses&&fd_rev.rk_pending==0);
 assert((uint32_t)(fd_rev.sector_at-off)>=FD_RK_RELAX_TICKS+5u*160u);
 outputs=123; /* mock PWM service after actual bridge application */
}
int main(void){
 const unsigned times[]={1,339,500,911,1000};
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned dir=0;dir<2;dir++)for(unsigned i=0;i<5;i++){
  uint32_t start=wrap?UINT32_MAX-1000u:1000u;begin(start,dir);
  for(unsigned k=0;k<3;k++){
   unsigned before=commutations;early(times[i]);
   assert(fd_rev.rk_count==k+1u&&fd_rev.confidence==0&&!fd_rev.measured);
   assert(!scheduled&&commutations==before&&fd_rk_diag[8+8*k+3]==1);
   wait_release();assert(fd_rk_diag[3]==k+1u);
  }
  unsigned before=commutations;early(times[i]);
  assert(last_reason==FD_BENCH_NO_LOCK&&fd_power_bench.inhibit&&!outputs&&!running);
  assert(commutations==before&&!scheduled&&fd_rev.rk_count==3&&fd_rk_diag[8+24+3]==3);
  fdBenchWritePwm(999);assert(!outputs);assert(fdBenchGuardInput());
  adjusted_input=0;assert(!fdBenchGuardInput());
 }
 /* Boundary1ms+one cycle and ordinary4/8ms events follow original500us path. */
 for(unsigned delay=0;delay<3;delay++){
  begin(0,1);uint32_t launch=fd_rev.sector_at;
  DWT->CYCCNT=launch+(delay==0?FD_RK_EARLY_TICKS+1u:(delay==1?4000u:8000u)*160u);
  fdRevObserveEdge();assert(!fd_rev.rk_count&&!fd_rev.rk_first&&fd_rev.confidence==1);
  assert(fd_rev.stage==FD_REV_PENDING&&fd_rev.delay==500u*160u&&scheduled==1);
  assert(fdAsyncAllowed());
  DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay-1;fdRevTick();assert(!commutations);
  DWT->CYCCNT++;fdRevTick();assert(commutations==1&&step==3&&!scheduled);
  /* A short interval in a subsequent vector is not another first-pulse exception. */
  DWT->CYCCNT=fd_rev.sector_at+500u*160u;fdRevObserveEdge();
  assert(!fd_rev.rk_count&&fd_rev.confidence==2&&step==4);
 }
 /* Success after each possible number of replacements earns six usable events. */
 for(unsigned kicks=0;kicks<=3;kicks++){
  begin(0,1);for(unsigned i=0;i<kicks;i++){early(500);wait_release();}
  for(unsigned i=0;i<6;i++){
   DWT->CYCCNT=fd_rev.sector_at+8000u*160u;fdRevObserveEdge();
   assert(fd_rev.confidence==i+1u&&fd_rev.rk_count==kicks);
   if(fd_rev.stage==FD_REV_PENDING){DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();}
  }
  assert(fd_rev.stage==FD_REV_CLOSED&&!fd_rev_acquiring&&zero_crosses==6);
  assert(fd_rk_diag[8+8*kicks+6]==6);
 }
 /* Missing initial event or missing subsequent event stops, never re-aligns. */
 begin(0,1);DWT->CYCCNT+=FD_RK_FIRST_TICKS;fdRevTick();assert(!outputs&&last_reason==FD_BENCH_NO_LOCK&&!fd_rev.rk_count);
 begin(0,1);early(500);wait_release();DWT->CYCCNT+=FD_RK_FIRST_TICKS;fdRevTick();assert(!outputs&&fd_rev.rk_count==1);
 begin(0,1);early(4000);DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();DWT->CYCCNT+=FD_REV_RETRY_TICKS;fdRevTick();assert(!outputs&&!fd_rev.rk_count);
 /* Zero/actual service cancels during align, pulse, off interval, or delayed commutation. */
 for(unsigned state=0;state<4;state++){
  begin(0,1);
  if(state==0){fdRevBeginPolicy(&fd_rev,DWT->CYCCNT);}
  if(state==2)early(500);
  if(state==3)early(4000);
  adjusted_input=0;fdBenchService();unsigned before=commutations;
  assert(!fd_rev_acquiring&&!fd_power_bench.active_id);
  DWT->CYCCNT+=500000u*160u;fdRevTick();assert(commutations==before);
 }
 for(unsigned fault=0;fault<3;fault++){
  begin(0,1);early(500);
  if(!fault)stuck_rotor_inhibit=1;else if(fault==1)drive_safety_limit_inhibit=1;else DWT->CYCCNT=FD_REV_ACQUIRE_TICKS;
  fdRevTick();assert(fd_power_bench.inhibit&&!outputs&&!scheduled&&!running);
 }
 begin(0,1);DWT->CYCCNT=fd_rev.sector_at+FD_RK_FIRST_TICKS;fdRevObserveEdge();assert(!outputs&&!scheduled&&!fd_rev.rk_count);
 puts("PASS EARLY_REKICK actual tick/service/event/PWM/stop: <=1ms inclusive, max3 replacements/4pulses,20ms all-off+5us gap, same original launch vector/no re-alignment, ordinary500us path and six usable events, no timeout rescue, faults/zero,630ms budget, directions/wrap");
}
