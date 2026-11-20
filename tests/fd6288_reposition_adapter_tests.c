#ifdef TARGET_FD_FULL_RETRY
#include "fd6288_full_retry_tests.c"
#define policyTests fullRetryPolicyTests
#else
#define main policyTests
#include "fd6288_reposition_tests.c"
#undef main
#endif
static struct { uint32_t CYCCNT; } dwt;
#define DWT (&dwt)
static struct { uint32_t inhibit,active_id; } fd_power_bench;
static struct { uint32_t started; } fd_power_policy;
static struct { uint32_t frozen; } fd_align_observe;
static FdRevPolicy fd_rev;
static uint32_t fd_reposition_diag[8], mask, outputs, off_calls, commutations;
static uint32_t running,adjusted_input,fd_rev_acquiring,forward,step;
static uint32_t zero_crosses,commutation_interval,thiszctime,commutation_intervals[6];
static uint32_t average_interval,last_average_interval,desync_check,bad_count;
static uint32_t duty_cycle_setpoint,duty_cycle,adjusted_duty_cycle,last_duty_cycle;
static uint32_t stuck_rotor_inhibit,drive_safety_limit_inhibit,heli_coast_active,bemf_timeout_happened;
static uint32_t __get_PRIMASK(void){return mask;}
static void __disable_irq(void){mask=1;}
static void __set_PRIMASK(uint32_t v){mask=v;}
#define ADC_ZCD_RESET_COUNT 0
#define ADC_ZCD_RESET_TIMEOUT 1
#define SET_DUTY_CYCLE_ALL(v) (outputs=(v))
#define SET_INTERVAL_TIMER_COUNT(v) ((void)(v))
#define SET_HELI_COAST_HIGH_Z_SAMPLE(v) (outputs=0)
#define fdTraceEvent(...) ((void)0)
#define fdLightHandoff(...) ((void)0)
#define fdLightRetry(...) ((void)0)
static void adcZcdSeedHandoff(uint32_t v){(void)v;}
static void adcZcdResetAcquisition(uint32_t v){(void)v;}
static unsigned stuckRotorGuardInput(void){return stuck_rotor_inhibit;}
static void allOff(void){off_calls++;outputs=0;}
static void commutate(void){step=forward?(step==6?1:step+1):(step==1?6:step-1);commutations++;}
static void fdBenchService(void){
    if(!adjusted_input) {fd_rev_acquiring=0;fd_power_bench.active_id=0;}
    if(stuck_rotor_inhibit || drive_safety_limit_inhibit ||
       fdRevDeadlinePolicy(&fd_rev,DWT->CYCCNT,fd_power_policy.started))fd_power_bench.inhibit=1;
}
/* ACTUAL_ADAPTER */
static void resetTest(void){
    dwt.CYCCNT=0; fd_power_policy.started=0;
    fd_power_bench.inhibit=0;fd_power_bench.active_id=1;
    running=adjusted_input=fd_rev_acquiring=1;
    stuck_rotor_inhibit=drive_safety_limit_inhibit=heli_coast_active=0;
    mask=off_calls=commutations=0; outputs=123;
    fdRevBeginPolicy(&fd_rev,0);align(&fd_rev);
}
static void syntheticEligible(void){
    /* Actual current schedule cannot reach this state at150ms. Inject it
     * only for safety-adapter coverage of the now-dormant fallback branch. */
#ifdef TARGET_FD_FULL_RETRY
    DWT->CYCCNT=391500u*FD_REV_US;
#else
    DWT->CYCCNT=150000u*FD_REV_US;
#endif
    fd_rev.rescue_used=1;fd_rev.rescue_left=0;
    fd_rev.period=FD_REV_RETRY_TICKS;
    fd_rev.sector_at=DWT->CYCCNT-fd_rev.period;
}
int main(void){
    policyTests();
    for(unsigned dir=0;dir<2;dir++) {
        resetTest();forward=dir;
        syntheticEligible();
        fdRevTick();assert(fd_rev.stage==FD_REV_RELAX && off_calls==1 && !outputs && !commutations && !mask);
        duty_cycle=duty_cycle_setpoint=adjusted_duty_cycle=last_duty_cycle=999;
        fdBenchWritePwm(999);assert(!outputs && !duty_cycle && !last_duty_cycle && !mask);
        DWT->CYCCNT+=20000u*FD_REV_US-1;fdRevTick();assert(!commutations);
        DWT->CYCCNT++;fdRevTick();assert(commutations==1 && step==(dir?1u:5u));
        assert(fd_rev.stage==FD_REV_ALIGN && fd_power_policy.started==0);
    }
    for(unsigned stop=0;stop<4;stop++) {
        resetTest();syntheticEligible();fdRevTick();
        DWT->CYCCNT+=20000u*FD_REV_US;
        if(stop==0)adjusted_input=0;
        if(stop==1)stuck_rotor_inhibit=1;
        if(stop==2)drive_safety_limit_inhibit=1;
        if(stop==3)DWT->CYCCNT=
#ifdef TARGET_FD_FULL_RETRY
            FD_FULL_RETRY_END;
#else
            FD_REV_ACQUIRE_TICKS;
#endif
        fdRevTick();assert(!commutations && !outputs && !mask);
    }
    puts("PASS actual restart adapter: release/PWM gating, reverse/forward realignment, zero/fault/deadline cancellation (mock service/peripherals)");
}
