#pragma once
#include <stdint.h>

enum {
    FD_M2_ADC_SAMPLES = 128u,
    FD_M2_ADC_POST_SAMPLES = 16u,
    FD_M2_ADC_BUDGET_CYCLES = 320u, /* 2 us at 160 MHz; recording alone. */
    FD_M2_ADC_ACTIVE = 0u, FD_M2_ADC_POST, FD_M2_ADC_FROZEN,
    FD_M2_ADC_STOPPED, FD_M2_ADC_BUDGET
};
typedef struct {
    uint32_t cycles;
    uint16_t a, b, c, elapsed, period, duty, trigger, cnt;
    uint8_t step, decision, flags, qualification;
} FdM2SineAdcSample;
typedef struct {
    uint32_t magic, abi, trial, count, state;
    uint32_t trigger_at, trigger_step, trigger_elapsed, trigger_period, post_remaining;
    uint32_t max_stage_cycles, max_commit_cycles, max_irq_cycles, irq_count, first_at;
    uint32_t budget_trips, reserved[4];
} FdM2SineAdcHeader;
typedef struct {
    FdM2SineAdcHeader header;
    FdM2SineAdcSample samples[FD_M2_ADC_SAMPLES];
} FdM2SineAdcCapture;
_Static_assert(sizeof(FdM2SineAdcSample) == 24u, "ADC sample ABI");
_Static_assert(sizeof(FdM2SineAdcCapture) == 3152u, "ADC ring ABI");

extern volatile FdM2SineAdcCapture fd_m2_sine_adc_capture;
extern volatile uint8_t fd_m2_sine_adc_enabled;
extern uint8_t fd_m2_sine_adc_pending_valid;
extern uint32_t fd_m2_sine_adc_origin;
extern FdM2SineAdcSample fd_m2_sine_adc_pending;

/* All callers already own the ADC IRQ or the firmware's existing IRQ mask.
 * No peripheral access, allocation, loops or controller state changes here. */
static inline void fdM2SineAdcReset(uint32_t trial, uint32_t origin)
{
    fd_m2_sine_adc_enabled = fd_m2_sine_adc_pending_valid = 0u;
    fd_m2_sine_adc_origin = origin;
    fd_m2_sine_adc_capture.header = (FdM2SineAdcHeader){
        .magic = 0x464d4431u, .abi = 1u, .trial = trial,
        .trigger_at = UINT32_MAX, .first_at = UINT32_MAX
    };
}

static inline void fdM2SineAdcStage(FdM2SineAdcSample sample)
{
    if (!fd_m2_sine_adc_enabled) return;
    fd_m2_sine_adc_pending = sample;
    fd_m2_sine_adc_pending_valid = 1u;
}

static inline void fdM2SineAdcCost(uint32_t cycles, uint8_t stage)
{
    volatile FdM2SineAdcHeader *h = &fd_m2_sine_adc_capture.header;
    if (stage) {
        if (cycles > h->max_stage_cycles) h->max_stage_cycles = cycles;
    } else if (cycles > h->max_commit_cycles) h->max_commit_cycles = cycles;
    if (cycles > FD_M2_ADC_BUDGET_CYCLES) {
        h->budget_trips++;
        h->state = FD_M2_ADC_BUDGET;
        fd_m2_sine_adc_enabled = fd_m2_sine_adc_pending_valid = 0u;
    }
}

static inline void fdM2SineAdcCommit(void)
{
    if (!fd_m2_sine_adc_pending_valid) return;
    fd_m2_sine_adc_pending_valid = 0u;
    volatile FdM2SineAdcHeader *h = &fd_m2_sine_adc_capture.header;
    if (h->state >= FD_M2_ADC_FROZEN && h->state != FD_M2_ADC_STOPPED) return;
    FdM2SineAdcSample sample = fd_m2_sine_adc_pending;
    sample.cycles -= fd_m2_sine_adc_origin;
    uint32_t count = h->count;
    fd_m2_sine_adc_capture.samples[count & (FD_M2_ADC_SAMPLES - 1u)] = sample;
    if (!count) h->first_at = sample.cycles;
    h->count = count + 1u;
    if (h->state == FD_M2_ADC_STOPPED) return;
    if (h->state == FD_M2_ADC_POST) {
        if (--h->post_remaining == 0u) {
            h->state = FD_M2_ADC_FROZEN;
            fd_m2_sine_adc_enabled = 0u;
        }
    } else if (sample.period &&
               sample.elapsed > sample.period + sample.period / 2u) {
        /* Freeze the first long interval at any speed, before the controller
         * can fold a late crossing into its smoothed period. Keep 16 groups
         * afterwards, so a late accepted edge is visible as well. The low-speed
         * handoff needs this too: otherwise its growing period prevents the
         * former <=1 ms gate from ever preserving the initial lost edge. */
        h->trigger_at = sample.cycles;
        h->trigger_step = sample.step;
        h->trigger_elapsed = sample.elapsed;
        h->trigger_period = sample.period;
        h->post_remaining = FD_M2_ADC_POST_SAMPLES;
        h->state = FD_M2_ADC_POST;
    }
}

static inline void fdM2SineAdcStop(void)
{
    fd_m2_sine_adc_enabled = 0u;
    if (fd_m2_sine_adc_capture.header.state < FD_M2_ADC_FROZEN)
        fd_m2_sine_adc_capture.header.state = FD_M2_ADC_STOPPED;
    /* An in-flight IRQ may still commit its one pending pre-stop sample. */
}
