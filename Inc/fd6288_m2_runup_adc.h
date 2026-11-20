#pragma once

enum { FD_M2_RUNUP_ADC_FIRST_STEP = 4u, FD_M2_RUNUP_ADC_STEPS = 3u,
       FD_M2_RUNUP_ADC_SAMPLES = 18u };
typedef struct {
    uint32_t trial, sector_at, active_step, seen;
    uint16_t counts[FD_M2_RUNUP_ADC_STEPS], reserved;
    FdEdgeSample samples[FD_M2_RUNUP_ADC_STEPS][FD_M2_RUNUP_ADC_SAMPLES];
} FdM2RunupAdcCapture;
_Static_assert(sizeof(FdM2RunupAdcCapture) == 1320u, "M2 run-up ADC ABI");
extern volatile FdM2RunupAdcCapture fd_m2_runup_adc;

static inline void fdM2RunupAdcReset(uint32_t trial)
{
    fd_m2_runup_adc.trial = trial;
    fd_m2_runup_adc.active_step = 0u;
    for (unsigned i = 0; i < FD_M2_RUNUP_ADC_STEPS; i++) fd_m2_runup_adc.counts[i] = 0u;
}

static inline void fdM2RunupAdcBegin(uint8_t step, uint32_t now)
{
    fd_m2_runup_adc.active_step = step;
    fd_m2_runup_adc.sector_at = now;
    fd_m2_runup_adc.seen = 0u;
}

static inline uint8_t fdM2RunupAdcKeep(uint8_t step, uint8_t reason)
{
    if (!fd_m2_runup_adc.active_step || step != fd_m2_runup_adc.active_step) return 0u;
    unsigned sector = step - FD_M2_RUNUP_ADC_FIRST_STEP;
    unsigned index = fd_m2_runup_adc.seen++;
    return fd_m2_runup_adc.counts[sector] < FD_M2_RUNUP_ADC_SAMPLES &&
        (index < 8u || (index & 15u) == 0u || reason == 7u);
}

static inline void fdM2RunupAdcStore(FdEdgeSample record)
{
    unsigned sector = record.step - FD_M2_RUNUP_ADC_FIRST_STEP;
    unsigned i = fd_m2_runup_adc.counts[sector];
    record.at -= fd_m2_runup_adc.sector_at;
    record.present = 1u;
    fd_m2_runup_adc.samples[sector][i] = record;
    fd_m2_runup_adc.counts[sector] = i + 1u;
}
