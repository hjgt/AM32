#pragma once
#include <stdint.h>
#include "fd6288_diagnostic_switch.h"
#ifndef FD6288_M2_HELI_RISE_MS
#define FD6288_M2_HELI_RISE_MS 3000u
#endif
/* Cycles at 160 MHz. One positive-output slew, not another throttle target. */
enum { FD_HELI_RISE, FD_HELI_TARGET, FD_HELI_BAD_EDGE,
    FD_HELI_OVERDUE, FD_HELI_STOPPED };
typedef struct {
#ifdef TARGET_FD_M2_PURE
    uint32_t at, last_edge, period, turn_sum, turn_edges, last_step, healthy;
    uint32_t output, credit, reason, active;
#else
    uint32_t magic, abi, trial, rise_ms, at, last_edge, period;
    uint32_t turn_sum, turn_edges, last_step, good_turns, bad_turns, healthy;
    uint32_t output, requested, credit, reason, holds;
    uint32_t first_hold_at, first_hold_duty, first_hold_age, first_hold_period, first_hold_reason, active;
#endif
} FdM2HeliStart;
#ifndef TARGET_FD_M2_PURE
_Static_assert(sizeof(FdM2HeliStart)==96u,"heli start RAM ABI");
#endif

static inline void fdM2HeliBegin(FdM2HeliStart *s, uint32_t trial,
    uint32_t now, uint32_t edge, uint32_t period, uint32_t step, uint32_t duty)
{
    (void)trial;
    *s=(FdM2HeliStart){.at=now,.last_edge=edge,.period=period,
        .last_step=step,.healthy=1u,.output=duty,.active=1u};
    FD_M2_DIAG(s->magic=0x46485331u;s->abi=1u;s->trial=trial;
        s->rise_ms=FD6288_M2_HELI_RISE_MS;s->requested=duty);
}

static inline void fdM2HeliEdge(FdM2HeliStart *s, uint32_t now,
    uint32_t step, uint32_t forward)
{
    if (!s->active) return;
    uint32_t gap=now-s->last_edge, p=s->period;
    uint32_t expected=forward?(s->last_step==6u?1u:s->last_step+1u):
        (s->last_step==1u?6u:s->last_step-1u);
    s->last_edge=now;s->last_step=step;
    if (step!=expected || !p || gap<p/2u || gap>p+p*3u/4u+13334u) {
        s->healthy=0u;s->turn_sum=s->turn_edges=0u;FD_M2_DIAG(s->bad_turns++);
        return;
    }
    s->turn_sum+=gap;
    if (++s->turn_edges==6u) {
        uint32_t turn=s->turn_sum, reference=p*6u;
        if (turn>=reference-reference/4u && turn<=reference+reference/4u) {
            s->period=turn/6u;s->healthy=1u;FD_M2_DIAG(s->good_turns++);
        } else {s->healthy=0u;FD_M2_DIAG(s->bad_turns++);}
        s->turn_sum=s->turn_edges=0u;
    }
}

/* Called with the existing timer/ADC writers excluded. No waiting or I/O.
 * Discard missed service time (>2ms), holds and decreases: no catch-up jump. */
static inline uint16_t fdM2HeliDuty(FdM2HeliStart *s, uint32_t now,
    uint16_t requested, uint16_t applied)
{
    uint32_t elapsed=now-s->at;s->at=now;
    if (applied<s->output) s->credit=0u; /* a protection cap won last tick */
    s->output=applied;FD_M2_DIAG(s->requested=requested);
    if (!s->active) return requested;
    if (requested<=applied) {
        s->credit=0u;s->reason=FD_HELI_TARGET;s->output=requested;
        return requested;
    }
    uint32_t age=now-s->last_edge;
    uint32_t reason=!s->healthy?FD_HELI_BAD_EDGE:
        (age>s->period+s->period/2u+13334u?FD_HELI_OVERDUE:FD_HELI_RISE);
    if (reason!=FD_HELI_RISE) {
        if (s->reason!=FD_HELI_BAD_EDGE && s->reason!=FD_HELI_OVERDUE) {
            FD_M2_DIAG(
            if (!s->holds) {
                s->first_hold_at=now;s->first_hold_duty=applied;
                s->first_hold_age=age;s->first_hold_period=s->period;s->first_hold_reason=reason;
            }
            s->holds++;
            );s->turn_sum=s->turn_edges=0u;
        }
        s->healthy=0u;s->credit=0u;s->reason=reason;
        return applied;
    }
    s->reason=FD_HELI_RISE;
    if (elapsed>320000u) elapsed=0u;
    const uint32_t span=FD6288_M2_HELI_RISE_MS*160000u;
    uint32_t credit=s->credit+elapsed*1600u;
    uint32_t output=applied+credit/span;
    s->credit=credit%span;
    if (output>=requested) {output=requested;s->credit=0u;s->reason=FD_HELI_TARGET;}
    s->output=output;
    return (uint16_t)output;
}
