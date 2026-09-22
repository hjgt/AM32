#pragma once
#include <stdint.h>

/* S3-D: three outcome classes, bounded RAM, no drive policy changes.
 * Tail only: samples decimated by TIME, events retain all accepted edges.
 * These are software-consumed sequential ADC results, not scope samples. */
#define FD_WAVE_MAGIC 0x46535731u
#define FD_WAVE_NONE 0xffffffffu
#define FD_WAVE_SAMPLES 256u
#define FD_WAVE_EVENTS 96u
#define FD_WAVE_SAMPLE_TICKS 16000u /* >=100 us between stored samples */
#define FD_PAIR_HYSTERESIS 8 /* shared detector compile-time contract */
enum { FD_WAVE_EMPTY, FD_WAVE_ACTIVE, FD_WAVE_ABORTED,
    FD_WAVE_NO_LOCK, FD_WAVE_LOST, FD_WAVE_SUCCESS };
#define FD_WAVE_ADC_ACCEPT 13u
typedef struct {
    uint32_t at;
    uint16_t va, vb, vc, elapsed, pwm, trigger;
    uint8_t step, flags, reason, state; /* state: stage[2:0], pre[3], confirmations[7:4] */
} FdWaveSample;
typedef struct {
    uint32_t at;
    uint16_t elapsed, interval, values[3], duty;
    /* values=aux/wait/input for control events, va/vb/vc for ADC_ACCEPT */
    uint8_t step, flags, kind, stage;
} FdWaveEvent;
typedef struct {
    uint32_t state, trial_id, started, ended, reason, final_stage, handoff_at;
    uint32_t samples_seen, samples_stored, events, no_fresh, accepted, closed_zc;
    uint32_t timeouts, max_sample_cycles, max_event_cycles, last_sample_at;
    uint32_t decisions[10], reserved[5];
    FdWaveSample samples[FD_WAVE_SAMPLES];
    FdWaveEvent events_ring[FD_WAVE_EVENTS];
} FdWaveBank;
typedef struct {
    uint32_t magic, abi, variant, clock_hz, size, command, active_bank;
    uint32_t no_lock_bank, lost_bank, success_bank, trials_seen;
    uint32_t rejected_commands, max_finish_cycles, reserved[3];
    FdWaveBank banks[3];
} FdStartupWave;
extern volatile FdStartupWave fd_startup_wave;
