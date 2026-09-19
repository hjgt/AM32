/* This harness is assembled with the real firmware by run_fd6288_host_tests.cjs.
 * Synthetic samples are NOT a physical motor/power-stage model. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define USE_ADC_ZCD
#define TARGET_ADC_ZCD_GUARD
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
static struct { unsigned ARR; } com_timer;
#define COM_TIMER (&com_timer)
#define INTERVAL_TIMER_COUNT interval_timer
#define SET_INTERVAL_TIMER_COUNT(t) (interval_timer = (t))
static unsigned com_irq, pwm, gates, gate_writes, bailout_calls;
#define DISABLE_COM_TIMER_INT() (com_irq = 0)
#define SET_DUTY_CYCLE_ALL(t) (pwm = (t))
static unsigned __get_PRIMASK(void) { return primask; }
static void __disable_irq(void) { primask = 1; }
static void __enable_irq(void) { primask = 0; }
static void __set_PRIMASK(unsigned p) { primask = p; }
static unsigned LL_ADC_IsActiveFlag_JEOS(int a) { (void)a; return fresh; }
static void LL_ADC_ClearFlag_JEOS(int a) { (void)a; fresh = 0; }
static uint16_t LL_ADC_INJ_ReadConversionData12(int a, unsigned rank)
{ (void)a; reads++; return samples[rank]; }
static void allOff(void) { gates = 0; }
static void comStep(unsigned s) { gates = s; gate_writes++; }
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
static uint16_t input, adjusted_input, duty_cycle_setpoint, duty_cycle;
static uint16_t adjusted_duty_cycle, last_duty_cycle;
static uint32_t zero_crosses;
static uint8_t zcfound, bemfcounter, bad_count, bad_count_threshold = 6;
static unsigned forward = 1, desync_check, average_interval;
static unsigned drive_safety_limit_inhibit;
static unsigned commutation_intervals[6];
static unsigned thiszctime, advance, temp_advance = 16;
static unsigned auto_advance_level = 16, waitTime;
static struct { unsigned auto_advance, rc_car_reverse; } eepromBuffer;

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

int main(void)
{
    test_all_phases();
    test_confirmation();
    test_time_gate_and_resets();
    test_coast_unchanged();
    test_trace();
    test_native_stuck_latch();
    test_commutation_atomicity();
    printf("PASS: %u assertions against actual ADC/commutation/fault functions.\n", checks);
    puts("Not a motor model: load, current, timing margins and startup still require hardware validation.");
}
