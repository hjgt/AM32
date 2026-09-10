/*
 * comparator.h
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "main.h"

void maskPhaseInterrupts(void);
void changeCompInput(void);
void enableCompInterrupts(void);
uint8_t getCompOutputLevel(void);

/* Returns 1 only when a fresh, post-blanking ADC ZCD sample was produced. */
uint8_t getCompOutputSample(uint8_t *level);

#ifdef HELI_COAST_ON_ZERO
void heliCoastResetAdcDiagnostics(void);
#endif

#ifdef ADC_ZCD_C01_DIAGNOSTICS
/* Write a non-zero token over SWD; the next control tick latches one snapshot
 * and clears the request after all snapshot fields are stable. */
extern volatile uint32_t zcd_c01_snapshot_request;
void zcdC01LatchSnapshot(void);
#endif

extern volatile char rising;
extern char step;

#endif /* COMPARATOR_H_ */
