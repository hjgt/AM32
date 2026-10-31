/* Bench-only rev-up policy. Times/duty are experimental, not motor ratings.
 * S1: 300 ms whole trial. S2/S3: 300 ms acquisition + at most 1 s closed loop.
 * Raw DWT differences are wrap safe over these bounded trials. */
#ifndef FD6288_STARTUP_SEQUENCE_H
#define FD6288_STARTUP_SEQUENCE_H
#include <stdint.h>
#include "fd6288_diagnostic_switch.h"
#include "fd6288_sector_diag.h"
enum { FD_REV_OFF, FD_REV_ALIGN, FD_REV_OPEN, FD_REV_PENDING, FD_REV_CLOSED, FD_REV_RELAX };
enum { FD_REV_NONE, FD_REV_STEP, FD_REV_HANDOFF, FD_REV_RETRY, FD_REV_LAUNCH, FD_REV_PLANNED, FD_REV_RELEASE, FD_REV_REALIGN };
enum { FD_REV_EDGE_IGNORED, FD_REV_EDGE_EARLY, FD_REV_EDGE_LATE,
    FD_REV_EDGE_SEED, FD_REV_EDGE_PERIOD, FD_REV_EDGE_GOOD, FD_REV_EDGE_READY };
enum {
    FD_REV_US = 160u, FD_REV_ALIGN_TICKS = 30000u * FD_REV_US,
#ifdef TARGET_FD_ALIGN_SETTLE
    FD_REV_SECOND_ALIGN_TICKS = 60000u * FD_REV_US,
#else
    FD_REV_SECOND_ALIGN_TICKS = FD_REV_ALIGN_TICKS,
#endif
    FD_REV_FIRST_PERIOD = 12000u * FD_REV_US,
    FD_REV_MIN_PERIOD = 4000u * FD_REV_US,
    FD_REV_REQUIRED_EDGES = 6u,
    /* S4L-B: preserve a nominal 300ms acquisition allowance after 60ms
     * positioning, still a HARD wall-clock bound from trial start. */
#ifdef TARGET_FD_ALIGN_BUDGET
    FD_REV_ACQUIRE_TICKS = 300000u * FD_REV_US + FD_REV_ALIGN_TICKS + FD_REV_SECOND_ALIGN_TICKS,
#else
    FD_REV_ACQUIRE_TICKS = 300000u * FD_REV_US,
#endif
    FD_REV_OBSERVE_TICKS = 1000000u * FD_REV_US
};
typedef struct {
#ifdef TARGET_FD_REPOSITION
    uint32_t acquisition_at, trial_started;
    uint8_t reposition_used, observed_once;
#ifdef TARGET_FD_STALE_RETRY
    uint8_t missing_sectors;
#endif
#endif
#ifdef TARGET_FD_DOUBLE_ALIGN
    uint8_t align_left;
#endif
    uint32_t sector_at, period, previous_period, edge_at, measured, delay;
    uint8_t stage, seen, confidence;
#ifdef TARGET_STARTUP_SEQUENCE_V2
    uint32_t closed_at;
    uint8_t max_confidence, settle_steps, decision, phase_percent;
#endif
#ifdef TARGET_STARTUP_S4
    uint8_t empty_retries, rescue_left, rescue_used;
#ifdef TARGET_STARTUP_S4E
    uint8_t observation_done;
#endif
#endif
#ifdef TARGET_STARTUP_HANDOFF_REAL
    uint32_t last_real_period;
#endif
} FdRevPolicy;
#ifdef TARGET_STARTUP_FEEDBACK
/* S3: return to native polling-style event-led acquisition. No fixed
 * alignment dwell or accelerating commutation table. The first five
 * qualified ADC edges advance immediately (as native zero_crosses < 5).
 * The sixth earns a measured post-ZC delay, then a bounded observation.
 * S4F optionally delays edges 2..5; S4G also adds a fixed seed dwell.
 * Neither changes the sixth-edge handoff.
 * "confidence" here counts consecutive ADC-qualified edges, NOT S2's
 * fixed-sector phase/period qualification. No claim of mechanical lock. */
