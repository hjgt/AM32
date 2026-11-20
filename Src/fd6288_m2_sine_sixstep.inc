#include "fd6288_m2_sine_sixstep.h"

static FdM2SinePickup fd_m2_sine_pickup;
static FdM2SineLock fd_m2_sine_lock;
static volatile uint8_t fd_m2_sine_pickup_ready, fd_m2_sine_timed;
static uint8_t fd_m2_sine_pickup_requested;

#ifdef TARGET_FD_M2_SINE_ADAPTIVE
enum { FD_M2_SINE_CAPTURE_ABI = 2u };
static struct {
    uint32_t ready_at;
    uint8_t late_mask, front_samples, retried, missed;
} fd_m2_sine_sampling;
volatile uint8_t fd_m2_sine_closed_late;
#ifdef TARGET_FD_M2_PURE
static struct {uint32_t reference_period;uint8_t attempts;} fd_m2_sine_recovery;
#define fdM2SineClosedEvent(...) ((void)0)
#else
enum { FD_M2_SINE_RECOVERY_SAMPLES = 256u };
volatile struct {
    uint32_t magic, abi, trial, attempts, off_at, resumed_at;
    uint32_t reference_period, measured_period, first_sector, step, seen, count;
    FdM2SineCoastSample samples[FD_M2_SINE_RECOVERY_SAMPLES];
} fd_m2_sine_recovery;
_Static_assert(sizeof(fd_m2_sine_recovery) == 3120u, "M2 recovery recorder ABI");
enum { FD_M2_SINE_CLOSED_EVENTS = 32u };
volatile struct {
    uint32_t magic, abi, trial, count;
    struct {
        uint32_t at, elapsed;
        uint16_t duty, trigger;
        uint8_t step, kind;
        uint16_t reserved;
    } events[FD_M2_SINE_CLOSED_EVENTS];
} fd_m2_sine_closed;
_Static_assert(sizeof(fd_m2_sine_closed) == 528u, "M2 closed-loop recorder ABI");

/* 0 = actual bridge step, 1 = controller edge, 2 = stop before clearing PWM.
 * Keep the most recent events through a feedback failure, without ADC work. */
static void fdM2SineClosedEvent(uint8_t kind)
{
    if (!fd_power_bench.active_id || fd_native_sine.phase < 2u ||
        fd_native_sine.phase >= 4u) return;
    unsigned i = fd_m2_sine_closed.count % FD_M2_SINE_CLOSED_EVENTS;
    fd_m2_sine_closed.events[i].at = DWT->CYCCNT - fd_native_sine.started;
    fd_m2_sine_closed.events[i].elapsed = INTERVAL_TIMER_COUNT;
    fd_m2_sine_closed.events[i].duty = TIM1->CCR1;
    fd_m2_sine_closed.events[i].trigger = TIM1->CCR4;
    fd_m2_sine_closed.events[i].step = step;
    fd_m2_sine_closed.events[i].kind = kind;
    fd_m2_sine_closed.count++;
}
volatile struct {
    uint32_t magic, abi, trial, count;
    struct {
        uint32_t period, cycle_period, due;
        uint16_t trigger;
        uint8_t decision, front_samples;
    } sector[FD_M2_SINE_MAX_SECTORS];
} fd_m2_sine_adaptation = { .magic = 0x464d4131u, .abi = 1u };
_Static_assert(sizeof(fd_m2_sine_adaptation) == 400u, "M2 adaptation recorder ABI");
#endif
#else
enum { FD_M2_SINE_CAPTURE_ABI = 1u };
#endif

#ifdef TARGET_FD_M2_PURE
static struct {uint32_t sectors;} fd_m2_sine_handoff; /* Acquisition bound. */
#else
volatile struct {
    uint32_t magic, abi, trial, pickup_at, coast_period, closed_at, closed_period, sectors;
    struct { uint32_t at, edge, step, streak; } sector[FD_M2_SINE_MAX_SECTORS];
} fd_m2_sine_handoff = { .magic = 0x464d4831u, .abi = FD_M2_SINE_CAPTURE_ABI };
_Static_assert(sizeof(fd_m2_sine_handoff) == 32u + FD_M2_SINE_MAX_SECTORS * 16u, "M2 sine handoff recorder ABI");

