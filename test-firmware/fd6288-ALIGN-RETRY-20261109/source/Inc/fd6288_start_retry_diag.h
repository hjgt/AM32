#pragma once
#include <stdint.h>
/* Event-only observation: never used for a drive/control decision. */
typedef struct {
    uint32_t begin_at, launch_at, first_edge_at, last_edge_at, qualified_edges;
    uint32_t fail_at, off_at, end_at, failure_kind, handoff_at;
} FdStartRetryRow;
typedef struct {
    uint32_t magic, abi, trial, attempts, failures, stage, confidence;
    uint32_t stop_reason, successful_attempt, finished_at, max_attempts, total_limit;
    FdStartRetryRow rows[3];
} FdStartRetryDiagnostics;
_Static_assert(sizeof(FdStartRetryDiagnostics) == 168u, "startup retry summary ABI");
