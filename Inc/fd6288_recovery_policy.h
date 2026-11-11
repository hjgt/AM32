#pragma once
#include <stdint.h>
/* OFFLINE prototype, deliberately NOT wired into MCU code. Units: us and
 * native duty /2000. A future adapter must serialize epochs/actual commutations,
 * apply duty cap before any probe, and keep all electrical safety gates.
 * Outputs are requests, never proof that a physical commutation took place.
 * Experimental constants are host-test boundaries, not validated motor limits. */
enum { FD_REC_TRACK, FD_REC_SEARCH, FD_REC_RAMP, FD_REC_STOPPED };
enum { FD_REC_NONE, FD_REC_ENTER, FD_REC_PROBE, FD_REC_REGAINED,
       FD_REC_FINISHED, FD_REC_STOP, FD_REC_CANCEL };
enum { FD_REC_MIN_PERIOD = 200u, FD_REC_MAX_PERIOD = 2000u,
       FD_REC_STABLE_EDGES = 48u, FD_REC_STABLE_US = 20000u,
       FD_REC_MAX_US = 10000u, FD_REC_REAL_EDGES = 6u,
       FD_REC_MAX_PROBES = 2u, FD_REC_RELEASE_US = 20000u };
typedef struct {
    uint32_t last_real, stable_since, period, started, budget, probe_at, ramp_at;
    uint16_t entry_duty, limited_duty;
    uint8_t state, have_real, stable_edges, eligible, used, probes, streak;
} FdRecoveryPolicy;
static inline void fdRecoveryReset(FdRecoveryPolicy *s)
{ *s = (FdRecoveryPolicy){0}; }
static inline uint8_t fdRecoveryStop(FdRecoveryPolicy *s)
{ s->state = FD_REC_STOPPED; return FD_REC_STOP; }
/* Run before tick/edge. Zero cancels and resets the per-command budget.
 * Direction/epoch/coast incompatibility must be supplied as fault by adapter. */
static inline uint8_t fdRecoveryGate(FdRecoveryPolicy *s, uint8_t commanded,
                                    uint8_t fault)
{
    if (fault) return fdRecoveryStop(s);
    if (!commanded) { fdRecoveryReset(s); return FD_REC_CANCEL; }
    return s->state == FD_REC_STOPPED ? FD_REC_STOP : FD_REC_NONE;
}
static inline uint8_t fdRecoveryExpired(const FdRecoveryPolicy *s, uint32_t now)
{ return (uint32_t)(now - s->started) >= s->budget; }
/* Call ONLY for qualified physical feedback, never for a probe/timer expiry. */
static inline uint8_t fdRecoveryRealEdge(FdRecoveryPolicy *s, uint32_t now)
{
    if (s->state == FD_REC_STOPPED) return FD_REC_STOP;
    if (!s->have_real) {
        s->have_real = 1u; s->last_real = s->stable_since = now;
        s->stable_edges = 1u; return FD_REC_NONE;
    }
    uint32_t elapsed = now - s->last_real;
    if (!elapsed) return FD_REC_NONE; /* duplicate cannot earn confidence */
    if (s->state == FD_REC_SEARCH) {
        if (fdRecoveryExpired(s, now)) return fdRecoveryStop(s);
        /* First reacquired edge is a seed, not a measured real-edge pair. */
        if (!s->streak || elapsed < s->period / 2u ||
            elapsed > s->period + s->period / 2u) s->streak = 1u;
        else s->streak++;
        s->last_real = now;
        s->probe_at = now + s->period + s->period / 2u;
        if (s->streak == FD_REC_REAL_EDGES) {
            s->state = FD_REC_RAMP; s->ramp_at = now;
            return FD_REC_REGAINED;
        }
        return FD_REC_NONE;
    }
    if (s->state == FD_REC_RAMP) {
        if (elapsed < s->period / 2u || elapsed > s->period + s->period / 2u)
            return fdRecoveryStop(s);
        s->last_real = now;
        return FD_REC_NONE;
    }
    uint8_t plausible = elapsed >= FD_REC_MIN_PERIOD && elapsed <= FD_REC_MAX_PERIOD &&
        (!s->period || (elapsed >= s->period / 2u && elapsed <= s->period + s->period / 2u));
    s->last_real = now;
    if (!plausible) {
        s->stable_since = now; s->stable_edges = 1u; s->eligible = 0u;
        s->period = 0u; return FD_REC_NONE;
    }
    s->period = elapsed;
    if (s->stable_edges < FD_REC_STABLE_EDGES) s->stable_edges++;
    if (s->stable_edges >= FD_REC_STABLE_EDGES &&
        (uint32_t)(now - s->stable_since) >= FD_REC_STABLE_US) s->eligible = 1u;
    return FD_REC_NONE;
}
static inline uint8_t fdRecoveryTick(FdRecoveryPolicy *s, uint32_t now,
                                     uint16_t duty, uint16_t minimum_valid_duty)
{
    if (s->state == FD_REC_STOPPED) return FD_REC_STOP;
    if (s->state == FD_REC_TRACK) {
        if (!s->eligible || !s->period ||
            (uint32_t)(now - s->last_real) < s->period + s->period / 2u)
            return FD_REC_NONE;
        /* Never attempt recovery using an already stale rotor estimate. */
        if (s->used || (uint32_t)(now - s->last_real) >= 2u * s->period)
            return fdRecoveryStop(s);
        uint16_t cap = duty / 2u;
        if (cap > 400u) cap = 400u;
        if (!minimum_valid_duty || cap < minimum_valid_duty) return fdRecoveryStop(s);
        s->entry_duty = duty; s->limited_duty = cap;
        s->used = 1u; s->state = FD_REC_SEARCH; s->started = now;
        s->budget = 12u * s->period;
        if (s->budget > FD_REC_MAX_US) s->budget = FD_REC_MAX_US;
        s->probes = s->streak = 0u;
        s->probe_at = s->last_real + 2u * s->period;
        return FD_REC_ENTER; /* cap first, no blind commutation on entry */
    }
    if (s->state == FD_REC_SEARCH) {
        if (fdRecoveryExpired(s, now)) return fdRecoveryStop(s);
        if ((int32_t)(now - s->probe_at) < 0) return FD_REC_NONE;
        if (s->probes >= FD_REC_MAX_PROBES ||
            (uint32_t)(now - s->probe_at) > s->period / 4u) return fdRecoveryStop(s);
        s->probes++; s->streak = 0u;
        s->probe_at = now + s->period;
        return FD_REC_PROBE; /* at most one request per call, no catch-up loop */
    }
    if ((uint32_t)(now - s->last_real) >= s->period + s->period / 2u)
        return fdRecoveryStop(s);
    if ((uint32_t)(now - s->ramp_at) >= FD_REC_RELEASE_US) {
        s->state = FD_REC_TRACK;
        return FD_REC_FINISHED; /* used remains set until operator zero */
    }
    return FD_REC_NONE;
}
static inline uint16_t fdRecoveryDutyCap(const FdRecoveryPolicy *s, uint32_t now,
                                        uint16_t requested)
{
    if (s->state == FD_REC_STOPPED) return 0u;
    uint32_t cap = requested;
    if (s->state == FD_REC_SEARCH) cap = s->limited_duty;
    if (s->state == FD_REC_RAMP) {
        uint32_t age = now - s->ramp_at;
        if (age > FD_REC_RELEASE_US) age = FD_REC_RELEASE_US;
        cap = s->limited_duty + (uint32_t)(s->entry_duty - s->limited_duty) * age / FD_REC_RELEASE_US;
    }
    return cap < requested ? cap : requested;
}