enum { FD_M2_SINE_POWERED_SAMPLES = 24u };
typedef struct {
    uint32_t cycles;
    uint16_t a, b, c, flags;
} FdM2SinePoweredSample;

volatile struct {
    uint32_t magic, abi, trial;
    uint16_t counts[FD_M2_SINE_MAX_SECTORS];
    FdM2SinePoweredSample samples[FD_M2_SINE_MAX_SECTORS][FD_M2_SINE_POWERED_SAMPLES];
} fd_m2_sine_powered; /* BSS: avoid a Flash copy of the zero sample area. */
_Static_assert(sizeof(fd_m2_sine_powered) == 12u + FD_M2_SINE_MAX_SECTORS * 290u, "M2 sine powered recorder ABI");
static uint16_t fd_m2_sine_powered_seen;

#endif
#ifdef TARGET_FD_M2_SINE_STEP4_COAST
/* Keep the initial pickup recording and the post-pulse coast recording
 * together, each with its own actual off timestamp. */
volatile struct {
    uint32_t magic, abi, trial, off_at, sector_at, prior_edge, seen, count;
    FdM2SineCoastSample samples[FD_M2_SINE_COAST_SAMPLES];
} fd_m2_sine_step4_coast = { .magic = 0x464d3431u, .abi = 1u };
_Static_assert(sizeof(fd_m2_sine_step4_coast) == 6176u, "M2 step-4 coast recorder ABI");
static uint8_t fd_m2_sine_step4_probe;

static void fdM2SineStartStep4Coast(unsigned previous)
{
    allOff();
    fd_m2_sine_run.coast_at = DWT->CYCCNT;
    fd_m2_sine_run.coast = 1u;
    fd_m2_sine_timed = fd_m2_sine_pickup_requested = 0u;
    fd_m2_sine_step4_coast.off_at = fd_m2_sine_run.coast_at - fd_native_sine.started;
    fd_m2_sine_step4_coast.sector_at = fd_m2_sine_lock.sector_at - fd_native_sine.started;
    fd_m2_sine_step4_coast.prior_edge = fd_m2_sine_handoff.sector[previous].edge;
    SET_HELI_COAST_HIGH_Z_SAMPLE(FD_M2_SINE_PERIOD);
    generatePwmTimerEvent();
}

static void fdM2SineObserveStep4Coast(uint32_t elapsed)
{
    unsigned n = fd_m2_sine_step4_coast.count;
    uint32_t seen = fd_m2_sine_step4_coast.seen++;
    if (n >= FD_M2_SINE_COAST_SAMPLES ||
        (elapsed >= 1000u * FD_M2_SINE_CYCLES_PER_US && (seen & 1u))) return;
    fd_m2_sine_step4_coast.samples[n] = (FdM2SineCoastSample){
        .cycles = elapsed,
        .a = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1),
        .b = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2),
        .c = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3)
    };
    fd_m2_sine_step4_coast.count = n + 1u;
}
#endif

