/* This harness is assembled with the real firmware by run_fd6288_host_tests.cjs.
 * Synthetic samples are NOT a physical motor/power-stage model. */
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define USE_ADC_ZCD
#define TARGET_ADC_ZCD_GUARD
#define TARGET_STARTUP_TRACE
#define TARGET_STUCK_ROTOR_LATCH
#define HELI_COAST_ON_ZERO
#define HELI_COAST_BAILOUT
#define DRIVE_SAFETY_LIMIT_PRIORITY
#define HELI_COAST_ADC_MIN_SPAN 64u
#define STMICRO
#define ADC1 0
#define LL_ADC_INJ_RANK_1 0
#define LL_ADC_INJ_RANK_2 1
#define LL_ADC_INJ_RANK_3 2
static uint16_t samples[3];
static uint8_t fresh;
static unsigned reads;
static unsigned primask;
static uint32_t interval_timer;
static unsigned simulate_zc_preemption;
static struct { uint32_t CYCCNT, CTRL; } fake_dwt;
static struct { uint32_t DEMCR; } fake_core;
static struct { uint32_t CCR1, CCR4; } fake_tim1;
#define DWT (&fake_dwt)
#define CoreDebug (&fake_core)
#define TIM1 (&fake_tim1)
#define CoreDebug_DEMCR_TRCENA_Msk 1u
#define DWT_CTRL_CYCCNTENA_Msk 1u
#define __DMB() ((void)0)
static struct { unsigned ARR; } com_timer;
#define COM_TIMER (&com_timer)
static uint32_t wait_cycle_increment;
static uint32_t readInterval(void) {
    fake_dwt.CYCCNT += wait_cycle_increment;
    return interval_timer;
}
#define INTERVAL_TIMER_COUNT readInterval()
#define SET_INTERVAL_TIMER_COUNT(t) (interval_timer = (t))
static unsigned com_irq, pwm, gates, gate_writes, bailout_calls;
#define DISABLE_COM_TIMER_INT() (com_irq = 0)
#define SET_DUTY_CYCLE_ALL(t) (pwm = (t))
#define SET_HELI_COAST_HIGH_Z_SAMPLE(t) ((void)(t), pwm = 0)
static unsigned __get_PRIMASK(void) { return primask; }
static void __disable_irq(void) {
    if (simulate_zc_preemption && !primask) {
        interval_timer = 0; // model an accepted ISR ZC just before critical entry
        simulate_zc_preemption = 0;
    }
    primask = 1;
}
static void __enable_irq(void) { primask = 0; }
static void __set_PRIMASK(unsigned p) { primask = p; }
static unsigned LL_ADC_IsActiveFlag_JEOS(int a) { (void)a; return fresh; }
static void LL_ADC_ClearFlag_JEOS(int a) { (void)a; fresh = 0; }
static uint16_t LL_ADC_INJ_ReadConversionData12(int a, unsigned rank)
{ (void)a; reads++; return samples[rank]; }
static void allOff(void) { gates = 0; }
#ifndef TARGET_STARTUP_POWER_BENCH
static void comStep(unsigned s) { gates = s; gate_writes++; }
#else
static void phaseAPWM(void) { gates |= 1u; gate_writes++; }
static void phaseBPWM(void) { gates |= 2u; gate_writes++; }
static void phaseCPWM(void) { gates |= 4u; gate_writes++; }
static void phaseALOW(void) { gates |= 8u; gate_writes++; }
static void phaseBLOW(void) { gates |= 16u; gate_writes++; }
static void phaseCLOW(void) { gates |= 32u; gate_writes++; }
static void phaseAFLOAT(void) { gates &= ~(1u | 8u); }
static void phaseBFLOAT(void) { gates &= ~(2u | 16u); }
static void phaseCFLOAT(void) { gates &= ~(4u | 32u); }
void comStep(int newStep);
void allpwm(void);
#endif
static uint8_t heliCoastBailoutAtBoundary(void) { bailout_calls++; return 0; }

