/* Bench-only rev-up policy. Times/duty are experimental, not motor ratings.
 * S1: 300 ms whole trial. S2/S3: 300 ms acquisition + at most 1 s closed loop.
 * Raw DWT differences are wrap safe over these bounded trials. */
#ifndef FD6288_STARTUP_SEQUENCE_H
#define FD6288_STARTUP_SEQUENCE_H
#include <stdint.h>
enum { FD_REV_OFF, FD_REV_ALIGN, FD_REV_OPEN, FD_REV_PENDING, FD_REV_CLOSED };
enum { FD_REV_NONE, FD_REV_STEP, FD_REV_HANDOFF, FD_REV_RETRY };
enum { FD_REV_EDGE_IGNORED, FD_REV_EDGE_EARLY, FD_REV_EDGE_LATE,
    FD_REV_EDGE_SEED, FD_REV_EDGE_PERIOD, FD_REV_EDGE_GOOD, FD_REV_EDGE_READY };
enum {
    FD_REV_US = 160u, FD_REV_ALIGN_TICKS = 30000u * FD_REV_US,
    FD_REV_FIRST_PERIOD = 12000u * FD_REV_US,
    FD_REV_MIN_PERIOD = 4000u * FD_REV_US,
    FD_REV_REQUIRED_EDGES = 6u,
    FD_REV_ACQUIRE_TICKS = 300000u * FD_REV_US,
    FD_REV_OBSERVE_TICKS = 1000000u * FD_REV_US
};
typedef struct {
    uint32_t sector_at, period, previous_period, edge_at, measured, delay;
    uint8_t stage, seen, confidence;
#ifdef TARGET_STARTUP_SEQUENCE_V2
    uint32_t closed_at;
    uint8_t max_confidence, settle_steps, decision, phase_percent;
#endif
#ifdef TARGET_STARTUP_S4
    uint8_t empty_retries, rescue_left, rescue_used;
#endif
} FdRevPolicy;
#ifdef TARGET_STARTUP_FEEDBACK
/* S3: return to native polling-style event-led acquisition. No fixed
 * alignment dwell or accelerating commutation table. The first five
 * qualified ADC edges advance immediately (as native zero_crosses < 5).
 * The sixth earns a measured post-ZC delay, then a bounded observation.
 * "confidence" here counts consecutive ADC-qualified edges, NOT S2's
 * fixed-sector phase/period qualification. No claim of mechanical lock. */
enum { FD_REV_RETRY_TICKS = 22500u * FD_REV_US };
#ifdef TARGET_STARTUP_S4
/* One bounded escape attempt ONLY if no qualified edge has ever appeared.
 * Six shorter sectors are not six synthetic crossings or proof of rotation.
 * Every expiry still enters the native timeout counter/latch path. */
enum { FD_S4_RESCUE_TICKS = 8000u * FD_REV_US, FD_S4_RESCUE_SECTORS = 6u,
    FD_S4_HOLD_TICKS = 150000u * FD_REV_US,
    FD_S4_RELEASE_TICKS = 300000u * FD_REV_US };
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
}
static inline uint8_t fdRevAdvancePolicy(FdRevPolicy *s, uint32_t now)
{
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
    /* Missing feedback, not a synthetic ZC. At most one retry per service. */
    s->sector_at = now;
    s->seen = s->confidence = 0u;
    s->measured = 0u;
#ifdef TARGET_STARTUP_S4
    if (s->rescue_left) {
        if (!--s->rescue_left) s->period = FD_REV_RETRY_TICKS;
    } else if (!s->rescue_used && ++s->empty_retries == 2u) {
        s->rescue_used = 1u;
        s->rescue_left = FD_S4_RESCUE_SECTORS;
        s->period = FD_S4_RESCUE_TICKS;
    }
#endif
    return FD_REV_RETRY;
}
static inline uint8_t fdRevEdgePolicy(FdRevPolicy *s, uint32_t now, uint8_t advance64)
{
    s->decision = FD_REV_EDGE_IGNORED;
    s->phase_percent = 255u; /* no commanded-period phase window in S3 */
    if (s->stage != FD_REV_OPEN || s->seen) return 0u;
    /* A late IRQ cannot rescue an already expired no-feedback sector. */
    if ((uint32_t)(now - s->sector_at) >= fdRevRetryTicks(s)) return 0u;
    uint32_t elapsed = now - s->edge_at;
    if (s->confidence && !elapsed) return 0u;
    s->seen = 1u;
#ifdef TARGET_STARTUP_S4
    /* Even one real edge cancels/prevents the rescue for this entire trial.
     * Feedback leads immediately; never run an open-loop schedule over it. */
    s->rescue_used = 1u;
    s->rescue_left = 0u;
    s->period = FD_REV_RETRY_TICKS;
#endif
    if (s->confidence) {
        s->measured = s->confidence == 1u ? elapsed :
            (3u * s->measured + elapsed) / 4u;
        s->decision = FD_REV_EDGE_GOOD;
    } else {
        s->measured = 0u; /* first edge is not a ZC-to-ZC period */
        s->decision = FD_REV_EDGE_SEED;
    }
    s->edge_at = now;
    s->confidence++;
    if (s->confidence > s->max_confidence) s->max_confidence = s->confidence;
    s->delay = 0u;
    if (s->confidence == FD_REV_REQUIRED_EDGES) {
        if (advance64 > 24u) advance64 = 24u;
        s->delay = s->measured / 2u - s->measured * advance64 / 64u;
        s->decision = FD_REV_EDGE_READY;
    }
    s->stage = FD_REV_PENDING;
    return s->confidence == FD_REV_REQUIRED_EDGES;
}
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