static void fdM2SineHandoffReset(uint32_t trial)
{
#ifdef TARGET_FD_M2_HELI_START
#ifdef TARGET_FD_M2_PURE
    fd_m2_heli=(FdM2HeliStart){0};
#else
    fd_m2_heli=(FdM2HeliStart){.magic=0x46485331u,.abi=1u,.trial=trial,
        .rise_ms=FD6288_M2_HELI_RISE_MS};
#endif
#endif
#ifdef TARGET_FD_M2_RUN_FIX
FD_M2_DIAG(
    fd_m2_wire.magic=0x464d5731u;fd_m2_wire.abi=1u;fd_m2_wire.trial=trial;
    fd_m2_wire.count=fd_m2_wire.last_at=fd_m2_wire.max_correction=fd_m2_wire.corrected_edges=0u;
    fd_m2_wire.origin=fd_native_sine.started;
);
#endif
#ifdef TARGET_FD_M2_SINE_ADC_CAPTURE
    fdM2SineAdcReset(trial, fd_native_sine.started);
#endif
    fd_m2_sine_pickup = (FdM2SinePickup){0};
    fd_m2_sine_lock = (FdM2SineLock){0};
    fd_m2_sine_timed = fd_m2_sine_pickup_ready = fd_m2_sine_pickup_requested = 0u;
FD_M2_DIAG(
    fd_m2_sine_handoff.trial = trial;
    fd_m2_sine_handoff.pickup_at = fd_m2_sine_handoff.closed_at = UINT32_MAX;
    fd_m2_sine_handoff.coast_period = fd_m2_sine_handoff.closed_period = 0u;
);
    fd_m2_sine_handoff.sectors = 0u;
FD_M2_DIAG(
    fd_m2_sine_powered.magic=0x464d5031u;
    fd_m2_sine_powered.abi=FD_M2_SINE_CAPTURE_ABI;
    fd_m2_sine_powered.trial = trial;
);
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    fd_m2_sine_sampling.late_mask = 1u << 3; /* Known C-phase settling aperture. */
    fd_m2_sine_sampling.front_samples = fd_m2_sine_sampling.retried = 0u;
    fd_m2_sine_sampling.missed = fd_m2_sine_closed_late = 0u;
    fd_m2_sine_sampling.ready_at = 0u;
FD_M2_DIAG(
    fd_m2_sine_recovery.magic = 0x464d5231u;
    fd_m2_sine_recovery.abi = 1u;
    fd_m2_sine_recovery.trial = trial;
);
    fd_m2_sine_recovery.attempts = 0u;
FD_M2_DIAG(
    fd_m2_sine_recovery.off_at = fd_m2_sine_recovery.resumed_at = UINT32_MAX;
);
    fd_m2_sine_recovery.reference_period = 0u;
    FD_M2_DIAG(fd_m2_sine_recovery.measured_period=0u);
    FD_M2_DIAG(
    fd_m2_sine_recovery.first_sector = fd_m2_sine_recovery.step = 0u;
    fd_m2_sine_recovery.seen = fd_m2_sine_recovery.count = 0u;
    fd_m2_sine_closed.magic = 0x464d4c31u;
    fd_m2_sine_closed.abi = 1u;
    fd_m2_sine_closed.trial = trial;
    fd_m2_sine_closed.count = 0u;
    fd_m2_sine_adaptation.trial = trial;
    fd_m2_sine_adaptation.count = 0u;
    );
#endif
FD_M2_DIAG(
    for (unsigned i = 0; i < FD_M2_SINE_MAX_SECTORS; i++)
        fd_m2_sine_powered.counts[i] = 0u;
);
#ifdef TARGET_FD_M2_SINE_STEP4_COAST
    fd_m2_sine_step4_probe = 0u;
    fd_m2_sine_step4_coast.trial = trial;
    fd_m2_sine_step4_coast.off_at = fd_m2_sine_step4_coast.sector_at =
        fd_m2_sine_step4_coast.prior_edge = UINT32_MAX;
    fd_m2_sine_step4_coast.seen = fd_m2_sine_step4_coast.count = 0u;
#endif
}

static void fdM2SineObservePickup(uint32_t elapsed, const FdM2SineCoastSample *sample)
{
    if (!fd_m2_sine_pickup_requested || fd_m2_sine_pickup_ready ||
        elapsed < FD_M2_PICKUP_SETTLE) return;
    const uint16_t phase[3] = {sample->a, sample->b, sample->c};
    uint32_t expected = (60u * FD_M2_SINE_DEGREE / fd_m2_sine_run.rate) *
        FD_M2_SINE_CYCLES_PER_US;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    if (fd_m2_sine_recovery.attempts) expected = fd_m2_sine_recovery.reference_period;
#endif
    fd_m2_sine_pickup_ready = fdM2SinePickupSample(&fd_m2_sine_pickup,
        fd_m2_sine_run.coast_at + elapsed, phase, expected, forward);
#ifdef TARGET_FD_M2_SINE_RUNUP
    if (fd_m2_sine_pickup_ready) {
        uint32_t period = fd_m2_sine_pickup.period;
        uint32_t minimum = FD_M2_SINE_RUNUP_MIN_SECTOR;
        uint32_t maximum = FD_M2_SINE_RUNUP_MAX_SECTOR;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
        if (fd_m2_sine_recovery.attempts) {
            minimum = expected - expected / 4u;
            maximum = expected + expected / 4u;
            FD_M2_DIAG(fd_m2_sine_recovery.measured_period = period);
        } else
#endif
            FD_M2_DIAG(fd_m2_sine_handoff.coast_period = period);
        if (period < minimum || period > maximum) {
            fd_m2_sine_pickup.intervals = 0u;
            fd_m2_sine_pickup_ready = 0u;
        }
    }
#endif
}

