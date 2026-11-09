#pragma once
#include <stdint.h>
/* Event-only observation: never used for a drive/control decision.
 * Legacy300ms/2retries keeps ABI1/168B; other bounded profiles use ABI2. */
#define FD_SR_DIAG_ABI ((FD6288_SECOND_ALIGN_MS == 300 && FD_SR_MAX_ATTEMPTS == 3u) ? 1u : 2u)
typedef struct {
    uint32_t begin_at, launch_at, first_edge_at, last_edge_at, qualified_edges;
    uint32_t fail_at, off_at, end_at, failure_kind, handoff_at;
} FdStartRetryRow;
typedef struct {
    uint32_t magic, abi, trial, attempts, failures, stage, confidence;
    uint32_t stop_reason, successful_attempt, finished_at, max_attempts, total_limit;
    FdStartRetryRow rows[FD_SR_MAX_ATTEMPTS];
} FdStartRetryDiagnostics;
_Static_assert(sizeof(FdStartRetryDiagnostics) == 48u + 40u * FD_SR_MAX_ATTEMPTS, "startup retry summary ABI");
