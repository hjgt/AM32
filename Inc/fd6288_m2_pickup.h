#pragma once
#include <stdint.h>

/* Classify all-off phase ordering. Timestamps are 160 MHz DWT cycles. */
enum {
    FD_M2_PICKUP_SETTLE = 2000u * 160u,
    FD_M2_PICKUP_BUCKET = 500u * 160u,
    FD_M2_PICKUP_MIN = 4000u * 160u,
    FD_M2_PICKUP_MAX = 10000u * 160u,
    FD_M2_PICKUP_FRESH = 1000u * 160u,
    FD_M2_PICKUP_DRIVE_TICKS = 100000u * 160u
};

typedef struct {
    uint32_t off_at, bucket_at, sum[3], stable_at, period;
    uint16_t count;
    uint8_t pending, stable, transitions, ready;
} FdM2Pickup;

static inline uint8_t fdM2PickupSector(const FdM2Pickup *s)
{
    if (s->count < 4u) return 0u;
    unsigned high = 0u, low = 0u;
    for (unsigned i = 1u; i < 3u; i++) {
        if (s->sum[i] > s->sum[high]) high = i;
        if (s->sum[i] < s->sum[low]) low = i;
    }
    if (high == low) return 0u;
    unsigned mid = 3u - high - low;
    if (s->sum[high] - s->sum[low] < 24u * s->count ||
        s->sum[high] - s->sum[mid] < 4u * s->count ||
        s->sum[mid] - s->sum[low] < 4u * s->count) return 0u;
    /* Rows are the PWM phase, columns the low phase; matches comStep(). */
    static const uint8_t steps[3][3] = {{0,1,6},{4,0,5},{3,2,0}};
    return steps[high][low];
}

static inline void fdM2PickupObserve(FdM2Pickup *s, uint32_t now, const uint16_t phase[3],
                                    uint8_t forward)
{
    if ((uint32_t)(now - s->off_at) < FD_M2_PICKUP_SETTLE) return;
    if (!s->count) s->bucket_at = now;
    if ((uint32_t)(now - s->bucket_at) >= FD_M2_PICKUP_BUCKET) {
        uint32_t bucket_age = now - s->bucket_at;
        if (bucket_age >= 2u * FD_M2_PICKUP_BUCKET) {
            s->pending = s->stable = s->transitions = s->ready = 0u;
            s->period = 0u;
        }
        uint8_t sector = bucket_age < 2u * FD_M2_PICKUP_BUCKET ? fdM2PickupSector(s) : 0u;
        if (sector && sector == s->pending && sector != s->stable) {
            uint8_t next = forward ? (s->stable == 6u ? 1u : s->stable + 1u)
                                   : (s->stable <= 1u ? 6u : s->stable - 1u);
            uint32_t elapsed = now - s->stable_at;
            s->ready = 0u;
            if (s->stable && sector == next && elapsed >= FD_M2_PICKUP_MIN &&
                elapsed <= FD_M2_PICKUP_MAX) {
                if (s->transitions && elapsed <= s->period + s->period / 2u &&
                    s->period <= elapsed + elapsed / 2u) s->ready = sector;
                s->transitions = 1u;
                s->period = elapsed;
            } else {
                s->transitions = 0u;
                s->period = 0u;
            }
            s->stable = sector;
            s->stable_at = now;
        }
        if (sector != s->stable) s->ready = 0u;
        s->pending = sector;
        s->bucket_at = now;
        s->count = 0u;
        s->sum[0] = s->sum[1] = s->sum[2] = 0u;
    }
    s->count++;
    for (unsigned i = 0u; i < 3u; i++) s->sum[i] += phase[i];
}

static inline uint8_t fdM2PickupReady(const FdM2Pickup *s, uint32_t now)
{
    return (uint32_t)(now - s->stable_at) < FD_M2_PICKUP_FRESH ? s->ready : 0u;
}