volatile char rising;
char step, old_routine;
volatile uint32_t commutation_interval;
volatile uint32_t polling_mode_changeover = 2000;
volatile uint8_t heli_coast_active, heli_coast_bailout_requested;
volatile uint8_t heli_coast_restart_inhibit;
volatile uint16_t heli_coast_valid_crossings;
volatile uint8_t stuck_rotor_inhibit;
volatile uint32_t stuck_rotor_trip_count;
static uint8_t running, prop_brake_active, bemf_timeout_happened;
static uint8_t bemf_timeout = 10, stuck_enabled = 1;
#define EFFECTIVE_STUCK_ROTOR_PROTECTION stuck_enabled
#define EFFECTIVE_STALL_PROTECTION 0
static uint16_t input, adjusted_input, duty_cycle_setpoint, battery_voltage;
volatile uint16_t duty_cycle, waitTime, thiszctime, newinput;
static int16_t actual_current;
static uint16_t adjusted_duty_cycle, last_duty_cycle;
volatile uint32_t zero_crosses;
static uint8_t zcfound, bemfcounter, bad_count, bad_count_threshold = 6;
char forward = 1;
static unsigned desync_check, average_interval;
static unsigned drive_safety_limit_inhibit;
static unsigned commutation_intervals[6];
static unsigned advance, temp_advance = 16;
static unsigned auto_advance_level = 16;
static struct { unsigned auto_advance, rc_car_reverse, pwm_frequency,
    variable_pwm, comp_pwm, use_sine_start, bi_direction; } eepromBuffer;
#ifdef TARGET_STARTUP_POWER_BENCH
static uint16_t min_startup_duty = 360, minimum_duty_cycle = 210;
static uint16_t startup_max_duty_cycle = 610, tim1_arr = 6665;
static unsigned use_speed_control_loop, stepper_sine, armed, hard_cap = 2000;
static uint16_t ramp_count;
static uint8_t ramp_divider = 9, max_ramp_startup = 2;
static uint8_t max_ramp_low_rpm = 2, max_ramp_high_rpm = 2, max_duty_cycle_change;
static uint16_t driveEffectiveDutyLimit(void) { return hard_cap; }
#endif

/* FIRMWARE_FUNCTIONS */

static unsigned checks;
#define CHECK(x) do { checks++; assert(x); } while (0)

static void sector(unsigned s, unsigned edge, unsigned interval, unsigned coast)
{
    step = s;
    rising = edge;
    commutation_interval = interval;
    heli_coast_active = coast;
    bemfcounter = bad_count = 0;
    changeCompInput();
}

static void floating(unsigned value, unsigned elapsed)
{
    const unsigned high[] = {0, 0, 2, 2, 1, 1, 0};
    const unsigned low[] = {0, 1, 1, 0, 0, 2, 2};
    const unsigned open[] = {0, 2, 0, 1, 2, 0, 1};
    samples[high[(unsigned)step]] = 1600;
    samples[low[(unsigned)step]] = 0;
    samples[open[(unsigned)step]] = value;
    interval_timer = elapsed;
    fresh = 1;
    getBemfState();
}
static void pre(unsigned t) { floating(rising ? 400 : 1200, t); }
static void post(unsigned t) { floating(rising ? 1200 : 400, t); }
static void plateau(unsigned t) { floating(rising ? 1600 : 0, t); }

static void test_all_phases(void)
{
    for (unsigned s = 1; s <= 6; ++s) {
        for (unsigned edge = 0; edge <= 1; ++edge) {
            for (unsigned interval = 1500; interval <= 3000; interval += 1500) {
                adcZcdResetAcquisition(ADC_ZCD_RESET_START);
                sector(s, edge, interval, 0);
                unsigned read_before = reads;
                getBemfState();
                CHECK(reads == read_before); // no fresh sequence
                for (unsigned i = 0; i < 20; ++i) plateau(100 + 100 * i);
                CHECK(bemfcounter == 0); // constant post-cross level is not ZC
                pre(2200);
                post(2300);
                if (interval >= polling_mode_changeover) {
                    CHECK(bemfcounter == 0);
                    for (unsigned i = 0; i < 20; ++i) getBemfState();
                    CHECK(bemfcounter == 0); // duplicate polls cannot confirm
                    post(2400);
                }
                CHECK(bemfcounter == 1);
                post(2500);
                CHECK(bemfcounter == 1); // at most one event per sector

                sector(s, edge, interval, 0);
                plateau(100); // blanking must not arm the next sector
                plateau(200);
                plateau(300);
                CHECK(bemfcounter == 0);
            }
        }
    }
}

