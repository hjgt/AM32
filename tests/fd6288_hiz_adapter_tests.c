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
static struct { uint32_t inhibit, active_id, floor; } fd_power_bench;
static struct { uint32_t started; } fd_power_policy;
static struct { uint32_t frozen; } fd_align_observe;
static struct { uint8_t auto_advance; } eepromBuffer;
static uint32_t mask,outputs,commutations,scheduled,last_reason,failure_pin;
static struct { uint32_t CNT,CCR1,CCR4; } timer;
#define TIM1 (&timer)
static uint32_t tim1_arr=6665u;
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
#define SET_DUTY_CYCLE_ALL(v) (outputs=(v),timer.CCR1=(v),timer.CCR4=0u)
#define SET_INTERVAL_TIMER_COUNT(v) ((void)(v))
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
static void fdBenchFinish(uint8_t reason){last_reason=reason;fdEdgeFinish(DWT->CYCCNT);fd_power_bench.active_id=0;}
static void fdPowerRequestStop(void *p,uint32_t reason){(void)p;(void)reason;}
static void fdPowerOperatorZero(void *p){(void)p;}
static unsigned fdBenchConfigOk(void){return 1;}
static void heliCoastTrackingLost(uint8_t inhibit){assert(fd_power_bench.inhibit);allOff();running=0;heli_coast_restart_inhibit=inhibit;}
static void fdBenchStop(uint8_t reason);
#define fdBenchObserve(...) ((void)0)
#define fdTraceTrialFinish(...) ((void)0)
static unsigned min_startup_duty=360,minimum_duty_cycle=210;
static unsigned driveEffectiveDutyLimit(void){return 610;}
/* ACTUAL_SERVICE */
/* ACTUAL_ADAPTER */

