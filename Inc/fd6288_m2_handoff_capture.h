#pragma once

/* Capture the most recent ADC and command events after actual handoff. */
enum { FD_M2_RUN_CAPACITY = 96u, FD_M2_RUN_SWITCH = 240u, FD_M2_RUN_FEEDBACK = 241u };
typedef struct {
    uint32_t trial, started, active, next, count, sector_samples;
    uint32_t switches, feedbacks, seed_ticks, last_startup_ticks;
    uint32_t stopped, stop_reason, stop_step, stop_timer_ticks, max_cycles;
    FdEdgeSample records[FD_M2_RUN_CAPACITY];
} FdM2HandoffCapture;
_Static_assert(sizeof(FdM2HandoffCapture) == 2364u, "M2 handoff capture ABI");
extern volatile FdM2HandoffCapture fd_m2_handoff;

static inline void fdM2HandoffReset(uint32_t trial)
{
    fd_m2_handoff.trial = trial;
    fd_m2_handoff.active = fd_m2_handoff.started = fd_m2_handoff.count = 0u;
    fd_m2_handoff.next = fd_m2_handoff.sector_samples = fd_m2_handoff.max_cycles = 0u;
    fd_m2_handoff.switches = fd_m2_handoff.feedbacks = 0u;
    fd_m2_handoff.seed_ticks = fd_m2_handoff.last_startup_ticks = 0u;
    fd_m2_handoff.stopped = fd_m2_handoff.stop_reason = 0u;
    fd_m2_handoff.stop_step = fd_m2_handoff.stop_timer_ticks = 0u;
}

static inline void fdM2HandoffBegin(uint32_t now, uint32_t seed_ticks, uint32_t last_ticks)
{
    fd_m2_handoff.started = now;
    fd_m2_handoff.seed_ticks = seed_ticks;
    fd_m2_handoff.last_startup_ticks = last_ticks;
    fd_m2_handoff.active = 1u;
}

static inline void fdM2HandoffStore(FdEdgeSample record)
{
    unsigned i = fd_m2_handoff.next;
    record.present = 1u;
    record.reserved = (uint16_t)fd_m2_handoff.switches;
    fd_m2_handoff.records[i] = record;
    if (++i == FD_M2_RUN_CAPACITY) i = 0u;
    fd_m2_handoff.next = i;
    if (fd_m2_handoff.count < FD_M2_RUN_CAPACITY) fd_m2_handoff.count++;
}

static inline uint8_t fdM2HandoffKeepAdc(uint8_t step, uint8_t reason)
{
    uint32_t index = fd_m2_handoff.sector_samples++;
    unsigned dense_count = 3u;
#ifdef TARGET_FD_M2_CLOSED_STEP4_DENSE
    if (step == 4u) dense_count = 16u;
#else
    (void)step;
#endif
    return reason == 7u || index < dense_count || (index & 7u) == 0u;
}

static inline void fdM2HandoffSwitch(uint32_t now, uint8_t step, uint16_t interval,
                                      uint16_t compare, uint16_t trigger)
{
    fd_m2_handoff.switches++;
    fd_m2_handoff.sector_samples = 0u;
    fdM2HandoffStore((FdEdgeSample){ .at=now, .step=step, .reason=FD_M2_RUN_SWITCH,
        .cnt=interval, .duty=compare, .trigger=trigger });
}

static inline void fdM2HandoffFeedback(uint32_t now, uint8_t step, uint16_t interval,
                                        uint16_t smoothed, uint16_t delay)
{
    fd_m2_handoff.feedbacks++;
    fdM2HandoffStore((FdEdgeSample){ .at=now, .step=step, .reason=FD_M2_RUN_FEEDBACK,
        .a=interval, .b=smoothed, .c=delay });
}

static inline void fdM2HandoffFinish(uint32_t now, uint8_t reason, uint8_t step, uint32_t timer_ticks)
{
    if (!fd_m2_handoff.active) return;
    fd_m2_handoff.stopped = now;
    fd_m2_handoff.stop_reason = reason;
    fd_m2_handoff.stop_step = step;
    fd_m2_handoff.stop_timer_ticks = timer_ticks;
    fd_m2_handoff.active = 0u;
}