static void test_confirmation(void)
{
    adcZcdResetAcquisition(ADC_ZCD_RESET_START);
    sector(1, 1, 3000, 0);
    pre(10); // blank: must not establish observed pre polarity
    post(100); post(200);
    CHECK(bemfcounter == 0);
    pre(300); post(400);
    floating(800, 450); // Schmitt band cannot count as another post sample
    post(500);
    CHECK(bemfcounter == 0);
    samples[0] = samples[1] = samples[2] = 800;
    fresh = 1;
    getBemfState(); // invalid driven-phase difference breaks confirmation
    post(600);
    CHECK(bemfcounter == 0);
    post(700);
    CHECK(bemfcounter == 1);
}

/* Capture 2 (2026-09-20) never accepted a DRIVE crossing. Its retained tail
 * moved post -> hysteresis -> pre in sectors 4/5, opposite to the requested
 * edge. Exercise that SHAPE for every phase/polarity, not a motor simulation
 * or replay of omitted ADC samples. Do not "fix" acquisition by accepting
 * the wrong slope or a post-side plateau. A later proper edge must still work.
 */
static void test_failed_start_edge_shape(void)
{
    for (unsigned s = 1; s <= 6; ++s) {
        for (unsigned edge = 0; edge <= 1; ++edge) {
            adcZcdResetAcquisition(ADC_ZCD_RESET_START);
            sector(s, edge, 45000, 0);
            unsigned accepted = adc_zcd_diag.accepted;
            post(100); // the first fresh sequence is blanked
            post(200); post(300); // post side without an observed pre side
            CHECK(bemfcounter == 0 && !adc_zcd_before_seen);
            floating(800, 400); // neutral / Schmitt band
            pre(500); pre(600); // reverse slope only arms a future edge
            CHECK(bemfcounter == 0 && adc_zcd_before_seen);
            CHECK(adc_zcd_diag.accepted == accepted);
            post(700); // genuine forward slope, first confirmation
            CHECK(bemfcounter == 0);
            post(800);
            CHECK(bemfcounter == 1);
            CHECK(adc_zcd_diag.accepted == accepted + 1);
        }
    }
}

static void acquire(void)
{
    adcZcdResetAcquisition(ADC_ZCD_RESET_START);
    for (unsigned s = 1; s <= 5; ++s) {
        sector(s, s & 1u, 1500, 0);
        pre(10); pre(20); post(1500);
        CHECK(bemfcounter == 1); // no trusted-period gate during acquisition
    }
    CHECK(adc_zcd_drive_crossings == 5);
}

static void test_time_gate_and_resets(void)
{
    acquire();
    sector(6, 0, 1500, 0);
    CHECK(adc_zcd_earliest == 750);
    pre(10); pre(20); post(750);
    CHECK(bemfcounter == 0); // strictly greater, matching native G431
    post(800); post(850);
    CHECK(bemfcounter == 0); // an early plateau cannot become a later edge
    pre(900); post(1000);
    CHECK(bemfcounter == 1);
    /* An artificial 5 ms startup seed must not mask a real earlier ZC. */
    sector(1, 1, 10000, 0);
    CHECK(adc_zcd_earliest == 500); // preceding accepted elapsed was 1000
    pre(10); pre(20); post(600); post(700);
    CHECK(bemfcounter == 1);
    for (unsigned why = 0; why < ADC_ZCD_RESET_COUNT; ++why) {
        adc_zcd_drive_crossings = 5;
        adcZcdResetAcquisition(why);
        sector(1, 1, 1500, 0);
        CHECK(adc_zcd_earliest == 0);
        CHECK(adc_zcd_drive_crossings == 0);
        CHECK(adc_zcd_diag.resets[why] > 0);
    }
    CHECK(adc_zcd_diag.trace_frozen == 1);
}

