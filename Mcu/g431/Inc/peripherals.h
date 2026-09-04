/*
 * peripherals.h
 *
 *  Created on: Sep. 26, 2020
 *      Author: Alka
 */

#ifndef PERIPHERALS_H_
#define PERIPHERALS_H_

#endif /* PERIPHERALS_H_ */

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
 * trigger (no GPIO pin); its OC4REF falling edge (at CNT == CCR4) launches the
 * injected BEMF conversion.
 *
 * Experiment A (low-throttle jitter fix): CCR4 is no longer a plain duty/2.
 * At low duty the ON window is narrow, so duty/2 lands inside the dead-time /
 * switching-ringing region right after the high-side turns on, and the sampled
 * BEMF is garbage -> the loop cannot lock (observed as sub-20% throttle
 * jitter). zcd_ccr4_from_duty() clamps the sample point to a floor that sits
 * just past the dead time, while still keeping it inside the ON window when the
 * ON window is physically wide enough. See porting notes section 12.15.
 */
#include "targets.h"   /* DEAD_TIME */

/*
 * ZCD_SAMPLE_FLOOR: earliest CCR4 (in TIM1 ticks) at which the injected sample
 * is allowed to fire. Must clear the dead time plus a small settle margin so
 * the high-side is fully on and the switching ring has decayed.
 * DEAD_TIME is in the same TIM1-clock ticks as CCR (PSC = 0).
 */
#ifndef ZCD_SAMPLE_FLOOR
#define ZCD_SAMPLE_FLOOR ((uint16_t)(DEAD_TIME + 20u))
#endif

/*
 * Compute the CCR4 (injected-trigger) position from the requested duty.
 *   duty == 0                      -> 0 (output off, do not sample)
 *   ON window too narrow (duty <=  -> duty/2 (degenerate; open-loop ramp owns
 *     ZCD_SAMPLE_FLOOR)               this regime anyway, keep old behaviour)
 *   normal                         -> max(duty/2, ZCD_SAMPLE_FLOOR), but never
 *                                     later than (duty - 1) so it stays inside
 *                                     the ON window.
 */
static inline uint16_t zcd_ccr4_from_duty(uint16_t duty)
{
    if (duty == 0u) {
        return 0u;
    }
    uint16_t mid = (uint16_t)(duty >> 1);
    if (duty <= ZCD_SAMPLE_FLOOR) {
        /* ON window too narrow to place a clamped sample inside it. */
        return mid;
    }
    uint16_t ccr4 = (mid < ZCD_SAMPLE_FLOOR) ? ZCD_SAMPLE_FLOOR : mid;
    if (ccr4 > (uint16_t)(duty - 1u)) {
        ccr4 = (uint16_t)(duty - 1u);
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
