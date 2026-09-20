#pragma once
#include <stdint.h>
#include "targets.h"

#ifndef FD6288_STARTUP_TIMING_FIX
#define FD6288_STARTUP_TIMING_FIX 0
#endif
#ifndef FD6288_TRACE_VARIANT
#define FD6288_TRACE_VARIANT FD6288_STARTUP_TIMING_FIX
#endif

#ifdef TARGET_STARTUP_TRACE
#define FD_TRACE_MAGIC 0x46535431u /* FST1, ABI 1 */
#define FD_TRACE_SAMPLE_FIRST 64u
#define FD_TRACE_SAMPLE_RING 512u
#define FD_TRACE_EVENT_FIRST 32u
#define FD_TRACE_EVENT_RING 128u
#define FD_TRACE_ARM_COMMAND 0x41524d31u
#define FD_TRACE_FREEZE_COMMAND 0x53544f50u
enum { FD_TRACE_ARMED = 1, FD_TRACE_ACTIVE, FD_TRACE_FROZEN };
enum { FD_END_NONE, FD_END_ZERO, FD_END_DEADLINE, FD_END_FAULT, FD_END_REQUEST,
    FD_END_BENCH };
enum {
    FD_SAMPLE_INVALID, FD_SAMPLE_BLANK, FD_SAMPLE_PRE, FD_SAMPLE_HYST,
    FD_SAMPLE_EARLY, FD_SAMPLE_UNARMED, FD_SAMPLE_CONFIRM, FD_SAMPLE_ACCEPT,
    FD_SAMPLE_REPORTED, FD_SAMPLE_COAST, FD_SAMPLE_REASON_COUNT
};
enum {
    FD_EVENT_START = 1, FD_EVENT_COMMUTATE, FD_EVENT_ZC, FD_EVENT_TIMEOUT,
    FD_EVENT_DESYNC, FD_EVENT_STOP, FD_EVENT_FAULT, FD_EVENT_BENCH_STOP
};
/* Flags: bit0 forward, bit1 rising, bit2 running, bit3 Coast, bit4 old_routine.
 * ADC values are software-consumed injected results, NOT edge timestamps or
 * simultaneous ADC samples. No claim of mechanical direction is implied. */
typedef struct {
    uint32_t sequence, cycles;
    uint16_t elapsed, interval, va, vb, vc;
    int16_t margin;
    uint16_t duty, pwm, trigger, flags;
    uint8_t step, reason, pre_seen, confirmations;
} FdTraceSample;
typedef struct {
    uint32_t sequence, cycles;
    uint16_t elapsed, interval, wait, duty, input;
    int16_t current;
    uint16_t voltage, pwm, zero_crosses, aux;
    uint8_t step, flags, kind, reserved;
} FdTraceEvent;
typedef struct {
    uint32_t magic, abi, variant, clock_hz;
    uint32_t state, command, capture_id, start_cycles, end_cycles, end_reason;
    uint32_t sample_sequence, event_sequence, no_fresh;
    uint32_t decisions[FD_SAMPLE_REASON_COUNT];
    uint32_t max_sample_cycles, max_event_cycles, rejected_commands;
    FdTraceSample sample_first[FD_TRACE_SAMPLE_FIRST];
    FdTraceSample sample_ring[FD_TRACE_SAMPLE_RING];
    FdTraceEvent event_first[FD_TRACE_EVENT_FIRST];
    FdTraceEvent event_ring[FD_TRACE_EVENT_RING];
} FdStartupTrace;
extern volatile FdStartupTrace fd_startup_trace;
void fdTraceInit(void);
void fdTraceControlTick(void);
void fdTraceBegin(void);
void fdTraceEvent(uint8_t kind, uint16_t aux);
void fdTraceSample(uint16_t va, uint16_t vb, uint16_t vc, int16_t margin,
    uint8_t reason, uint8_t pre_seen, uint8_t confirmations);
void fdTraceNoFresh(void);
void fdTraceFreeze(uint32_t reason);
#else
#define fdTraceInit() ((void)0)
#define fdTraceControlTick() ((void)0)
#define fdTraceBegin() ((void)0)
#define fdTraceEvent(kind, aux) ((void)0)
#define fdTraceSample(a, b, c, m, r, p, n) ((void)0)
#define fdTraceNoFresh() ((void)0)
#define fdTraceFreeze(reason) ((void)0)
#endif
