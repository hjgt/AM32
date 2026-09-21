#include "fd6288_startup_trace.h"
#ifdef TARGET_STARTUP_TRACE
#include <stddef.h>
#include "common.h"
#include "peripherals.h"

extern char step, old_routine;
extern volatile char rising;
extern volatile uint32_t commutation_interval, zero_crosses;
extern volatile uint16_t duty_cycle, waitTime, thiszctime;
extern volatile uint8_t heli_coast_active;

_Static_assert(sizeof(FdTraceSample) == 32, "trace sample ABI");
_Static_assert(sizeof(FdTraceEvent) == 32, "trace event ABI");
_Static_assert(offsetof(FdStartupTrace, command) == 20, "trace command ABI");
_Static_assert(offsetof(FdStartupTrace, sample_first) == 104, "trace header ABI");
_Static_assert(sizeof(FdStartupTrace) == 23656, "trace block ABI");
volatile FdStartupTrace fd_startup_trace;
#ifdef TARGET_STARTUP_SEQUENCE
static uint8_t fd_trace_failure_pinned;
void fdTraceTrial(uint32_t id)
{
    /* Called with IRQs masked before energizing. Reuse the one RAM buffer,
     * but never overwrite the first failed trial until explicit manual ARM. */
    if (fd_trace_failure_pinned) return;
    fd_startup_trace.state = FD_TRACE_ARMED;
    fd_startup_trace.capture_id = id;
    fd_startup_trace.start_cycles = fd_startup_trace.end_cycles = 0u;
    fd_startup_trace.end_reason = FD_END_NONE;
    fd_startup_trace.sample_sequence = fd_startup_trace.event_sequence = 0u;
    fd_startup_trace.no_fresh = 0u;
    fd_startup_trace.max_sample_cycles = fd_startup_trace.max_event_cycles = 0u;
    for (uint32_t i = 0u; i < FD_SAMPLE_REASON_COUNT; i++) fd_startup_trace.decisions[i] = 0u;
}
void fdTracePinFailure(void) { fd_trace_failure_pinned = 1u; }
#endif
static uint8_t fdTraceFlags(void)
{
    return (forward ? 1u : 0u) | (rising ? 2u : 0u) | (running ? 4u : 0u) |
        (heli_coast_active ? 8u : 0u) | (old_routine ? 16u : 0u);
}
void fdTraceInit(void)
{
    /* Timebase only: no trace output pins, interrupt or PWM changes. Do not
     * reset CYCCNT, which might also be used by a connected debugger. */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    fd_startup_trace.magic = FD_TRACE_MAGIC;
    fd_startup_trace.abi = 1u;
    fd_startup_trace.variant = FD6288_TRACE_VARIANT;
    fd_startup_trace.clock_hz = 160000000u;
    fd_startup_trace.state = FD_TRACE_ARMED;
}
void fdTraceEvent(uint8_t kind, uint16_t aux)
{
    if (fd_startup_trace.state != FD_TRACE_ACTIVE) return;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uint32_t began = DWT->CYCCNT;
    if (fd_startup_trace.state == FD_TRACE_ACTIVE) {
        uint32_t n = fd_startup_trace.event_sequence + 1u;
        FdTraceEvent e = {
            .sequence = n, .cycles = began,
            .elapsed = (kind == FD_EVENT_ZC) ? thiszctime : INTERVAL_TIMER_COUNT,
            .interval = commutation_interval, .wait = waitTime,
            .duty = duty_cycle, .input = adjusted_input, .current = actual_current,
            .voltage = battery_voltage, .pwm = TIM1->CCR1,
            .zero_crosses = (zero_crosses > 65535u) ? 65535u : zero_crosses,
            .aux = aux, .step = step, .flags = fdTraceFlags(), .kind = kind,
        };
        fd_startup_trace.event_ring[(n - 1u) % FD_TRACE_EVENT_RING] = e;
        if (n <= FD_TRACE_EVENT_FIRST) fd_startup_trace.event_first[n - 1u] = e;
        __DMB();
        fd_startup_trace.event_sequence = n;
        uint32_t cost = DWT->CYCCNT - began;
        if (cost > fd_startup_trace.max_event_cycles) fd_startup_trace.max_event_cycles = cost;
    }
    __set_PRIMASK(mask);
}
void fdTraceSample(uint16_t va, uint16_t vb, uint16_t vc, int16_t margin,
    uint8_t reason, uint8_t pre_seen, uint8_t confirmations)
{
    if (fd_startup_trace.state != FD_TRACE_ACTIVE) return;
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uint32_t began = DWT->CYCCNT;
    if (fd_startup_trace.state == FD_TRACE_ACTIVE) {
        uint32_t n = fd_startup_trace.sample_sequence + 1u;
        FdTraceSample s = {
            .sequence = n, .cycles = began, .elapsed = INTERVAL_TIMER_COUNT,
            .interval = commutation_interval, .va = va, .vb = vb, .vc = vc,
            .margin = margin, .duty = duty_cycle, .pwm = TIM1->CCR1,
            .trigger = TIM1->CCR4, .flags = fdTraceFlags(), .step = step,
            .reason = reason, .pre_seen = pre_seen, .confirmations = confirmations,
        };
        fd_startup_trace.sample_ring[(n - 1u) % FD_TRACE_SAMPLE_RING] = s;
        if (n <= FD_TRACE_SAMPLE_FIRST) fd_startup_trace.sample_first[n - 1u] = s;
        if (reason < FD_SAMPLE_REASON_COUNT) fd_startup_trace.decisions[reason]++;
        __DMB();
        fd_startup_trace.sample_sequence = n;
        uint32_t cost = DWT->CYCCNT - began;
        if (cost > fd_startup_trace.max_sample_cycles) fd_startup_trace.max_sample_cycles = cost;
    }
    __set_PRIMASK(mask);
}
void fdTraceNoFresh(void)
{
    if (fd_startup_trace.state == FD_TRACE_ACTIVE) fd_startup_trace.no_fresh++;
}
void fdTraceFreeze(uint32_t reason)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (fd_startup_trace.state == FD_TRACE_ACTIVE) {
        fdTraceEvent(FD_EVENT_STOP, reason);
        fd_startup_trace.end_cycles = DWT->CYCCNT;
        fd_startup_trace.end_reason = reason;
        __DMB();
        fd_startup_trace.state = FD_TRACE_FROZEN;
    }
    __set_PRIMASK(mask);
}
void fdTraceBegin(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (fd_startup_trace.state == FD_TRACE_ARMED) {
        fd_startup_trace.start_cycles = DWT->CYCCNT;
        fd_startup_trace.state = FD_TRACE_ACTIVE;
        fdTraceEvent(FD_EVENT_START, 0u);
    }
    __set_PRIMASK(mask);
}
void fdTraceControlTick(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    uint32_t command = fd_startup_trace.command;
    if (command != 0u) {
        fd_startup_trace.command = 0u;
        if (command == FD_TRACE_ARM_COMMAND && adjusted_input == 0u &&
            newinput == 0u && input == 0u && !running && !heli_coast_active) {
#ifdef TARGET_STARTUP_SEQUENCE
            fd_trace_failure_pinned = 0u;
#endif
            /* Only bounded header clearing; buffers are indexed by sequence
             * and need no large memset in the control ISR. */
            fd_startup_trace.state = FD_TRACE_ARMED;
            fd_startup_trace.capture_id++;
            fd_startup_trace.start_cycles = fd_startup_trace.end_cycles = 0u;
            fd_startup_trace.end_reason = FD_END_NONE;
            fd_startup_trace.sample_sequence = fd_startup_trace.event_sequence = 0u;
            fd_startup_trace.no_fresh = 0u;
            fd_startup_trace.max_sample_cycles = fd_startup_trace.max_event_cycles = 0u;
            for (uint32_t i = 0u; i < FD_SAMPLE_REASON_COUNT; i++) fd_startup_trace.decisions[i] = 0u;
        } else if (command == FD_TRACE_FREEZE_COMMAND) {
            fdTraceFreeze(FD_END_REQUEST); /* freezes RAM logging, NOT the CPU */
        } else {
            fd_startup_trace.rejected_commands++;
        }
    }
    if (fd_startup_trace.state == FD_TRACE_ACTIVE) {
        if (adjusted_input == 0u) fdTraceFreeze(FD_END_ZERO);
        else if ((uint32_t)(DWT->CYCCNT - fd_startup_trace.start_cycles) >=
#ifdef TARGET_STARTUP_SEQUENCE_V2
            320000000u) /* S2 motor window <= 1.3 s, retain final stop event */
#else
            160000000u)
#endif
            fdTraceFreeze(FD_END_DEADLINE); /* logging cap only; NO motor action */
    }
    __set_PRIMASK(mask);
}
#endif