enum { FD_REV_RETRY_TICKS = 22500u * FD_REV_US };
#ifdef TARGET_STARTUP_S4
/* S4/S4B: one escape attempt only if no qualified edge ever appeared.
 * S4C: retain an UNUSED attempt after a broken acquisition streak; two
 * consecutive no-feedback sectors are still required since the last edge.
 * Six shorter sectors are not six synthetic crossings or proof of rotation.
 * Every expiry still enters the native timeout counter/latch path. */
enum { FD_S4_RESCUE_TICKS = 8000u * FD_REV_US, FD_S4_RESCUE_SECTORS = 6u,
    FD_S4_HOLD_TICKS = 150000u * FD_REV_US,
    FD_S4_RELEASE_TICKS = 300000u * FD_REV_US };
static inline uint32_t fdS4RescuePeriod(uint8_t left)
{
    if (!left) return FD_REV_RETRY_TICKS;
#ifdef TARGET_STARTUP_RESCUE_RAMP
    /* S4H: experimental one-shot acceleration, not a position estimate.
     * Index by unspent budget: edges consume slots too, never restart ramp.
     * Caller supplies 1..6; clamp defensively without adding retry budget. */
    static const uint16_t us[] = { 8000, 9000, 10000, 12000, 14000, 16000 };
    return us[(left > 6u ? 6u : left) - 1u] * FD_REV_US;
#else
    return FD_S4_RESCUE_TICKS;
#endif
}
static inline uint16_t fdS4ClosedFloor(uint32_t age, uint16_t base, uint16_t minimum)
{
    if (age < FD_S4_HOLD_TICKS) return base;
    age -= FD_S4_HOLD_TICKS;
    if (age >= FD_S4_RELEASE_TICKS) return 0u;
    if (minimum >= base) return base;
    /* Scale to microseconds before multiplying: raw DWT cycles would
     * overflow even for the normal 150-duty difference. */
    return minimum + (uint32_t)(base - minimum) *
        ((FD_S4_RELEASE_TICKS - age) / FD_REV_US) /
        (FD_S4_RELEASE_TICKS / FD_REV_US);
}
#endif
static inline uint32_t fdRevRetryTicks(const FdRevPolicy *s)
{
#ifdef TARGET_STARTUP_S4
    return s->period;
#else
    (void)s;
    return FD_REV_RETRY_TICKS;
#endif
}
static inline void fdRevBeginPolicy(FdRevPolicy *s, uint32_t now)
{
    *s = (FdRevPolicy){ .sector_at = now, .period = FD_REV_RETRY_TICKS,
        .stage = FD_REV_OPEN, .phase_percent = 255u };
#ifdef TARGET_STARTUP_FIRST_WAIT
    /* S4I: only the first cold-start sector gets a 30ms deadline.
     * An ADC-qualified edge still advances via the existing pending delay.
     * No alignment state, fabricated crossing or extension of 300ms budget. */
    s->period = 30000u * FD_REV_US;
#endif
#ifdef TARGET_FD_DOUBLE_ALIGN
    s->stage = FD_REV_ALIGN;
    s->align_left = 2u;
#endif
#ifdef TARGET_FD_PLANNED_ACQ
    s->period = s->previous_period = 16000u * FD_REV_US;
#endif
#ifdef TARGET_FD_REPOSITION
    s->trial_started = now;
#endif
}
#ifdef TARGET_FD_DOUBLE_ALIGN
/* Two adjacent 60-electrical-degree vectors, 30ms each. These are bench
 * parameters, not proof of settled rotor position. S4L total budget 300ms;
 * S4L-B adds the two planned dwells, with no renewable retry budget.
 * Forward physical sequence 5,6 -> acquisition1; reverse 3,2 -> acquisition1. */
