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
#define TARGET_FD_HIZ_OBSERVE
#define TARGET_FD_EDGE_RAW
#define TARGET_FD_EDGE_OBSERVE
#define TARGET_FD_ALIGN_SAMPLE_PAIR
#define TARGET_FD_ALIGN_TAIL
#define TARGET_FD_LAUNCH_RAW
#define TARGET_FD_ADC_COHERENT
#define FD6288_SECOND_ALIGN_MS 300
#include "fd6288_startup_sequence.h"
#include "fd6288_edge_observe.h"
volatile FdEdgeObserve fd_edge_observe;
volatile uint8_t fd_hiz_active;
/* ACTUAL_REASONS */
static struct { uint32_t CYCCNT; } dwt;
#define DWT (&dwt)
static FdRevPolicy fd_rev;
#ifdef TARGET_FD_M2_LOW_SPEED_PICKUP
/* ACTUAL_PICKUP_DATA */
#endif
#ifdef TARGET_FD_M2_HIZ_AFTER_TWO
#include "fd6288_start_retry_diag.h"
static FdStartRetryDiagnostics fd_start_retry_diag;
#define fdStartRetryRecord(...) ((void)0)
#define fdStartRetryEdgeRecord(...) ((void)0)
#endif
static struct { uint32_t inhibit, active_id, floor; } fd_power_bench;
static struct { uint32_t started; } fd_power_policy;
static struct { uint32_t frozen; } fd_align_observe;
static struct { uint8_t auto_advance; } eepromBuffer;
static uint32_t mask,outputs,commutations,scheduled,last_reason,failure_pin;
static struct { uint32_t CNT,CCR1,CCR4; } timer;
#define TIM1 (&timer)
static uint32_t tim1_arr=6665u;
#ifdef TARGET_FD_M2_POWERED_CAPTURE
static int16_t actual_current;
static uint16_t battery_voltage;
#endif
static unsigned pair_resets,adc_flag,adc_reads,rearms,bemf_calls;
static uint16_t adc_values[3];
static unsigned old_routine=1,stepper_sine,zcfound,bemfcounter,min_bemf_counts_up,min_bemf_counts_down,rising;
#define ADC1 1
#define LL_ADC_INJ_RANK_1 0
#define LL_ADC_INJ_RANK_2 1
#define LL_ADC_INJ_RANK_3 2
#define LL_ADC_IsActiveFlag_JEOS(x) (adc_flag)
#define LL_ADC_ClearFlag_JEOS(x) (adc_flag=0)
static uint16_t adc_read(unsigned r){adc_reads++;return adc_values[r];}
#define LL_ADC_INJ_ReadConversionData12(x,r) adc_read(r)
static void adcZcdRearmInjected(void){rearms++;}
static void getBemfState(void){bemf_calls++;adc_flag=0;}
static void heliCoastRecordZeroCross(void){}
static void zcfoundroutine(void){}
static void fdObserveAlignmentAdc(void){adc_flag=0;}
static void fdAlignmentPairReset(void){pair_resets++;}

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
static uint8_t auto_advance_level,temp_advance;
static uint32_t __get_PRIMASK(void){return mask;}
static void __disable_irq(void){mask=1;}
static void __set_PRIMASK(uint32_t v){mask=v;}
#define ADC_ZCD_RESET_COUNT 0
#define ADC_ZCD_RESET_TIMEOUT 1
#define FD_POWER_WINDOW_END 1
#define FD_POWER_FADE_TICKS (20000u*160u)
#if defined(TARGET_FD_M2_STEP4_ADC_PAIR) || defined(TARGET_FD_M2_ON_ADC_PAIR) || \
    defined(TARGET_FD_M2_STEP5_LATE_ADC)
