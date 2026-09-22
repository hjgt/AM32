#include "fd6288_startup_trace.h"
#ifdef TARGET_STARTUP_PAIR_TRACE
#include <stddef.h>
#include "common.h"
#include "peripherals.h"

extern char step, old_routine;
extern volatile char rising;
extern volatile uint16_t duty_cycle;
extern volatile uint8_t heli_coast_active;

_Static_assert(sizeof(FdPairSector) == 128, "pair sector ABI");
_Static_assert(offsetof(FdPairBank, records) == 64, "pair bank ABI");
_Static_assert(offsetof(FdStartupPair, banks) == 64, "pair header ABI");
_Static_assert(sizeof(FdStartupPair) == 18624, "pair block ABI");
volatile FdStartupPair fd_startup_pair;
static uint8_t fd_pair_begun, fd_pair_stage, fd_pair_side;
static uint16_t fd_pair_commanded_us;

static volatile FdPairBank *fdPairActive(void)
{
    if (fd_startup_pair.active_bank > 1u) return 0;
    return &fd_startup_pair.banks[fd_startup_pair.active_bank];
}
static volatile FdPairSector *fdPairSector(volatile FdPairBank *b)
{
    if (!b || !fd_pair_begun || b->handoff_at != FD_PAIR_NONE ||
        !b->sectors || b->overflow) return 0;
    return &b->records[b->sectors - 1u];
}
static uint8_t fdPairFlags(void)
{
    return (forward ? 1u : 0u) | (rising ? 2u : 0u) | (running ? 4u : 0u) |
        (heli_coast_active ? 8u : 0u) | (old_routine ? 16u : 0u);
}
static void fdPairInc(volatile uint16_t *n, volatile FdPairSector *s)
{
    if (*n != UINT16_MAX) (*n)++;
    else s->issues |= FD_PAIR_SATURATED;
}
void fdTraceInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    fd_startup_pair.magic = FD_PAIR_MAGIC;
    fd_startup_pair.abi = 1u;
    fd_startup_pair.variant = FD6288_TRACE_VARIANT;
    fd_startup_pair.clock_hz = 160000000u;
    fd_startup_pair.size = sizeof(FdStartupPair);
    fd_startup_pair.active_bank = fd_startup_pair.failure_bank =
        fd_startup_pair.success_bank = FD_PAIR_NONE;
}
void fdTraceTrial(uint32_t id)
{
    /* Called inside the existing bench critical section, before bridge start.
     * Clear a 64-byte header only; records are initialized one sector at a time.
     * Once both classes are retained, subsequent trials do not overwrite them. */
    fd_startup_pair.trials_seen = id;
    fd_pair_begun = 0u;
    fd_startup_pair.active_bank = FD_PAIR_NONE;
    for (uint32_t i = 0u; i < 2u; i++) {
        if (i == fd_startup_pair.failure_bank || i == fd_startup_pair.success_bank) continue;
        volatile FdPairBank *b = &fd_startup_pair.banks[i];
        b->started = b->ended = b->reason = b->final_stage = b->sectors = 0u;
        b->overflow = b->accepted_total = b->closed_zc = b->timeouts = 0u;
        b->max_sample_cycles = b->max_event_cycles = b->max_boundary_cycles = 0u;
        b->freeze_reason = 0u;
        b->state = FD_PAIR_ACTIVE;
        b->trial_id = id;
        b->handoff_at = FD_PAIR_NONE;
        fd_startup_pair.active_bank = i;
#ifdef TARGET_STARTUP_FEEDBACK
        fd_pair_stage = 2u;
        fd_pair_commanded_us = 22500u; /* retry deadline, not scheduled dwell */
#else
        fd_pair_stage = 1u;
        fd_pair_commanded_us = 30000u;
#endif
        break;
    }
}
void fdTraceBegin(void)
{
    volatile FdPairBank *b = fdPairActive();
    if (b && !fd_pair_begun) { b->started = DWT->CYCCNT; fd_pair_begun = 1u; }
}
void fdTracePinFailure(void) { /* Classification uses actual bench outcome. */ }