static inline uint8_t fdDoubleAlignSeed(uint8_t forward)
{
    (void)forward;
    return 4u; /* commutate increments/decrements to 5/3 */
}
#ifdef TARGET_FD_LAUNCH_ADVANCE
static inline uint8_t fdDoubleAlignLaunchSeed(void)
{
    /* One bridge application: predecessor1 -> forward2 / reverse6.
     * Skip physical1 without energizing it or counting a crossing.
     * Second alignment was forward6 / reverse2: commanded vector distance
     * becomes120deg rather than60deg, NOT a measured rotor torque angle. */
    return 1u;
}
#endif
static inline uint16_t fdDoubleAlignDuty(const FdRevPolicy *s, uint32_t now)
{
#ifdef TARGET_FD_ALIGN_HOLD
    /* S4N bench comparison: ramp only from rest. Moving to the second
     * positioning vector keeps the prior 360 request instead of dropping
     * to 210 for a second 10ms ramp. This is duty, not regulated torque.
     * No change to vector order, dwell, acquisition budget or duty cap. */
    if (s->align_left == 1u) return 360u;
#endif
    uint32_t us = (uint32_t)(now - s->sector_at) / FD_REV_US;
    if (us > 10000u) us = 10000u;
    return 210u + 150u * us / 10000u;
}
#endif
#ifdef TARGET_FD_REPOSITION
/* Reserve the SAME post-positioning allowance S4R intended to provide.
 * This is a scheduling floor, not proof that130ms can start this motor.
 * With the current390ms whole trial, the full original rescue leaves too
 * little time: deliberately skip reposition rather than steal its budget. */
static inline uint8_t fdRetryLaunchSeed(uint8_t forward)
{
#ifdef TARGET_FD_RETRY_ADJACENT
    /* Retry alignment ends at forward2 / reverse4. Advance one sector
     * in the requested direction: physical3 in either case. This is a
     * commanded60deg spacing, not a measured rotor torque angle. */
    return forward ? 2u : 4u;
#else
    (void)forward;
    return 3u; /* original retry: forward4 / reverse2 */
#endif
}
enum { FD_REPOSITION_RELEASE_TICKS = 20000u*FD_REV_US,
    FD_REPOSITION_ACQ_RESERVE = 130000u*FD_REV_US,
    FD_REPOSITION_REQUIRED = FD_REPOSITION_RELEASE_TICKS + FD_REV_ALIGN_TICKS +
        FD_REV_SECOND_ALIGN_TICKS + FD_REPOSITION_ACQ_RESERVE };
#ifdef TARGET_FD_FULL_RETRY
/* S4T: preserve the entire first390ms attempt. Admit ONE restart only
 * at an expired sector within390..410ms, then hard stop at820ms total.
 * Latest entry leaves20ms release +90ms alignment +300ms acquisition.
 * Bounds are experimental wall-clock limits, not a thermal guarantee. */
enum { FD_FULL_RETRY_ENTRY_END = 410000u*FD_REV_US,
    FD_FULL_RETRY_END = 820000u*FD_REV_US };
