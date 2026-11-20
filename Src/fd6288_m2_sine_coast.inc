/* The native sine adapter owns output interlocks and the one-shot latch.
 * Foreground owns the powered ramp. ADC1 JEOS only observes after allOff. */
#include "fd6288_m2_sine.h"

static void fdBenchStop(uint8_t reason);
uint8_t fdNativeSineBridgeAllowed(void);
extern int16_t pwmSin[360];

enum { FD_M2_SINE_COAST_SAMPLES = 512u };
typedef struct {
    uint32_t cycles; /* software read time relative to allOff */
    uint16_t a, b, c, reserved;
} FdM2SineCoastSample;

static struct {
    uint32_t frame_at, drive_at, rate_at, rate, angle, slow_travel, platform_travel;
#ifdef TARGET_FD_M2_SINE_RUNUP
    uint32_t runup_hold_us;
    uint8_t runup;
#endif
    volatile uint32_t coast_at;
    volatile uint8_t coast;
} fd_m2_sine_run;

#ifdef TARGET_FD_M2_PURE
static struct {uint8_t frames;} fd_m2_sine_coast; /* First-frame control latch. */
#else
volatile struct {
    uint32_t magic, abi, trial, off_at, rate, angle, frames, clamp_frames;
    uint32_t max_gap_cycles, max_span_ticks, seen, count;
    FdM2SineCoastSample samples[FD_M2_SINE_COAST_SAMPLES];
} fd_m2_sine_coast = { .magic = 0x464d5331u, .abi = 1u };
_Static_assert(sizeof(fd_m2_sine_coast) == 6192u, "M2 sine/coast recorder ABI");
#endif

#ifdef TARGET_FD_M2_SINE_SIXSTEP
#include "fd6288_m2_sine_sixstep.inc"
#endif

static void fdM2SineReset(uint32_t trial)
{
    fd_m2_sine_run.frame_at = fd_m2_sine_run.drive_at = fd_m2_sine_run.rate_at = 0u;
    fd_m2_sine_run.rate = fdM2SineRequestedRate(48u);
    fd_m2_sine_run.angle = fd_m2_sine_run.slow_travel = fd_m2_sine_run.platform_travel = 0u;
#ifdef TARGET_FD_M2_SINE_RUNUP
    fd_m2_sine_run.runup_hold_us = fd_m2_sine_run.runup = 0u;
#endif
    fd_m2_sine_run.coast_at = 0u;
    fd_m2_sine_run.coast = 0u;
    fd_m2_sine_coast.frames=0u;
    FD_M2_DIAG(
    fd_m2_sine_coast.trial = trial;
    fd_m2_sine_coast.off_at = UINT32_MAX;
    fd_m2_sine_coast.rate = fd_m2_sine_coast.angle = 0u;
    fd_m2_sine_coast.frames = fd_m2_sine_coast.clamp_frames = 0u;
    fd_m2_sine_coast.max_gap_cycles = fd_m2_sine_coast.max_span_ticks = 0u;
    fd_m2_sine_coast.count = fd_m2_sine_coast.seen = 0u;
    );
#ifdef TARGET_FD_M2_SINE_SIXSTEP
    fdM2SineHandoffReset(trial);
#endif
}

static void fdM2SineBeginCoast(void)
{
    allOff();
    fd_m2_sine_run.coast_at = DWT->CYCCNT;
    fd_m2_sine_run.coast = 1u;
FD_M2_DIAG(
    fd_m2_sine_coast.off_at = fd_m2_sine_run.coast_at - fd_native_sine.started;
    fd_m2_sine_coast.rate = fd_m2_sine_run.rate;
    fd_m2_sine_coast.angle = (uint16_t)phase_A_position;
);
    GPIOB->BRR = 1u << 7;
    SET_HELI_COAST_HIGH_Z_SAMPLE(FD_M2_SINE_PERIOD);
    generatePwmTimerEvent();
}

