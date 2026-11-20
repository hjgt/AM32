#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "fd6288_m2_sine_adaptive.h"

static FdM2SineLock lockAt(uint32_t period, uint8_t forward)
{
    FdM2SineLock lock = {
        .period = period,
        .reference_period = period,
        .forward = forward
    };
    return lock;
}

static uint8_t crossing(FdM2SineLock *lock, unsigned sector,
    uint32_t boundary, uint32_t at)
{
    lock->sector = (uint8_t)sector;
    lock->sector_at = boundary;
    lock->nominal_due = boundary + lock->period;
    lock->due = lock->nominal_due;
    lock->observed = 0u;
    return fdM2SineLockObserve(lock, at);
}

static void stableTrain(uint8_t forward, uint32_t start)
{
    const uint32_t period = 100000u;
    FdM2SineLock lock = lockAt(period, forward);
    for (unsigned i = 0u; i < 18u; i++) {
        unsigned sector = forward ? i % 6u : (12u - i % 6u) % 6u;
        uint32_t boundary = start + i * period;
        assert(crossing(&lock, sector, boundary, boundary + period / 2u));
        if (i < 8u) assert(!fdM2SineLockReady(&lock, lock.due));
    }
    assert(lock.cycle_mask == 0x3fu && lock.gap_mask == 0x3fu);
    assert(lock.period == period && lock.last_period == period);
    assert(fdM2SineAdaptiveCyclePeriod(&lock) == period);
    assert(fdM2SineLockReady(&lock, lock.last_edge + period / 2u));
    assert(!fdM2SineLockReady(&lock, lock.last_edge + period / 5u - 1u));
    assert(!fdM2SineLockReady(&lock, lock.last_edge + period * 3u / 5u + 1u));
}

static void phaseLearning(void)
{
    const uint32_t period = 100000u;
    FdM2SineLock lock = lockAt(period, 1u);
    assert(!crossing(&lock, 0u, 1000000u, 1018749u));
    assert(lock.observed && !lock.streak);
    lock = lockAt(period, 1u);
    assert(crossing(&lock, 0u, 1000000u, 1018750u));
    assert(lock.streak == 1u);
    lock = lockAt(period, 1u);
    assert(!crossing(&lock, 0u, 1000000u, 1015000u));
    assert(lock.observed && !lock.streak && lock.due == 1075000u);
    assert(lock.nominal_due == lock.due);
    uint32_t first_due = lock.due;
    assert(!fdM2SineLockObserve(&lock, 1016000u));
    assert(lock.due == first_due);
    for (unsigned i = 1u; i < 16u; i++) {
        uint32_t boundary = 1000000u + i * 6u * period;
        assert(!crossing(&lock, 0u, boundary, boundary + 15000u));
    }
    assert(lock.due == 1000000u + 15u * 6u * period + 75000u);
    assert(!fdM2SineLockReady(&lock, lock.last_edge + period / 2u));

    lock = lockAt(period, 1u);
    assert(!crossing(&lock, 1u, 1000000u, 1090000u));
    assert(lock.due == 1125000u && !lock.streak);
    lock = lockAt(period, 1u);
    assert(crossing(&lock, 1u, UINT32_MAX - 60000u, UINT32_MAX - 30000u));
    assert(lock.due == 19999u && lock.nominal_due == lock.due);
    lock = lockAt(period, 1u);
    assert(!crossing(&lock, 1u, 1000000u, 1010000u));
    assert(!lock.observed && !lock.edge_mask);
    assert(!crossing(&lock, 1u, 1000000u, 1130000u));
    assert(!lock.observed && !lock.edge_mask);
}

static void phaseBiasDoesNotChangeSpeed(void)
{
    const uint32_t period = 100000u;
    const unsigned phase_percent[6] = {30u, 50u, 65u, 45u, 60u, 40u};
    FdM2SineLock lock = lockAt(period, 1u);
    for (unsigned i = 0u; i < 24u; i++) {
        unsigned sector = i % 6u;
        uint32_t boundary = 1000000u + i * period;
        assert(crossing(&lock, sector, boundary,
            boundary + period * phase_percent[sector] / 100u));
        assert(lock.period == period);
        assert(lock.last_period == period);
    }
    assert(lock.gap[5] == period * 4u / 5u);
    assert(fdM2SineLockReady(&lock, lock.last_edge + period / 2u));
}

static void boundedSpeedTracking(void)
{
    const uint32_t reference = 100000u;
    const uint32_t actual = 85000u;
    FdM2SineLock lock = lockAt(reference, 1u);
    for (unsigned i = 0u; i < 24u; i++) {
        uint32_t previous = lock.period;
        uint32_t boundary = 1000000u + i * actual;
        assert(crossing(&lock, i % 6u, boundary, boundary + actual / 2u));
        assert(previous - lock.period <= previous / 32u);
        assert(lock.period >= reference - reference / 4u);
        assert(lock.last_period == lock.period);
    }
    assert(lock.period >= actual && lock.period < actual * 9u / 8u);
    assert(fdM2SineLockReady(&lock, lock.last_edge + lock.period / 2u));
}

static void badIntervalsAndMissingSectors(void)
{
    const uint32_t period = 100000u;
    FdM2SineLock lock = lockAt(period, 1u);
    const unsigned biased_phase[6] = {30u, 80u, 80u, 30u, 30u, 30u};
    for (unsigned i = 0u; i < 18u; i++) {
        unsigned sector = i % 6u;
        uint32_t boundary = 1000000u + i * period;
        assert(crossing(&lock, sector, boundary,
            boundary + period * biased_phase[sector] / 100u));
    }
    assert(lock.streak >= 6u && lock.cycle_mask == 0x3fu);
    assert(!fdM2SineLockReady(&lock, lock.last_edge + period / 2u));

    /* Keep old cycle estimates, then omit sector 0. Consecutive evidence
     * must restart despite those older speed measurements. */
    assert(crossing(&lock, 1u, 2900000u, 2950000u));
    assert(lock.streak == 1u && !lock.gap_mask);
    assert(!fdM2SineLockReady(&lock, lock.last_edge + period / 2u));
    assert(crossing(&lock, 1u, 3000000u, 3050000u));
    assert(lock.streak == 1u && !lock.gap_mask);
    assert(!fdM2SineLockReady(&lock, lock.last_edge + period / 2u));

    lock = lockAt(period, 1u);
    for (unsigned i = 0u; i < 18u; i++) {
        uint32_t boundary = 1000000u + i * period;
        assert(crossing(&lock, i % 6u, boundary, boundary + period / 2u));
    }
    /* Missing an entire revolution cannot become a 2x period update. */
    assert(crossing(&lock, 0u, 3400000u, 3450000u));
    assert(!(lock.cycle_mask & 1u) && lock.period == period);
    assert(!fdM2SineLockReady(&lock, lock.last_edge + period / 2u));
}

int main(void)
{
    stableTrain(1u, 1000000u);
    stableTrain(0u, 1000000u);
    stableTrain(1u, UINT32_MAX - 150000u);
    stableTrain(0u, UINT32_MAX - 150000u);
    phaseLearning();
    phaseBiasDoesNotChangeSpeed();
    boundedSpeedTracking();
    badIntervalsAndMissingSectors();
    puts("PASS adaptive full-turn speed, immediate bounded next-boundary correction, seed, direction, wrap, gaps and missing-sector guards");
    return 0;
}
