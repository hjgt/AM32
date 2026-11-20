#pragma once
#include <stdint.h>
void fdM2WireRecord(uint16_t period, uint16_t payload);

typedef struct {
    uint32_t previous_at, crossing_at;
    int16_t previous_margin;
    uint8_t previous, crossing;
} FdM2CrossingTime;

/* Raw ADC margins are within +/-4095; the product fits 32 bits.
 * Never interpolate across missing carrier groups or a blanked sample. */
static inline void fdM2CrossingSample(FdM2CrossingTime *s,
    uint32_t at, int32_t margin)
{
    uint32_t gap = at - s->previous_at;
    if (s->previous && gap > 9000u) s->crossing = 0u;
    if (s->previous && gap <= 9000u && s->previous_margin <= 0 && margin > 0) {
        uint32_t before = (uint32_t)(-s->previous_margin);
        s->crossing_at = s->previous_at + gap * before / (before + (uint32_t)margin);
        s->crossing = 1u;
    }
    if (margin < -8) s->crossing = 0u;
    s->previous_at = at;
    s->previous_margin = (int16_t)margin;
    s->previous = 1u;
}

static inline uint32_t fdM2CrossingDelay(const FdM2CrossingTime *s,
    uint32_t accepted_at, uint32_t wait_ticks)
{
    if (!s->crossing) return 0u;
    uint32_t age = accepted_at - s->crossing_at;
    if (age > 16000u || wait_ticks <= 20u) return 0u;
    uint32_t limit = (wait_ticks - 20u) * 80u; /* leave 10 us service lead */
    return age < limit ? age : limit;
}

/* First coherent group only: estimate its current-carrier trigger relative
 * to the already-applied bridge step. Never use a late/ambiguous read. */
static inline uint8_t fdM2StablePreTime(uint32_t age, uint32_t gap,
    uint32_t cnt, uint32_t trigger, uint32_t duty, uint32_t floor,
    uint32_t window)
{
    return age>=3200u && age<=9000u && gap<=9000u && trigger>=floor &&
        cnt>=trigger+window && cnt<duty &&
        age>cnt-trigger+floor+160u;
}