static void fdM2SineObserveCoast(void)
{
    uint32_t elapsed = DWT->CYCCNT - fd_m2_sine_run.coast_at;
    LL_ADC_ClearFlag_JEOS(ADC1);
    if (elapsed >= FD_M2_SINE_COAST_US * FD_M2_SINE_CYCLES_PER_US) return;
#ifdef TARGET_FD_M2_SINE_STEP4_COAST
    if (fd_m2_sine_step4_coast.off_at != UINT32_MAX) {
        fdM2SineObserveStep4Coast(elapsed);
        return;
    }
#endif
#ifdef TARGET_FD_M2_PURE
    if (fd_m2_sine_pickup_requested) {
        FdM2SineCoastSample sample={.cycles=elapsed,
            .a=LL_ADC_INJ_ReadConversionData12(ADC1,LL_ADC_INJ_RANK_1),
            .b=LL_ADC_INJ_ReadConversionData12(ADC1,LL_ADC_INJ_RANK_2),
            .c=LL_ADC_INJ_ReadConversionData12(ADC1,LL_ADC_INJ_RANK_3)};
        fdM2SineObservePickup(elapsed,&sample);
    }
#else
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
    if (fd_m2_sine_recovery.attempts) {
        uint32_t seen = fd_m2_sine_recovery.seen++;
        unsigned i = fd_m2_sine_recovery.count;
        FdM2SineCoastSample sample = {
            .cycles = elapsed,
            .a = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1),
            .b = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2),
            .c = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3)
        };
        if (i < FD_M2_SINE_RECOVERY_SAMPLES && !(seen % 3u)) {
            fd_m2_sine_recovery.samples[i] = sample;
            fd_m2_sine_recovery.count = i + 1u;
        }
        fdM2SineObservePickup(elapsed, &sample);
        return;
    }
#endif
    uint32_t n = fd_m2_sine_coast.seen++;
    unsigned i = fd_m2_sine_coast.count;
    uint8_t keep = i < FD_M2_SINE_COAST_SAMPLES &&
        (elapsed < 1000u * FD_M2_SINE_CYCLES_PER_US || !(n & 1u));
#ifdef TARGET_FD_M2_SINE_SIXSTEP
    if (!keep && !fd_m2_sine_pickup_requested) return;
#else
    if (!keep) return;
#endif
    FdM2SineCoastSample sample = {
        .cycles = elapsed,
        .a = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1),
        .b = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2),
        .c = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3)
    };
    if (keep) {
        fd_m2_sine_coast.samples[i] = sample;
        fd_m2_sine_coast.count = i + 1u;
    }
#ifdef TARGET_FD_M2_SINE_SIXSTEP
    fdM2SineObservePickup(elapsed, &sample);
#endif
#endif
}