void fdTraceEvent(uint8_t kind, uint16_t aux)
{
    if (fd_startup_pair.active_bank > 1u) return;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    volatile FdPairBank *b = fdPairActive();
    if (!b || !fd_pair_begun) { __set_PRIMASK(mask); return; }
    uint32_t began = DWT->CYCCNT, at = began - b->started;
    volatile FdPairSector *s = fdPairSector(b);
    if (kind == FD_EVENT_ZC) b->closed_zc++;
    if (kind == FD_EVENT_TIMEOUT) b->timeouts++;
    if (kind == FD_EVENT_REV_STEP) { fd_pair_stage = 2u; fd_pair_commanded_us = aux; }
    if (kind == FD_EVENT_REV_EDGE && s) {
        s->stage = aux >> 8; s->confidence = aux & 255u;
    }
    if (kind == FD_EVENT_REV_DECISION && s) {
        s->decision = aux & 255u; s->sector_percent = aux >> 8;
    }
    if (kind == FD_EVENT_REV_HANDOFF) {
        if (s) s->ended = at;
        b->handoff_at = at;
        fd_pair_stage = 4u;
    }
    if (kind == FD_EVENT_COMMUTATE && b->handoff_at == FD_PAIR_NONE) {
        if (s) s->ended = at;
        if (b->sectors >= FD_PAIR_SECTORS) b->overflow++;
        else {
            s = &b->records[b->sectors++];
            /* Direct volatile stores: the compound-literal assignment used
             * a stack temporary plus newlib memset/memcpy on this toolchain,
             * costing ~12.4 us inside the commutation critical section.
             * Initialize every ABI field, including reused-slot counters,
             * without a temporary, bulk copy, or changes to drive policy. */
            s->began = at;
            s->ended = s->last_sample = s->first_pre = s->first_post =
                s->forward_at = s->reverse_at = s->accepted_at = FD_PAIR_NONE;
            for (unsigned i = 0u; i < 3u; i++) {
                s->first_adc[i] = s->last_adc[i] = s->max_adc[i] = 0u;
                s->min_adc[i] = UINT16_MAX;
            }
            s->directed_min = INT16_MAX; s->directed_max = INT16_MIN;
            s->directed_first = s->directed_last = 0;
            for (unsigned i = 0u; i < FD_SAMPLE_REASON_COUNT; i++) s->reasons[i] = 0u;
            s->samples = s->no_fresh = s->forward_crossings = s->reverse_crossings = 0u;
            s->duty_first = s->duty_last = duty_cycle;
            s->pwm_first = s->pwm_last = TIM1->CCR1;
            s->trigger_first = s->trigger_last = TIM1->CCR4;
            s->input_first = s->input_last = adjusted_input;
            s->current_max = actual_current; s->voltage_min = battery_voltage;
            s->commanded_us = fd_pair_commanded_us; s->step = step;
            s->flags = fdPairFlags(); s->stage = fd_pair_stage;
            s->decision = s->sector_percent = 255u; s->confidence = 0u;
            s->valid_samples = s->issues = 0u; s->reserved = 0u;
            fd_pair_side = 0u;
        }
        uint32_t cost = DWT->CYCCNT - began;
        if (cost > b->max_boundary_cycles) b->max_boundary_cycles = cost;
    }
    uint32_t cost = DWT->CYCCNT - began;
    if (cost > b->max_event_cycles) b->max_event_cycles = cost;
    __set_PRIMASK(mask);
}
void fdTraceSample(uint16_t va, uint16_t vb, uint16_t vc, int16_t margin,
    uint8_t reason, uint8_t pre_seen, uint8_t confirmations)
{
    (void)pre_seen; (void)confirmations;
    if (fd_startup_pair.active_bank > 1u) return;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    volatile FdPairBank *b = fdPairActive();
    if (!b || !fd_pair_begun) { __set_PRIMASK(mask); return; }
    uint32_t began = DWT->CYCCNT, at = began - b->started;
    if (reason == FD_SAMPLE_ACCEPT) b->accepted_total++;
    volatile FdPairSector *s = fdPairSector(b);
    if (s) {
        fdPairInc(&s->samples, s);
        if (reason < FD_SAMPLE_REASON_COUNT) fdPairInc(&s->reasons[reason], s);
        s->last_sample = at;
        s->duty_last = duty_cycle; s->pwm_last = TIM1->CCR1;
        s->trigger_last = TIM1->CCR4; s->input_last = adjusted_input;
        if (actual_current > s->current_max) s->current_max = actual_current;
        if (battery_voltage < s->voltage_min) s->voltage_min = battery_voltage;
        if (step != s->step || ((fdPairFlags() ^ s->flags) & 3u))
            s->issues |= FD_PAIR_PHASE_MISMATCH;
        /* Ignore invalid/blank/Coast. The shared diagnostic hysteresis is
         * compile-time checked against the existing detector's constant.
         * These observed signal transitions are NOT inferred mechanical direction. */
        if (reason >= FD_SAMPLE_PRE && reason <= FD_SAMPLE_REPORTED) {
            int16_t directed = rising ? margin : -margin;
            uint16_t adc[3] = {va, vb, vc};
            for (unsigned i = 0; i < 3; i++) {
                if (!s->valid_samples) s->first_adc[i] = adc[i];
                s->last_adc[i] = adc[i];
                if (adc[i] < s->min_adc[i]) s->min_adc[i] = adc[i];
                if (adc[i] > s->max_adc[i]) s->max_adc[i] = adc[i];
            }
            if (!s->valid_samples) s->directed_first = directed;
            s->directed_last = directed;
            if (directed < s->directed_min) s->directed_min = directed;
            if (directed > s->directed_max) s->directed_max = directed;
            fdPairInc(&s->valid_samples, s);
            uint8_t side = directed < -FD_PAIR_HYSTERESIS ? 1u :
                (directed > FD_PAIR_HYSTERESIS ? 2u : 0u);
            if (side == 1u && s->first_pre == FD_PAIR_NONE) s->first_pre = at;
            if (side == 2u && s->first_post == FD_PAIR_NONE) s->first_post = at;
            if (side && fd_pair_side && side != fd_pair_side) {
                if (side == 2u) {
                    fdPairInc(&s->forward_crossings, s);
                    if (s->forward_at == FD_PAIR_NONE) s->forward_at = at;
                } else {
                    fdPairInc(&s->reverse_crossings, s);
                    if (s->reverse_at == FD_PAIR_NONE) s->reverse_at = at;
                }
            }
            if (side) fd_pair_side = side;
            /* A rejected fresh sample breaks cross-observation continuity. */
        } else fd_pair_side = 0u;
        if (reason == FD_SAMPLE_ACCEPT && s->accepted_at == FD_PAIR_NONE) s->accepted_at = at;
    }
    uint32_t cost = DWT->CYCCNT - began;
    if (cost > b->max_sample_cycles) b->max_sample_cycles = cost;
    __set_PRIMASK(mask);
}
void fdTraceNoFresh(void)
{
    if (fd_startup_pair.active_bank > 1u) return;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    volatile FdPairSector *s = fdPairSector(fdPairActive());
    if (s) fdPairInc(&s->no_fresh, s);
    __set_PRIMASK(mask);
}
void fdTraceTrialFinish(uint8_t reason, uint8_t stage)
{
    volatile FdPairBank *b = fdPairActive();
    if (!b) return;
    uint32_t began = DWT->CYCCNT;
    volatile FdPairSector *s = fdPairSector(b);
    b->ended = began - b->started;
    if (s) s->ended = b->ended;
    b->reason = reason; b->final_stage = stage;
    uint32_t state = reason == 1u && stage == 4u && b->handoff_at != FD_PAIR_NONE ?
        FD_PAIR_SUCCESS : ((reason == 2u || reason == 3u || reason == 4u ||
        reason == 8u || reason == 9u) ? FD_PAIR_FAILURE : FD_PAIR_ABORTED);
    /* Preserve first completed example of each class. Duplicates are reusable;
     * early operator zero is NOT a failed-start result and cannot pin a bank. */
    if (state == FD_PAIR_SUCCESS && fd_startup_pair.success_bank == FD_PAIR_NONE)
        fd_startup_pair.success_bank = fd_startup_pair.active_bank;
    if (state == FD_PAIR_FAILURE && fd_startup_pair.failure_bank == FD_PAIR_NONE)
        fd_startup_pair.failure_bank = fd_startup_pair.active_bank;
    __DMB(); b->state = state;
    fd_startup_pair.active_bank = FD_PAIR_NONE;
    fd_pair_begun = 0u;
    uint32_t cost = DWT->CYCCNT - began;
    if (cost > fd_startup_pair.max_finish_cycles) fd_startup_pair.max_finish_cycles = cost;
}
void fdTraceFreeze(uint32_t reason)
{
    /* Native bench finish owns classification; no logger command may stop drive. */
    volatile FdPairBank *b = fdPairActive();
    if (b) b->freeze_reason = reason;
}
void fdTraceControlTick(void)
{
    /* Paired capture automatically selects trials. No RAM arm/reset interface:
     * do not discard either retained example or alter the motor on a command. */
    if (fd_startup_pair.command) {
        fd_startup_pair.command = 0u;
        fd_startup_pair.rejected_commands++;
    }
}
#endif