static void fdM2SineTimedSector(void)
{
    /* Discard the former sector's detector state. Only the timed observer
     * qualifies feedback until INTERVAL_TIMER is rebased for closed loop. */
    adcZcdResetAcquisition(ADC_ZCD_RESET_START);
    commutate();
    if (fd_power_bench.inhibit) return;
    fd_m2_sine_lock.sector_at = DWT->CYCCNT;
    fd_m2_sine_lock.observed = 0u;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    fd_m2_sine_lock.sector = step - 1u;
    fd_m2_sine_sampling.front_samples = fd_m2_sine_sampling.retried = 0u;
    fd_m2_sine_sampling.ready_at = fd_m2_sine_lock.sector_at + 2u * FD_M2_SINE_PERIOD;
#endif
    FD_M2_DIAG(fd_m2_sine_powered_seen = 0u);
    unsigned i = fd_m2_sine_handoff.sectors++;
    (void)i;
    FD_M2_DIAG(
    fd_m2_sine_handoff.sector[i].at = fd_m2_sine_lock.sector_at - fd_native_sine.started;
    fd_m2_sine_handoff.sector[i].edge = UINT32_MAX;
    fd_m2_sine_handoff.sector[i].step = step;
    fd_m2_sine_handoff.sector[i].streak = 0u;
    fd_native_sine.last_step = step;
    );
#ifdef TARGET_FD_M2_SINE_STEP4_COAST
    fd_m2_sine_step4_probe = forward && step == 4 && i > 0u &&
        fd_m2_sine_handoff.sector[i-1u].step == 3u &&
        fd_m2_sine_handoff.sector[i-1u].streak;
#endif
#ifdef TARGET_FD_M2_SINE_STEP4_LATE_ADC
    uint16_t trigger = zcd_ccr4_from_duty((uint16_t)TIM1->CCR1);
#ifdef TARGET_FD_M2_SINE_STEP4_COAST
    if (fd_m2_sine_step4_probe)
#elif defined(TARGET_FD_M2_SINE_ADAPTIVE)
    if (fd_m2_sine_sampling.late_mask & (1u << (step - 1u)))
#else
    if (forward && step == 4u)
#endif
        trigger = FD6288_M2_SINE_STEP4_ADC_TICKS;
    if (trigger < ZCD_SAMPLE_FLOOR || trigger + ZCD_ADC_WINDOW_TICKS > TIM1->CCR1) {
        fdBenchStop(FD_BENCH_BAD_CONFIG);
        return;
    }
    /* Load on the natural carrier update. Recompute on every timed
     * sector so the step-4 aperture ends at the next commutation. */
    TIM1->CCR4 = trigger;
#endif
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    FD_M2_DIAG(
    fd_m2_sine_adaptation.sector[i].period = fd_m2_sine_lock.period;
    fd_m2_sine_adaptation.sector[i].cycle_period = fdM2SineAdaptiveCyclePeriod(&fd_m2_sine_lock);
    fd_m2_sine_adaptation.sector[i].due = fd_m2_sine_lock.due - fd_native_sine.started;
    fd_m2_sine_adaptation.sector[i].trigger = TIM1->CCR4;
    fd_m2_sine_adaptation.sector[i].decision = 0u;
    fd_m2_sine_adaptation.sector[i].front_samples = 0u;
    fd_m2_sine_adaptation.count = i + 1u;
    );
#endif
}