#define ZCD_SAMPLE_FLOOR 460u
#define ZCD_ADC_WINDOW_TICKS 244u
#define ZCD_MINIMUM_ON_TICKS (ZCD_SAMPLE_FLOOR+ZCD_ADC_WINDOW_TICKS)
#ifdef TARGET_FD_M2_STEP4_ADC_PAIR
enum { FD_STEP4_PAIR_EARLY=600u, FD_STEP4_PAIR_LATE=780u };
volatile uint16_t fd_step4_pair_compare;
#endif
static uint8_t fd_align_sample_late;
/* ACTUAL_PAIR */
#define SET_DUTY_CYCLE_ALL(v) (outputs=(v),timer.CCR1=(v),timer.CCR4=zcd_ccr4_from_duty(v))
#else
#define SET_DUTY_CYCLE_ALL(v) (outputs=(v),timer.CCR1=(v),timer.CCR4=0u)
#endif
#define SET_INTERVAL_TIMER_COUNT(v) ((void)(v))
#ifdef TARGET_FD_M2_HANDOFF_CAPTURE
static uint32_t test_interval_timer;
#define INTERVAL_TIMER_COUNT test_interval_timer
#endif
#define SET_HELI_COAST_HIGH_Z_SAMPLE(v) (outputs=0,timer.CCR1=0,timer.CCR4=(v)/2u)
#define fdTraceEvent(...) ((void)0)
#define fdTraceFreeze(...) ((void)0)
#define fdTracePinFailure() (failure_pin++)
#define fdLightHandoff(...) ((void)0)
#define fdLightRetry(...) ((void)0)
#define fdLightEdge(...) ((void)0)
static void adcZcdSeedHandoff(uint32_t v){(void)v;}
static void adcZcdResetAcquisition(uint32_t v){(void)v;}
static unsigned stuckRotorGuardInput(void){return stuck_rotor_inhibit;}
static void allOff(void){outputs=0;scheduled=0;}
static void commutate(void){step=forward?(step==6?1:step+1):(step==1?6:step-1);commutations++;
#ifdef TARGET_FD_M2_HANDOFF_CAPTURE
 if(fd_m2_handoff.active)
  fdM2HandoffSwitch(DWT->CYCCNT,step,INTERVAL_TIMER_COUNT,TIM1->CCR1,TIM1->CCR4);
#endif
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
static void fdBenchFinish(uint8_t reason)
{
 last_reason=reason;
#ifdef TARGET_FD_M2_RUNUP_ADC_CAPTURE
 fd_m2_runup_adc.active_step=0u;
#endif
#ifdef TARGET_FD_M2_HANDOFF_CAPTURE
 fdM2HandoffFinish(DWT->CYCCNT,reason,step,INTERVAL_TIMER_COUNT);
#endif
 fdEdgeFinish(DWT->CYCCNT);
#ifdef TARGET_FD_M2_POWERED_CAPTURE
 fdM2PoweredFinish(DWT->CYCCNT);
#endif
 fd_power_bench.active_id=0;
}
static void fdPowerRequestStop(void *p,uint32_t reason){(void)p;(void)reason;}
static void fdPowerOperatorZero(void *p){(void)p;}
static unsigned fdBenchConfigOk(void){return 1;}
static void heliCoastTrackingLost(uint8_t inhibit){assert(fd_power_bench.inhibit);allOff();running=0;heli_coast_restart_inhibit=inhibit;}
static void fdBenchStop(uint8_t reason);
#define fdBenchObserve(...) ((void)0)
#define fdTraceTrialFinish(...) ((void)0)
static unsigned min_startup_duty=360,minimum_duty_cycle=210;
#ifdef TARGET_FD_M2_LOW_SPEED_PICKUP
static unsigned test_duty_limit=610u, pwm_updates;
static unsigned driveEffectiveDutyLimit(void){return test_duty_limit;}
static unsigned driveZcdMinimumDuty(void){return 212u;}
static void generatePwmTimerEvent(void)
{
 assert(fd_hiz_active && commutations==2u && timer.CCR1>0u);
 pwm_updates++;
}
#else
static unsigned driveEffectiveDutyLimit(void){return 610;}
#endif
/* ACTUAL_SERVICE */
#ifdef TARGET_FD_M2_LOW_SPEED_PICKUP
/* ACTUAL_PICKUP_ENTRY */
#endif
/* ACTUAL_ADAPTER */

static void begin(uint32_t start,unsigned dir){
 fd_hiz_active=0;fd_power_policy.started=start;fdRevBeginPolicy(&fd_rev,start);
#ifdef TARGET_FD_M2_LOW_SPEED_PICKUP
 fd_m2_pickup_capture=(FdM2PickupCapture){.trial=1u};
#ifdef TARGET_FD_M2_TIMED_RUNUP
 fd_m2_runup=(FdM2RunupCapture){.trial=1u};
#ifdef TARGET_FD_M2_RUNUP_ADC_CAPTURE
 fdM2RunupAdcReset(1u);
#endif
#endif
#ifdef TARGET_FD_M2_HANDOFF_CAPTURE
 fdM2HandoffReset(1u);
 test_interval_timer=0u;
#endif
#ifdef TARGET_FD_M2_STEP5_LATE_ADC
 fd_m2_startup_late_adc=fd_m2_aperture_discard=0u;
#endif
#ifdef TARGET_FD_M2_ON_ADC_PAIR
#ifndef TARGET_FD_M2_RUNUP_STEP4_PAIR
 fd_m2_on_adc_pair.trial=1u;
 fd_m2_on_adc_pair.count=fd_m2_on_adc_pair.max_cycles=0u;
#endif
 fd_m2_on_adc_step=fd_m2_on_adc_late=0u;
#endif
#ifdef TARGET_FD_M2_STEP5_COAST_COMPARE
 fd_m2_step5_coast=(FdM2Step5CoastCapture){.trial=1u,
  .pulse_us=FD_M2_STEP5_COAST_PULSE_US};
#endif
 test_duty_limit=610u;
 pwm_updates=0u;
#ifdef TARGET_FD_M2_POWERED_CAPTURE
 actual_current=80;
 battery_voltage=1150u;
 fdM2PoweredReset(1u);
#endif
#endif
 DWT->CYCCNT=start+30000u*160u;
 assert(fdRevAdvancePolicy(&fd_rev,DWT->CYCCNT)==FD_REV_STEP);
 fdEdgeBegin(1,start);
 fdEdgeStore(0,(FdEdgeSample){.at=start+327000u*160u,.a=1500,.b=800,.c=10,.reason=255,.step=6,.flags=5});
 fdEdgeStore(1,(FdEdgeSample){.at=start+329800u*160u,.a=1500,.b=801,.c=10,.reason=255,.step=6,.flags=5});
 fd_power_bench.inhibit=0;fd_power_bench.active_id=1;fd_power_bench.floor=360;
 armed=running=adjusted_input=input=fd_rev_acquiring=1;forward=dir;step=dir?6u:2u;
 outputs=123;commutations=last_reason=failure_pin=0;scheduled=1;
 mask=stuck_rotor_inhibit=drive_safety_limit_inhibit=heli_coast_active=heli_coast_restart_inhibit=0;
 timer.CCR1=1200;timer.CCR4=600;pair_resets=adc_reads=rearms=bemf_calls=0;
 DWT->CYCCNT=start+330000u*160u;
}
#ifdef TARGET_FD_M2_HIZ_AFTER_TWO
enum { HIZ_WHOLE_US=343000u+FD6288_M2_HIZ_STEP3_US+FD_M2_HIZ_STEP4_MS*1000u+FD_HIZ_OBSERVE_US,
 HIZ_OFF_US=342005u+FD6288_M2_HIZ_STEP3_US+FD_M2_HIZ_STEP4_MS*1000u,
 HIZ_DRIVEN_STEPS=2u+(FD_M2_HIZ_STEP4_MS!=0u) };
#else
enum { HIZ_WHOLE_US=336000u, HIZ_OFF_US=330000u, HIZ_DRIVEN_STEPS=0u };
#endif
static void release(void)
{
#ifdef TARGET_FD_M2_HIZ_AFTER_TWO
 fdRevTick();
 assert(commutations==1u && !fd_hiz_active && fd_rev.ramp_sector==1u);
 assert(step==(forward?2u:6u));
 for(unsigned i=0;i<HIZ_DRIVEN_STEPS;i++) {
  uint32_t start=fd_rev.sector_at;
  const unsigned dwell_us[]={12000u,FD6288_M2_HIZ_STEP3_US,FD_M2_HIZ_STEP4_MS*1000u};
  uint32_t dwell=dwell_us[i]*160u;
  assert(fd_rev.period==dwell && step==(forward?2u+i:6u-i));
#ifdef TARGET_FD_M2_STEP4_ADC_PAIR
  if(i==2u){
   assert(fd_step4_pair.trial==fd_power_bench.active_id);
   assert(fd_step4_pair_compare==600u&&timer.CCR4==600u);
   assert(zcd_ccr4_from_duty(0u)==0u&&zcd_ccr4_from_duty(1000u)==500u);
   rising=1;
   unsigned groups=FD_M2_HIZ_STEP4_MS==11u?261u:6u;
   for(unsigned n=0;n<groups;n++){
    DWT->CYCCNT=start+(n+1u)*42u*160u;
    fdBenchWritePwm(1334u);
    assert(timer.CCR1==1334u&&timer.CCR4==fd_step4_pair_compare);
    timer.CNT=timer.CCR4+260u;
    adc_values[0]=0;adc_values[1]=900;adc_values[2]=n<3u?450u:480u;
    adc_flag=1;fdAdcJeosService();
    assert(fd_step4_pair.count==(n+1u)/3u&&commutations==3u&&!bemf_calls);
   }
   assert(fd_step4_pair.samples[0].margin==0&&fd_step4_pair.samples[1].margin==20);
   adc_reads=rearms=pair_resets=0u;
  }
#endif
  DWT->CYCCNT=start+dwell-1u;fdRevObserveEdge();
  assert(commutations==i+1u&&!fd_rev.confidence&&!fd_hiz_active);
  fdRevTick();
  assert(commutations==i+1u&&!fd_hiz_active);
  DWT->CYCCNT++;fdRevTick();
 }
 assert(step==(forward?1u+HIZ_DRIVEN_STEPS:7u-HIZ_DRIVEN_STEPS)&&!fd_rev.ramp_sector&&fd_rev.sr_attempts==1u);
#else
 fdRevTick();
#endif
}
enum { CAPTURE_INPUT_GROUPS=FD_HIZ_OBSERVE_US*24u/1000u };
#ifdef TARGET_FD_M2_HIZ_40MS
enum { CAPTURE_RETAINED_GROUPS=486u };
#else
enum { CAPTURE_RETAINED_GROUPS=120u };
#endif

static uint32_t captureAge(unsigned group)
{
 return 160u+group*6667u; /* 24 kHz, plus a software service offset */
}
#ifdef TARGET_FD_M2_LOW_SPEED_PICKUP
static void replayPickup(const char *fixture)
{
 FILE *f=fopen(fixture,"r");assert(f);
 uint32_t off=DWT->CYCCNT;
 unsigned age,a,b,c;
 while(fscanf(f,"%u %u %u %u",&age,&a,&b,&c)==4){
  DWT->CYCCNT=off+age;
  adc_values[0]=a;adc_values[1]=b;adc_values[2]=c;
  adc_flag=1;fdAdcJeosService();fdRevTick();
  if(!fd_hiz_active||!running)break;
 }
 fclose(f);
}

#ifdef TARGET_FD_M2_TIMED_RUNUP
static void completeRunup(unsigned confirm)
{
 assert(fd_rev.runup_left==6u&&step==1u&&fd_rev_acquiring&&fd_m2_runup.sectors==1u);
 assert(!fd_m2_handoff.active&&fd_rev.confidence==6u&&!fd_m2_powered_capture.active);
 uint32_t base=fd_rev.period;
 const unsigned percent[]={100u,96u,92u,88u,84u,84u};
#ifdef TARGET_FD_M2_RUNUP_STEP4_ANCHOR
 temp_advance=8u;
#endif
 for(unsigned i=0;i<6u;i++){
  uint32_t start=fd_rev.sector_at, period=fd_rev.period;
  assert(step==i+1u&&fd_m2_runup.sectors==i+1u&&fd_m2_runup.sector[i].step==step);
  assert(period==base*percent[i]/100u&&fd_m2_runup.sector[i].period==period);
  assert(timer.CCR4==(i>=4u?960u:667u));
#ifdef TARGET_FD_M2_RUNUP_ADC_CAPTURE
#ifdef TARGET_FD_M2_RUNUP_STEP4_PAIR
  if(i==3u){
   unsigned prior=bemf_calls;
   for(unsigned j=0;j<80u;j++){
    DWT->CYCCNT=start+(j+1u)*42u*FD_REV_US;
    adc_values[0]=20u;adc_values[1]=2250u;
    adc_values[2]=timer.CCR4==960u?1240u:1050u;
    timer.CNT=2000u;adc_flag=1;fdAdcJeosService();
   }
   assert(bemf_calls==prior&&fd_m2_runup_adc.counts[0]==10u);
   assert(fd_m2_runup_adc.samples[0][0].margin>8&&fd_m2_runup_adc.samples[0][1].margin<-8);
  }else
#endif
  {
  for(unsigned j=0;j<112u;j++){
   uint8_t reason=j==31u?7u:2u;
   if(fdM2RunupAdcKeep(step,reason))
    fdM2RunupAdcStore((FdEdgeSample){.at=start+period*j/112u,.step=step,
        .reason=reason,.flags=1u,.trigger=timer.CCR4});
  }
  if(i>=3u)assert(fd_m2_runup_adc.counts[i-3u]==15u);
  }
#endif
  uint8_t diagnostic=0u;
#ifdef TARGET_FD_M2_RUNUP_STEP4_PAIR
  diagnostic=i==3u;
#endif
  uint8_t expect_qualified=confirm&&i!=0u&&!diagnostic;
  uint32_t age=i==0u?period/8u:period*5u/8u;
#ifdef TARGET_FD_M2_RUNUP_STEP4_ANCHOR
  if(i==1u)age=period*44u/100u;
  if(i==2u){
   age=period*34u/100u;
   expect_qualified=1u; /* An isolated edge must leave the original timer. */
  }
#endif
  if(!diagnostic){
   DWT->CYCCNT=start+age;
   if(expect_qualified||i==0u)fdRevObserveEdge();
  }
  assert(fd_m2_runup.sector[i].qualified==expect_qualified);
  assert(fd_rev_acquiring&&step==i+1u);
  uint32_t dwell=period;
#ifdef TARGET_FD_M2_RUNUP_STEP4_ANCHOR
  assert(fd_m2_runup.sector[i].phase_anchored==(i==2u&&confirm));
  if(i==2u&&confirm){
   dwell=age+fd_rev.last_real_period/2u-fd_rev.last_real_period/8u;
   assert(dwell<period&&fd_rev.runup_step4_after==dwell);
  }else{
   assert(!fd_rev.runup_step4_after);
  }
#endif
  DWT->CYCCNT=start+dwell-1u;fdRevTick();
  assert(fd_rev_acquiring&&step==i+1u);
  DWT->CYCCNT++;fdRevTick();
#ifdef TARGET_FD_M2_RUNUP_STEP4_ANCHOR
  if(i==2u)assert(fd_m2_runup.sector[3].at-fd_m2_runup.sector[2].at==dwell);
#endif
 }
 assert(fd_m2_runup.sectors==6u&&!fd_rev.runup_left);
#ifdef TARGET_FD_M2_RUNUP_ADC_CAPTURE
 assert(!fd_m2_runup_adc.active_step);
#endif
 if(confirm){
  assert(fd_rev.stage==FD_REV_CLOSED&&step==1u&&fd_m2_handoff.active&&timer.CCR4==667u);
  assert(fd_rev.measured==fd_rev.last_real_period&&fd_rev.measured==fd_rev.period);
 }else{
  assert(!running&&!outputs&&last_reason==FD_BENCH_NO_LOCK&&!fd_m2_handoff.active);
 }
}

static void checkRunupFailure(const char *coast)
{
 begin(UINT32_MAX-1000u,1u);release();replayPickup(coast);
 for(unsigned n=0;n<FD_REV_REQUIRED_EDGES;n++){
  DWT->CYCCNT=fd_rev.sector_at+4300u*FD_REV_US;
  fdRevObserveEdge();
  if(fd_rev.stage==FD_REV_PENDING){
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
  }
 }
 completeRunup(0u);
 puts("PASS bounded timed run-up: cadence, early-edge rejection, qualified handoff and timeout");
#ifdef TARGET_FD_M2_RUNUP_STEP4_ANCHOR
 puts("PASS timed step 4 anchor: coherent edges advance, isolated edge keeps original dwell");
#endif
}
#endif
static void checkPickup(const char *fixture)
{
 const unsigned trials =
#ifdef TARGET_FD_M2_EARLY_STEP5_DWELL
     7u;
#else
     5u;
#endif
 for(unsigned test=0;test<trials;test++){
  uint32_t start=UINT32_MAX-1000u;
  begin(start,1u);release();
  uint32_t off=DWT->CYCCNT;
  if(test==0) test_duty_limit=210u;
  replayPickup(fixture);
  if(test==0){
   assert(fd_hiz_active&&!running&&!outputs&&!pwm_updates&&commutations==2u);
   assert(last_reason==FD_BENCH_SAFETY_CAP);
   continue;
  }
  assert(!fd_hiz_active&&fd_rev.stage==FD_REV_OPEN&&fd_rev.pickup_started);
  assert(step==1u&&commutations==3u&&pwm_updates==1u&&fd_edge_observe.frozen);
  assert(DWT->CYCCNT-off>17000u*FD_REV_US&&DWT->CYCCNT-off<22000u*FD_REV_US);
  assert(fd_m2_pickup_capture.sector==1u&&fd_m2_pickup_capture.duty==400u);
  assert(fd_m2_pickup_capture.sector_ticks>=6000u*FD_REV_US);
  assert(!fd_rev.confidence&&!zero_crosses&&!scheduled);
  if(test==1){
   adjusted_input=0u;fdRevTick();
   assert(!running&&!outputs&&last_reason==FD_BENCH_OPERATOR_ZERO);
  }else if(test==2){
   DWT->CYCCNT=fd_rev.sector_at+FD_REV_RETRY_TICKS;fdRevTick();
   assert(!running&&!outputs&&last_reason==FD_BENCH_NO_LOCK&&!scheduled);
  }else if(test==3){
   DWT->CYCCNT=fd_rev.pickup_at+FD_M2_PICKUP_DRIVE_TICKS;
   fdRevTick();
   assert(!running&&!outputs&&last_reason==FD_BENCH_NO_LOCK&&!scheduled);
  }else{
   for(unsigned n=0;n<FD_REV_REQUIRED_EDGES;n++){
#ifdef TARGET_FD_M2_EARLY_STEP5_DWELL
    if(n==4u && test<6u){
     uint32_t start=fd_rev.sector_at;
     uint8_t early=test==4u;
     assert(step==5u&&fd_rev.confidence==4u&&timer.CCR4==960u);
     DWT->CYCCNT=start+(early?555u:1000u)*FD_REV_US;
     fdRevObserveEdge();
     assert(fd_rev.confidence==5u&&fd_rev.seen==early);
     if(early){
      assert(step==5u&&fd_rev.stage==FD_REV_PENDING&&scheduled);
      assert(fd_rev.edge_at+fd_rev.delay-start==2500u*FD_REV_US);
      DWT->CYCCNT=start+2500u*FD_REV_US-1u;fdRevTick();fdRevObserveEdge();
      assert(step==5u&&fd_rev.confidence==5u);
      DWT->CYCCNT++;fdRevTick();
     }
     assert(step==6u&&timer.CCR4==960u);
     break;
    }
#endif
    DWT->CYCCNT=fd_rev.sector_at+3000u*FD_REV_US;
    fdRevObserveEdge();
    if(fd_rev.stage==FD_REV_PENDING){DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();}
   }
#ifdef TARGET_FD_M2_EARLY_STEP5_DWELL
   if(test<6u){
    adjusted_input=0u;fdRevTick();
    assert(!running&&!outputs&&last_reason==FD_BENCH_OPERATOR_ZERO);
    continue;
   }
#endif
#ifdef TARGET_FD_M2_TIMED_RUNUP
   completeRunup(1u);
#endif
   assert(fd_rev.stage==FD_REV_CLOSED&&!fd_rev_acquiring&&zero_crosses==FD_REV_REQUIRED_EDGES);
   assert(!fdRevDeadlinePolicy(&fd_rev,start+500000u*FD_REV_US,start));
  }
 }
#ifdef TARGET_FD_M2_EARLY_STEP5_DWELL
 puts("PASS early step 5 dwell: fifth edge retained, minimum dwell and normal edge boundary");
#endif
 begin(0u,0u);release();replayPickup(fixture);
 assert(fd_hiz_active&&!fd_rev.pickup_started&&!pwm_updates);
 DWT->CYCCNT=fd_rev.sector_at+FD_HIZ_TICKS;fdRevTick();
 assert(!running&&!outputs&&commutations==2u);
}
#endif

#ifdef TARGET_FD_M2_POWERED_CAPTURE
static void poweredSample(uint32_t at, uint8_t reason)
{
 DWT->CYCCNT=at;
#ifdef TARGET_FD_M2_HANDOFF_CAPTURE
 if(fd_m2_handoff.active){
  if(fdM2HandoffKeepAdc(step,reason))
   fdM2HandoffStore((FdEdgeSample){.at=at,.step=step,.reason=reason,.flags=1u,
       .a=10u,.b=900u,.c=400u,.duty=timer.CCR1});
  return;
 }
#endif
 if(fdEdgeNeeded(0,reason,1u)){
  fdEdgeSample((FdEdgeSample){.at=at,.step=step,.reason=reason,.flags=1u,
      .a=10u,.b=900u,.c=400u,.duty=timer.CCR1});
  fdEdgeCost(100u);
 }
}

static void checkPoweredCapture(const char *coast, const char *output)
{
 begin(UINT32_MAX-1000u,1u);
 release();
 replayPickup(coast);
 assert(fd_m2_powered_capture.active && fd_m2_powered_capture.sectors==1u);
 assert(fd_m2_powered_capture.sector[0].step==1u && fd_m2_powered_capture.sector[0].compare>0u);
 unsigned count=fd_edge_observe.count;
 for(unsigned n=0;n<3u;n++){
  uint32_t at=fd_rev.sector_at;
  for(unsigned i=1u;i<=10u;i++)poweredSample(at+i*40u*FD_REV_US,2u);
  poweredSample(at+(n?4800u:440u)*FD_REV_US,7u);
  fdRevObserveEdge();
#ifdef TARGET_FD_M2_STEP4_DELAY_2200
  if(n==0u) {
   assert(fd_rev.stage==FD_REV_PENDING);
#ifdef TARGET_FD_M2_PICKUP_FIRST_DWELL_4000
   assert(fd_rev.edge_at+fd_rev.delay-fd_rev.pickup_at==4000u*FD_REV_US);
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay-1u;fdRevTick();
   assert(step==1u&&fd_m2_powered_capture.sectors==1u);
#else
   assert(fd_rev.delay==2000u*FD_REV_US);
#endif
  }
  if(n==1u) {
#ifdef TARGET_FD_M2_PICKUP_SECOND_DELAY_2000
   assert(fd_rev.stage==FD_REV_PENDING&&fd_rev.delay==2000u*FD_REV_US&&step==2u);
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay-1u;fdRevTick();
   assert(step==2u&&fd_m2_powered_capture.sectors==2u);
#else
   assert(fd_rev.stage==FD_REV_OPEN&&fd_rev.delay==0u&&step==3u);
#endif
  }
  if(n==2u){
   assert(fd_rev.stage==FD_REV_PENDING&&fd_rev.delay==2200u*FD_REV_US&&step==3u);
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay-1u;fdRevTick();
   assert(step==3u&&fd_m2_powered_capture.sectors==3u);
  }
#endif
  if(fd_rev.stage==FD_REV_PENDING){DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();}
 }
 assert(fd_m2_powered_capture.sectors==4u);
 fdM2PoweredSupply(fd_rev.sector_at+40000u,220,1110u);
 for(unsigned i=1u;i<540u;i++)poweredSample(fd_rev.sector_at+i*40u*FD_REV_US,2u);
 DWT->CYCCNT=fd_rev.sector_at+FD_REV_RETRY_TICKS;fdRevTick();
 assert(!fd_m2_powered_capture.active&&fd_m2_powered_capture.ended>30000u*FD_REV_US);
 assert(fd_m2_powered_capture.count<FD_M2_POWERED_MAX_SAMPLES && fd_edge_observe.count==count);
 assert(!running&&!outputs&&last_reason==FD_BENCH_NO_LOCK);
 FILE *f=fopen(output,"wb");assert(f);
 assert(fwrite((const void*)&fd_m2_powered_capture,sizeof(fd_m2_powered_capture),1,f)==1);
 fclose(f);

 begin(0u,1u);release();replayPickup(coast);
 assert(fd_m2_powered_capture.active&&fd_m2_powered_capture.sectors==1u&&!fd_m2_powered_capture.count);
 poweredSample(fd_m2_powered_capture.started+FD_M2_POWERED_CAPTURE_TICKS,2u);
 assert(!fd_m2_powered_capture.active&&running);
 assert(fd_m2_powered_capture.ended==FD_M2_POWERED_CAPTURE_TICKS);
}

#ifdef TARGET_FD_M2_STEP5_COAST_COMPARE
static void checkStep5Coast(const char *coast)
{
 const unsigned expected_us[] = {0u, 4000u, 9000u};
 for (unsigned test=0;test<2u;test++){
  begin(UINT32_MAX-1000u,1u);release();replayPickup(coast);
  for (unsigned n=0;n<3u;n++){
   DWT->CYCCNT=fd_rev.sector_at+(n?4800u:440u)*FD_REV_US;
   fdRevObserveEdge();
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
  }
  assert(step==4u&&fd_rev.confidence==3u);
  DWT->CYCCNT=fd_rev.sector_at+(test?600u:240u)*FD_REV_US;
  fdRevObserveEdge();
  if(test){
   assert(step==5u&&!fd_m2_step5_coast.edge_at&&!fd_hiz_active);
   continue;
  }
  assert(fd_m2_step5_coast.edge_at&&fd_rev.confidence==4u);
  if(FD_M2_STEP5_COAST_PULSE_US){
   assert(step==5u&&fd_m2_step5_coast.step5_at&&!fd_m2_step5_coast.off_at&&outputs);
   uint32_t off=fd_power_policy.started+fd_m2_step5_coast.step5_at+
       FD_M2_STEP5_COAST_PULSE_US*FD_REV_US;
   DWT->CYCCNT=off-1u;fdRevObserveEdge();fdRevTick();
   assert(step==5u&&!fd_hiz_active&&outputs&&fd_rev.confidence==4u);
   DWT->CYCCNT=off;fdRevTick();
  }else{
   assert(step==4u&&!fd_m2_step5_coast.step5_at);
  }
  uint32_t off=DWT->CYCCNT;
  assert(fd_hiz_active&&!outputs&&!scheduled&&!timer.CCR1);
  assert(fd_m2_step5_coast.off_at==off-fd_power_policy.started);
  if(fd_m2_step5_coast.step5_at)
   assert(fd_m2_step5_coast.off_at-fd_m2_step5_coast.step5_at==
       expected_us[FD6288_M2_STEP5_COAST_COMPARE-1u]*FD_REV_US);
  assert(!fd_edge_observe.frozen&&fd_edge_observe.start0==fd_m2_step5_coast.off_at);
  assert(!fd_m2_powered_capture.active&&fd_m2_pickup_capture.coast_at<fd_m2_step5_coast.off_at);
  unsigned before=commutations;
  for(unsigned i=0;i<CAPTURE_INPUT_GROUPS;i++){
   DWT->CYCCNT=off+captureAge(i);
   adc_flag=1;fdAdcJeosService();fdRevTick();
   assert(!outputs&&commutations==before);
  }
  DWT->CYCCNT=off+FD_HIZ_TICKS;fdRevTick();
  assert(!running&&!outputs&&last_reason==FD_BENCH_HIZ_DONE);
  assert(fd_edge_observe.frozen&&fd_edge_observe.first_samples==CAPTURE_RETAINED_GROUPS);
 }
 puts("PASS early step 4 comparison: gated trigger, bounded pulse, final coast, no re-energizing");
}
#endif
#ifdef TARGET_FD_M2_STEP35_ADC_PAIR
static void checkStep35Samples(uint32_t start)
{
 unsigned count=fd_m2_on_adc_pair.count;
 unsigned detector_calls=bemf_calls;
 for(unsigned n=0;n<7u;n++){
  DWT->CYCCNT=start+(n+1u)*42u*FD_REV_US;
  uint16_t floating=timer.CCR4==FD_M2_LATE_ADC_TICKS ? 1050u : 1240u;
  adc_values[0]=step==3u ? 20u : floating;
  adc_values[1]=step==3u ? floating : 2250u;
  adc_values[2]=step==3u ? 2250u : 20u;
  timer.CNT=2000u;adc_flag=1;fdAdcJeosService();
 }
 assert(bemf_calls==detector_calls&&fd_m2_on_adc_pair.count==count+2u);
 assert(fd_m2_on_adc_pair.samples[count].margin>8);
 assert(fd_m2_on_adc_pair.samples[count+1u].margin< -8);
}

static void checkStep35Pair(const char *coast,const char *output)
{
 begin(0u,1u);release();replayPickup(coast);
 for(unsigned n=0;n<2u;n++){
  DWT->CYCCNT=fd_rev.sector_at+(n?4800u:440u)*FD_REV_US;
  fdRevObserveEdge();DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
 }
 assert(step==3u&&fd_rev.stage==FD_REV_OPEN);
 DWT->CYCCNT+=3000u*FD_REV_US;
 unsigned detector_calls=bemf_calls;
 adc_flag=1;fdAdcJeosService();
 assert(bemf_calls==detector_calls+1u&&fd_m2_on_adc_pair.count==0u);
 fdRevObserveEdge();
 assert(fd_rev.stage==FD_REV_PENDING&&fd_rev.delay==2200u*FD_REV_US);
 checkStep35Samples(fd_rev.edge_at);
 DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay-500u*FD_REV_US;
 adc_flag=1;fdAdcJeosService();
 assert(!fd_m2_on_adc_step&&timer.CCR4==667u);
 DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
 assert(step==4u&&timer.CCR4==667u);
 DWT->CYCCNT+=240u*FD_REV_US;fdRevObserveEdge();
 assert(step==5u&&fd_m2_step5_coast.step5_at);
 checkStep35Samples(DWT->CYCCNT);
 DWT->CYCCNT=fd_power_policy.started+fd_m2_step5_coast.step5_at+9000u*FD_REV_US;
 fdRevTick();
 assert(fd_hiz_active&&!outputs&&!fd_m2_on_adc_late&&timer.CCR4==3332u);
 FILE *f=fopen(output,"wb");assert(f);
 size_t size=12u+fd_m2_on_adc_pair.count*sizeof(FdEdgeSample);
 assert(fwrite((const void*)&fd_m2_on_adc_pair,size,1,f)==1);fclose(f);
 puts("PASS step 3 keeps its controlling ADC, step 5 samples are diagnostic, full-off restores ADC");
}
#endif
#ifdef TARGET_FD_M2_STEP6_ADC_PAIR
static void checkStep6Pair(const char *coast, const char *output)
{
 begin(UINT32_MAX-1000u,1u);release();replayPickup(coast);
 for(unsigned n=0;n<5u;n++){
  DWT->CYCCNT=fd_rev.sector_at+(n?3000u:440u)*FD_REV_US;
  fdRevObserveEdge();
  if(fd_rev.stage==FD_REV_PENDING){
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
  }
 }
 assert(step==6u&&fd_rev.confidence==5u&&timer.CCR4==667u);
 assert(!fd_m2_startup_late_adc&&fd_m2_on_adc_pair.count==0u);
 uint32_t start=fd_rev.sector_at;
 unsigned detector_calls=bemf_calls;
 for(unsigned n=1u;n<=535u;n++){
  DWT->CYCCNT=start+n*42u*FD_REV_US;
  adc_values[0]=2250u;
  adc_values[1]=timer.CCR4==960u ? 1240u : 1050u;
  adc_values[2]=20u;
  timer.CNT=2000u;adc_flag=1;fdAdcJeosService();
 }
 assert(bemf_calls==detector_calls&&step==6u&&fd_m2_on_adc_pair.count==90u);
 assert(fd_m2_on_adc_pair.samples[0].margin>8);
 assert(fd_m2_on_adc_pair.samples[1].margin< -8);
 assert(fd_m2_on_adc_pair.samples[89].at-fd_m2_on_adc_pair.samples[0].at>22000u*FD_REV_US);
 DWT->CYCCNT=start+FD_REV_RETRY_TICKS;fdRevTick();
 assert(!running&&!outputs&&last_reason==FD_BENCH_NO_LOCK);
 FILE *f=fopen(output,"wb");assert(f);
 size_t size=12u+fd_m2_on_adc_pair.count*sizeof(FdEdgeSample);
 assert(fwrite((const void*)&fd_m2_on_adc_pair,size,1,f)==1);fclose(f);
 puts("PASS step 6 pairs retain the full sector, exclude feedback, and time out normally");
}
#endif
#ifdef TARGET_FD_M2_STEP5_LATE_ADC
static void checkStep5LateAdc(const char *coast)
{
 begin(UINT32_MAX-1000u,1u);release();replayPickup(coast);
 assert(step==1u&&timer.CCR4==667u&&!fd_m2_startup_late_adc);
 for(unsigned n=0;n<4u;n++){
  DWT->CYCCNT=fd_rev.sector_at+(n?3000u:440u)*FD_REV_US;
  fdRevObserveEdge();
  if(fd_rev.stage==FD_REV_PENDING){
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
  }
 }
 assert(step==5u&&fd_rev.confidence==4u&&fd_m2_startup_late_adc&&timer.CCR4==960u);
 unsigned before=bemf_calls;
 fdBenchWritePwm(1203u);
 assert(timer.CCR4==0u);
 adc_flag=1;fdAdcJeosService();
 assert(bemf_calls==before);
 fdBenchWritePwm(1334u);
 assert(timer.CCR4==960u);
 for(unsigned i=0;i<2u;i++){adc_flag=1;fdAdcJeosService();}
 assert(bemf_calls==before);
 adc_flag=1;fdAdcJeosService();
 assert(bemf_calls==before+1u);
 DWT->CYCCNT=fd_rev.sector_at+4300u*FD_REV_US;
 fdRevObserveEdge();
#ifdef TARGET_FD_M2_STEP6_LATE_ADC
 assert(step==6u&&fd_rev.confidence==5u&&fd_m2_startup_late_adc&&timer.CCR4==960u);
 adc_flag=1;fdAdcJeosService();
 assert(bemf_calls==before+2u);
 DWT->CYCCNT=fd_rev.sector_at+4600u*FD_REV_US;
 fdRevObserveEdge();
 assert(fd_rev.stage==FD_REV_PENDING&&timer.CCR4==960u);
 DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
#ifdef TARGET_FD_M2_TIMED_RUNUP
 completeRunup(1u);
#endif
 assert(step==1u&&fd_rev.stage==FD_REV_CLOSED&&!fd_m2_startup_late_adc&&timer.CCR4==667u);
 assert(fd_m2_handoff.records[0].trigger==667u&&fd_m2_aperture_discard==2u);
#else
 assert(step==6u&&fd_rev.confidence==5u&&!fd_m2_startup_late_adc&&timer.CCR4==667u);
#ifndef TARGET_FD_M2_STEP6_ADC_PAIR
 for(unsigned i=0;i<2u;i++){adc_flag=1;fdAdcJeosService();}
 assert(bemf_calls==before+1u);
 adc_flag=1;fdAdcJeosService();
 assert(bemf_calls==before+2u);
#endif
#endif
 puts("PASS startup late control, narrow-window rejection, preload discard and normal aperture restoration");
}
#endif
#ifdef TARGET_FD_M2_HANDOFF_CAPTURE
static void checkHandoffCapture(const char *coast,const char *output)
{
#ifdef TARGET_FD_M2_CLOSED_STEP4_DENSE
 for(uint8_t vector=3u;vector<=4u;vector++){
  fdM2HandoffReset(1u);
  unsigned kept=0u;
  for(unsigned i=0;i<16u;i++)kept+=fdM2HandoffKeepAdc(vector,2u);
  assert(kept==(vector==4u?16u:4u));
 }
#endif
 begin(UINT32_MAX-389000u*FD_REV_US,1u);release();replayPickup(coast);
 for(unsigned n=0;n<FD_REV_REQUIRED_EDGES;n++){
  DWT->CYCCNT=fd_rev.sector_at+(n?3000u:440u)*FD_REV_US;
  fdRevObserveEdge();
  if(fd_rev.stage==FD_REV_PENDING){
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
  }
 }
#ifdef TARGET_FD_M2_TIMED_RUNUP
 completeRunup(1u);
#endif
 assert(fd_rev.stage==FD_REV_CLOSED&&step==1u&&fd_m2_handoff.active);
 assert(!fd_m2_powered_capture.active&&fd_m2_handoff.seed_ticks>0u);
 assert(fd_m2_handoff.switches==1u&&fd_m2_handoff.count==1u);
 assert(fd_m2_handoff.records[0].trigger==667u);
 for(unsigned i=0;i<800u;i++)
  poweredSample(DWT->CYCCNT+42u*FD_REV_US,2u);
 assert(fd_m2_handoff.count==FD_M2_RUN_CAPACITY);
 poweredSample(DWT->CYCCNT+42u*FD_REV_US,7u);
 fdM2HandoffFeedback(DWT->CYCCNT+1u,step,8700u,9000u,3400u);
 DWT->CYCCNT+=1700u*FD_REV_US;commutate();
 DWT->CYCCNT+=22500u*FD_REV_US;test_interval_timer=45000u;
 fdBenchStop(FD_BENCH_LOST_FEEDBACK);
 assert(!fd_m2_handoff.active&&fd_m2_handoff.stop_reason==FD_BENCH_LOST_FEEDBACK);
 assert(fd_m2_handoff.stop_step==2u&&fd_m2_handoff.feedbacks==1u);
 FILE *f=fopen(output,"wb");assert(f);
 assert(fwrite((const void*)&fd_m2_handoff,sizeof(fd_m2_handoff),1,f)==1);fclose(f);
 puts("PASS runtime handoff capture: first switch, rolling ADC and feedback, timeout freeze");
}
#endif
#ifdef TARGET_FD_M2_STEP5_DELAY_4900
static void checkStep5Delay(const char *coast)
{
 for(unsigned late=0;late<2u;late++){
  begin(UINT32_MAX-1000u,1u);release();replayPickup(coast);
  for(unsigned n=0;n<3u;n++){
   DWT->CYCCNT=fd_rev.sector_at+(n?4800u:440u)*FD_REV_US;
   fdRevObserveEdge();
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay;fdRevTick();
  }
  assert(step==4u&&fd_rev.confidence==3u);
  DWT->CYCCNT=fd_rev.sector_at+(late?600u:240u)*FD_REV_US;
  fdRevObserveEdge();
  if(!late){
   assert(step==4u&&fd_rev.stage==FD_REV_PENDING&&scheduled);
   assert(fd_rev.delay==4900u*FD_REV_US&&fd_rev.confidence==4u);
   DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay-1u;fdRevTick();fdRevObserveEdge();
   assert(step==4u&&outputs&&fd_rev.confidence==4u);
   DWT->CYCCNT++;fdRevTick();
  }
  assert(step==5u&&fd_rev.stage==FD_REV_OPEN&&fd_rev.confidence==4u&&!fd_hiz_active);
  DWT->CYCCNT=fd_rev.sector_at+2700u*FD_REV_US;
  fdRevObserveEdge();
  assert(step==6u&&fd_rev.confidence==5u&&!fd_hiz_active);
 }
 puts("PASS step 5 timing: early edge holds step 4, late edge unaffected, fifth edge drives step 6");
}
#endif
#endif

int main(int argc,char **argv){
 assert(FD_HIZ_TOTAL_TICKS==HIZ_WHOLE_US*160u);
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned dir=0;dir<2;dir++){
  uint32_t start=wrap?UINT32_MAX-1000u:1000u;begin(start,dir);
  release();uint32_t off=DWT->CYCCNT;
  uint32_t last_step=step;
  assert(fd_hiz_active&&fd_rev.stage==FD_REV_HIZ&&!outputs&&!scheduled&&commutations==HIZ_DRIVEN_STEPS);
#ifdef TARGET_FD_M2_STEP4_ADC_PAIR
  assert(!fd_step4_pair_compare&&fd_step4_pair.count==(FD_M2_HIZ_STEP4_MS==11u?87u:2u));
#endif
  assert(!timer.CCR1&&timer.CCR4==3332&&pair_resets==1&&!fd_power_bench.floor);
  assert(fd_edge_observe.abi==FD_HIZ_OBSERVE_ABI&&fd_edge_observe.samples[0].b==800&&fd_edge_observe.samples[1].b==801);
  assert(fd_edge_observe.start0==HIZ_OFF_US*160u&&fd_edge_observe.start1==HIZ_OFF_US*160u);
  for(unsigned i=0;i<CAPTURE_INPUT_GROUPS;i++){
   DWT->CYCCNT=off+captureAge(i);
   fdBenchWritePwm(999);assert(!outputs&&!timer.CCR1&&timer.CCR4==3332);
   adc_flag=1;adc_values[0]=100+i;adc_values[1]=80+i;adc_values[2]=60+i;
   fdAdcJeosService();assert(!adc_flag&&rearms==i+1&&adc_reads==(i+1)*3&&!bemf_calls);
   fdRevObserveEdge();assert(commutations==HIZ_DRIVEN_STEPS&&!scheduled&&!fd_rev.confidence&&step==last_step);
  }
  assert(fd_edge_observe.count==CAPTURE_RETAINED_GROUPS&&fd_edge_observe.samples[2].flags&16);
  assert(!(fd_edge_observe.samples[10].flags&16));
  assert(fd_edge_observe.seen==CAPTURE_INPUT_GROUPS);
  assert(fd_edge_observe.post_samples==CAPTURE_INPUT_GROUPS-CAPTURE_RETAINED_GROUPS);
  DWT->CYCCNT=off+FD_HIZ_TICKS;fdRevTick();
  assert(!outputs&&!running&&!fd_rev_acquiring&&fd_power_bench.inhibit&&fd_hiz_active);
  assert(last_reason==FD_BENCH_HIZ_DONE&&!failure_pin&&commutations==HIZ_DRIVEN_STEPS&&!scheduled);
  assert(fd_edge_observe.frozen&&fd_edge_observe.count==CAPTURE_RETAINED_GROUPS);
  fdBenchWritePwm(999);assert(!outputs&&!timer.CCR4);
  adc_flag=1;fdAdcJeosService();assert(!bemf_calls&&adc_reads==3u*CAPTURE_INPUT_GROUPS);
  assert(fdBenchGuardInput());adjusted_input=0;assert(!fdBenchGuardInput()&&!fd_hiz_active&&!heli_coast_restart_inhibit);
 }
 for(unsigned fault=0;fault<4;fault++){
  begin(0,1);release();
  if(!fault)stuck_rotor_inhibit=1;else if(fault==1)drive_safety_limit_inhibit=1;else if(fault==2)DWT->CYCCNT=HIZ_WHOLE_US*160u;else adjusted_input=0;
  fdBenchWritePwm(999);assert(!outputs&&!scheduled&&!timer.CCR4&&fd_power_bench.inhibit);
 }
 /* Zero-duration stale queue records stay flagged, late data freezes without store. */
 begin(0,1);release();uint32_t off=DWT->CYCCNT;
 for(unsigned n=0;n<512;n++)fdEdgeHiZSample((FdEdgeSample){.at=off+200u*160u,.a=10,.b=20,.c=30,.step=6,.reason=253,.flags=33});
 assert(fd_edge_observe.count==512&&fd_edge_observe.frozen);
 assert(fd_edge_observe.samples[2].flags&16);assert(fd_edge_observe.samples[3].flags&16);assert(!(fd_edge_observe.samples[4].flags&16));
 fdEdgeHiZSample((FdEdgeSample){.at=off+FD_HIZ_TICKS-1u});assert(fd_edge_observe.count==512);
 begin(0,1);release();fdEdgeHiZSample((FdEdgeSample){.at=DWT->CYCCNT+FD_HIZ_TICKS});assert(fd_edge_observe.frozen&&!fd_edge_observe.count);
 /* Save a full-window capture for the JS decoder. */
 begin(0,1);release();off=DWT->CYCCNT;
 for(unsigned n=0;n<CAPTURE_INPUT_GROUPS;n++)fdEdgeHiZSample((FdEdgeSample){.at=off+captureAge(n),.a=40,.b=20,.c=0,.step=step,.reason=253,.flags=33});
 DWT->CYCCNT=off+FD_HIZ_TICKS;fdRevTick();
 if(argc>=2){FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite((const void*)&fd_edge_observe,sizeof(fd_edge_observe),1,f)==1);fclose(f);}