static void fdM2SineDriveFrame(uint32_t now)
{
    uint8_t first = fd_m2_sine_coast.frames == 0u;
    if (first) {
        if (TIM1->ARR != FD_M2_SINE_PERIOD - 1u || TIM1->PSC != 0u ||
            (TIM1->CR1 & (TIM_CR1_CKD_Msk | TIM_CR1_UDIS | TIM_CR1_CMS_Msk)) ||
            (TIM1->BDTR & TIM_BDTR_DTG_Msk) != TARGET_FIXED_DEAD_TIME_CODE) {
            fdBenchStop(FD_BENCH_BAD_CONFIG);
            return;
        }
        fd_m2_sine_run.drive_at = fd_m2_sine_run.rate_at = fd_m2_sine_run.frame_at = now;
    }
    uint32_t gap = now - fd_m2_sine_run.frame_at;
    if (!first && gap < FD_M2_SINE_PERIOD) return;
    FD_M2_DIAG(if (gap > fd_m2_sine_coast.max_gap_cycles) fd_m2_sine_coast.max_gap_cycles = gap);
    if (gap > 250u * FD_M2_SINE_CYCLES_PER_US) {
        fdBenchStop(FD_BENCH_COMM_LATE);
        return;
    }
    uint32_t elapsed_us = gap / FD_M2_SINE_CYCLES_PER_US;
    fd_m2_sine_run.frame_at += elapsed_us * FD_M2_SINE_CYCLES_PER_US;
    uint32_t age = (now - fd_m2_sine_run.drive_at) / FD_M2_SINE_CYCLES_PER_US;
    uint32_t max_drive_us = FD_M2_SINE_MAX_DRIVE_US;
#ifdef TARGET_FD_M2_SINE_RUNUP
    if (fd_m2_sine_run.runup) {
        max_drive_us = FD_M2_SINE_RUNUP_MAX_DRIVE_US;
    }
#endif
    if (age >= max_drive_us) {
        fdM2SineBeginCoast();
        return;
    }

    if (age >= FD_M2_SINE_MOTION_AT_US) {
        uint32_t delta = fd_m2_sine_run.rate * elapsed_us;
        fd_m2_sine_run.angle += delta;
        if (fd_m2_sine_run.angle >= FD_M2_SINE_ELECTRICAL_TURN)
            fd_m2_sine_run.angle -= FD_M2_SINE_ELECTRICAL_TURN;
        if (fd_m2_sine_run.slow_travel < FD_M2_SINE_SLOW_TRAVEL * FD_M2_SINE_DEGREE) {
            fd_m2_sine_run.slow_travel += delta;
            fd_m2_sine_run.rate_at = now;
        } else if (now - fd_m2_sine_run.rate_at >= 1000u * FD_M2_SINE_CYCLES_PER_US) {
            fd_m2_sine_run.rate_at += 1000u * FD_M2_SINE_CYCLES_PER_US;
            uint32_t target_rate = fdM2SineRequestedRate(input);
#ifdef TARGET_FD_M2_SINE_RUNUP
            if (fd_m2_sine_run.runup) target_rate = FD_M2_SINE_RUNUP_RATE;
#endif
            fd_m2_sine_run.rate = fdM2SineSlewRate(fd_m2_sine_run.rate, target_rate);
        }
#ifdef TARGET_FD_M2_SINE_RUNUP
        if (fd_m2_sine_run.runup) {
            if (fd_m2_sine_run.rate == FD_M2_SINE_RUNUP_RATE) {
                fd_m2_sine_run.runup_hold_us += elapsed_us;
                if (fd_m2_sine_run.runup_hold_us >= FD_M2_SINE_RUNUP_HOLD_US) {
#ifndef TARGET_FD_M2_SINE_RUNUP_COAST_ONLY
                    fd_m2_sine_pickup_requested = 1u;
#endif
                    fdM2SineBeginCoast();
                    return;
                }
            }
        } else
#endif
        {
            if (fd_m2_sine_run.rate == fdM2SineRequestedRate(120u))
                fd_m2_sine_run.platform_travel += delta;
            else
                fd_m2_sine_run.platform_travel = 0u;
            if (fd_m2_sine_run.platform_travel >= FD_M2_SINE_PLATFORM_TRAVEL * FD_M2_SINE_DEGREE) {
#ifdef TARGET_FD_M2_SINE_SIXSTEP
                if (input >= FD_M2_SINE_EXIT_INPUT) {
#ifdef TARGET_FD_M2_SINE_RUNUP
                    fd_m2_sine_run.runup = 1u;
#else
                    fd_m2_sine_pickup_requested = 1u;
                    fdM2SineBeginCoast();
                    return;
#endif
                }
                fd_m2_sine_run.platform_travel = FD_M2_SINE_PLATFORM_TRAVEL * FD_M2_SINE_DEGREE;
#else
                fdM2SineBeginCoast();
                return;
#endif
            }
        }
    }

    unsigned angle = fd_m2_sine_run.angle / FD_M2_SINE_DEGREE;
    /* With B=A+119 and C=A+239, increasing the table index follows the
     * FD6288 six-step forward sequence. */
    phase_A_position = (forward ? 180u + angle : 540u - angle) % 360u;
    phase_B_position = (phase_A_position + 119u) % 360u;
    phase_C_position = (phase_A_position + 239u) % 360u;
    /* AM32 2.16's common gate offset cancels in line voltage. The same
     * power-10 differential is faded in during the initial stationary hold. */
    uint32_t fade = age < FD_M2_SINE_ALIGN_US ? age / 1000u : FD_M2_SINE_ALIGN_US / 1000u;
    uint16_t raw[3] = {
        fdM2SineRaw(pwmSin[phase_A_position], fade),
        fdM2SineRaw(pwmSin[phase_B_position], fade),
        fdM2SineRaw(pwmSin[phase_C_position], fade)
    };
    uint16_t compares[3];
    fdM2SineCompares(raw, ZCD_SAMPLE_FLOOR, compares);
    uint16_t lo = compares[0], hi = compares[0];
    for (unsigned i = 1; i < 3; ++i) {
        if (compares[i] < lo) lo = compares[i];
        if (compares[i] > hi) hi = compares[i];
    }
    uint16_t span = hi - lo;
    if ((uint32_t)span * 2000u > (uint32_t)driveEffectiveDutyLimit() * FD_M2_SINE_PERIOD ||
        hi >= FD_M2_SINE_PERIOD - ZCD_SAMPLE_FLOOR) {
        fdBenchStop(FD_BENCH_SAFETY_CAP);
        return;
    }
FD_M2_DIAG(
    if (span > fd_m2_sine_coast.max_span_ticks) fd_m2_sine_coast.max_span_ticks = span;
    if (!lo) fd_m2_sine_coast.clamp_frames++;
);

    uint32_t cr1 = TIM1->CR1;
    TIM1->CR1 = cr1 | TIM_CR1_UDIS;
    TIM1->CCR1 = compares[0];
    TIM1->CCR2 = compares[1];
    TIM1->CCR3 = compares[2];
    TIM1->CCR4 = 0u;
    TIM1->CR1 = cr1;
    if (first) {
        DISABLE_COM_TIMER_INT();
        maskPhaseInterrupts();
        generatePwmTimerEvent();
        allpwm();
        GPIOB->BSRR = 1u << 7;
    }
#ifdef TARGET_FD_M2_PURE
    fd_m2_sine_coast.frames=1u;
#else
    fd_m2_sine_coast.frames++;
    fd_native_sine.sine_steps++;
#endif
}