static void test_coast_unchanged(void)
{
    for (unsigned s = 1; s <= 6; ++s) {
        for (unsigned edge = 0; edge <= 1; ++edge) {
            adc_zcd_drive_crossings = 5;
            sector(s, edge, 1500, 1);
            plateau(10); // original one-sample blank remains
            CHECK(bemfcounter == 0);
            plateau(20);
            CHECK(bemfcounter == 1); // original Coast level semantics
            CHECK(adc_zcd_drive_crossings == 0);
            sector(s, edge, 1500, 0); // bailout must reacquire DRIVE
            plateau(10); plateau(20);
            CHECK(bemfcounter == 0);
        }
    }
    sector(1, 1, 1500, 1);
    samples[0] = samples[1] = samples[2] = 800;
    fresh = 1;
    getBemfState();
    CHECK(bemfcounter == 0); // Coast still requires a meaningful phase span
}

static void test_trace(void)
{
    adc_zcd_diag.trace_frozen = 0;
    for (unsigned i = 0; i < ADC_ZCD_TRACE_LENGTH + 5; ++i) {
        adcZcdResetAcquisition(ADC_ZCD_RESET_START);
        sector(1, 1, 1500, 0);
        pre(10); pre(20); post(30);
    }
    unsigned n = adc_zcd_diag.trace_sequence;
    CHECK(adc_zcd_trace[(n - 1) % ADC_ZCD_TRACE_LENGTH].sequence == n);
    CHECK(adc_zcd_trace[(n - 1) % ADC_ZCD_TRACE_LENGTH].elapsed == 30);
    adcZcdResetAcquisition(ADC_ZCD_RESET_FAULT);
    sector(1, 1, 1500, 0);
    pre(10); pre(20); post(30);
    CHECK(adc_zcd_diag.trace_sequence == n);
}

static void test_native_stuck_latch(void)
{
    stuck_rotor_inhibit = 0;
    stuck_enabled = 1;
    adjusted_input = input = 1000;
    bemf_timeout_happened = bemf_timeout;
    CHECK(stuckRotorGuardInput() == 0); // preserve native > threshold
    bemf_timeout_happened++;
    stuck_enabled = 0;
    CHECK(stuckRotorGuardInput() == 0); // preserve native Off switch
    stuck_enabled = 1;
    running = gates = pwm = com_irq = 1;
    heli_coast_active = heli_coast_bailout_requested = 1;
    duty_cycle_setpoint = duty_cycle = last_duty_cycle = 1000;
    adjusted_duty_cycle = 3000;
    primask = 0;
    CHECK(stuckRotorGuardInput() == 1);
    CHECK(primask == 0);
    CHECK(stuck_rotor_inhibit == 1 && stuck_rotor_trip_count == 1);
    CHECK(!running && !gates && !pwm && !com_irq);
    CHECK(!input && !duty_cycle_setpoint && !last_duty_cycle);
    CHECK(!heli_coast_active && !heli_coast_bailout_requested);
    CHECK(heli_coast_restart_inhibit == 1);
    CHECK(adc_zcd_diag.trace_frozen == 1);
    unsigned writes = gate_writes, calls = bailout_calls;
    commutate(); // pending callback must not restart bridge or bailout
    zcfoundroutine();
    CHECK(gate_writes == writes && bailout_calls == calls);
    bemf_timeout_happened = 0; // counter clearing must not release the latch
    stuck_enabled = 0; // nor may toggling the EEPROM switch at positive input
    CHECK(stuckRotorGuardInput() == 1);
    CHECK(stuck_rotor_trip_count == 1);
    CHECK(!gates && !pwm);
    adjusted_input = 0; // actual operator zero, not the internally forced input
    CHECK(stuckRotorGuardInput() == 0);
    CHECK(stuck_rotor_inhibit == 0 && bemf_timeout_happened == 0);
    primask = 1;
    stuckRotorStop();
    CHECK(primask == 1); // nested/IRQ caller mask is preserved
    primask = 0;
}