static void begin(uint32_t start,unsigned dir){
 fd_hiz_active=0;fd_power_policy.started=start;fdRevBeginPolicy(&fd_rev,start);
 DWT->CYCCNT=start+30000u*160u;
 assert(fdRevAdvancePolicy(&fd_rev,DWT->CYCCNT)==FD_REV_STEP);
 fdEdgeBegin(1,start);
 fdEdgeStore(0,(FdEdgeSample){.at=start+327000u*160u,.a=1500,.b=800,.c=10,.reason=255,.step=6,.flags=5});
 fdEdgeStore(1,(FdEdgeSample){.at=start+329800u*160u,.a=1500,.b=801,.c=10,.reason=255,.step=6,.flags=5});
 fd_power_bench.inhibit=0;fd_power_bench.active_id=1;fd_power_bench.floor=360;
 armed=running=adjusted_input=input=fd_rev_acquiring=1;forward=dir;step=6;
 outputs=123;commutations=last_reason=failure_pin=0;scheduled=1;
 mask=stuck_rotor_inhibit=drive_safety_limit_inhibit=heli_coast_active=heli_coast_restart_inhibit=0;
 timer.CCR1=1200;timer.CCR4=600;pair_resets=adc_reads=rearms=bemf_calls=0;
 DWT->CYCCNT=start+330000u*160u;
}
int main(int argc,char **argv){
 assert(FD_HIZ_TOTAL_TICKS==336000u*160u);
 for(unsigned wrap=0;wrap<2;wrap++)for(unsigned dir=0;dir<2;dir++){
  uint32_t start=wrap?UINT32_MAX-1000u:1000u;begin(start,dir);
  fdRevTick();uint32_t off=DWT->CYCCNT;
  assert(fd_hiz_active&&fd_rev.stage==FD_REV_HIZ&&!outputs&&!scheduled&&!commutations);
  assert(!timer.CCR1&&timer.CCR4==3332&&pair_resets==1&&!fd_power_bench.floor);
  assert(fd_edge_observe.abi==7&&fd_edge_observe.samples[0].b==800&&fd_edge_observe.samples[1].b==801);
  assert(fd_edge_observe.start0==330000u*160u&&fd_edge_observe.start1==330000u*160u);
  for(unsigned i=0;i<120;i++){
   DWT->CYCCNT=off+i*42u*160u;if(i==119)DWT->CYCCNT=off+4999u*160u;
   fdBenchWritePwm(999);assert(!outputs&&!timer.CCR1&&timer.CCR4==3332);
   adc_flag=1;adc_values[0]=100+i;adc_values[1]=80+i;adc_values[2]=60+i;
   fdAdcJeosService();assert(!adc_flag&&rearms==i+1&&adc_reads==(i+1)*3&&!bemf_calls);
   fdRevObserveEdge();assert(!commutations&&!scheduled&&!fd_rev.confidence&&step==6);
  }
  assert(fd_edge_observe.count==120&&fd_edge_observe.samples[2].flags&16);
  assert(!(fd_edge_observe.samples[10].flags&16));
  DWT->CYCCNT=off+5000u*160u;fdRevTick();
  assert(!outputs&&!running&&!fd_rev_acquiring&&fd_power_bench.inhibit&&fd_hiz_active);
  assert(last_reason==FD_BENCH_HIZ_DONE&&!failure_pin&&!commutations&&!scheduled);
  assert(fd_edge_observe.frozen&&fd_edge_observe.count==120);
  fdBenchWritePwm(999);assert(!outputs&&!timer.CCR4);
  adc_flag=1;fdAdcJeosService();assert(!bemf_calls&&adc_reads==360);
  assert(fdBenchGuardInput());adjusted_input=0;assert(!fdBenchGuardInput()&&!fd_hiz_active&&!heli_coast_restart_inhibit);
 }
 for(unsigned fault=0;fault<4;fault++){
  begin(0,1);fdRevTick();
  if(!fault)stuck_rotor_inhibit=1;else if(fault==1)drive_safety_limit_inhibit=1;else if(fault==2)DWT->CYCCNT=336000u*160u;else adjusted_input=0;
  fdBenchWritePwm(999);assert(!outputs&&!scheduled&&!timer.CCR4&&fd_power_bench.inhibit);
 }
 /* Zero-duration stale queue records stay flagged, late data freezes without store. */
 begin(0,1);fdRevTick();uint32_t off=DWT->CYCCNT;
 for(unsigned n=0;n<512;n++)fdEdgeHiZSample((FdEdgeSample){.at=off+200u*160u,.a=10,.b=20,.c=30,.step=6,.reason=253,.flags=33});
 assert(fd_edge_observe.count==512&&fd_edge_observe.frozen);
 assert(fd_edge_observe.samples[2].flags&16);assert(fd_edge_observe.samples[3].flags&16);assert(!(fd_edge_observe.samples[4].flags&16));
 fdEdgeHiZSample((FdEdgeSample){.at=off+4999u*160u});assert(fd_edge_observe.count==512);
 begin(0,1);fdRevTick();fdEdgeHiZSample((FdEdgeSample){.at=DWT->CYCCNT+5000u*160u});assert(fd_edge_observe.frozen&&!fd_edge_observe.count);
 /* Save representative120-group capture for real JS decoder validation. */
 begin(0,1);fdRevTick();off=DWT->CYCCNT;
 for(unsigned n=0;n<120;n++)fdEdgeHiZSample((FdEdgeSample){.at=off+n*42u*160u,.a=100,.b=80,.c=60,.step=6,.reason=253,.flags=33});
 DWT->CYCCNT=off+5000u*160u;fdRevTick();
 if(argc==2){FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite((const void*)&fd_edge_observe,sizeof(fd_edge_observe),1,f)==1);fclose(f);}
 puts("PASS Hi-Z actual tick/stop/event/PWM/JEOS: no launch,5ms,336ms cap, frozen512, boundary flags, no detector, faults, zero rearm, directions, DWT wrap");
}
