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

extern volatile char rising;
extern char step;

#endif /* COMPARATOR_H_ */