static void test_commutation_atomicity(void)
{
    stuck_rotor_inhibit = 0;
    heli_coast_active = prop_brake_active = drive_safety_limit_inhibit = 0;
    for (unsigned mask = 0; mask <= 1; ++mask) {
        for (forward = 0; forward <= 1; ++forward) {
            step = forward ? 6 : 1;
            for (unsigned i = 0; i < 6; ++i) {
                primask = mask;
                unsigned previous = step;
                commutate();
                unsigned next = forward ? previous % 6 + 1 : (previous + 4) % 6 + 1;
                CHECK((unsigned)step == next);
                CHECK(adc_zcd_phase == next && gates == next);
                CHECK((unsigned)rising == (forward ? next % 2 : !(next % 2)));
                CHECK(primask == mask);
                CHECK(!adc_zcd_before_seen && !adc_zcd_reported);
            }
        }
    }
    primask = 0;
    forward = 1;
}

static void trace_rearm(void)
{
    input = newinput = adjusted_input = running = heli_coast_active = 0;
    fd_startup_trace.command = FD_TRACE_ARM_COMMAND;
    fdTraceControlTick();
    CHECK(fd_startup_trace.state == FD_TRACE_ARMED);
    CHECK(fd_startup_trace.sample_sequence == 0);
    CHECK(fd_startup_trace.event_sequence == 0);
}

static void test_startup_logging(void)
{
    memset((void *)&fd_startup_trace, 0, sizeof(fd_startup_trace));
    fdTraceInit();
    CHECK(sizeof(fd_startup_trace) == 23656u);
    CHECK(fd_startup_trace.magic == FD_TRACE_MAGIC);
    CHECK(fd_startup_trace.variant == FD6288_STARTUP_TIMING_FIX);
    trace_rearm();
    adjusted_input = input = newinput = 200;
    DWT->CYCCNT = UINT32_MAX - 1000u;
    fdTraceBegin();
    CHECK(fd_startup_trace.state == FD_TRACE_ACTIVE);
    fd_startup_trace.command = FD_TRACE_ARM_COMMAND;
    fdTraceControlTick();
    CHECK(fd_startup_trace.state == FD_TRACE_ACTIVE);
    CHECK(fd_startup_trace.rejected_commands == 1);
    for (unsigned i = 0; i < FD_TRACE_SAMPLE_RING + 100u; i++) {
        DWT->CYCCNT += 100u;
        fdTraceSample(100, 200, 300, -17, FD_SAMPLE_PRE, 1, 0);
    }
    CHECK(fd_startup_trace.sample_first[0].sequence == 1);
    CHECK(fd_startup_trace.sample_first[63].sequence == 64);
    CHECK(fd_startup_trace.sample_ring[99].sequence == 612);
    CHECK(fd_startup_trace.decisions[FD_SAMPLE_PRE] == 612);
    fdTraceNoFresh();
    CHECK(fd_startup_trace.no_fresh == 1);
    thiszctime = 1234; interval_timer = 0;
    fdTraceEvent(FD_EVENT_ZC, 1);
    CHECK(fd_startup_trace.event_first[1].elapsed == 1234);
    CHECK(fd_startup_trace.event_first[1].aux == 1);
    adjusted_input = 0;
    fdTraceControlTick();
    CHECK(fd_startup_trace.state == FD_TRACE_FROZEN);
    CHECK(fd_startup_trace.end_reason == FD_END_ZERO);
    fdTraceSample(0, 0, 0, 0, 0, 0, 0);
    CHECK(fd_startup_trace.sample_sequence == 612);
    trace_rearm();
    adjusted_input = 200;
    fdTraceBegin();
    running = gates = pwm = 1;
    DWT->CYCCNT += 160000000u;
    fdTraceControlTick();
    CHECK(fd_startup_trace.end_reason == FD_END_DEADLINE);
    CHECK(running == 1 && gates == 1 && pwm == 1); // capture cap is NOT a motor timeout
    trace_rearm();
    adjusted_input = 200;
    fdTraceBegin();
    for (unsigned i = 0; i < FD_TRACE_EVENT_RING + 8; i++) fdTraceEvent(FD_EVENT_COMMUTATE, i);
    CHECK(fd_startup_trace.event_first[0].kind == FD_EVENT_START);
    CHECK(fd_startup_trace.event_ring[8].sequence == 137);
    primask = 1;
    adcZcdResetAcquisition(ADC_ZCD_RESET_FAULT);
    CHECK(primask == 1);
    CHECK(fd_startup_trace.end_reason == FD_END_FAULT);
    CHECK(fd_startup_trace.state == FD_TRACE_FROZEN);
    primask = 0;
    trace_rearm();
    adjusted_input = 200;
    fdTraceBegin();
    fd_startup_trace.command = FD_TRACE_FREEZE_COMMAND;
    fdTraceControlTick();
    CHECK(fd_startup_trace.end_reason == FD_END_REQUEST);
}