#ifdef TARGET_FD_M2_HIZ_AFTER_TWO
#ifdef TARGET_FD_M2_STEP4_ADC_PAIR
 if(argc==3){FILE *f=fopen(argv[2],"wb");assert(f);assert(fwrite((const void*)&fd_step4_pair,sizeof(fd_step4_pair),1,f)==1);fclose(f);}
#endif
 /* An overdue drive service cuts power without advancing another vector. */
 begin(0,1);fdRevTick();assert(commutations==1);
 DWT->CYCCNT=HIZ_WHOLE_US*160u;fdRevTick();
 assert(commutations==1&&!outputs&&!running&&fd_power_bench.inhibit);
#endif
#ifdef TARGET_FD_M2_LOW_SPEED_PICKUP
 assert(argc>=4);checkPickup(argv[3]);
#ifdef TARGET_FD_M2_TIMED_RUNUP
 checkRunupFailure(argv[3]);
#ifdef TARGET_FD_M2_RUNUP_ADC_CAPTURE
 assert(argc==7);
 FILE *runup=fopen(argv[6],"wb");assert(runup);
 assert(fwrite((const void*)&fd_m2_runup_adc,sizeof(fd_m2_runup_adc),1,runup)==1);
 fclose(runup);
#endif
#endif
#ifdef TARGET_FD_M2_POWERED_CAPTURE
#ifdef TARGET_FD_M2_STEP35_ADC_PAIR
 assert(argc==6);checkStep35Pair(argv[3],argv[5]);
