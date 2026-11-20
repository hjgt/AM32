#pragma once

#include <stdint.h>

#ifndef FD6288_M2_SINE_ALIGN_HOLD_US
#define FD6288_M2_SINE_ALIGN_HOLD_US 0u
#endif
#ifndef FD6288_M2_SINE_RUNUP_RPM
#define FD6288_M2_SINE_RUNUP_RPM 500u
#endif

enum {
    FD_M2_SINE_CYCLES_PER_US = 160u,
    FD_M2_SINE_PERIOD = 6666u,
    FD_M2_SINE_ALIGN_US = 150000u,
    FD_M2_SINE_MOTION_AT_US = FD_M2_SINE_ALIGN_US + FD6288_M2_SINE_ALIGN_HOLD_US,
    FD_M2_SINE_SLOW_US = 7000u / 24u,
    FD_M2_SINE_MIN_US = 42u, /* about one degree per 24 kHz carrier */
    FD_M2_SINE_SLOW_TRAVEL = 720u,
    FD_M2_SINE_PLATFORM_TRAVEL = 720u,
    FD_M2_SINE_MAX_DRIVE_US = 2000000u + FD6288_M2_SINE_ALIGN_HOLD_US,
    FD_M2_SINE_COAST_US = 40000u,
    FD_M2_SINE_DEGREE = 1000000u, /* angle accumulator in microdegrees */
    FD_M2_SINE_ELECTRICAL_TURN = 360u * FD_M2_SINE_DEGREE,
    FD_M2_SINE_CURRENT_CENTIAMPS = 400u
};

#ifdef TARGET_FD_M2_SINE_RUNUP
enum {
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    FD_M2_SINE_RUNUP_RATE = FD6288_M2_SINE_RUNUP_RPM * 72u,
    FD_M2_SINE_RUNUP_LOW_RPM = FD6288_M2_SINE_RUNUP_RPM * 9u / 10u,
    FD_M2_SINE_RUNUP_HIGH_RPM = FD6288_M2_SINE_RUNUP_RPM * 11u / 10u,
    FD_M2_SINE_RUNUP_MIN_SECTOR = ((5000000u + 6u * FD_M2_SINE_RUNUP_HIGH_RPM - 1u) /
        (6u * FD_M2_SINE_RUNUP_HIGH_RPM)) * FD_M2_SINE_CYCLES_PER_US,
    FD_M2_SINE_RUNUP_MAX_SECTOR = (5000000u / (6u * FD_M2_SINE_RUNUP_LOW_RPM)) *
        FD_M2_SINE_CYCLES_PER_US,
#else
    FD_M2_SINE_RUNUP_RATE = 420u * 72u, /* 24 poles, millidegrees per ms */
    FD_M2_SINE_RUNUP_MIN_SECTOR = 1812u * FD_M2_SINE_CYCLES_PER_US, /* 460 RPM */
    FD_M2_SINE_RUNUP_MAX_SECTOR = 2193u * FD_M2_SINE_CYCLES_PER_US, /* 380 RPM */
#endif
    FD_M2_SINE_RUNUP_HOLD_US = 40000u,
    /* At the existing 333 RPM/s slew, every extra RPM needs 3 ms. */
    FD_M2_SINE_RUNUP_EXTRA_US = (FD6288_M2_SINE_RUNUP_RPM - 500u) * 3000u,
    FD_M2_SINE_RUNUP_MAX_DRIVE_US = 2000000u + FD6288_M2_SINE_ALIGN_HOLD_US +
        FD_M2_SINE_RUNUP_EXTRA_US
};
#endif

static inline uint16_t fdM2SineRaw(uint16_t table_value, uint32_t fade_ms)
{
    /* AM32 2.16 at power 10, divider 2, with the shared offset omitted. */
    return (uint32_t)table_value * (FD_M2_SINE_PERIOD - 1u) * fade_ms /
        (2000u * (FD_M2_SINE_ALIGN_US / 1000u));
}

/* Preserve all three ideal line-to-line differences. Clamp the minimum
 * leg with CCR=0 when the other pulses clear dead time. Near a minimum-leg
 * change, raise all three so a tiny second pulse does not disappear. */
static inline void fdM2SineCompares(const uint16_t raw[3], uint16_t floor,
                                   uint16_t compares[3])
{
    uint16_t lowest = raw[0], next = raw[1];
    if (next < lowest) {
        lowest = raw[1];
        next = raw[0];
    }
    if (raw[2] < lowest) {
        next = lowest;
        lowest = raw[2];
    } else if (raw[2] < next) {
        next = raw[2];
    }
    uint16_t common = next - lowest >= floor ? 0u : floor;
    for (unsigned i = 0; i < 3u; i++) {
        compares[i] = raw[i] - lowest + common;
    }
}

/* AM32 2.16 maps internal input 48..120 to 7000/poles..810/poles us/deg.
 * Keep the slow endpoint and cap the fast endpoint to the carrier here. */
static inline uint32_t fdM2SineRequestedRate(uint16_t input)
{
    if (input > 120u) input = 120u;
    if (input < 48u) input = 48u;
    uint32_t period = FD_M2_SINE_SLOW_US -
        (input - 48u) * (FD_M2_SINE_SLOW_US - 810u / 24u) / 72u;
    if (period < FD_M2_SINE_MIN_US) period = FD_M2_SINE_MIN_US;
    return 1000000u / period; /* millidegrees per ms */
}

static inline uint32_t fdM2SineSlewRate(uint32_t rate, uint32_t target)
{
    /* Ramp takes about 0.85 s from 48 to 331 mechanical RPM. */
    const uint32_t rate_per_ms = 24u;
    if (target > rate) return target - rate > rate_per_ms ? rate + rate_per_ms : target;
    return rate - target > rate_per_ms ? rate - rate_per_ms : target;
}