static __attribute__((noinline)) uint8_t fdRevRetryEligible(const FdRevPolicy *s, uint32_t now)
{
    uint8_t feedback_absent = !s->observed_once;
#ifdef TARGET_FD_STALE_RETRY
    /* An isolated old edge is not sustained synchronization. Require both
     * three completed empty sectors AND three normal timeout intervals
     * without an accepted edge. Do not interrupt a pending/current streak. */
    feedback_absent |= s->missing_sectors >= 3u && !s->seen && !s->confidence &&
        (uint32_t)(now-s->edge_at) >= 3u*FD_REV_RETRY_TICKS;
#else
    (void)now;
#endif
    return s->stage == FD_REV_OPEN && !s->reposition_used && feedback_absent &&
        s->rescue_used && !s->rescue_left;
}
#endif
static inline uint8_t fdRevCanReposition(const FdRevPolicy *s, uint32_t now)
{
    uint32_t age = now-s->trial_started;
#ifdef TARGET_FD_FULL_RETRY
    /* S4V2: do not evaluate recovery eligibility on the normal first-attempt
     * path. Keep the uncommon classifier out of the masked hot code. */
    return age >= FD_REV_ACQUIRE_TICKS && age < FD_FULL_RETRY_ENTRY_END &&
        (uint32_t)(now-s->sector_at) >= s->period && fdRevRetryEligible(s,now);
#else
    return s->stage == FD_REV_OPEN && !s->reposition_used && !s->observed_once &&
        s->rescue_used && !s->rescue_left &&
        (uint32_t)(now-s->sector_at) >= s->period &&
        age < FD_REV_ACQUIRE_TICKS && FD_REV_ACQUIRE_TICKS-age >= FD_REPOSITION_REQUIRED;
#endif
}
#endif
static inline uint8_t fdRevAdvancePolicy(FdRevPolicy *s, uint32_t now)
{
#ifdef TARGET_FD_REPOSITION
    /* Only AFTER the original rescue is exhausted, at an expired sector
     * boundary, with enough total time for the complete replacement attempt. */
    if (fdRevCanReposition(s,now)) {
        s->reposition_used = 1u;
        s->stage = FD_REV_RELAX;
        s->sector_at = now;
        s->seen = s->confidence = 0u;
        s->measured = s->last_real_period = 0u;
        return FD_REV_RELEASE;
    }
    if (s->stage == FD_REV_RELAX) {
        if ((uint32_t)(now-s->sector_at) < FD_REPOSITION_RELEASE_TICKS) return FD_REV_NONE;
        /* Reset only the acquisition policy. The external trial start,
         * protection latches and native timeout count must not reset. */
#ifndef TARGET_FD_FULL_RETRY
        uint8_t rescue_used=s->rescue_used;
#endif
        uint32_t trial_started=s->trial_started;
        fdRevBeginPolicy(s,now);
        s->trial_started=trial_started;
        s->reposition_used=1u;
#ifndef TARGET_FD_FULL_RETRY
        s->rescue_used=rescue_used;
#endif
        /* S4T permits one six-sector rescue in the second attempt only.
         * reposition_used survives, so no third attempt can replenish it.
         * External native protection counts/latches remain untouched. */
        return FD_REV_REALIGN;
    }
#endif
#ifdef TARGET_FD_DOUBLE_ALIGN
    if (s->stage == FD_REV_ALIGN) {
        uint32_t dwell = s->align_left == 1u ? FD_REV_SECOND_ALIGN_TICKS : FD_REV_ALIGN_TICKS;
        if ((uint32_t)(now - s->sector_at) < dwell) return FD_REV_NONE;
        s->sector_at = now;
        if (!--s->align_left) {
            s->stage = FD_REV_OPEN;
#ifdef TARGET_FD_REPOSITION
            s->acquisition_at = now;
#endif
#ifdef TARGET_FD_LAUNCH_ADVANCE
            return FD_REV_LAUNCH;
#endif
        }
        return FD_REV_STEP; /* planned positioning, never a feedback timeout */
    }
#endif
    if (s->stage == FD_REV_PENDING) {
        if ((uint32_t)(now - s->edge_at) < s->delay) return FD_REV_NONE;
        if (s->confidence >= FD_REV_REQUIRED_EDGES) {
            s->stage = FD_REV_CLOSED;
            s->closed_at = now;
            return FD_REV_HANDOFF;
        }
        s->stage = FD_REV_OPEN;
        s->sector_at = now;
        s->seen = 0u;
        return FD_REV_STEP;
    }
    if (s->stage != FD_REV_OPEN ||
        (uint32_t)(now - s->sector_at) < fdRevRetryTicks(s)) return FD_REV_NONE;
#ifdef TARGET_FD_PLANNED_ACQ
    /* A commanded boundary is not feedback or a native timeout. No catch-up
     * burst after a delayed service, and no extension of the trial budget. */
    if (!s->seen) s->confidence = 0u;
    s->previous_period = s->period;
    if (s->confidence < 2u && s->period > 8000u * FD_REV_US)
        s->period -= 1000u * FD_REV_US;
    s->sector_at = now;
    s->seen = 0u;
    return FD_REV_PLANNED;
#endif
    /* Missing feedback, not a synthetic ZC. At most one retry per service. */
#ifdef TARGET_FD_STALE_RETRY
    if (s->missing_sectors < 255u) s->missing_sectors++;
#endif
    s->sector_at = now;
    s->seen = s->confidence = 0u;
    s->measured = 0u;
#ifdef TARGET_STARTUP_HANDOFF_REAL
    s->last_real_period = 0u;
#endif
#ifdef TARGET_STARTUP_S4
#ifdef TARGET_STARTUP_FIRST_WAIT
    /* Consume extended initial wait once, before the normal rescue policy.
     * Never restore it on streak loss or a return to physical step1. */
    if (s->period == 30000u * FD_REV_US) s->period = FD_REV_RETRY_TICKS;
#endif
    if (s->rescue_left) {
#ifdef TARGET_STARTUP_S4E
        /* A feedback-led waiting sector timed out: consume its existing
         * budget and resume the remaining blind attempt, never refill. */
#ifdef TARGET_STARTUP_RESCUE_RAMP
        s->period = fdS4RescuePeriod(--s->rescue_left);
#else
        s->period = --s->rescue_left ? FD_S4_RESCUE_TICKS : FD_REV_RETRY_TICKS;
#endif
#else
        if (!--s->rescue_left) s->period = FD_REV_RETRY_TICKS;
#endif
    } else if (!s->rescue_used && ++s->empty_retries == 2u) {
        s->rescue_used = 1u;
        s->rescue_left = FD_S4_RESCUE_SECTORS;
#ifdef TARGET_STARTUP_RESCUE_RAMP
        s->period = fdS4RescuePeriod(s->rescue_left);
#else
        s->period = FD_S4_RESCUE_TICKS;
#endif
    }
#endif
    return FD_REV_RETRY;
}
static inline uint8_t fdRevEdgePolicy(FdRevPolicy *s, uint32_t now, uint8_t advance64)
{
#ifdef TARGET_FD_PLANNED_ACQ
    /* Observe the first five edges without moving the commanded boundary.
     * Six consecutive eligible sectors are evidence, not proof of rotation.
     * Broad phase screen excludes switching-boundary events; pair timing
     * and phase continuity prevent isolated crossings earning handoff. */
    if (s->stage != FD_REV_OPEN || s->seen) return 0u;
    uint32_t phase = now - s->sector_at;
    if (phase >= s->period) return 0u;
    s->seen = 1u;
    uint32_t elapsed = now - s->edge_at;
    uint32_t old_phase = (uint32_t)s->phase_percent * s->previous_period / 100u;
    uint32_t delta = phase > old_phase ? phase-old_phase : old_phase-phase;
    uint8_t pair = s->confidence && elapsed >= s->previous_period/2u &&
        elapsed <= s->previous_period+s->previous_period/2u &&
        delta <= s->previous_period/4u;
    s->phase_percent = phase * 100u / s->period;
    s->edge_at = now;
    if (phase < s->period/16u || phase > s->period-s->period/16u) {
        s->confidence = 0u;
        return 0u;
    }
    if (!pair) {
        s->confidence = 1u;
        s->measured = s->last_real_period = 0u;
    } else {
        s->last_real_period = elapsed;
        s->measured = s->confidence == 1u ? elapsed : (3u*s->measured+elapsed)/4u;
        if (s->confidence < FD_REV_REQUIRED_EDGES) s->confidence++;
    }
    if (s->confidence > s->max_confidence) s->max_confidence = s->confidence;
    if (s->confidence < FD_REV_REQUIRED_EDGES) return 0u;
    if (advance64 > 24u) advance64 = 24u;
    s->delay = s->measured/2u-s->measured*advance64/64u;
    uint32_t boundary = phase+s->delay;
    uint32_t error = boundary > s->period ? boundary-s->period : s->period-boundary;
    if (error > s->period/4u) return 0u;
    s->stage = FD_REV_PENDING;
    return 1u;
#else
    FD_DIAG(s->decision = FD_REV_EDGE_IGNORED);
    FD_DIAG(s->phase_percent = 255u); /* no commanded-period phase window in S3 */
    if (s->stage != FD_REV_OPEN || s->seen) return 0u;
    /* A late IRQ cannot rescue an already expired no-feedback sector. */
    if ((uint32_t)(now - s->sector_at) >= fdRevRetryTicks(s)) return 0u;
#ifdef TARGET_FD_REPOSITION
    s->observed_once = 1u;
#endif
    uint32_t elapsed = now - s->edge_at;
    if (s->confidence && !elapsed) return 0u;
#ifdef TARGET_FD_STALE_RETRY
    s->missing_sectors = 0u;
#endif
    s->seen = 1u;
#ifdef TARGET_STARTUP_S4
#ifdef TARGET_STARTUP_RESCUE_RETAIN
    /* A real edge immediately wins. It resets the consecutive missing-sector
     * count, not the one-shot budget. Only starting rescue consumes budget
     * (in fdRevAdvancePolicy). S4C discards the remainder; S4D below keeps
     * only the unspent sectors until a second consecutive edge. */
    s->empty_retries = 0u;
#else
    /* Even one real edge cancels/prevents the rescue for this entire trial.
     * Feedback leads immediately; never run an open-loop schedule over it. */
    s->rescue_used = 1u;
#endif
#ifdef TARGET_STARTUP_RESCUE_BRIDGE
    /* S4D: a solitary edge still leads commutation immediately, but does
     * not discard the remaining short-sector budget. Consume this sector
     * on an edge just as on a timeout; never refund or extend six sectors.
     * A second consecutive edge cancels the remaining budget. This is only
     * an acquisition deadline, never permission to generate synthetic ZCs. */
    if (s->rescue_left && !s->confidence) {
        if (!--s->rescue_left) s->period = FD_REV_RETRY_TICKS;
#ifdef TARGET_STARTUP_S4E
        /* First real edge leads immediately, but next feedback may need
         * longer than the blind probe cadence. Preserve only unspent budget. */
        s->period = FD_REV_RETRY_TICKS;
#endif
    } else {
        s->rescue_left = 0u;
        s->period = FD_REV_RETRY_TICKS;
    }
#else
    s->rescue_left = 0u;
    s->period = FD_REV_RETRY_TICKS;
#endif
#endif
    if (s->confidence) {
#ifdef TARGET_STARTUP_HANDOFF_REAL
        /* Keep the newest adjacent REAL edge interval separate from the
         * slow IIR. Only the first closed-loop ADC gate uses this value. */
        s->last_real_period = elapsed;
#endif
        s->measured = s->confidence == 1u ? elapsed :
            (3u * s->measured + elapsed) / 4u;
        FD_DIAG(s->decision = FD_REV_EDGE_GOOD);
    } else {
        s->measured = 0u; /* first edge is not a ZC-to-ZC period */
#ifdef TARGET_STARTUP_HANDOFF_REAL
        s->last_real_period = 0u;
#endif
        FD_DIAG(s->decision = FD_REV_EDGE_SEED);
    }
    s->edge_at = now;
    s->confidence++;
    FD_DIAG(if (s->confidence > s->max_confidence) s->max_confidence = s->confidence);
    s->delay = 0u;
#if defined(TARGET_STARTUP_SEED_DELAY) && !defined(TARGET_FD_NATIVE_EARLY)
    /* S4G bench A/B: no valid period exists for a seed. A fixed 500us
     * dwell tests first-edge commutation phase without inventing a period.
     * Applies again only after a real acquisition timeout resets the streak.
     * No budget refill or deadline extension; not a rotor-position estimate. */
    if (s->confidence == 1u) s->delay = 500u * FD_REV_US;
#endif
#if defined(TARGET_STARTUP_PHASE_DELAY) && !defined(TARGET_FD_NATIVE_EARLY)
    /* S4F bench A/B: the seed has no real period. Edges 2..5 wait a
     * bounded post-ZC phase delay using adjacent REAL edge timestamps.
     * Do not change sixth-edge handoff, rescue budget or retry accounting.
     * 3 ms is an experimental cap, not a validated motor parameter. */
    if (s->confidence >= 2u && s->confidence < FD_REV_REQUIRED_EDGES) {
        uint8_t advance = advance64 > 24u ? 24u : advance64;
        s->delay = elapsed / 2u - elapsed * advance / 64u;
        if (s->delay > 3000u * FD_REV_US) s->delay = 3000u * FD_REV_US;
    }
#endif
#ifdef TARGET_FD_FIRST_EDGE_500US
    /* Single-variable probe: only the seed of each new feedback streak.
     * Keep NATIVE_EARLY, so edges 2..5 retain zero policy delay. */
    if (s->confidence == 1u) s->delay = 500u * FD_REV_US;
#endif
    if (s->confidence == FD_REV_REQUIRED_EDGES) {
        if (advance64 > 24u) advance64 = 24u;
        s->delay = s->measured / 2u - s->measured * advance64 / 64u;
        FD_DIAG(s->decision = FD_REV_EDGE_READY);
    }
    s->stage = FD_REV_PENDING;
    return s->confidence == FD_REV_REQUIRED_EDGES;
#endif
}
#ifdef TARGET_FD_STARTUP_LEVEL
/* Level-only events may move the bridge but never seed an interval or
 * increase the evidence streak. Keep the existing pending timer path. */
