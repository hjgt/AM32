/*
 * comparator.c
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#include "comparator.h"
#include "common.h"
#include "targets.h"

#ifdef USE_ADC_ZCD
#include "ADC.h"

/*
 * ADC-based Zero Crossing Detection
 * ----------------------------------
 * Hardware comparators are not usable for a shared-neutral 6-step ZCD on
 * this pin layout (PA2 cannot be any comparator plus input, and the PA5
 * neutral candidate reaches only COMP2 minus). So we sample the three BEMF
 * phase dividers with ADC1 and detect the zero crossing in software.
 *
 * The board has NO physical virtual-neutral node (confirmed by multimeter:
 * PA5 is only a 1.0k pulldown, not tied to any phase). The neutral is
 * therefore reconstructed in software as the average of the three phase
 * readings inside getCompOutputLevel().
 *
 * adc_zcd_phase: tracks which phase is currently floating
 *   1 or 4 -> phase C floating
 *   2 or 5 -> phase A floating
 *   3 or 6 -> phase B floating
 */
static uint8_t adc_zcd_phase = 0;

/*
 * Post-commutation blanking (Route C).
 * ------------------------------------
 * Immediately after a commutation the freewheeling diodes and the phase
 * inductance produce a large flyback transient on the just-opened phase.
 * Sampling the BEMF during this window yields false zero crossings. We
 * therefore skip the first ADC_ZCD_BLANK_CALLS getCompOutputLevel() results
 * after each changeCompInput(): during blanking the function returns the last
 * known polarity WITHOUT updating it, so the AM32 zero-cross counter does not
 * advance on transient noise. This complements the existing consecutive-count
 * filter (TARGET_MIN_BEMF_COUNTS).
 *
 * The polling path calls getCompOutputLevel() many times per commutation, so
 * a small count (a few calls) is enough to clear the electrical transient
 * without eating into the real BEMF observation window.
 */
#ifndef ADC_ZCD_BLANK_CALLS
#define ADC_ZCD_BLANK_CALLS 2u /* Experiment B: 3 -> 2, widen observation window, see notes 12.15 */
#endif
/*
 * Experiment D (see notes 12.20): blanking is measured in FRESH injected
 * samples (real PWM periods) rather than in getCompOutputLevel() call count.
 * The main loop polls getCompOutputLevel() far faster than the ADC injected
 * group is re-triggered (TIM1 CH4 once per PWM period, ~41.6us at 24kHz), so a
 * call-counted blank of 2 covered only a few microseconds - far too short to
 * mask the post-commutation freewheeling transient, which lasts on the order
 * of a PWM period. A too-short blank lets the flyback spike be mistaken for a
 * zero crossing, advancing commutation early -> phase error, extra current,
 * and a speed ceiling. Counting real samples makes the blank a deterministic
 * physical time window regardless of loop speed.
 */
#ifndef ADC_ZCD_BLANK_SAMPLES
#define ADC_ZCD_BLANK_SAMPLES 2u /* fresh PWM-period samples to skip after commutation */
#endif
static volatile uint8_t adc_zcd_blank = 0;
static uint8_t          adc_zcd_last_out = 0;

/*
 * Diagnostic snapshot (see chapter 12 of the porting notes).
 * These volatile file-scope variables mirror the last values computed inside
 * getCompOutputLevel() so they can be observed live over SWD with GDB
 * (local variables are not visible after the function returns). They do NOT
 * affect the control logic in any way - remove once debugging is complete.
 *   zcd_dbg_va/vb/vc : last three phase-divider ADC readings
 *   zcd_dbg_neutral  : software neutral = (va+vb+vc)/3
 *   zcd_dbg_bemf     : the floating-phase reading selected this call
 *   zcd_dbg_out      : last returned polarity (1 = bemf>neutral, else 0)
 *   zcd_dbg_calls    : total getCompOutputLevel() call counter
 *   zcd_dbg_flips    : number of times the polarity output changed
 */
volatile uint16_t zcd_dbg_va = 0;
volatile uint16_t zcd_dbg_vb = 0;
volatile uint16_t zcd_dbg_vc = 0;
volatile uint16_t zcd_dbg_neutral = 0;
volatile uint16_t zcd_dbg_bemf = 0;
volatile uint8_t  zcd_dbg_out = 0;
volatile uint8_t  zcd_dbg_phase = 0;
volatile uint32_t zcd_dbg_calls = 0;
volatile uint32_t zcd_dbg_flips = 0;