static void fdM2SineStartSixstep(uint32_t now)
{
    uint32_t period = fd_m2_sine_pickup.period;
    uint32_t delay = period - period / 4u;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    /* A high-pair crossing identifies the start of a known sector.
     * Enter its successor at the next boundary with 1/8-sector advance. */
    delay = period - period / 8u;
#elif defined(TARGET_FD_M2_SINE_RUNUP)
    /* At 405 RPM, the first four powered crossings arrived at 12–16%
     * of a sector. Advance this boundary and the following timed train
     * by another 3/8 sector to place those crossings near the midpoint. */
    delay = period * 3u / 8u;
#endif
    uint32_t due = fd_m2_sine_pickup.stable_at + delay;
    if ((int32_t)(now - due) < 0) return;
    fd_m2_sine_pickup_ready = 0u;
    if (now - due > 250u * FD_M2_SINE_CYCLES_PER_US) {
        fdBenchStop(FD_BENCH_COMM_LATE);
        return;
    }
    if (driveEffectiveDutyLimit() < FD_M2_SINE_SIXSTEP_DUTY) {
        fdBenchStop(FD_BENCH_SAFETY_CAP);
        return;
    }
    allOff();
    fd_m2_sine_run.coast = 0u;
    fd_m2_sine_timed = 1u;
    uint16_t ticks = (uint32_t)FD_M2_SINE_SIXSTEP_DUTY * tim1_arr / 2000u + 1u;
    SET_DUTY_CYCLE_ALL(ticks);
    generatePwmTimerEvent();
    commutation_interval = period / 80u;
    fd_m2_sine_lock.period = period;
    fd_m2_sine_lock.due = due + period;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    fd_m2_sine_lock.reference_period = period;
    fd_m2_sine_lock.nominal_due = due + period;
    fd_m2_sine_lock.forward = forward;
#endif
    step = fd_m2_sine_pickup.stable;
    fdM2SineTimedSector();
    if (fd_power_bench.inhibit) return;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    if (fd_m2_sine_recovery.attempts) {
FD_M2_DIAG(
        fd_m2_sine_recovery.resumed_at = fd_m2_sine_lock.sector_at - fd_native_sine.started;
        fd_m2_sine_recovery.measured_period = period;
);
        return;
    }
#endif
FD_M2_DIAG(
    fd_m2_sine_handoff.pickup_at = fd_m2_sine_lock.sector_at - fd_native_sine.started;
    fd_m2_sine_handoff.coast_period = period;
    fd_native_sine.sixstep_at = fd_m2_sine_handoff.pickup_at;
    fd_native_sine.input_at_handoff = adjusted_input;
);
}

#ifdef TARGET_FD_M2_SINE_ADAPTIVE
static void fdM2SineRecover(void)
{
    /* Two missing physical crossings end blind powered commutation.
     * Permit one coast-only phase/speed reacquisition in this throttle episode.
     * Preserve the first coast and all powered samples for diagnosis. */
    if (fd_m2_sine_recovery.attempts) {
        fdBenchStop(FD_BENCH_NO_LOCK);
        return;
    }
    allOff();
    fdAsyncCancel();
    uint32_t now = DWT->CYCCNT;
    fd_m2_sine_recovery.attempts = 1u;
FD_M2_DIAG(
    fd_m2_sine_recovery.off_at = now - fd_native_sine.started;
);
    fd_m2_sine_recovery.reference_period = fd_m2_sine_lock.period;
FD_M2_DIAG(
    fd_m2_sine_recovery.first_sector = fd_m2_sine_handoff.sectors;
    fd_m2_sine_recovery.step = step;
);
    fd_m2_sine_run.coast_at = now;
    fd_m2_sine_run.coast = 1u;
    fd_m2_sine_timed = fd_m2_sine_pickup_ready = 0u;
    fd_m2_sine_pickup_requested = 1u;
    fd_m2_sine_pickup = (FdM2SinePickup){0};
    fd_m2_sine_lock = (FdM2SineLock){0};
    fd_m2_sine_sampling.missed = 0u;
    adcZcdResetAcquisition(ADC_ZCD_RESET_START);
    GPIOB->BRR = 1u << 7;
    SET_HELI_COAST_HIGH_Z_SAMPLE(FD_M2_SINE_PERIOD);
    generatePwmTimerEvent();
}