static inline uint8_t fdRevStartupEvent(FdRevPolicy *s, uint32_t now,
                                      uint8_t advance64, uint8_t crossing)
{
    if (s->stage != FD_REV_OPEN || s->seen ||
        (uint32_t)(now-s->sector_at) >= fdRevRetryTicks(s)) return 0u;
    if (crossing) return fdRevEdgePolicy(s, now, advance64);
    s->confidence = 0u;
    uint8_t max_before = s->max_confidence;
#ifdef TARGET_FD_REPOSITION
    uint8_t observed_before = s->observed_once;
#endif
    (void)fdRevEdgePolicy(s, now, advance64);
    s->confidence = 0u;
    s->measured = 0u;
    s->max_confidence = max_before;
#ifdef TARGET_STARTUP_HANDOFF_REAL
    s->last_real_period = 0u;
#endif
#ifdef TARGET_FD_REPOSITION
    s->observed_once = observed_before;
#endif
    return 0u;
}
#endif

#else
static inline void fdRevBeginPolicy(FdRevPolicy *s, uint32_t now)
{
    *s = (FdRevPolicy){ .sector_at = now, .period = FD_REV_FIRST_PERIOD,
        .previous_period = FD_REV_FIRST_PERIOD, .stage = FD_REV_ALIGN };
}
static inline uint8_t fdRevAdvancePolicy(FdRevPolicy *s, uint32_t now)
{
    if (s->stage == FD_REV_PENDING) {
        if ((uint32_t)(now - s->edge_at) < s->delay) return FD_REV_NONE;
        s->stage = FD_REV_CLOSED;
#ifdef TARGET_STARTUP_SEQUENCE_V2
        s->closed_at = now;
#endif
        return FD_REV_HANDOFF;
    }
    if (s->stage == FD_REV_ALIGN) {
        if ((uint32_t)(now - s->sector_at) < FD_REV_ALIGN_TICKS) return FD_REV_NONE;
        s->stage = FD_REV_OPEN;
    } else if (s->stage == FD_REV_OPEN) {
        if ((uint32_t)(now - s->sector_at) < s->period) return FD_REV_NONE;
        if (!s->seen) s->confidence = 0u;
        s->previous_period = s->period;
#ifdef TARGET_STARTUP_SEQUENCE_V2
        /* Two coherent edges near the sector end are a reason to stop
         * demanding more acceleration, NOT permission to close the loop.
         * Hold up to one electrical cycle, bounded by the original timeout. */
        if (s->settle_steps) s->settle_steps--;
        else s->period = s->period * 31u / 32u;
#else
        s->period = s->period * 15u / 16u;
#endif
        if (s->period < FD_REV_MIN_PERIOD) s->period = FD_REV_MIN_PERIOD;
    } else return FD_REV_NONE;
    /* Never issue catch-up bursts after a delayed control service. */
    s->sector_at = now;
    s->seen = 0u;
    return FD_REV_STEP;
}
static inline uint8_t fdRevEdgePolicy(FdRevPolicy *s, uint32_t now, uint8_t advance64)
{
#ifdef TARGET_STARTUP_SEQUENCE_V2
    s->decision = FD_REV_EDGE_IGNORED;
    s->phase_percent = 255u;
#endif
    if (s->stage != FD_REV_OPEN || s->seen) return 0u;
    s->seen = 1u; /* at most one accepted ADC edge per commanded sector */
    uint32_t phase = now - s->sector_at;
#ifdef TARGET_STARTUP_SEQUENCE_V2
    /* Clamp before multiplying, even for an unexpectedly delayed caller. */
    s->phase_percent = phase >= s->period ? 100u : phase * 100u / s->period;
#endif
    if (phase < s->period / 8u || phase > s->period * 7u / 8u) {
#ifdef TARGET_STARTUP_SEQUENCE_V2
        s->decision = phase < s->period / 8u ? FD_REV_EDGE_EARLY : FD_REV_EDGE_LATE;
#endif
        s->confidence = 0u;
        return 0u;
    }
    uint32_t elapsed = now - s->edge_at;
    if (s->confidence && elapsed >= s->previous_period * 3u / 4u &&
        elapsed <= s->previous_period * 5u / 4u) {
        s->measured = s->confidence == 1u ? elapsed :
            (3u * s->measured + elapsed) / 4u;
        s->confidence++;
#ifdef TARGET_STARTUP_SEQUENCE_V2
        s->decision = FD_REV_EDGE_GOOD;
        if (phase >= s->period * 3u / 4u) s->settle_steps = 6u;
#endif
    } else {
#ifdef TARGET_STARTUP_SEQUENCE_V2
        s->decision = s->confidence ? FD_REV_EDGE_PERIOD : FD_REV_EDGE_SEED;
#endif
        s->confidence = 1u;
        s->measured = 0u;
    }
    s->edge_at = now;
#ifdef TARGET_STARTUP_SEQUENCE_V2
    if (s->confidence > s->max_confidence) s->max_confidence = s->confidence;
#endif
    if (s->confidence < FD_REV_REQUIRED_EDGES) return 0u;
    if (advance64 > 24u) advance64 = 24u;
    s->delay = s->measured / 2u - s->measured * advance64 / 64u;
    s->stage = FD_REV_PENDING;
#ifdef TARGET_STARTUP_SEQUENCE_V2
    s->decision = FD_REV_EDGE_READY;
#endif
    return 1u;
}
#endif /* TARGET_STARTUP_FEEDBACK */
#ifdef TARGET_STARTUP_SEQUENCE_V2
static inline uint8_t fdRevDeadlinePolicy(const FdRevPolicy *s, uint32_t now,
    uint32_t started)
{
#ifdef TARGET_STARTUP_S4E
    if (s->stage == FD_REV_CLOSED) return 0u;
#endif
#ifdef TARGET_FD_FULL_RETRY
    if (s->reposition_used)
        return (uint32_t)(now-started) >= FD_FULL_RETRY_END;
    if ((uint32_t)(now-started) < FD_REV_ACQUIRE_TICKS) return 0u;
    if ((uint32_t)(now-started) < FD_FULL_RETRY_ENTRY_END && fdRevRetryEligible(s,now))
        return 0u; /* allow the next expired sector to release the bridge */
#endif
    /* Only the explicit six-edge handoff earns an observation window. A
     * partial streak or pending handoff never extends acquisition. */
    return s->stage == FD_REV_CLOSED ?
        (uint32_t)(now - s->closed_at) >= FD_REV_OBSERVE_TICKS :
        (uint32_t)(now - started) >= FD_REV_ACQUIRE_TICKS;
}
#endif
#ifdef TARGET_STARTUP_SEQUENCE
extern volatile uint8_t fd_rev_acquiring;
#endif
#endif