#elif defined(TARGET_FD_M2_STEP6_ADC_PAIR)
 assert(argc==6);checkStep6Pair(argv[3],argv[5]);
#elif defined(TARGET_FD_M2_HANDOFF_CAPTURE)
#ifdef TARGET_FD_M2_RUNUP_ADC_CAPTURE
 assert(argc==7);
#else
 assert(argc==6);
#endif
 checkHandoffCapture(argv[3],argv[5]);
#else
 assert(argc==5);
#endif
 checkPoweredCapture(argv[3],argv[4]);
#ifdef TARGET_FD_M2_STEP5_COAST_COMPARE
 checkStep5Coast(argv[3]);
#endif
#ifdef TARGET_FD_M2_STEP5_DELAY_4900
 checkStep5Delay(argv[3]);
#endif
#ifdef TARGET_FD_M2_STEP5_LATE_ADC
 checkStep5LateAdc(argv[3]);
#endif
#endif
 puts("PASS real M2 coast replay: correct sector, current cap, zero, timeout, six-feedback handoff, opposite direction");
#endif
 puts("PASS Hi-Z actual tick/stop/event/PWM/JEOS: drive boundary, observation duration, trial cap, frozen512, boundary flags, no detector in Hi-Z, faults, zero rearm, directions, DWT wrap");
}