/*
 * 5-minute diagnostic capture buffer (see §12).
 * getCompOutputLevel() stores one entry every ~1s so the host can read
 * the buffer out over SWD post-test without disrupting the running motor.
 *   dbg_buf[DBG_BUF_SIZE]  — circular buffer (volatile to prevent dead-store
 *                             elimination since the host reads via SWD)
 *   dbg_idx                — write index (stops when full)
 *   dbg_skip               — down-counter, stores when it reaches 0
 */
#define DBG_BUF_SIZE  512

struct dbg_entry {
    uint16_t va, vb, vc, neutral, bemf;
    uint16_t zero_crosses;
    uint16_t ci;        /* commutation_interval */
    uint8_t  step;
    uint8_t  running;
    uint8_t  out;
    uint8_t  phase;
};

static volatile struct dbg_entry dbg_buf[DBG_BUF_SIZE];
static volatile uint16_t         dbg_idx  = 0;
static uint32_t                  dbg_skip = 0;

/* Externals from main.c */
extern uint16_t zero_crosses;
extern uint32_t commutation_interval;
extern uint8_t  running;

/**
 * getCompOutputLevel()
 *
 * Returns 1 when the floating BEMF phase voltage is ABOVE the virtual
 * neutral (equivalent to comparator output HIGH), 0 when below.
 *
 * Called repeatedly from the main-loop polling path (old_routine / bemfcounter).
 */
uint8_t getCompOutputLevel(void)
{
    /*
     * PWM-synchronous virtual-neutral zero-cross detection (Route C).
     *
     * The three BEMF phase dividers are sampled by the ADC1 INJECTED group,
     * hardware-triggered by TIM1 CH4 at the ON-pulse midpoint of the PWM
     * carrier (see ADC.c / peripherals.c / peripherals.h). We simply read the
     * most recent injected results here - NON-BLOCKING, no reconfiguration of
     * the DMA-driven regular group (temp/volts), so telemetry is untouched.
     *
     * rank 1/2/3 = BEMF A/B/C (see ADC_Init injected-group setup).
     *
     * This board has NO physical virtual-neutral node (PA5 is only a 1.0k
     * pulldown, tied to no phase). The neutral is reconstructed in software as
     * (Va+Vb+Vc)/3: at the true zero-cross instant the floating phase voltage
     * equals that average. All three dividers are identical (5.1k/1.0k), so
     * the reference is independent of divider ratio and bus voltage.
     */
    /*
     * Fresh-sample gate (Experiment D, notes 12.20). Only act on a newly
     * completed injected sequence. JEOS is set by hardware at the end of each
     * TIM1 CH4 triggered 3-rank conversion and is not auto-cleared. If it is
     * not set, no new BEMF data has arrived since the previous call, so we
     * return the last polarity WITHOUT re-reading, WITHOUT touching the
     * blanking counter, and WITHOUT double-counting the same sample. This
     * decouples the ZCD from the faster, variable main-loop poll rate.
     */
    if (!LL_ADC_IsActiveFlag_JEOS(ADC1)) {
        return adc_zcd_last_out;
    }
    LL_ADC_ClearFlag_JEOS(ADC1);

    uint16_t va = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1);
    uint16_t vb = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2);
    uint16_t vc = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3);

    uint16_t neutral = (uint16_t)(((uint32_t)va + vb + vc) / 3u);

    uint16_t bemf;
    switch (adc_zcd_phase) {
        case 1: case 4: bemf = vc; break;   /* phase C floating */
        case 2: case 5: bemf = va; break;   /* phase A floating */
        case 3: case 6: bemf = vb; break;   /* phase B floating */
        default:        bemf = va; break;
    }

    uint8_t out;
    if (adc_zcd_blank != 0u) {
        /*
         * Post-commutation blanking: ignore the flyback transient. Return the
         * last stable polarity WITHOUT updating it, so the AM32 zero-cross
         * counter cannot advance on transient noise during this window.
         */
        adc_zcd_blank--;
        out = adc_zcd_last_out;
    } else {
        out = (bemf > neutral) ? 1u : 0u;
        adc_zcd_last_out = out;
    }

    /* Diagnostic snapshot for GDB live watch (see chapter 12). No control
     * effect: only mirrors internal values into observable globals. */
    zcd_dbg_va      = va;
    zcd_dbg_vb      = vb;
    zcd_dbg_vc      = vc;
    zcd_dbg_neutral = neutral;
    zcd_dbg_bemf    = bemf;
    zcd_dbg_phase   = adc_zcd_phase;
    zcd_dbg_calls++;
    if (out != zcd_dbg_out) {
        zcd_dbg_flips++;
    }
    zcd_dbg_out = out;

    /* 5‑minute capture: store one entry per second (~16800 calls at ~16.8k/s) */
    if (++dbg_skip >= 16800) {
        dbg_skip = 0;
        if (dbg_idx < DBG_BUF_SIZE) {
            volatile struct dbg_entry *e = &dbg_buf[dbg_idx++];
            e->va      = va;
            e->vb      = vb;
            e->vc      = vc;
            e->neutral = neutral;
            e->bemf    = bemf;
            e->zero_crosses    = zero_crosses;
            e->ci      = (uint16_t)commutation_interval;
            e->step    = step;
            e->running = running;
            e->out     = out;
            e->phase   = (uint8_t)adc_zcd_phase;
        }
    }

    return out;
}


