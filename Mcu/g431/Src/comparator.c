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
 * PA5 is only a 1.0k pulldown and is not a usable neutral input. The neutral
 * used by this route is therefore reconstructed in software as the average
 * of the three identically-scaled phase readings.
 *
 * adc_zcd_phase: tracks which phase is currently floating
 *   1 or 4 -> phase C floating
 *   2 or 5 -> phase A floating
 *   3 or 6 -> phase B floating
 */
static uint8_t adc_zcd_phase = 0;

/*
 * Post-commutation blanking is measured in valid ON-window injected sequences,
 * not main-loop calls. This keeps its duration tied to real PWM periods.
 */
#ifndef ADC_ZCD_BLANK_SAMPLES
#define ADC_ZCD_BLANK_SAMPLES 1u
#endif

/* Small Schmitt band around the reconstructed neutral (12-bit ADC counts). */
#ifndef ADC_ZCD_HYSTERESIS_COUNTS
#define ADC_ZCD_HYSTERESIS_COUNTS 8
#endif

/* Reject conversions that did not observe the driven phase in its ON state. */
#ifndef ADC_ZCD_MIN_DRIVE_DELTA
#define ADC_ZCD_MIN_DRIVE_DELTA 64
#endif
static volatile uint8_t adc_zcd_blank = 0;
static uint8_t          adc_zcd_last_out = 0;

/*
 * Diagnostic snapshot (see chapter 12 of the porting notes).
 * These volatile file-scope variables mirror the last values computed inside
 * getCompOutputSample() so they can be observed live over SWD with GDB
 * (local variables are not visible after the function returns). They do NOT
 * affect the control logic in any way - remove once debugging is complete.
 *   zcd_dbg_va/vb/vc : last three phase-divider ADC readings
 *   zcd_dbg_neutral  : software neutral = (va+vb+vc)/3
 *   zcd_dbg_bemf     : the floating-phase reading selected this call
 *   zcd_dbg_drive_*  : driven-high/low phase readings used as an ON check
 *   zcd_dbg_out      : raw COMP-equivalent level (1 = neutral>bemf)
 *   zcd_dbg_valid    : sample passed ON-window check and post-comm blanking
 *   zcd_dbg_calls    : number of completed injected ADC sequences
 *   zcd_dbg_flips    : number of times the polarity output changed
 */
volatile uint16_t zcd_dbg_va = 0;
volatile uint16_t zcd_dbg_vb = 0;
volatile uint16_t zcd_dbg_vc = 0;
volatile uint16_t zcd_dbg_neutral = 0;
volatile uint16_t zcd_dbg_bemf = 0;
volatile uint16_t zcd_dbg_drive_high = 0;
volatile uint16_t zcd_dbg_drive_low = 0;
volatile uint8_t  zcd_dbg_out = 0;
volatile uint8_t  zcd_dbg_valid = 0;
volatile uint8_t  zcd_dbg_phase = 0;
volatile uint32_t zcd_dbg_calls = 0;
volatile uint32_t zcd_dbg_flips = 0;



/*
 * 5-minute diagnostic capture buffer (see §12).
 * getCompOutputSample() stores one entry every ~1s so the host can read
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
extern volatile uint32_t zero_crosses;
extern volatile uint32_t commutation_interval;
extern uint8_t  running;

/**
 * getCompOutputSample()
 *
 * Writes a raw hardware-COMP-equivalent level to *level: 1 when the virtual
 * neutral (COMP plus input) is above the floating phase (COMP minus input),
 * otherwise 0. Returns 1 only when a fresh, post-blanking injected sequence
 * was consumed. A repeated main-loop poll therefore cannot count the same ADC
 * conversion more than once.
 *
 * This polarity contract deliberately matches the hardware path below;
 * main.c performs the common inversion before applying its rising/falling
 * zero-cross logic.
 */
