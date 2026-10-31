#pragma once
#include <stdint.h>

/* Three outcome classes, bounded RAM, no drive policy changes.
 * Default: ADC tail. ACQ_LOG: non-overwriting 300 ms acquisition prefix.
 * EDGE_LOG: tail frozen 12 ms after first accepted edge, with exact trigger.
 * Samples decimated by TIME, event ring retains accepted edges until wrapped.
 * These are software-consumed sequential ADC results, not scope samples. */
#define FD_WAVE_MAGIC 0x46535731u
#define FD_WAVE_NONE 0xffffffffu
#define FD_WAVE_SAMPLES 256u
#define FD_WAVE_EVENTS 96u
#define FD_WAVE_EDGE_POST_TICKS 1920000u /* 12 ms after first accepted ADC edge */
/* ABI2/variant23 reuses the same arrays, no additional RAM.
 * samples_stored packs prefix count[15:0], ring write count[31:16].
 * samples[0..127]: immutable <=50ms prefix, >=400us spacing.
 * samples[128..255]: >=100us ring, ends <=6ms after first acceptance
 * (or earlier handoff/feedback timeout); no-edge ring ends at300ms.
 * events_ring: first96 events, events counts ALL attempts (suffix omitted). */
#define FD_WAVE_PREFIX_TICKS 64000u
#define FD_WAVE_PREFIX_END 8000000u
#define FD_WAVE_SPLIT_POST 960000u
#ifdef TARGET_STARTUP_ACQ_LOG
#define FD_WAVE_SAMPLE_TICKS 192000u /* >=1200 us, full acquisition in 256 slots */
#else
#define FD_WAVE_SAMPLE_TICKS 16000u /* >=100 us between stored samples */
#endif
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
#ifdef TARGET_FD_DENSE_LOG
typedef struct {
    uint32_t at;
    uint16_t va, vb, vc, pwm, trigger;
    uint8_t meta, state;
} FdDenseSample;
typedef struct {
    uint32_t magic, abi, variant, clock_hz, size, command, active_bank;
    uint32_t no_lock_bank, lost_bank, success_bank, trials_seen;
    uint32_t rejected_commands, max_finish_cycles, reserved[3];
    uint32_t state, trial_id, started, ended, reason, final_stage, handoff_at;
    uint32_t seen, stored, events, no_fresh, accepted, timeouts;
    uint32_t max_sample_cycles, max_event_cycles, last_at;
    FdDenseSample samples[1280];
    FdWaveEvent events_ring[48];
} FdStartupWave;
#else
typedef struct {
    uint32_t magic, abi, variant, clock_hz, size, command, active_bank;
    uint32_t no_lock_bank, lost_bank, success_bank, trials_seen;
    uint32_t rejected_commands, max_finish_cycles, reserved[3];
    FdWaveBank banks[3];
} FdStartupWave;
#endif
extern volatile FdStartupWave fd_startup_wave;
