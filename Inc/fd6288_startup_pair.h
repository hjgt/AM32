#pragma once
#include <stdint.h>

/* S2D diagnostic ABI 1. Two banks retain first failed completed trial and
 * first full closed-loop observation, NOT two motor-control state machines.
 * 72 sectors cover 30 ms alignment + <=270 ms at >=4 ms/sector (<=69).
 * Counts/timestamps describe software-consumed samples, not rotor position. */
#define FD_PAIR_MAGIC 0x46535031u
#define FD_PAIR_SECTORS 72u
#define FD_PAIR_NONE 0xffffffffu
#define FD_PAIR_HYSTERESIS 8
enum { FD_PAIR_EMPTY, FD_PAIR_ACTIVE, FD_PAIR_ABORTED, FD_PAIR_FAILURE,
    FD_PAIR_SUCCESS };
enum { FD_PAIR_SATURATED = 1u, FD_PAIR_PHASE_MISMATCH = 2u };
typedef struct {
    /* Cycle offsets from bank.started; NONE means not observed. */
    uint32_t began, ended, last_sample, first_pre, first_post;
    uint32_t forward_at, reverse_at, accepted_at;
    uint16_t first_adc[3], last_adc[3], min_adc[3], max_adc[3];
    int16_t directed_min, directed_max, directed_first, directed_last;
    uint16_t reasons[10];
    uint16_t samples, no_fresh, forward_crossings, reverse_crossings;
    uint16_t duty_first, duty_last, pwm_first, pwm_last, trigger_first, trigger_last;
    uint16_t input_first, input_last;
    int16_t current_max;
    uint16_t voltage_min, commanded_us;
    uint8_t step, flags, stage, decision, sector_percent, confidence;
    uint16_t valid_samples, issues;
    uint32_t reserved;
} FdPairSector;
typedef struct {
    uint32_t state, trial_id, started, ended, reason, final_stage, sectors;
    uint32_t overflow, handoff_at, accepted_total, closed_zc, timeouts;
    uint32_t max_sample_cycles, max_event_cycles, max_boundary_cycles, freeze_reason;
    FdPairSector records[FD_PAIR_SECTORS];
} FdPairBank;
typedef struct {
    uint32_t magic, abi, variant, clock_hz, size, command;
    uint32_t active_bank, failure_bank, success_bank, trials_seen, rejected_commands;
    uint32_t max_finish_cycles, reserved[4];
    FdPairBank banks[2];
} FdStartupPair;
extern volatile FdStartupPair fd_startup_pair;