static uint8_t fdM2SineCurrentOrTemperatureLimited(void)
{
#ifdef TARGET_FD_M2_SINE_CONTINUOUS
    return (use_current_limit &&
        (int32_t)actual_current >= (int32_t)eepromBuffer.limits.current * 200) ||
        degrees_celsius >= eepromBuffer.limits.temperature;
#else
    uint16_t limit = FD_M2_SINE_CURRENT_CENTIAMPS;
    if (use_current_limit && eepromBuffer.limits.current * 200u < limit)
        limit = eepromBuffer.limits.current * 200u;
    return actual_current >= (int16_t)limit || degrees_celsius >= eepromBuffer.limits.temperature;
#endif
}

static void fdM2SineCoastTick(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
#ifdef TARGET_FD_M2_SINE_SIXSTEP
    if (fd_m2_sine_timed || fd_m2_sine_pickup_ready) {
        if (input < FD_M2_SINE_EXIT_INPUT) fdBenchStop(FD_BENCH_OPERATOR_ZERO);
        else if (fd_m2_sine_timed) fdM2SineTimedTick(DWT->CYCCNT);
        else if (fdM2SineCurrentOrTemperatureLimited()) fdBenchStop(FD_BENCH_SAFETY_CAP);
        else fdM2SineStartSixstep(DWT->CYCCNT);
        __set_PRIMASK(mask);
        return;
    }
#endif
    if (fdNativeSineBridgeAllowed()) {
        if (fdM2SineCurrentOrTemperatureLimited())
            fdBenchStop(FD_BENCH_SAFETY_CAP);
        else if (input > 48u)
            fdM2SineDriveFrame(DWT->CYCCNT);
        else if (fd_m2_sine_coast.frames)
            fdBenchStop(FD_BENCH_OPERATOR_ZERO);
    }
    __set_PRIMASK(mask);
}
