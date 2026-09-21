/* S1 bench-only rev-up policy. Times/duty are experimental, not motor ratings.
 * Raw DWT differences are wrap safe over this bounded 300 ms trial. */
#ifndef FD6288_STARTUP_SEQUENCE_H
#define FD6288_STARTUP_SEQUENCE_H
#include <stdint.h>
enum { FD_REV_OFF, FD_REV_ALIGN, FD_REV_OPEN, FD_REV_PENDING, FD_REV_CLOSED };
enum { FD_REV_NONE, FD_REV_STEP, FD_REV_HANDOFF };
enum {
    FD_REV_US = 160u, FD_REV_ALIGN_TICKS = 30000u * FD_REV_US,
    FD_REV_FIRST_PERIOD = 12000u * FD_REV_US,
    FD_REV_MIN_PERIOD = 4000u * FD_REV_US,
    FD_REV_REQUIRED_EDGES = 6u
};
typedef struct {
    uint32_t sector_at, period, previous_period, edge_at, measured, delay;
    uint8_t stage, seen, confidence;
} FdRevPolicy;
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
        return FD_REV_HANDOFF;
    }
    if (s->stage == FD_REV_ALIGN) {
        if ((uint32_t)(now - s->sector_at) < FD_REV_ALIGN_TICKS) return FD_REV_NONE;
        s->stage = FD_REV_OPEN;
    } else if (s->stage == FD_REV_OPEN) {
        if ((uint32_t)(now - s->sector_at) < s->period) return FD_REV_NONE;
        if (!s->seen) s->confidence = 0u;
        s->previous_period = s->period;
        s->period = s->period * 15u / 16u;
        if (s->period < FD_REV_MIN_PERIOD) s->period = FD_REV_MIN_PERIOD;
    } else return FD_REV_NONE;
    /* Never issue catch-up bursts after a delayed control service. */
    s->sector_at = now;
    s->seen = 0u;
    return FD_REV_STEP;
}
static inline uint8_t fdRevEdgePolicy(FdRevPolicy *s, uint32_t now, uint8_t advance64)
{
    if (s->stage != FD_REV_OPEN || s->seen) return 0u;
    s->seen = 1u; /* at most one accepted ADC edge per commanded sector */
    uint32_t phase = now - s->sector_at;
    if (phase < s->period / 8u || phase > s->period * 7u / 8u) {
        s->confidence = 0u;
        return 0u;
    }
    uint32_t elapsed = now - s->edge_at;
    if (s->confidence && elapsed >= s->previous_period * 3u / 4u &&
        elapsed <= s->previous_period * 5u / 4u) {
        s->measured = s->confidence == 1u ? elapsed :
            (3u * s->measured + elapsed) / 4u;
        s->confidence++;
    } else {
        s->confidence = 1u;
        s->measured = 0u;
    }
    s->edge_at = now;
    if (s->confidence < FD_REV_REQUIRED_EDGES) return 0u;
    if (advance64 > 24u) advance64 = 24u;
    s->delay = s->measured / 2u - s->measured * advance64 / 64u;
    s->stage = FD_REV_PENDING;
    return 1u;
}
#ifdef TARGET_STARTUP_SEQUENCE
extern volatile uint8_t fd_rev_acquiring;
#endif
#endif