static uint8_t fdM2SineRetryLateSample(uint32_t now)
{
    if (fd_m2_sine_sampling.retried) return 0u;
    const uint16_t trigger = FD6288_M2_SINE_STEP4_ADC_TICKS;
    if (TIM1->CCR4 == trigger) {
        /* An already late aperture gains nothing from resetting a detector
         * that may have accumulated the only useful pre-crossing samples. */
        fd_m2_sine_sampling.retried = 1u;
        return 0u;
    }
    if (trigger + ZCD_ADC_WINDOW_TICKS > TIM1->CCR1) return 0u;
    fd_m2_sine_sampling.late_mask |= 1u << (step - 1u);
    fd_m2_sine_sampling.retried = 1u;
    fd_m2_sine_sampling.front_samples = 0u;
    fd_m2_sine_sampling.ready_at = now + 2u * FD_M2_SINE_PERIOD;
    adcZcdResetAcquisition(ADC_ZCD_RESET_START);
    TIM1->CCR4 = trigger;
    return 1u;
}

static uint8_t fdM2SineAdaptiveSample(uint32_t now, uint8_t edge,
    uint16_t a, uint16_t b, uint16_t c)
{
    unsigned i = fd_m2_sine_handoff.sectors - 1u;
    (void)i;
    uint32_t age = now - fd_m2_sine_lock.sector_at;
    uint16_t phases[3] = {a, b, c};
    const uint8_t high[6] = {0u, 2u, 2u, 1u, 1u, 0u};
    const uint8_t low[6] = {1u, 1u, 0u, 0u, 2u, 2u};
    const uint8_t floating[6] = {2u, 0u, 1u, 2u, 0u, 1u};
    unsigned sector = step - 1u;
    int32_t margin = (int32_t)phases[floating[sector]] - (int32_t)((a + b + c) / 3u);
    if (!rising) margin = -margin;
    uint8_t settled = (int32_t)(now - fd_m2_sine_sampling.ready_at) >= 0;
    if (settled && phases[high[sector]] >= phases[low[sector]] + 64u &&
        phases[floating[sector]] > 8u && phases[floating[sector]] < 4087u &&
        margin < -8 && fd_m2_sine_sampling.front_samples < 3u)
        fd_m2_sine_sampling.front_samples++;

    uint8_t decision = 0u, qualified = 0u;
    if (edge) {
        if (!settled || fd_m2_sine_sampling.front_samples < 2u) {
            decision = 5u;
            if (fdM2SineRetryLateSample(now)) decision = 3u;
        } else if (age < fd_m2_sine_lock.period / 8u &&
                   fdM2SineRetryLateSample(now)) {
            decision = 3u;
        } else {
            /* A settled crossing with two pre samples can teach phase
             * even when it precedes the lock window. */
            qualified = fdM2SineLockObserve(&fd_m2_sine_lock, now);
            decision = qualified ? 1u : 4u;
        }
    } else if (settled && !fd_m2_sine_lock.observed &&
               age >= fd_m2_sine_lock.period / 4u &&
               fd_m2_sine_sampling.front_samples < 2u &&
               fdM2SineRetryLateSample(now)) {
        decision = 3u;
    }
    (void)decision;
    FD_M2_DIAG(
    if (decision) fd_m2_sine_adaptation.sector[i].decision = decision;
    if (edge) {
        fd_m2_sine_adaptation.sector[i].period = fd_m2_sine_lock.period;
        fd_m2_sine_adaptation.sector[i].cycle_period = fdM2SineAdaptiveCyclePeriod(&fd_m2_sine_lock);
    }
    fd_m2_sine_adaptation.sector[i].due = fd_m2_sine_lock.due - fd_native_sine.started;
    fd_m2_sine_adaptation.sector[i].trigger = TIM1->CCR4;
    fd_m2_sine_adaptation.sector[i].front_samples = fd_m2_sine_sampling.front_samples;
    );
    return qualified;
}
#endif

