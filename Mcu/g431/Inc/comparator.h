/*
 * comparator.h
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "main.h"
#include "targets.h"

void maskPhaseInterrupts(void);
void changeCompInput(void);
void enableCompInterrupts(void);
uint8_t getCompOutputLevel(void);

/* Returns 1 only when a fresh, post-blanking ADC ZCD sample was produced. */
uint8_t getCompOutputSample(uint8_t *level);

#ifdef TARGET_ADC_ZCD_GUARD
enum {
    ADC_ZCD_RESET_START,
    ADC_ZCD_RESET_TIMEOUT,
    ADC_ZCD_RESET_DESYNC,
    ADC_ZCD_RESET_FAULT,
    ADC_ZCD_RESET_COUNT
};

/* Accepted DRIVE crossings only; no printing, allocation or peripheral I/O.
 * Inspect after stopping. Sequence is published after the entry is complete. */
#define ADC_ZCD_TRACE_LENGTH 32u
typedef struct {
    uint32_t sequence;
    uint16_t elapsed;
    uint16_t interval;
    int16_t margin;
    uint8_t step;
    uint8_t confirmations;
} AdcZcdTrace;
typedef struct {
    uint32_t accepted;
    uint32_t rejected_early;
    uint32_t rejected_unarmed;
    uint32_t waiting_confirmation;
    uint32_t resets[ADC_ZCD_RESET_COUNT];
    uint32_t trace_sequence;
    uint8_t trace_frozen;
} AdcZcdDiagnostics;
extern volatile AdcZcdDiagnostics adc_zcd_diag;
extern volatile AdcZcdTrace adc_zcd_trace[ADC_ZCD_TRACE_LENGTH];
void adcZcdResetAcquisition(uint8_t reason);
#ifdef TARGET_STARTUP_POWER_BENCH
uint8_t adcZcdDriveCrossings(void);
#endif
#if FD6288_STARTUP_TIMING_FIX
uint32_t adcZcdDrivePeriod(uint32_t elapsed, uint32_t previous);
#endif
#endif

extern volatile char rising;
extern char step;

#endif /* COMPARATOR_H_ */
