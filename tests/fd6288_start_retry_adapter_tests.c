#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
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
#define TARGET_FD_START_RETRY
#define TARGET_FD_NATIVE_EARLY
#define TARGET_FD_FIRST_EDGE_500US
#ifndef FD6288_SECOND_ALIGN_MS
#define FD6288_SECOND_ALIGN_MS 300
#endif
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
static uint32_t average_interval,last_average_interval,desync_check,bad_count,bemfcounter,bemf_timeout_happened;
static uint32_t fd_reposition_diag[8];
#include "fd6288_start_retry_diag.h"
static volatile FdStartRetryDiagnostics fd_start_retry_diag;
/* ACTUAL_RETRY_RECORD */
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
static unsigned detector_timeouts;
static void adcZcdResetAcquisition(uint32_t v){if(v==ADC_ZCD_RESET_TIMEOUT)detector_timeouts++;scheduled=0;}
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
static void fdBenchFinish(uint8_t reason){fdStartRetryFinish(DWT->CYCCNT,reason);last_reason=reason;fd_power_bench.active_id=0;}
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



static void begin(uint32_t start, unsigned dir)
{
    DWT->CYCCNT = start;
    fd_power_policy.started = start;
    fdRevBeginPolicy(&fd_rev, start);
    fd_power_bench.inhibit = 0; fd_power_bench.active_id = 1; fd_power_bench.floor = 360;
    armed = running = fd_rev_acquiring = 1; adjusted_input = input = 215;
    forward = dir; step = dir ? 5 : 3;
    mask = stuck_rotor_inhibit = drive_safety_limit_inhibit = heli_coast_active = 0;
    heli_coast_restart_inhibit = prop_brake_active = stepper_sine = 0;
    outputs = 123; commutations = scheduled = last_reason = 0;
    bemf_timeout_happened = detector_timeouts = 0;
    fdStartRetryBegin(start);
}
static void position(void)
{
    unsigned before = commutations;
    DWT->CYCCNT = fd_rev.sector_at + 30000u*160u - 1;
    fdRevTick(); assert(commutations == before && fd_rev.align_left == 2);
    DWT->CYCCNT++; fdRevTick();
    assert(step == (forward ? 6u : 2u) && fd_rev.align_left == 1);
    DWT->CYCCNT = fd_rev.sector_at + FD_REV_SECOND_ALIGN_TICKS - 1;
    fdRevTick(); assert(fd_rev.stage == FD_REV_ALIGN);
    DWT->CYCCNT++; fdRevTick();
    assert(fd_rev.stage == FD_REV_OPEN && step == (forward ? 2u : 6u));
    assert(fd_rev.period == 30000u*160u && !fd_rev.confidence);
    assert(fd_start_retry_diag.rows[fd_rev.sr_attempts-1u].launch_at ==
        (uint32_t)(fd_rev.sector_at - fd_power_policy.started));
    outputs = 123;
}
static void edge(unsigned us)
{
    DWT->CYCCNT = fd_rev.sector_at + us*160u;
    fdRevObserveEdge();
    if (fd_rev.stage == FD_REV_PENDING) {
        DWT->CYCCNT = fd_rev.edge_at + fd_rev.delay;
        fdRevTick();
    }
}
static void fail_sector(void)
{
    unsigned before = commutations;
    DWT->CYCCNT = fd_rev.sector_at + fd_rev.period - 1;
    fdRevTick(); assert(commutations == before && fd_rev.stage == FD_REV_OPEN);
    DWT->CYCCNT++; fdRevTick();
    assert(!outputs && !scheduled && commutations == before);
}
static void restart(void)
{
    assert(fd_rev.stage == FD_REV_RELAX && !outputs && !fdAsyncAllowed());
    uint32_t off = fd_rev.sector_at;
    unsigned before = commutations, attempts = fd_rev.sr_attempts;
    fdBenchWritePwm(999); assert(!outputs);
    DWT->CYCCNT = off + FD_SR_RELAX_TICKS - 1;
    fdRevTick(); assert(fd_rev.stage == FD_REV_RELAX && !outputs);
    DWT->CYCCNT++; fdRevTick();
    assert(commutations == before + 1 && fd_rev.stage == FD_REV_ALIGN);
    assert(fd_rev.sr_attempts == attempts + 1 && fd_rev.sr_failures == attempts);
    assert(step == (forward ? 5u : 3u) && fd_rev.align_left == 2);
    assert(!fd_rev.confidence && !fd_rev.seen && !fd_rev.reposition_used);
    assert(!fd_rev.measured && !fd_rev.last_real_period && !zero_crosses);
    assert(fd_rev.sr_attempt_at == DWT->CYCCNT);
}
int main(void)
{
    if(FD6288_START_RETRIES == 5) {
        assert(FD_SR_MAX_ATTEMPTS == 6u);
        assert(FD_SR_ATTEMPT_TICKS == (FD6288_SECOND_ALIGN_MS==100?430000u:630000u)*160u);
        assert(FD_SR_TOTAL_TICKS == (FD6288_SECOND_ALIGN_MS==100?3080000u:4280000u)*160u);
        assert(sizeof(FdStartRetryDiagnostics) == 288u);
    } else {
        assert(FD_SR_MAX_ATTEMPTS == 3u);
        assert(FD_SR_TOTAL_TICKS == 2090000u*160u);
        assert(sizeof(FdStartRetryDiagnostics) == 168u);
    }
    for (unsigned wrap=0; wrap<2; wrap++) for (unsigned dir=0; dir<2; dir++) {
        uint32_t start = wrap ? UINT32_MAX-1000u : 1000u;
        /* Missing first or any subsequent startup edge gets full recovery,
         * not a blind commutation, rotated seed, or same-vector re-kick. */
        for (unsigned accepted=0; accepted<6; accepted++) {
            begin(start,dir);
            for (unsigned attempt=1; attempt<=FD_SR_MAX_ATTEMPTS; attempt++) {
                position();
                for (unsigned n=0;n<accepted;n++) edge(8000);
                fail_sector();
                assert(fd_rev.sr_failures == attempt);
                assert(bemf_timeout_happened == attempt && detector_timeouts == attempt);
                assert(fd_start_retry_diag.rows[attempt-1].qualified_edges == accepted);
                assert(fd_start_retry_diag.rows[attempt-1].failure_kind == FD_SR_FAIL_SECTOR);
                if (attempt<FD_SR_MAX_ATTEMPTS) restart();
            }
            assert(fd_power_bench.inhibit && !running && !outputs && !scheduled);
            assert(last_reason == FD_BENCH_NO_LOCK && fd_rev.sr_attempts == FD_SR_MAX_ATTEMPTS);
            fdBenchWritePwm(999); assert(!outputs && fdBenchGuardInput());
            unsigned before = commutations;
            DWT->CYCCNT += 2000000u*160u; fdRevTick();
            assert(commutations == before && fd_rev.sr_attempts == FD_SR_MAX_ATTEMPTS);
            adjusted_input=0; assert(!fdBenchGuardInput());
        }
        /* Six real feedback events on any attempt reach normal operation. */
        for (unsigned success=1; success<=FD_SR_MAX_ATTEMPTS; success++) {
            begin(start,dir);
            for(unsigned attempt=1; attempt<success; attempt++) {
                position(); edge(8000); fail_sector(); restart();
            }
            position();
            for(unsigned n=0;n<6;n++) edge(n ? 8000 : 500);
            assert(fd_rev.stage == FD_REV_CLOSED && !fd_rev_acquiring && zero_crosses == 6);
            assert(fd_start_retry_diag.successful_attempt == success);
            assert(fd_start_retry_diag.rows[success-1].qualified_edges == 6);
            assert(fd_rev.sr_failures == success-1 && bemf_timeout_happened == success-1);
            assert(!fdRevDeadlinePolicy(&fd_rev,start+FD_SR_TOTAL_TICKS,start));
            /* Runtime loss uses a stop, never startup re-entry. */
            unsigned attempts=fd_rev.sr_attempts;
            fdBenchStop(FD_BENCH_LOST_FEEDBACK);
            assert(!running && !outputs && !scheduled && fd_power_bench.inhibit);
            DWT->CYCCNT+=FD_SR_RELAX_TICKS; fdRevTick(); assert(fd_rev.sr_attempts==attempts);
        }
    }
    /* Very early qualified feedback is adopted, never fixed30ms forced dwell. */
    const unsigned early_us[]={1,339,500,1000,4000};
    for(unsigned i=0;i<5;i++) {
        begin(0,1); position(); uint32_t at=fd_rev.sector_at;
        DWT->CYCCNT=at+early_us[i]*160u; fdRevObserveEdge();
        assert(fd_rev.stage==FD_REV_PENDING && fd_rev.confidence==1);
        assert(fd_rev.delay==500u*160u && scheduled && !fd_rev.sr_failures);
        unsigned before=commutations;
        DWT->CYCCNT=fd_rev.edge_at+fd_rev.delay-1; fdRevTick(); assert(commutations==before);
        DWT->CYCCNT++; fdRevTick(); assert(commutations==before+1);
        assert(fd_rev.period==FD_REV_RETRY_TICKS && fd_rev.stage==FD_REV_OPEN);
    }
    /* Event at exactly a sector deadline cannot rescue an expired attempt. */
    begin(0,1); position();
    DWT->CYCCNT=fd_rev.sector_at+fd_rev.period; fdRevObserveEdge();
    assert(fd_rev.stage==FD_REV_RELAX && fd_rev.sr_failures==1 && !fd_rev.confidence && !outputs);
    /* Partial/pending handoff cannot renew per-attempt or global caps. */
    for(unsigned phase=0;phase<3;phase++) {
        begin(0,1); position();
        if(phase){fd_rev.stage=FD_REV_PENDING;fd_rev.confidence=phase==2?6:1;fd_rev.edge_at=FD_SR_ATTEMPT_TICKS-1;fd_rev.delay=500u*160u;scheduled=1;}
        DWT->CYCCNT=FD_SR_ATTEMPT_TICKS; fdRevTick();
        assert(fd_rev.stage==FD_REV_RELAX && !outputs && !scheduled);
        assert(fd_rev.sr_failure_kind==FD_SR_FAIL_ATTEMPT);
    }
    for(unsigned state=0;state<3;state++) {
        begin(0,1); position();
        if(state==1)fail_sector();
        if(state==2){fd_rev.stage=FD_REV_PENDING;fd_rev.confidence=6;scheduled=1;}
        DWT->CYCCNT=FD_SR_TOTAL_TICKS; fdRevTick();
        assert(fd_power_bench.inhibit && !outputs && !scheduled && !running);
    }
    /* Operator zero, disarm and native faults in alignment/open/pending/off wait. */
    for(unsigned state=0;state<4;state++) for(unsigned fault=0;fault<4;fault++) {
        begin(0,1);
        if(state)position();
        if(state==2){DWT->CYCCNT=fd_rev.sector_at+500u*160u;fdRevObserveEdge();}
        if(state==3)fail_sector();
        unsigned attempts=fd_rev.sr_attempts,before=commutations;
        if(fault==0)adjusted_input=0;
        if(fault==1)armed=0;
        if(fault==2)stuck_rotor_inhibit=1;
        if(fault==3)drive_safety_limit_inhibit=1;
        fdBenchService();
        assert(!fd_rev_acquiring && !fd_power_bench.active_id);
        DWT->CYCCNT+=FD_SR_RELAX_TICKS+500000u*160u;fdRevTick();
        assert(commutations==before && fd_rev.sr_attempts==attempts);
    }
    const char *record_path = getenv("FD_SR_TEST_RECORD");
    if (record_path) {
        FILE *f = fopen(record_path,"wb"); assert(f);
        for (unsigned which=0;which<3;which++) {
            /* First/last attempt success and all attempts failed. */
            unsigned success=which==0?1u:which==1?FD_SR_MAX_ATTEMPTS:FD_SR_MAX_ATTEMPTS+1u;
            begin(0,1);
            for(unsigned attempt=1;attempt<=FD_SR_MAX_ATTEMPTS;attempt++) {
                position();
                if(attempt==success) {
                    for(unsigned n=0;n<6;n++)edge(8000);
                    fdBenchFinish(FD_BENCH_OPERATOR_ZERO);
                    break;
                }
                fail_sector();if(attempt<FD_SR_MAX_ATTEMPTS)restart();
            }
            FdStartRetryDiagnostics record=fd_start_retry_diag;
            assert(fwrite(&record,sizeof(record),1,f)==1);
        }
        assert(fclose(f)==0);
    }
    puts("PASS ALIGN_RETRY actual policy/tick/event/PWM/stop: normal500us and six-edge handoff, no forced first step; profile-specific positioning after100ms off; bounded attempts/native timeout count; first/later-sector failures; per-attempt/whole-trial caps; held-throttle latch; faults/zero/disarm; directions/wrap");
}
