/*
 * IO.c
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#include "IO.h"

#include "common.h"
#include "dshot.h"
#include "functions.h"
#include "serial_telemetry.h"
#include "targets.h"

uint8_t buffer_padding = 7;
char ic_timer_prescaler = CPU_FREQUENCY_MHZ / 6;
uint32_t dma_buffer[64] = { 0 };
volatile char out_put = 0;

void receiveDshotDma()
{
#ifdef USE_TIMER_3_CHANNEL_1
    RCC->APB1RSTR1 |= LL_APB1_GRP1_PERIPH_TIM3;
    RCC->APB1RSTR1 &= ~LL_APB1_GRP1_PERIPH_TIM3;
    IC_TIMER_REGISTER->CCMR1 = 0x41;   // CC1S=01: CH1 input capture
    IC_TIMER_REGISTER->CCER = 0xa;     // CC1P+CC1NP: both edges
#elif defined(USE_TIMER_3_CHANNEL_2)
    RCC->APB1RSTR1 |= LL_APB1_GRP1_PERIPH_TIM3;
    RCC->APB1RSTR1 &= ~LL_APB1_GRP1_PERIPH_TIM3;
    IC_TIMER_REGISTER->CCMR1 = 0x4100; // CC2S=01: CH2 input capture
    IC_TIMER_REGISTER->CCER = 0xa0;    // CC2P+CC2NP: both edges
#else
    LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_TIM15); // de-init TIM15
    LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_TIM15);
    IC_TIMER_REGISTER->CCMR1 = 0x41;
    IC_TIMER_REGISTER->CCER = 0xa;
#endif
    IC_TIMER_REGISTER->PSC = ic_timer_prescaler;
    IC_TIMER_REGISTER->ARR = 0xFFFF;
    IC_TIMER_REGISTER->EGR |= TIM_EGR_UG;
    out_put = 0;
    IC_TIMER_REGISTER->CNT = 0;
    DMA1_Channel1->CMAR = (uint32_t)&dma_buffer;
#ifdef USE_TIMER_3_CHANNEL_2
    DMA1_Channel1->CPAR = (uint32_t)&IC_TIMER_REGISTER->CCR2;
#else
    DMA1_Channel1->CPAR = (uint32_t)&IC_TIMER_REGISTER->CCR1;
#endif
    DMA1_Channel1->CNDTR = buffersize;
    DMA1_Channel1->CCR = 0x98b;
#ifdef USE_TIMER_3_CHANNEL_2
    IC_TIMER_REGISTER->DIER |= TIM_DIER_CC2DE;
#else
    IC_TIMER_REGISTER->DIER |= TIM_DIER_CC1DE;
#endif
    IC_TIMER_REGISTER->CCER |= IC_TIMER_CHANNEL;
    IC_TIMER_REGISTER->CR1 |= TIM_CR1_CEN;
}

void sendDshotDma()
{
#ifdef USE_TIMER_3_CHANNEL_1
    RCC->APB1RSTR1 |= LL_APB1_GRP1_PERIPH_TIM3;
    RCC->APB1RSTR1 &= ~LL_APB1_GRP1_PERIPH_TIM3;
    IC_TIMER_REGISTER->CCMR1 = 0x60;   // OC1M=PWM1 mode on CH1
    IC_TIMER_REGISTER->CCER = 0x3;     // CC1E + CC1P
#elif defined(USE_TIMER_3_CHANNEL_2)
    RCC->APB1RSTR1 |= LL_APB1_GRP1_PERIPH_TIM3;
    RCC->APB1RSTR1 &= ~LL_APB1_GRP1_PERIPH_TIM3;
    IC_TIMER_REGISTER->CCMR1 = 0x6000; // OC2M=PWM1 mode on CH2
    IC_TIMER_REGISTER->CCER = 0x30;    // CC2E + CC2P
#else
    LL_APB2_GRP1_ForceReset(LL_APB2_GRP1_PERIPH_TIM15); // de-init TIM15
    LL_APB2_GRP1_ReleaseReset(LL_APB2_GRP1_PERIPH_TIM15);
    IC_TIMER_REGISTER->CCMR1 = 0x60;
    IC_TIMER_REGISTER->CCER = 0x3;
#endif

    IC_TIMER_REGISTER->PSC = output_timer_prescaler;
    IC_TIMER_REGISTER->ARR = 108;
    out_put = 1;
    LL_TIM_GenerateEvent_UPDATE(IC_TIMER_REGISTER);

    DMA1_Channel1->CMAR = (uint32_t)&gcr;
#ifdef USE_TIMER_3_CHANNEL_2
    DMA1_Channel1->CPAR = (uint32_t)&IC_TIMER_REGISTER->CCR2;
#else
    DMA1_Channel1->CPAR = (uint32_t)&IC_TIMER_REGISTER->CCR1;
#endif
    DMA1_Channel1->CNDTR = 23 + buffer_padding;
    DMA1_Channel1->CCR = 0x99b;
#ifdef USE_TIMER_3_CHANNEL_2
    IC_TIMER_REGISTER->DIER |= TIM_DIER_CC2DE;
#else
    IC_TIMER_REGISTER->DIER |= TIM_DIER_CC1DE;
#endif
    IC_TIMER_REGISTER->CCER |= IC_TIMER_CHANNEL;
    IC_TIMER_REGISTER->BDTR |= TIM_BDTR_MOE;
    IC_TIMER_REGISTER->CR1 |= TIM_CR1_CEN;
}

uint8_t getInputPinState() { return (INPUT_PIN_PORT->IDR & INPUT_PIN); }

void setInputPolarityRising()
{
    LL_TIM_IC_SetPolarity(IC_TIMER_REGISTER, IC_TIMER_CHANNEL,
        LL_TIM_IC_POLARITY_RISING);
}

void setInputPullDown()
{
    LL_GPIO_SetPinPull(INPUT_PIN_PORT, INPUT_PIN, LL_GPIO_PULL_DOWN);
}

void setInputPullUp()
{
    LL_GPIO_SetPinPull(INPUT_PIN_PORT, INPUT_PIN, LL_GPIO_PULL_UP);
}

void enableHalfTransferInt() { LL_DMA_EnableIT_HT(DMA1, INPUT_DMA_CHANNEL); }
void setInputPullNone()
{
    LL_GPIO_SetPinPull(INPUT_PIN_PORT, INPUT_PIN, LL_GPIO_PULL_NO);
}