static void fdM2SineObserveSixstep(void)
{
    uint32_t now = DWT->CYCCNT;
    uint8_t level;
    uint8_t edge = 0u;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    if ((int32_t)(now - fd_m2_sine_sampling.ready_at) < 0)
        LL_ADC_ClearFlag_JEOS(ADC1);
    else
#endif
        edge = getCompOutputSample(&level);
    uint16_t a = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1);
    uint16_t b = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2);
    uint16_t c = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3);
    unsigned i = fd_m2_sine_handoff.sectors - 1u;
    (void)i;
    uint8_t qualified = 0u;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    qualified = fdM2SineAdaptiveSample(now, edge, a, b, c);
#endif
    if (edge) {
        FD_M2_DIAG(fd_m2_sine_handoff.sector[i].edge = now - fd_native_sine.started);
#ifndef TARGET_FD_M2_SINE_ADAPTIVE
        qualified = fdM2SineLockObserve(&fd_m2_sine_lock, now);
#endif
#ifdef TARGET_FD_M2_SINE_STEP5_ADVANCE
        /* The measured step-5 event preceded its phase window by 247 us.
         * After a qualified step 4, advance the next boundary by 350 us.
         * Leave foreground service time before the new deadline. */
        if (qualified && forward && step == 4u &&
            (int32_t)(fd_m2_sine_lock.due - now) >
                (int32_t)((FD6288_M2_SINE_STEP5_ADVANCE_US + 250u) * FD_M2_SINE_CYCLES_PER_US))
            fd_m2_sine_lock.due -= FD6288_M2_SINE_STEP5_ADVANCE_US * FD_M2_SINE_CYCLES_PER_US;
#endif
        FD_M2_DIAG(fd_m2_sine_handoff.sector[i].streak = fd_m2_sine_lock.observed ? fd_m2_sine_lock.streak : 0u);
    }
    (void)qualified;
#ifndef TARGET_FD_M2_PURE
    uint16_t seen = fd_m2_sine_powered_seen++;
    unsigned n = fd_m2_sine_powered.counts[i];
    unsigned decimation_mask = 3u;
#if defined(TARGET_FD_M2_SINE_STEP4_COAST) && FD6288_M2_SINE_STEP4_HOLD_US > 500
    if (fd_m2_sine_step4_probe) decimation_mask = 1u;
#endif
    if (n >= FD_M2_SINE_POWERED_SAMPLES || (!edge && seen >= 8u && (seen & decimation_mask))) return;
    uint32_t age = now - fd_m2_sine_lock.sector_at;
    uint16_t flags = (uint16_t)(edge | (qualified << 1) | ((rising != 0) << 2));
#ifdef TARGET_FD_M2_SINE_STEP4_LATE_ADC
    /* Leave boundary groups unlabeled. CCR4 reads its preload, so
     * allow two complete carrier periods before assigning an aperture. */
    if (TIM1->CCR4 == FD6288_M2_SINE_STEP4_ADC_TICKS &&
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
        (int32_t)(now - fd_m2_sine_sampling.ready_at) >= 0 &&
#else
        forward && step == 4u &&
#endif
        seen >= 2u && age >= 2u * FD_M2_SINE_PERIOD)
        flags |= (uint16_t)(FD6288_M2_SINE_STEP4_ADC_TICKS << 3);
#endif
    /* The coherent ADC context stays empty until JEOS service returns. */
    fd_m2_sine_powered.samples[i][n] = (FdM2SinePoweredSample){
        .cycles = age,
        .a = a, .b = b, .c = c,
        .flags = flags
    };
    fd_m2_sine_powered.counts[i] = n + 1u;
#endif
}

