#pragma once
#include <stdint.h>
#include "targets.h"
#include "fd6288_diagnostic_switch.h"
#ifdef TARGET_STARTUP_POWER_BENCH
#include "fd6288_startup_power_policy.h"
#include "fd6288_startup_sequence.h"
#define FD_POWER_SUMMARIES 16u
#define FD_POWER_BENCH_MAGIC 0x46504231u
enum {
    FD_BENCH_WINDOW = 1, FD_BENCH_NATIVE_STUCK, FD_BENCH_SAFETY_CAP,
    FD_BENCH_NOT_RUNNING, FD_BENCH_BAD_CONFIG, FD_BENCH_NO_ZERO,
    FD_BENCH_OPERATOR_ZERO, FD_BENCH_NO_LOCK, FD_BENCH_LOST_FEEDBACK,
    FD_BENCH_COMM_LATE, FD_BENCH_COMM_INVALID
};
typedef struct {
    uint32_t id, elapsed_cycles, accepted, timeouts;
    uint32_t first_observed_cycles, fifth_observed_cycles;
    int16_t peak_filtered_current;
    uint16_t min_voltage, max_duty;
    uint8_t reason, reserved;
} FdPowerSummary;
typedef struct {
    uint32_t magic, abi, extra, trials, completed, stops;
    uint32_t inhibit, last_reason, active_id, floor;
#ifndef TARGET_FD_NO_DIAGNOSTICS
    FdPowerSummary records[FD_POWER_SUMMARIES];
#endif
} FdPowerDiagnostics;
extern volatile FdPowerDiagnostics fd_power_bench;
#endif