/**
 * changeCompInput()
 *
 * Called by the commutation routine to inform the ZCD which phase is
 * now floating so getCompOutputLevel() reads the correct ADC channel.
 * No hardware comparator registers are touched.
 */
void changeCompInput(void)
{
    adc_zcd_phase = step;   /* 'step' is the global commutation step (1-6) */
    adc_zcd_blank = ADC_ZCD_BLANK_SAMPLES; /* re-arm blanking in fresh-sample (PWM-period) units, see 12.20 */
    LL_ADC_ClearFlag_JEOS(ADC1); /* drop any sample straddling the commutation edge so the blank starts clean */
}

/**
 * maskPhaseInterrupts() / enableCompInterrupts()
 *
 * With ADC ZCD there are no comparator EXTI interrupts.
 * maskPhaseInterrupts() is a no-op.
 *
 * enableCompInterrupts() is called by zcfoundroutine() when it tries to
 * switch to the hardware-comparator interrupt path (old_routine = 0).
 * Since we have no hardware comparator available, we force old_routine
 * back to 1 here so the system stays in ADC polling mode permanently.
 */
extern char old_routine;   /* defined in main.c */

void maskPhaseInterrupts(void)
{
    /* no-op: ADC polling, no EXTI comparator lines used */
}

void enableCompInterrupts(void)
{
    /* 无硬件比较器：拒绝切换到中断路径，强制保持 ADC 轮询模式 */
    old_routine = 1;
}

#else /* !USE_ADC_ZCD — original hardware comparator path */

COMP_TypeDef* active_COMP = COMP2;
uint32_t current_EXTI_LINE = LL_EXTI_LINE_22;

uint8_t getCompOutputLevel(void)
{
    return LL_COMP_ReadOutputLevel(active_COMP);
}

void maskPhaseInterrupts(void)
{
    EXTI->IMR1 &= ~(1 << 21);
    EXTI->IMR1 &= ~(1 << 22);
    EXTI->PR1 = LL_EXTI_LINE_22;
    EXTI->PR1 = LL_EXTI_LINE_21;
}

void enableCompInterrupts(void)
{
    EXTI->IMR1 |= current_EXTI_LINE;
}

void changeCompInput(void)
{
    if (step == 1 || step == 4) { /* C floating */
        current_EXTI_LINE = PHASE_C_EXTI_LINE;
        active_COMP       = PHASE_C_COMP_NUMBER;
        LL_COMP_ConfigInputs(active_COMP, PHASE_C_COMP, PHASE_C_INPUT_PLUS);
    }
    if (step == 2 || step == 5) { /* A floating */
        current_EXTI_LINE = PHASE_A_EXTI_LINE;
        active_COMP       = PHASE_A_COMP_NUMBER;
        LL_COMP_ConfigInputs(active_COMP, PHASE_A_COMP, PHASE_A_INPUT_PLUS);
    }
    if (step == 3 || step == 6) { /* B floating */
        current_EXTI_LINE = PHASE_B_EXTI_LINE;
        active_COMP       = PHASE_B_COMP_NUMBER;
        LL_COMP_ConfigInputs(active_COMP, PHASE_B_COMP, PHASE_B_INPUT_PLUS);
    }
    if (rising) {
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_22);
        LL_EXTI_DisableRisingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_EnableFallingTrig_0_31(current_EXTI_LINE);
    } else {
        LL_EXTI_EnableRisingTrig_0_31(current_EXTI_LINE);
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_21);
        LL_EXTI_DisableFallingTrig_0_31(LL_EXTI_LINE_22);
    }
}

#endif /* USE_ADC_ZCD */