static void fdM2SineCloseLoop(uint32_t now)
{
    uint32_t period = fd_m2_sine_lock.last_period / 80u;
    if (driveEffectiveDutyLimit() < FD_M2_SINE_SIXSTEP_DUTY) {
        fdBenchStop(FD_BENCH_SAFETY_CAP);
        return;
    }
    fd_m2_sine_timed = 0u;
    commutation_interval = average_interval = last_average_interval = period;
    thiszctime = lastzctime = period;
    for (unsigned i = 0; i < 6u; i++) commutation_intervals[i] = period;
    SET_INTERVAL_TIMER_COUNT((now - fd_m2_sine_lock.last_edge) / 80u);
    adcZcdSeedHandoff(period);
    zero_crosses = fd_m2_sine_lock.streak;
    old_routine = 1u;
    fd_power_bench.floor = FD_M2_SINE_SIXSTEP_DUTY;
    duty_cycle_setpoint = duty_cycle = last_duty_cycle = FD_M2_SINE_SIXSTEP_DUTY;
    adjusted_duty_cycle = TIM1->CCR1;
    running = 1u;
    stepper_sine = 0u;
    fd_native_sine.phase = 2u;
#ifdef TARGET_FD_M2_HELI_START
    fdM2HeliBegin(&fd_m2_heli,FD_M2_TRIAL_ID,DWT->CYCCNT,
        fd_m2_sine_lock.last_edge,fd_m2_sine_lock.last_period,step,FD_M2_SINE_SIXSTEP_DUTY);
#endif
#ifdef TARGET_FD_M2_SINE_ADC_CAPTURE
    fd_m2_sine_adc_enabled = 1u;
#endif
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    fd_m2_sine_closed_late = fd_m2_sine_sampling.late_mask;
#endif
#ifdef TARGET_FD_M2_SINE_STEP4_LATE_ADC
    TIM1->CCR4 = zcd_ccr4_from_duty((uint16_t)TIM1->CCR1);
#endif
    commutate();
    if (fd_power_bench.inhibit) return;
FD_M2_DIAG(
    fd_m2_sine_handoff.closed_at = DWT->CYCCNT - fd_native_sine.started;
    fd_m2_sine_handoff.closed_period = fd_m2_sine_lock.last_period;
);
}

static void fdM2SineTimedTick(uint32_t now)
{
#ifdef TARGET_FD_M2_SINE_STEP4_COAST
    if (fd_m2_sine_step4_probe) {
        if (now - fd_m2_sine_lock.sector_at >=
            FD6288_M2_SINE_STEP4_HOLD_US * FD_M2_SINE_CYCLES_PER_US)
            fdM2SineStartStep4Coast(fd_m2_sine_handoff.sectors - 2u);
        return;
    }
#endif
    if ((int32_t)(now - fd_m2_sine_lock.due) < 0) return;
    if (now - fd_m2_sine_lock.due > 250u * FD_M2_SINE_CYCLES_PER_US) {
        fdBenchStop(FD_BENCH_COMM_LATE);
    } else if (fdM2SineLockReady(&fd_m2_sine_lock, now)) {
        fdM2SineCloseLoop(now);
    } else if (fd_m2_sine_handoff.sectors >= FD_M2_SINE_MAX_SECTORS) {
        fdBenchStop(FD_BENCH_NO_LOCK);
    } else {
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
        if (fd_m2_sine_lock.observed) fd_m2_sine_sampling.missed = 0u;
        else if (++fd_m2_sine_sampling.missed >= 2u) {
            fdM2SineRecover();
            return;
        }
#endif
        if (!fd_m2_sine_lock.observed) fd_m2_sine_lock.streak = 0u;
        fdM2SineTimedSector();
        if (fd_power_bench.inhibit) return;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
        fd_m2_sine_lock.nominal_due += fd_m2_sine_lock.period;
        fd_m2_sine_lock.due = fd_m2_sine_lock.nominal_due;
FD_M2_DIAG(
        unsigned i = fd_m2_sine_handoff.sectors - 1u;
    (void)i;
        fd_m2_sine_adaptation.sector[i].due = fd_m2_sine_lock.due - fd_native_sine.started;
);
#else
        fd_m2_sine_lock.due = fd_m2_sine_lock.sector_at + fd_m2_sine_lock.period;
#endif
    }
}
