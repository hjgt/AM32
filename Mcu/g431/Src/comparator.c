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

#ifdef HELI_COAST_ON_ZERO
extern volatile uint8_t heli_coast_active;
#endif

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
     * injected results here - NON-BLOCKING, with no reconfiguration of ADC1's
     * temperature rank or ADC2's voltage/current ranks.
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
     * auto-cleared. Do not consume blanking until a genuinely new set of phase
     * samples exists.
     */
    if (!LL_ADC_IsActiveFlag_JEOS(ADC1)) {
        *level = adc_zcd_last_out;
        return 0u;
    }
    LL_ADC_ClearFlag_JEOS(ADC1);

    uint16_t va = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1);
    uint16_t vb = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2);
    uint16_t vc = (uint16_t)LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3);

#ifdef HELI_COAST_ON_ZERO
    /* Snapshot the mode once so a state transition cannot mix DRIVE and Coast
     * validity rules within one three-rank ADC sequence. */
    uint8_t coast_sample = heli_coast_active;
    uint16_t phase_span = 0u;
    if (coast_sample != 0u) {
        uint16_t phase_max = (va > vb) ? va : vb;
        uint16_t phase_min = (va < vb) ? va : vb;
        if (vc > phase_max) {
            phase_max = vc;
        }
        if (vc < phase_min) {
            phase_min = vc;
        }
        phase_span = (uint16_t)(phase_max - phase_min);
    }
#endif

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
    uint8_t sample_valid;
#ifdef HELI_COAST_ON_ZERO
    if (coast_sample != 0u) {
        /* With all six MOSFETs off there is no DRIVE ON-window. Require a
         * meaningful three-phase BEMF span instead; the expected floating
         * phase is still selected by adc_zcd_phase below. */
        sample_valid = phase_valid &&
            (phase_span >= (uint16_t)HELI_COAST_ADC_MIN_SPAN);
    } else
#endif
    {
        sample_valid = phase_valid &&
            (((int32_t)drive_high - (int32_t)drive_low) >=
                ADC_ZCD_MIN_DRIVE_DELTA);
    }
    if (sample_valid && (adc_zcd_blank != 0u)) {
        /*
         * Post-commutation blanking: ignore the flyback transient. Return the
         * last stable polarity WITHOUT updating it, so the AM32 zero-cross
         * counter cannot advance on transient noise during this window.
         */
        adc_zcd_blank--;
    } else if (sample_valid) {
        int32_t margin = (int32_t)bemf - (int32_t)neutral;
        if (margin > ADC_ZCD_HYSTERESIS_COUNTS) {
            out = 0u; /* COMP minus (phase) is higher than plus (neutral). */
        } else if (margin < -ADC_ZCD_HYSTERESIS_COUNTS) {
            out = 1u; /* COMP plus (neutral) is higher than minus (phase). */
        }
        adc_zcd_last_out = out;
        valid = 1u;
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
