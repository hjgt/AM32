#include "fd6288_startup_trace.h"
#ifdef TARGET_STARTUP_WAVE_TRACE
#include <stddef.h>
#include "common.h"
#include "peripherals.h"

extern char step, old_routine;
extern volatile char rising;
extern volatile uint16_t duty_cycle, waitTime;
extern volatile uint32_t commutation_interval;
extern volatile uint8_t heli_coast_active;
_Static_assert(sizeof(FdWaveSample) == 20, "wave sample ABI");
_Static_assert(sizeof(FdWaveEvent) == 20, "wave event ABI");
_Static_assert(offsetof(FdWaveBank, samples) == 128, "wave bank ABI");
_Static_assert(sizeof(FdWaveBank) == 7168, "wave bank size");
_Static_assert(offsetof(FdStartupWave, banks) == 64, "wave header ABI");
_Static_assert(sizeof(FdStartupWave) == 21568, "wave RAM budget");
volatile FdStartupWave fd_startup_wave;
static uint8_t fd_wave_begun, fd_wave_stage;
#ifdef TARGET_STARTUP_LOSS_LOG
static uint8_t fd_wave_sample_frozen;
#endif
static inline volatile FdWaveBank *fdWaveActive(void)
{
    if (fd_startup_wave.active_bank >= 3u || !fd_wave_begun) return 0;
    return &fd_startup_wave.banks[fd_startup_wave.active_bank];
}
static inline uint8_t fdWaveFlags(void)
{
    return (forward ? 1u : 0u) | (rising ? 2u : 0u) | (running ? 4u : 0u) |
        (heli_coast_active ? 8u : 0u) | (old_routine ? 16u : 0u);
}
static inline __attribute__((always_inline)) void fdWaveEventStore(
    volatile FdWaveBank *b, uint32_t at, uint8_t kind,
    uint16_t a, uint16_t c, uint16_t d)
{
    volatile FdWaveEvent *e = &b->events_ring[b->events % FD_WAVE_EVENTS];
    e->at = at; e->elapsed = INTERVAL_TIMER_COUNT;
    e->interval = commutation_interval;
    e->values[0] = a; e->values[1] = c; e->values[2] = d;
    e->duty = duty_cycle; e->step = step; e->flags = fdWaveFlags();
    e->kind = kind; e->stage = fd_wave_stage;
    b->events++;
}
void fdTraceInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    fd_startup_wave.magic = FD_WAVE_MAGIC; fd_startup_wave.abi = 1u;
    fd_startup_wave.variant = FD6288_TRACE_VARIANT;
    fd_startup_wave.clock_hz = 160000000u;
    fd_startup_wave.size = sizeof(FdStartupWave);
#ifdef TARGET_STARTUP_NOEDGE_LOG
    fd_startup_wave.reserved[2] = 192000u; /* >=1.2ms before first acceptance */
#endif
#ifdef TARGET_STARTUP_ACQ_LOG
    fd_startup_wave.reserved[0] = FD_WAVE_SAMPLE_TICKS;
#endif
#ifdef TARGET_STARTUP_EDGE_LOG
    fd_startup_wave.reserved[0] = FD_WAVE_SAMPLE_TICKS;
    fd_startup_wave.reserved[1] = FD_WAVE_EDGE_POST_TICKS;
#ifdef TARGET_STARTUP_LOSS_LOG
    fd_startup_wave.reserved[1] = 0u; /* event-triggered, not a timed window */
