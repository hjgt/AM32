#pragma once
#include <stdint.h>
/* Independent switch: no ADC samples, no polling writes, no control reads.
 * Only native-early experiments opt in. Set to 0 for a zero-recorder build. */
#ifndef FD6288_LIGHT_LOG
#define FD6288_LIGHT_LOG 1
#endif
#if FD6288_LIGHT_LOG && defined(TARGET_FD_NATIVE_EARLY)
typedef struct {
    uint32_t id, started, first_edge, handoff, edges, retries;
    uint32_t max_streak, reason, stage, input_at_stop, interval_at_stop;
    int32_t current_at_stop;
} FdLightRecord;
typedef struct {
    uint32_t magic, abi, trials, completed;
    FdLightRecord work, records[32];
} FdLightLog;
static volatile FdLightLog fd_light_log = { .magic = 0x464c4731u, .abi = 1u };
static inline void fdLightBegin(uint32_t now)
{
    fd_light_log.work = (FdLightRecord){ .id = ++fd_light_log.trials,
        .started = now, .first_edge = UINT32_MAX, .handoff = UINT32_MAX };
}
static inline void fdLightEdge(uint32_t now, uint32_t streak)
{
    fd_light_log.work.edges++;
    if (fd_light_log.work.first_edge == UINT32_MAX)
        fd_light_log.work.first_edge = now - fd_light_log.work.started;
    if (streak > fd_light_log.work.max_streak)
        fd_light_log.work.max_streak = streak;
}
static inline void fdLightHandoff(uint32_t now)
{ fd_light_log.work.handoff = now - fd_light_log.work.started; }
static inline void fdLightRetry(void) { fd_light_log.work.retries++; }
static inline void fdLightFinish(uint32_t reason, uint32_t stage,
    uint32_t command, uint32_t interval, int32_t current)
{
    if (!fd_light_log.work.id) return;
    fd_light_log.work.reason = reason;
    fd_light_log.work.stage = stage;
    fd_light_log.work.input_at_stop = command;
    fd_light_log.work.interval_at_stop = interval;
    fd_light_log.work.current_at_stop = current;
    fd_light_log.records[(fd_light_log.work.id - 1u) % 32u] = fd_light_log.work;
    fd_light_log.completed++;
    fd_light_log.work.id = 0u;
}
#else
#define fdLightBegin(...) ((void)0)
#define fdLightEdge(...) ((void)0)
#define fdLightHandoff(...) ((void)0)
#define fdLightRetry(...) ((void)0)
#define fdLightFinish(...) ((void)0)
#endif
