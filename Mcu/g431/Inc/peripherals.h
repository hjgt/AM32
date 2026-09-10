/*
 * peripherals.h
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#include "ADC.h"
#include "main.h"

#define INTERVAL_TIMER_COUNT (INTERVAL_TIMER->CNT)
#define RELOAD_WATCHDOG_COUNTER() (LL_IWDG_ReloadCounter(IWDG))
#define DISABLE_COM_TIMER_INT() (COM_TIMER->DIER &= ~((0x1UL << (0U))))
#define ENABLE_COM_TIMER_INT() (COM_TIMER->DIER |= (0x1UL << (0U)))
#define SET_AND_ENABLE_COM_INT(time)                                  \
    (COM_TIMER->CNT = 0, COM_TIMER->ARR = time, COM_TIMER->SR = 0x00, \
        COM_TIMER->DIER |= (0x1UL << (0U)))
#define SET_INTERVAL_TIMER_COUNT(intertime) (INTERVAL_TIMER->CNT = intertime)
#define SET_PRESCALER_PWM(presc) (TIM1->PSC = presc)
#define SET_AUTO_RELOAD_PWM(relval) (TIM1->ARR = relval)
/*
 * SET_DUTY_CYCLE_ALL
 *   CCR1/2/3 = duty  -> three-phase complementary PWM (unchanged AM32 behaviour)
 *   CCR4     = ADC injected-group trigger position (see zcd_ccr4_from_duty()).
 *
 * Route C (PWM-synchronous BEMF sampling): TIM1 is edge-aligned (COUNTERMODE_UP),
 * so in PWM1 mode the high-side is ON while CNT < CCRx. The centre of the ON
 * pulse is at CNT = CCRx/2. TIM1 CH4 (OC4) is used only as an internal ADC
 * trigger (no GPIO pin); its OC4REF falling edge launches the injected BEMF
 * sequence. The complete three-rank sequence, not just its trigger, must stay
 * inside the driven phase's ON window.
 *
 * zcd_ccr4_from_duty() applies both a start floor (dead time plus gate-driver
 * settling) and an end limit (ADC trigger latency plus all three conversions).
 * If the requested pulse is too narrow for a trustworthy sequence, CH4 is held
 * inactive and comparator.c rejects any residual/off-window result.
 */
#include "targets.h"   /* DEAD_TIME */

/*
 * ZCD_SAMPLE_FLOOR: earliest CCR4 (in TIM1 ticks) at which the injected sample
 * is allowed to fire. It includes the programmed MCU dead time plus a 92-tick
 * board-level allowance for FD6288 propagation and switching settling.
 * DEAD_TIME is in the same TIM1-clock ticks as CCR (PSC = 0).
 */
#ifndef ZCD_SAMPLE_FLOOR
#define ZCD_SAMPLE_FLOOR ((uint16_t)(DEAD_TIME + 92u))
#endif

/*
 * ADC1 runs at 40 MHz while TIM1 runs at 160 MHz. At the configured 6.5-cycle
 * sample time, three 12-bit ranks take 3*(6.5+12.5)*4 = 228 TIM1 ticks.
 * Sixteen more ticks cover worst-case injected-trigger latency and rounding.
 */
#ifndef ZCD_ADC_WINDOW_TICKS
#define ZCD_ADC_WINDOW_TICKS 244u
#endif

#define ZCD_MINIMUM_ON_TICKS \
    ((uint16_t)(ZCD_SAMPLE_FLOOR + ZCD_ADC_WINDOW_TICKS))

/*
 * Compute the CCR4 (injected-trigger) position from the requested duty.
 *   incomplete clean ON window -> 0 (PWM1 CH4 stays low: no falling trigger)
 *   normal                     -> clamp(duty/2, floor, latest safe trigger)
 */
static inline uint16_t zcd_ccr4_from_duty(uint16_t duty)
{
    if (duty < ZCD_MINIMUM_ON_TICKS) {
        return 0u;
    }

    uint16_t mid = (uint16_t)(duty >> 1);
    uint16_t ccr4 = (mid < ZCD_SAMPLE_FLOOR) ? ZCD_SAMPLE_FLOOR : mid;
    uint16_t latest = (uint16_t)(duty - ZCD_ADC_WINDOW_TICKS);
    if (ccr4 > latest) {
        ccr4 = latest;
    }
    return ccr4;
}

#define SET_DUTY_CYCLE_ALL(newdc) \
    (TIM1->CCR1 = (newdc), TIM1->CCR2 = (newdc), TIM1->CCR3 = (newdc), \
        TIM1->CCR4 = zcd_ccr4_from_duty((uint16_t)(newdc)))

void initAfterJump(void);
void initCorePeripherals(void);
void SystemClock_Config(void);
void MX_GPIO_Init(void);
void MX_DMA_Init(void);
void MX_ADC1_Init(void);
void MX_COMP2_Init(void);
void MX_COMP1_Init(void);
void MX_TIM1_Init(void);
void MX_TIM2_Init(void);
void MX_TIM3_Init(void);
void MX_TIM14_Init(void);
void MX_TIM17_Init(void);
void MX_TIM16_Init(void);
void MX_IWDG_Init(void);
void MX_TIM6_Init(void);
void MX_TIM15_Init(void);
void resetInputCaptureTimer();
void setPWMCompare1(uint16_t compareone);
void setPWMCompare2(uint16_t comparetwo);
void setPWMCompare3(uint16_t comparethree);
void enableCorePeripherals(void);
void reloadWatchDogCounter(void);
void generatePwmTimerEvent(void);

#endif /* PERIPHERALS_H_ */
