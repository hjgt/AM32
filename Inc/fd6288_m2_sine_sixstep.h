#pragma once

#include "fd6288_m2_pickup.h"

enum {
    FD_M2_SINE_EXIT_INPUT = 137u,
    FD_M2_SINE_SIXSTEP_DUTY = 400u,
    FD_M2_SINE_LOCK_SECTORS = 6u,
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    FD_M2_SINE_MAX_SECTORS = 24u
#else
    FD_M2_SINE_MAX_SECTORS = 12u
#endif
};

static inline unsigned fdM2SineNextSector(unsigned sector, uint8_t forward)
{
    if (forward) return sector == 6u ? 1u : sector + 1u;
    return sector == 1u ? 6u : sector - 1u;
}

#ifdef TARGET_FD_M2_SINE_ADAPTIVE
#include "fd6288_m2_sine_cycle_pickup.h"
#else
typedef struct {
    FdM2Pickup bucket;
    uint32_t candidate_at, stable_at, sequence_at, period;
    uint8_t candidate, stable, intervals, boundary;
} FdM2SinePickup;

static inline uint8_t fdM2SinePeriodMatches(uint32_t observed, uint32_t expected)
{
    return observed >= expected - expected / 4u && observed <= expected + expected / 4u;
}

/* Four fresh samples per block, then two consecutive agreeing blocks.
 * Timestamp at the first block, so confirmation does not shift the sector. */
static inline uint8_t fdM2SinePickupSample(FdM2SinePickup *s, uint32_t now,
    const uint16_t phase[3], uint32_t expected, uint8_t forward)
{
    for (unsigned i = 0; i < 3u; i++) s->bucket.sum[i] += phase[i];
    if (++s->bucket.count < 4u) return 0u;
    uint8_t sector = fdM2PickupSector(&s->bucket);
    s->bucket.count = 0u;
    s->bucket.sum[0] = s->bucket.sum[1] = s->bucket.sum[2] = 0u;
    if (sector != s->candidate) {
        s->candidate = sector;
        s->candidate_at = now;
        return 0u;
    }
    if (!sector || sector == s->stable) return 0u;
    uint32_t gap = s->candidate_at - s->stable_at;
    uint8_t boundary = s->stable &&
        sector == fdM2SineNextSector(s->stable, forward);
    /* The first classification can start halfway through a sector.
     * Measure only intervals between observed sector transitions. */
    if (boundary && s->boundary &&
        fdM2SinePeriodMatches(gap, expected)) {
        if (!s->intervals) s->sequence_at = s->stable_at;
        s->intervals++;
        s->period = (s->candidate_at - s->sequence_at) / s->intervals;
    } else {
        s->intervals = 0u;
    }
    s->stable_at = s->candidate_at;
    s->stable = sector;
    s->boundary = boundary;
    return s->intervals >= 3u;
}
#endif

#ifdef TARGET_FD_M2_SINE_ADAPTIVE
#include "fd6288_m2_sine_adaptive.h"
#else
typedef struct {
    uint32_t period, last_edge, last_period, sector_at, due;
    uint8_t streak, observed;
} FdM2SineLock;

static inline uint8_t fdM2SineLockObserve(FdM2SineLock *s, uint32_t now)
{
    uint32_t age = now - s->sector_at;
    if (s->observed || age < s->period / 4u || age >= s->period * 7u / 8u)
        return 0u;
    uint32_t gap = now - s->last_edge;
    if (s->streak && fdM2SinePeriodMatches(gap, s->period)) {
        s->streak++;
        s->last_period = gap;
        /* Corrections start after a measured interval and remain local to
         * the commanded boundary while phase is being qualified. */
        uint32_t delay = gap / 2u;
        uint32_t proposed = now + delay;
        int32_t change = (int32_t)(proposed - s->due);
        int32_t max_change = (int32_t)(s->period / 8u);
        if (change > max_change) change = max_change;
        if (change < -max_change) change = -max_change;
        s->due += (uint32_t)change;
        s->period = (s->period * 3u + gap) / 4u;
    } else {
        s->streak = 1u;
    }
    s->observed = 1u;
    s->last_edge = now;
    return 1u;
}

static inline uint8_t fdM2SineLockReady(const FdM2SineLock *s, uint32_t now)
{
    uint32_t age = now - s->last_edge;
    return s->observed && s->streak >= FD_M2_SINE_LOCK_SECTORS &&
        age >= s->last_period / 5u && age <= s->last_period * 3u / 5u;
}
#endif