static void test_period_and_forced_steps(void)
{
    stuck_rotor_inhibit = 0;
    heli_coast_active = heli_coast_restart_inhibit = prop_brake_active = 0;
    running = 1; input = adjusted_input = 300; bemf_timeout_happened = 0;
    zero_crosses = 0; commutation_interval = 10000; interval_timer = 1000;
    adc_zcd_drive_crossings = 1;
    zcfoundroutine();
#if FD6288_STARTUP_TIMING_FIX
    CHECK(commutation_interval == 10000); // first edge cannot establish a period
#else
    CHECK(commutation_interval == 7750); // A retains baseline startup IIR
#endif
    adc_zcd_drive_crossings = 2; interval_timer = 1000;
    zcfoundroutine();
#if FD6288_STARTUP_TIMING_FIX
    CHECK(commutation_interval == 1000);
#else
    CHECK(commutation_interval == 6062);
#endif
    adc_zcd_drive_crossings = 3; interval_timer = 1000;
    zcfoundroutine();
#if FD6288_STARTUP_TIMING_FIX
    CHECK(commutation_interval == 1000);
    CHECK(adcZcdDrivePeriod(0, 1000) == 1000);
    unsigned writes = gate_writes;
    interval_timer = 45001; zero_crosses = 20;
    fd6288TimeoutStep();
    CHECK(commutation_interval == 1000 && zero_crosses == 0);
    CHECK(interval_timer == 0 && bemf_timeout_happened == 1);
    CHECK(gate_writes == writes + 1 && adc_zcd_drive_crossings == 0);
    CHECK(primask == 0);
    interval_timer = 45000;
    fd6288TimeoutStep();
    CHECK(gate_writes == writes + 1);
    interval_timer = 45001; simulate_zc_preemption = 1;
    fd6288TimeoutStep();
    CHECK(gate_writes == writes + 1 && bemf_timeout_happened == 1);
    CHECK(primask == 0 && interval_timer == 0); // recheck rejects stale timeout
    interval_timer = 45001; primask = 1;
    fd6288TimeoutStep();
    CHECK(primask == 1 && bemf_timeout_happened == 2);
    primask = 0;
    stuck_enabled = 1; bemf_timeout = 10;
    for (unsigned i = 2; i < 10; i++) { interval_timer = 45001; fd6288TimeoutStep(); }
    CHECK(bemf_timeout_happened == 10 && stuckRotorGuardInput() == 0);
    interval_timer = 45001; fd6288TimeoutStep();
    CHECK(stuckRotorGuardInput() == 1 && !running && !gates && !pwm);
    adjusted_input = 0; CHECK(stuckRotorGuardInput() == 0);
    running = 1; input = adjusted_input = 300; heli_coast_active = 1;
    interval_timer = 45001;
    fd6288TimeoutStep();
    CHECK(!heli_coast_active && !running && !gates);
    CHECK(heli_coast_restart_inhibit == 1);
#else
    CHECK(commutation_interval == 4796);
#endif
    heli_coast_active = 1; zero_crosses = 0; commutation_interval = 1000;
    interval_timer = 2000; adc_zcd_drive_crossings = 0;
    zcfoundroutine();
    CHECK(commutation_interval == 1250); // original Coast IIR in both variants
}

/* POWER_BENCH_TESTS */

int main(void)
{
#ifdef TARGET_STARTUP_POWER_BENCH
    test_power_adapter();
#else
    test_all_phases();
    test_confirmation();
    test_failed_start_edge_shape();
    test_time_gate_and_resets();
    test_coast_unchanged();
    test_trace();
    test_native_stuck_latch();
    test_commutation_atomicity();
    test_startup_logging();
    test_period_and_forced_steps();
#endif
    printf("PASS: %u assertions against actual ADC/commutation/fault functions.\n", checks);
    puts("Not a motor model: load, current, timing margins and startup still require hardware validation.");
}
