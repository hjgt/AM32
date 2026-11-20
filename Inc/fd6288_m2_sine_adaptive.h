#pragma once

#include <stdint.h>

enum { FD_M2_ADAPTIVE_SECTORS = 6u };

/* sector is step - 1. All timestamps and periods use the DWT clock.
 * The caller supplies physically qualified ADC crossings and maintains
 * nominal_due is rebased by trustworthy crossings, never by loop latency. */
typedef struct {
    uint32_t period, last_edge, last_period, sector_at, due, nominal_due;
    uint32_t reference_period;
    uint32_t edge_at[FD_M2_ADAPTIVE_SECTORS];
    uint32_t cycle_period[FD_M2_ADAPTIVE_SECTORS];
    uint32_t gap[FD_M2_ADAPTIVE_SECTORS];
    uint8_t streak, observed, sector, last_sector, forward;
    uint8_t edge_mask, cycle_mask, gap_mask, have_last_edge;
} FdM2SineLock;

/* Estimate one sector from repeated crossings of the same physical phase.
 * At least three same-sector estimates must agree within 12.5%. */
static inline uint32_t fdM2SineAdaptiveCyclePeriod(const FdM2SineLock *s)
{
    uint32_t sorted[FD_M2_ADAPTIVE_SECTORS];
    unsigned count = 0u;
    for (unsigned i = 0u; i < FD_M2_ADAPTIVE_SECTORS; i++) {
        if (!(s->cycle_mask & (1u << i))) continue;
        uint32_t value = s->cycle_period[i];
        unsigned at = count++;
        while (at && sorted[at - 1u] > value) {
            sorted[at] = sorted[at - 1u];
            at--;
        }
        sorted[at] = value;
    }
    if (count < 3u) return 0u;
    uint32_t median = sorted[(count - 1u) / 2u];
    uint32_t sum = 0u;
    unsigned agreeing = 0u;
    for (unsigned i = 0u; i < count; i++) {
        if (sorted[i] >= median - median / 8u &&
            sorted[i] <= median + median / 8u) {
            sum += sorted[i];
            agreeing++;
        }
    }
    return agreeing >= 3u ? sum / agreeing : 0u;
}

static inline uint8_t fdM2SineLockObserve(FdM2SineLock *s, uint32_t now)
{
    if (s->observed || s->sector >= FD_M2_ADAPTIVE_SECTORS || !s->period)
        return 0u;
    uint32_t period = s->period;
    uint32_t age = now - s->sector_at;
    if (age < period / 8u || age >= period + period / 4u) return 0u;
    s->observed = 1u;
    unsigned sector = s->sector;
    uint8_t bit = (uint8_t)(1u << sector);

    if (s->edge_mask & bit) {
        uint32_t measured = (now - s->edge_at[sector]) / FD_M2_ADAPTIVE_SECTORS;
        uint32_t reference = s->reference_period;
        if (reference && measured >= reference - reference / 4u &&
            measured <= reference + reference / 4u) {
            s->cycle_period[sector] = measured;
            s->cycle_mask |= bit;
        } else {
            s->cycle_mask &= (uint8_t)~bit;
        }
        uint32_t estimate = fdM2SineAdaptiveCyclePeriod(s);
        if (estimate) {
            int32_t change = ((int32_t)estimate - (int32_t)period) / 4;
            int32_t limit = (int32_t)(period / 32u);
            if (change > limit) change = limit;
            if (change < -limit) change = -limit;
            s->period += (uint32_t)change;
        }
    }
    s->edge_at[sector] = now;
    s->edge_mask |= bit;

    unsigned next_sector = (s->last_sector + (s->forward ? 1u : 5u)) %
        FD_M2_ADAPTIVE_SECTORS;
    uint8_t consecutive = s->have_last_edge && sector == next_sector;
    if (consecutive) {
        s->gap[sector] = now - s->last_edge;
        s->gap_mask |= bit;
    } else {
        s->gap_mask = 0u;
    }
    s->last_edge = now;
    s->last_sector = (uint8_t)sector;
    s->have_last_edge = 1u;
    /* The closed-loop seed is a full-turn estimate, including while an
     * individual adjacent interval is distorted by phase-dependent bias. */
    s->last_period = s->period;

    /* Bootstrap at 30 electrical degrees after the observed ADC event.
     * Do not wait another revolution to correct a known phase error.
     * Keep full-turn speed estimation separate from this phase correction:
     * phase-dependent ADC delay must not become a short speed estimate. */
    uint32_t proposed = now + s->period / 2u;
    int32_t correction = (int32_t)(proposed - s->due);
    int32_t limit = (int32_t)(period / 4u);
    if (correction > limit) correction = limit;
    if (correction < -limit) correction = -limit;
    s->due += (uint32_t)correction;
    s->nominal_due = s->due;

    if (age < period * 3u / 16u || age >= period * 7u / 8u) {
        s->streak = 0u;
        return 0u;
    }
    s->streak = consecutive && s->streak ?
        (s->streak < 255u ? s->streak + 1u : 255u) : 1u;
    return 1u;
}

static inline uint8_t fdM2SineLockReady(const FdM2SineLock *s, uint32_t now)
{
    uint32_t estimate = fdM2SineAdaptiveCyclePeriod(s);
    if (!s->observed || s->streak < FD_M2_ADAPTIVE_SECTORS ||
        s->gap_mask != 0x3fu || !estimate ||
        s->period < estimate - estimate / 8u ||
        s->period > estimate + estimate / 8u)
        return 0u;
    for (unsigned i = 0u; i < FD_M2_ADAPTIVE_SECTORS; i++) {
        if (s->gap[i] < s->period * 13u / 20u ||
            s->gap[i] > s->period * 27u / 20u)
            return 0u;
    }
    uint32_t age = now - s->last_edge;
    return age >= s->period / 5u && age <= s->period * 3u / 5u;
}
