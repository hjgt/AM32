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
#define TARGET_FD_LAUNCH_HOLD
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
static void commutate(void){step=forward?(step==6?1:step+1):(step==1?6:step-1);commutations++;}
static void fdAsyncScheduleDeadline(uint32_t a,uint32_t b){(void)a;(void)b;scheduled++;}
static void fdBenchFinish(uint8_t reason){last_reason=reason;fd_power_bench.active_id=0;}
static void fdPowerRequestStop(void *p,uint32_t reason){(void)p;(void)reason;}
static void fdPowerOperatorZero(void *p){(void)p;}
static unsigned fdBenchConfigOk(void){return 1;}
static void heliCoastTrackingLost(uint8_t inhibit){assert(fd_power_bench.inhibit);allOff();running=0;heli_coast_restart_inhibit=inhibit;}
static void fdBenchStop(uint8_t reason);
static void fdBenchService(void){
    if(!fd_power_bench.active_id || fd_power_bench.inhibit)return;
    if(stuck_rotor_inhibit)fdBenchStop(FD_BENCH_NATIVE_STUCK);
    else if(drive_safety_limit_inhibit)fdBenchStop(FD_BENCH_SAFETY_CAP);
    else if(fdRevDeadlinePolicy(&fd_rev,DWT->CYCCNT,fd_power_policy.started))fdBenchStop(FD_BENCH_WINDOW);
}
/* ACTUAL_ADAPTER */
static void beginHeld(uint32_t start,unsigned dir){
    fd_power_policy.started=start;fdRevBeginPolicy(&fd_rev,start);
    assert(fdRevAdvancePolicy(&fd_rev,start+30000u*160u)==FD_REV_STEP);
    assert(fdRevAdvancePolicy(&fd_rev,start+330000u*160u)==FD_REV_LAUNCH);
    /* The existing launch bridge/deadtime is not simulated here. Start at
     * its completed vector; tests below use the actual tick/event/stop code. */
    DWT->CYCCNT=start+330000u*160u;
    fd_power_bench.inhibit=0;fd_power_bench.active_id=1;fd_power_bench.floor=400;
    armed=running=adjusted_input=input=fd_rev_acquiring=1;
    forward=dir;step=dir?2:6;outputs=123;commutations=scheduled=last_reason=failure_pin=0;
    mask=stuck_rotor_inhibit=drive_safety_limit_inhibit=heli_coast_active=heli_coast_restart_inhibit=0;
}
int main(void){
    assert(FD_LAUNCH_HOLD_TOTAL_TICKS==361000u*160u);
    for(unsigned wrap=0;wrap<2;wrap++)for(unsigned dir=0;dir<2;dir++)for(unsigned event=0;event<3;event++){
        uint32_t start=wrap?UINT32_MAX-1000u:1000u;beginHeld(start,dir);
        uint32_t launch=fd_rev.sector_at;
        if(event){DWT->CYCCNT=launch+(event==1?900u:8000u)*160u;fdRevObserveEdge();}
        assert(fd_rev.stage==FD_REV_OPEN && !scheduled && !commutations && !fd_rev.confidence);
        assert(fd_rev.sector_at==launch && !fd_rev.reposition_used);
        DWT->CYCCNT=launch+FD_LAUNCH_HOLD_TICKS-1u;fdRevTick();
        assert(outputs==123 && !fd_power_bench.inhibit && !scheduled && !commutations);
        DWT->CYCCNT++;fdRevTick();
        assert(fd_power_bench.inhibit && !outputs && !running && !fd_rev_acquiring);
        assert(last_reason==FD_BENCH_LAUNCH_DONE && !failure_pin && !scheduled && !mask);
        fdBenchWritePwm(999);assert(!outputs && !commutations);
        assert(fdBenchGuardInput()); /* positive command cannot retrigger */
        adjusted_input=0;assert(!fdBenchGuardInput() && !heli_coast_restart_inhibit);
    }
    for(unsigned fault=0;fault<3;fault++){
        beginHeld(0,1);DWT->CYCCNT=331000u*160u;
        if(fault==0)stuck_rotor_inhibit=1;
        else if(fault==1)drive_safety_limit_inhibit=1;
        else DWT->CYCCNT=361000u*160u;
        scheduled=1;fdRevTick();
        assert(!outputs && !scheduled && !running && fd_power_bench.inhibit);
        assert(last_reason==(fault==0?FD_BENCH_NATIVE_STUCK:fault==1?FD_BENCH_SAFETY_CAP:FD_BENCH_LAUNCH_DONE));
    }
    /* Delayed service cannot renew whole-trial time, even in ALIGN. */
    fdRevBeginPolicy(&fd_rev,0);
    assert(!fdRevDeadlinePolicy(&fd_rev,361000u*160u-1,0));
    assert(fdRevDeadlinePolicy(&fd_rev,361000u*160u,0));
    puts("PASS actual launch-hold adapter: event cannot schedule/step,30ms stop,361ms cap, no retries/handoff, zero rearm, PWM gate, faults, both directions, wrap");
}