#endif
#endif
    fd_startup_wave.active_bank = fd_startup_wave.no_lock_bank =
        fd_startup_wave.lost_bank = fd_startup_wave.success_bank = FD_WAVE_NONE;
}
void fdTraceTrial(uint32_t id)
{
    /* Existing bench critical section; clear header only, never the arrays. */
    fd_startup_wave.trials_seen = id;
    fd_startup_wave.active_bank = FD_WAVE_NONE; fd_wave_begun = 0u;
#ifdef TARGET_STARTUP_LOSS_LOG
    fd_wave_sample_frozen = 0u;
#endif
    for (uint32_t i = 0; i < 3u; i++) {
        if (i == fd_startup_wave.no_lock_bank || i == fd_startup_wave.lost_bank ||
            i == fd_startup_wave.success_bank) continue;
        volatile FdWaveBank *b = &fd_startup_wave.banks[i];
        b->state = FD_WAVE_ACTIVE; b->trial_id = id;
        b->started = b->ended = b->reason = b->final_stage = 0u;
        b->handoff_at = FD_WAVE_NONE;
        b->samples_seen = b->samples_stored = b->events = b->no_fresh = 0u;
        b->accepted = b->closed_zc = b->timeouts = 0u;
        b->max_sample_cycles = b->max_event_cycles = b->last_sample_at = 0u;
        for (unsigned n = 0; n < 10u; n++) b->decisions[n] = 0u;
        for (unsigned n = 0; n < 5u; n++) b->reserved[n] = 0u;
#ifdef TARGET_STARTUP_EDGE_LOG
        b->reserved[0] = FD_WAVE_NONE; /* first accepted ADC timestamp */
#endif
        fd_wave_stage = 2u; fd_startup_wave.active_bank = i;
        break;
    }
}
void fdTraceBegin(void)
{
    if (fd_startup_wave.active_bank < 3u && !fd_wave_begun) {
        fd_startup_wave.banks[fd_startup_wave.active_bank].started = DWT->CYCCNT;
        fd_wave_begun = 1u;
    }
}
void fdTracePinFailure(void) { }
void fdTraceEvent(uint8_t kind, uint16_t aux)
{
    if (fd_startup_wave.active_bank >= 3u) return;
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    volatile FdWaveBank *b = fdWaveActive();
    if (b) {
        uint32_t began = DWT->CYCCNT, at = began - b->started;
#ifdef TARGET_STARTUP_LOSS_LOG
        /* Freeze only the first post-feedback acquisition timeout, or the
         * successful handoff. Blind retries before any acceptance do not
         * trigger. Continue all counters/events; never touch drive state. */
        if (!fd_wave_sample_frozen &&
            ((kind == FD_EVENT_TIMEOUT && b->accepted && fd_wave_stage != 4u) ||
                kind == FD_EVENT_REV_HANDOFF)) {
            fd_wave_sample_frozen = 1u;
            b->reserved[4] |= (kind == FD_EVENT_TIMEOUT ? 1u : 2u) << 24;
        }
#endif
        if (kind == FD_EVENT_ZC) b->closed_zc++;
        if (kind == FD_EVENT_TIMEOUT) b->timeouts++;
        if (kind == FD_EVENT_REV_STEP) fd_wave_stage = 2u;
        if (kind == FD_EVENT_REV_EDGE) fd_wave_stage = aux >> 8;
        if (kind == FD_EVENT_REV_HANDOFF) {
            fd_wave_stage = 4u; b->handoff_at = at;
        }
        fdWaveEventStore(b, at, kind, aux, waitTime, adjusted_input);
        uint32_t cost = DWT->CYCCNT - began;
        if (cost > b->max_event_cycles) b->max_event_cycles = cost;
    }
    __set_PRIMASK(mask);
}
void fdTraceSample(uint16_t va, uint16_t vb, uint16_t vc, int16_t margin,
    uint8_t reason, uint8_t pre_seen, uint8_t confirmations)
{
    (void)margin;
    if (fd_startup_wave.active_bank >= 3u) return;
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    volatile FdWaveBank *b = fdWaveActive();
    if (b) {
        uint32_t began = DWT->CYCCNT, at = began - b->started;
        b->samples_seen++;
        if (reason < FD_SAMPLE_REASON_COUNT) b->decisions[reason]++;
        if (reason == FD_SAMPLE_ACCEPT) {
            b->accepted++;
#ifdef TARGET_STARTUP_EDGE_LOG
            if (b->reserved[0] == FD_WAVE_NONE) {
                b->reserved[0] = at;
                b->reserved[1] = va | ((uint32_t)vb << 16);
                b->reserved[2] = vc | ((uint32_t)(uint16_t)INTERVAL_TIMER_COUNT << 16);
                b->reserved[3] = (uint16_t)TIM1->CCR1 | ((uint32_t)(uint16_t)TIM1->CCR4 << 16);
                b->reserved[4] = (uint8_t)step | ((uint32_t)fdWaveFlags() << 8) |
                    ((uint32_t)fd_wave_stage << 16);
            }
#endif
            /* Preserve exact accepted sample even when it lies between the
             * decimated waveform samples. Never shorten waveform tail span. */
            fdWaveEventStore(b, at, FD_WAVE_ADC_ACCEPT, va, vb, vc);
        }
        if (
#ifdef TARGET_STARTUP_LOSS_LOG
            !fd_wave_sample_frozen &&
#elif defined(TARGET_STARTUP_EDGE_LOG)
            /* Same 100 us ring, frozen 12 ms after the FIRST ADC acceptance.
             * Later edges never move the trigger. No-edge trials retain tail.
             * Preserve exact first-edge values in header even if event wraps. */
            (b->reserved[0] == FD_WAVE_NONE ||
                at - b->reserved[0] <= FD_WAVE_EDGE_POST_TICKS) &&
#endif
#ifdef TARGET_STARTUP_ACQ_LOG
            /* Retain acquisition from the first sample. Never wrap or let
             * closed-loop observation overwrite startup. Counters and exact
             * accepted-edge event logging above remain active throughout. */
            fd_wave_stage != 4u && at <= 48000000u &&
            b->samples_stored < FD_WAVE_SAMPLES &&
#endif
#ifdef TARGET_STARTUP_NOEDGE_LOG
            /* No accepted edge: retain the entire bounded acquisition at
             * coarse spacing. Once accepted, permanently switch this trial
             * to the existing 100us ring/freeze logic. No buffer reset, no IO.
             * Late service must not overwrite a no-edge 300ms prefix. */
            (b->accepted || at <= 48000000u) &&
            (!b->samples_stored || at - b->last_sample_at >=
                (b->accepted ? FD_WAVE_SAMPLE_TICKS : 192000u))) {
#else
            (!b->samples_stored || at - b->last_sample_at >= FD_WAVE_SAMPLE_TICKS)) {
#endif
            volatile FdWaveSample *s = &b->samples[b->samples_stored % FD_WAVE_SAMPLES];
            s->at = at; s->va = va; s->vb = vb; s->vc = vc;
            s->elapsed = INTERVAL_TIMER_COUNT;
            s->pwm = TIM1->CCR1; s->trigger = TIM1->CCR4;
            s->step = step; s->flags = fdWaveFlags(); s->reason = reason;
            s->state = fd_wave_stage | (pre_seen ? 8u : 0u) |
                ((confirmations > 15u ? 15u : confirmations) << 4);
            b->last_sample_at = at; b->samples_stored++;
        }
        uint32_t cost = DWT->CYCCNT - began;
        if (cost > b->max_sample_cycles) b->max_sample_cycles = cost;
    }
    __set_PRIMASK(mask);
}
void fdTraceNoFresh(void)
{
    if (fd_startup_wave.active_bank >= 3u) return;
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    volatile FdWaveBank *b = fdWaveActive();
    if (b) b->no_fresh++;
    __set_PRIMASK(mask);
}
void fdTraceTrialFinish(uint8_t reason, uint8_t stage)
{
    /* Called under the existing bench finish critical section. No drive IO. */
    if (fd_startup_wave.active_bank >= 3u) return;
    volatile FdWaveBank *b = &fd_startup_wave.banks[fd_startup_wave.active_bank];
    uint32_t began = DWT->CYCCNT;
    b->ended = fd_wave_begun ? began - b->started : 0u;
    b->reason = reason; b->final_stage = stage;
    uint32_t state = FD_WAVE_ABORTED;
    volatile uint32_t *slot = 0;
    if (fd_wave_begun && reason == 8u && stage != 4u) {
        state = FD_WAVE_NO_LOCK; slot = &fd_startup_wave.no_lock_bank;
    } else if (reason == 9u && stage == 4u && b->handoff_at != FD_WAVE_NONE) {
        state = FD_WAVE_LOST; slot = &fd_startup_wave.lost_bank;
    } else if (reason == 1u && stage == 4u && b->handoff_at != FD_WAVE_NONE) {
        state = FD_WAVE_SUCCESS; slot = &fd_startup_wave.success_bank;
    }
    if (slot && *slot == FD_WAVE_NONE) *slot = fd_startup_wave.active_bank;
    __DMB(); b->state = state;
    fd_startup_wave.active_bank = FD_WAVE_NONE; fd_wave_begun = 0u;
    uint32_t cost = DWT->CYCCNT - began;
    if (cost > fd_startup_wave.max_finish_cycles) fd_startup_wave.max_finish_cycles = cost;
}
void fdTraceFreeze(uint32_t reason) { (void)reason; }
void fdTraceControlTick(void)
{
    if (fd_startup_wave.command) {
        fd_startup_wave.command = 0u; fd_startup_wave.rejected_commands++;
    }
}
#endif
