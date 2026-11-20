#pragma once

#include <stdint.h>

/* Three crossings of the highest phase pair mark the even forward sectors.
 * Repeat each marker after a full electrical turn to measure speed. */
typedef struct {
    uint32_t previous_at, pending_at[3], marker_at[3], cycle[3], last_at;
    uint32_t stable_at, period;
    int16_t previous_delta[3];
    uint8_t confirmations[3], pending_mask, seen_mask, cycle_mask;
    uint8_t have_previous, have_marker, last_marker, intervals, stable;
} FdM2SinePickup;

static inline uint8_t fdM2SineCycleMarker(FdM2SinePickup *s,
    unsigned marker, uint32_t at, uint32_t expected, uint8_t forward)
{
    unsigned next = (s->last_marker + (forward ? 1u : 2u)) % 3u;
    uint32_t gap = at - s->last_at;
    uint32_t two_sectors = 2u * expected;
    if (s->have_marker &&
        (marker != next || gap < two_sectors - two_sectors / 4u ||
         gap > two_sectors + two_sectors / 4u)) {
        s->seen_mask = s->cycle_mask = s->intervals = 0u;
    } else if (s->have_marker && s->intervals < 6u) {
        s->intervals++;
    }
    uint8_t bit = (uint8_t)(1u << marker);
    if (s->seen_mask & bit) {
        uint32_t turn = at - s->marker_at[marker];
        uint32_t expected_turn = 6u * expected;
        if (turn >= expected_turn - expected_turn / 4u &&
            turn <= expected_turn + expected_turn / 4u) {
            s->cycle[marker] = turn;
            s->cycle_mask |= bit;
        } else {
            s->cycle_mask &= (uint8_t)~bit;
        }
    }
    s->marker_at[marker] = at;
    s->seen_mask |= bit;
    s->last_marker = (uint8_t)marker;
    s->last_at = at;
    s->have_marker = 1u;
    if (s->intervals < 5u || s->cycle_mask != 7u) return 0u;

    uint32_t mean = (s->cycle[0] + s->cycle[1] + s->cycle[2]) / 3u;
    for (unsigned i = 0u; i < 3u; i++) {
        if (s->cycle[i] < mean - mean / 8u ||
            s->cycle[i] > mean + mean / 8u) return 0u;
    }
    s->period = (mean + 3u) / 6u;
    s->stable_at = at; /* Interpolated crossing, independent of confirmation delay. */
    static const uint8_t forward_step[3] = {6u, 2u, 4u};
    static const uint8_t reverse_step[3] = {5u, 1u, 3u};
    s->stable = forward ? forward_step[marker] : reverse_step[marker];
    return 1u;
}

static inline uint8_t fdM2SinePickupSample(FdM2SinePickup *s, uint32_t now,
    const uint16_t phase[3], uint32_t expected, uint8_t forward)
{
    static const uint8_t left[3] = {0u, 2u, 1u};
    static const uint8_t right[3] = {1u, 0u, 2u};
    uint32_t gap = now - s->previous_at;
    if (s->have_previous && gap > 250u * 160u) {
        s->pending_mask = s->seen_mask = s->cycle_mask = s->intervals = 0u;
        s->have_marker = s->have_previous = 0u;
    }
    uint8_t ready = 0u;
    for (unsigned i = 0u; i < 3u; i++) {
        int32_t delta = (int32_t)phase[left[i]] - phase[right[i]];
        if (!forward) delta = -delta;
        unsigned third = 3u - left[i] - right[i];
        uint8_t high_pair = phase[left[i]] >= phase[third] + 12u &&
            phase[right[i]] >= phase[third] + 12u &&
            (uint32_t)phase[left[i]] + phase[right[i]] >= 2u * phase[third] + 48u;
        uint8_t bit = (uint8_t)(1u << i);
        int32_t previous = s->previous_delta[i];
        if (s->have_previous && previous < 0 && delta >= 0 && high_pair) {
            uint32_t offset = (uint64_t)gap * (uint32_t)(-previous) /
                (uint32_t)(delta - previous);
            s->pending_at[i] = s->previous_at + offset;
            s->pending_mask |= bit;
            s->confirmations[i] = 0u;
        }
        if (delta < 0 || !high_pair) {
            s->pending_mask &= (uint8_t)~bit;
            s->confirmations[i] = 0u;
        } else if (s->pending_mask & bit) {
            if (delta < 6) {
                s->confirmations[i] = 0u;
            } else if (++s->confirmations[i] >= 2u) {
                s->pending_mask &= (uint8_t)~bit;
                ready = fdM2SineCycleMarker(s, i, s->pending_at[i], expected, forward);
            }
        }
        s->previous_delta[i] = (int16_t)delta;
    }
    s->previous_at = now;
    s->have_previous = 1u;
    return ready;
}
