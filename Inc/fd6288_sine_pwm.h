#pragma once
#include <stdint.h>

/* Change common mode only. All three line-to-line compare differences remain
 * EXACTLY unchanged. Keep both high and complementary low intervals outside
 * the board's dead-time/settling exclusion band. No gain or dead-time change. */
static inline uint8_t fdSineCenterCompares(const uint16_t raw[3], uint16_t period,
    uint16_t margin, uint16_t output[3])
{
    uint16_t lo = raw[0], hi = raw[0];
    for (unsigned i = 1u; i < 3u; i++) {
        if (raw[i] < lo) lo = raw[i];
        if (raw[i] > hi) hi = raw[i];
    }
    uint32_t span = (uint32_t)hi - lo;
    if (!period || (uint32_t)margin * 2u + span > period || hi >= period)
        return 0u;
    uint32_t base = ((uint32_t)period - span) / 2u;
    for (unsigned i = 0u; i < 3u; i++) output[i] = (uint16_t)(base + raw[i] - lo);
    return 1u;
}