uint8_t getCompOutputSample(uint8_t *level)
{
    /*
     * PWM-synchronous virtual-neutral zero-cross detection (Route C).
     *
     * The three BEMF phase dividers are sampled by the ADC1 INJECTED group,
     * hardware-triggered by TIM1 CH4 inside the clean PWM ON window (see
     * ADC.c / peripherals.c / peripherals.h). We simply read the most recent
     * injected results here - NON-BLOCKING, with no reconfiguration of the
     * DMA-driven regular group (temp/volts), so telemetry is untouched.
     *
     * rank 1/2/3 = BEMF A/B/C (see ADC_Init injected-group setup).
     *
     * PA5 is only a 1.0k pulldown and is not the neutral reference. The neutral
     * is reconstructed in software as (Va+Vb+Vc)/3: at the true zero-cross
     * instant the floating phase voltage equals that average. All three
     * dividers are identical (5.1k/1.0k), so the reference is independent of
     * divider ratio and bus voltage.
     */
    /*
     * JEOS is set at the end of the three-rank injected sequence and is not
     * auto-cleared. Leave blanking and diagnostics untouched until a genuinely
     * new set of phase samples exists.
     */
    if (!LL_ADC_IsActiveFlag_JEOS(ADC1)) {
        *level = adc_zcd_last_out;
        return 0u;
    }
    LL_ADC_ClearFlag_JEOS(ADC1);

    uint16_t va = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1);
    uint16_t vb = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2);
    uint16_t vc = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3);

    uint16_t neutral = (uint16_t)(((uint32_t)va + vb + vc) / 3u);

    uint16_t bemf = 0u;
    uint16_t drive_high = 0u;
    uint16_t drive_low = 0u;
    uint8_t phase_valid = 1u;
    switch (adc_zcd_phase) {
        case 1: /* A PWM, B low, C floating */
            drive_high = va; drive_low = vb; bemf = vc; break;
        case 2: /* C PWM, B low, A floating */
            drive_high = vc; drive_low = vb; bemf = va; break;
        case 3: /* C PWM, A low, B floating */
            drive_high = vc; drive_low = va; bemf = vb; break;
        case 4: /* B PWM, A low, C floating */
            drive_high = vb; drive_low = va; bemf = vc; break;
        case 5: /* B PWM, C low, A floating */
            drive_high = vb; drive_low = vc; bemf = va; break;
        case 6: /* A PWM, C low, B floating */
            drive_high = va; drive_low = vc; bemf = vb; break;
        default:
            phase_valid = 0u;
            break;
    }

    uint8_t out = adc_zcd_last_out;
    uint8_t valid = 0u;
    uint8_t sample_in_on_window = phase_valid &&
        (((int32_t)drive_high - (int32_t)drive_low) >= ADC_ZCD_MIN_DRIVE_DELTA);
    if (!sample_in_on_window) {
        /* An off-window or incomplete sequence cannot represent BEMF. */
    } else if (adc_zcd_blank != 0u) {
        /*
         * Post-commutation blanking: ignore the flyback transient. Return the
         * last stable polarity WITHOUT updating it, so the AM32 zero-cross
         * counter cannot advance on transient noise during this window.
         */
        adc_zcd_blank--;
    } else {
        int32_t margin = (int32_t)bemf - (int32_t)neutral;
        if (margin > ADC_ZCD_HYSTERESIS_COUNTS) {
            out = 0u; /* COMP minus (phase) is higher than plus (neutral). */
        } else if (margin < -ADC_ZCD_HYSTERESIS_COUNTS) {
            out = 1u; /* COMP plus (neutral) is higher than minus (phase). */
        }
        adc_zcd_last_out = out;
        valid = 1u;
    }

    /* Diagnostic snapshot for GDB live watch (see chapter 12). No control
     * effect: only mirrors internal values into observable globals. */
    zcd_dbg_va      = va;
    zcd_dbg_vb      = vb;
    zcd_dbg_vc      = vc;
    zcd_dbg_neutral = neutral;
    zcd_dbg_bemf    = bemf;
    zcd_dbg_drive_high = drive_high;
    zcd_dbg_drive_low  = drive_low;
    zcd_dbg_phase   = adc_zcd_phase;
    zcd_dbg_valid   = valid;
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
            e->zero_crosses    = (uint16_t)zero_crosses;
            e->ci      = (uint16_t)commutation_interval;
            e->step    = step;
            e->running = running;
            e->out     = out;
            e->phase   = (uint8_t)adc_zcd_phase;
        }
    }

    *level = out;
    return valid;
}

uint8_t getCompOutputLevel(void)
{
    uint8_t level;
    (void)getCompOutputSample(&level);
    return level;
}


/**
 * changeCompInput()
 *
 * Called by the commutation routine to inform the ZCD which phase is
 * now floating so getCompOutputSample() reads the correct ADC channel.
 * No hardware comparator registers are touched.
 */
void changeCompInput(void)
{
    adc_zcd_phase = step;   /* 'step' is the global commutation step (1-6) */
    /* Before the expected edge, raw COMP output equals rising. Seeding this
     * reject state prevents blanking or startup from looking like a crossing. */
    adc_zcd_last_out = (rising != 0) ? 1u : 0u;
    adc_zcd_blank = ADC_ZCD_BLANK_SAMPLES;
    LL_ADC_ClearFlag_JEOS(ADC1); /* drop a sequence straddling commutation */
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
