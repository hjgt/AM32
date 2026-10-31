#pragma once
#include <stdint.h>
#include "fd6288_diagnostic_switch.h"

/* Experimental G431 @160MHz: DWT cycles, TIM16 ticks = 80 cycles.
 * All deadlines are <32.768ms, safely below the signed half-wrap range. */
static inline uint32_t fdAsyncRemaining(uint32_t now, uint32_t deadline)
{
    int32_t delta = (int32_t)(deadline - now);
    return delta > 0 ? ((uint32_t)delta + 79u) / 80u : 1u;
}
static inline uint8_t fdAsyncTooLate(uint32_t now, uint32_t deadline)
{
    /* Experimental hard stop beyond one former polling period, not a
     * validated timing-error tolerance. Never silently roll to next sector. */
    return (int32_t)(now - deadline) > 8000; /* 50us */
}
typedef struct {
    uint32_t abi, irq_count, irq_last_at, irq_max_cycles;
    uint32_t accepted_at, deadline, executed_at, late_max_cycles;
    uint32_t scheduled, executed, cancelled, stale, late_stop;
    uint32_t epoch, request_epoch, sample_collisions;
    uint32_t pending, request_coast, request_forward;
} FdAsyncDiagnostics;
extern volatile FdAsyncDiagnostics fd_async_diag;
void fdAsyncCancel(void);
void fdAdcJeosService(void);
