/* Native AM32 sine-start trial. Included by main.c instead of the old bench
 * adapter. No alignment, forced rev-up table, early-event rejection or retry.
 * The native sine table, input mapping and angle stepping remain in main.c.
 * A positive-throttle episode is bounded to 3 seconds, or one second after
 * six real feedback events, whichever comes first. Observe zero before reuse.
 */
#include "fd6288_async.h"
#include "fd6288_native_sine_config.h"

volatile FdPowerDiagnostics fd_power_bench = {
    .magic = FD_POWER_BENCH_MAGIC, .abi = 1u
};
/* Fixed event summary only. Times are DWT cycles relative to started. */
volatile struct {
    uint32_t magic, abi, trial, started, phase, reason;
    uint32_t sine_steps, sixstep_at, first_edge_at, sixth_edge_at, stopped_at;
    uint32_t edges, input_at_start, input_at_handoff, last_step;
} fd_native_sine = { .magic = 0x46534e31u, .abi = 1u };
_Static_assert(sizeof(fd_native_sine) == 60u, "native sine event ABI");
volatile uint8_t fd_rev_acquiring; /* always zero: no custom acquisition */
static uint8_t fd_power_starting, fd_native_ready;
void commutate(void);
static void fdBenchService(void);
/* DWT is a CONTROL clock here, even with every waveform logger disabled. */
static uint8_t fd_native_clock_ok;
#define FD_NATIVE_SINE_CLOCK_FAULT 15u
static void fdNativeSineClockInit(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    __DSB();
    __ISB();
    uint32_t before = DWT->CYCCNT;
    for (volatile uint32_t i = 0u; i < 32u; i++) __NOP();
    fd_native_clock_ok = (uint32_t)(DWT->CYCCNT - before) != 0u;
    __set_PRIMASK(mask);
}
static uint8_t fdNativeSineClockReady(void)
{
    return fd_native_clock_ok &&
        (CoreDebug->DEMCR & CoreDebug_DEMCR_TRCENA_Msk) &&
        (DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk);
}

uint8_t fdNativeSineBridgeAllowed(void)
{
    return fdNativeSineClockReady() && fd_power_bench.active_id && !fd_power_bench.inhibit && armed &&
        adjusted_input && !stuck_rotor_inhibit && !drive_safety_limit_inhibit;
}

static uint8_t fdBenchConfigOk(void)
{
    return eepromBuffer.use_sine_start == 1u && eepromBuffer.startup_power == FD_NATIVE_SINE_STARTUP_POWER &&
        eepromBuffer.motor_kv == FD_NATIVE_SINE_MOTOR_KV_CODE && eepromBuffer.motor_poles == FD_NATIVE_SINE_MOTOR_POLES &&
        eepromBuffer.sine_mode_changeover_thottle_level == FD_NATIVE_SINE_RANGE &&
        eepromBuffer.sine_mode_power == FD_NATIVE_SINE_POWER &&
        eepromBuffer.advance_level >= 10u && eepromBuffer.advance_level <= 42u &&
        eepromBuffer.auto_advance <= 1u && eepromBuffer.comp_pwm &&
        !eepromBuffer.variable_pwm && eepromBuffer.pwm_frequency == 24u &&
        !eepromBuffer.bi_direction && !eepromBuffer.rc_car_reverse &&
        !use_speed_control_loop && minimum_duty_cycle == 210u &&
        min_startup_duty == 360u && startup_max_duty_cycle == 610u;
}

static void fdBenchStop(uint8_t reason)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    fd_power_bench.inhibit = 1u;
    fd_native_ready = 0u;
    if (fd_power_bench.active_id) {
        fd_native_sine.reason = reason;
        fd_native_sine.stopped_at = DWT->CYCCNT - fd_native_sine.started;
        fd_native_sine.phase = 4u;
        fd_power_bench.completed++;
    }
    fd_power_bench.active_id = 0u;
    fd_power_bench.floor = 0u;
    fd_power_bench.last_reason = reason;
    fd_power_starting = 0u;
    fdAsyncCancel();
    heliCoastTrackingLost(1u);
    adcZcdResetAcquisition(ADC_ZCD_RESET_COUNT);
    stepper_sine = 1;
    do_once_sinemode = 1;
    input = 0u;
    __set_PRIMASK(mask);
}

static void fdBenchService(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    if (!adjusted_input) {
        if (fd_power_bench.active_id) fdBenchStop(FD_BENCH_OPERATOR_ZERO);
        fd_power_bench.inhibit = 0u;
        fd_native_ready = 1u;
        heli_coast_restart_inhibit = 0u;
    } else if (fd_power_bench.active_id && !fd_power_bench.inhibit) {
        uint32_t age = DWT->CYCCNT - fd_native_sine.started;
        if (!fdNativeSineClockReady())
            fdBenchStop(FD_NATIVE_SINE_CLOCK_FAULT);
        else if (!armed || stuck_rotor_inhibit || drive_safety_limit_inhibit)
            fdBenchStop(FD_BENCH_SAFETY_CAP);
        else if (age >= 3000000u * 160u || (fd_native_sine.edges >= 6u &&
            age - fd_native_sine.sixth_edge_at >= 1000000u * 160u))
            fdBenchStop(FD_BENCH_WINDOW);
        else if (fd_native_sine.phase >= 2u && !running)
            fdBenchStop(FD_BENCH_NOT_RUNNING);
    }
    __set_PRIMASK(mask);
}

static uint8_t fdBenchGuardInput(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    fdBenchService();
    if (armed && !fdNativeSineClockReady()) fdBenchStop(FD_NATIVE_SINE_CLOCK_FAULT);
    else if (armed && !fdBenchConfigOk()) fdBenchStop(FD_BENCH_BAD_CONFIG);
    if (armed && adjusted_input > 35u && !fd_power_bench.active_id &&
        !fd_power_bench.inhibit && fd_native_ready) {
        fd_native_ready = 0u;
        fd_power_bench.active_id = ++fd_power_bench.trials;
        fd_power_bench.floor = min_startup_duty;
        fd_native_sine.trial = fd_power_bench.active_id;
        fd_native_sine.started = DWT->CYCCNT;
        fd_native_sine.phase = 1u;
        fd_native_sine.reason = fd_native_sine.sine_steps = fd_native_sine.edges = 0u;
        fd_native_sine.sixstep_at = fd_native_sine.first_edge_at = UINT32_MAX;
        fd_native_sine.sixth_edge_at = fd_native_sine.stopped_at = UINT32_MAX;
        fd_native_sine.input_at_start = adjusted_input;
        fd_native_sine.input_at_handoff = fd_native_sine.last_step = 0u;
        fdAsyncCancel();
        allOff();
        SET_DUTY_CYCLE_ALL(0u);
        heli_coast_active = heli_coast_restart_inhibit = 0u;
        prop_brake_active = 0u;
        running = 0;
        stepper_sine = 1;
        do_once_sinemode = 1;
        phase_A_position = 180;
        phase_B_position = 299;
        phase_C_position = 59;
        step_delay = 100u;
    }
    uint8_t blocked = fd_power_bench.inhibit ||
        (adjusted_input > 35u && !fd_power_bench.active_id);
    if (blocked) input = duty_cycle_setpoint = 0u;
    __set_PRIMASK(mask);
    return blocked;
}

static uint8_t fdBenchBegin(void)
{
    fd_power_starting = 1u;
    return !fd_power_bench.inhibit && fd_power_bench.active_id != 0u;
}

static void fdBenchWritePwm(uint16_t ticks)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    fdBenchService();
    if (fd_power_bench.inhibit || stuck_rotor_inhibit || drive_safety_limit_inhibit ||
        !armed || !adjusted_input || !fd_power_bench.active_id) {
        allOff();
        SET_DUTY_CYCLE_ALL(0u);
    } else if (!stepper_sine) {
        last_duty_cycle = duty_cycle;
        SET_DUTY_CYCLE_ALL(ticks);
    }
    __set_PRIMASK(mask);
}

static uint8_t fdNativeSineHandoff(void)
{
    uint32_t mask = __get_PRIMASK();
    __disable_irq();
    fdBenchService();
    if (!fd_power_bench.active_id || fd_power_bench.inhibit || !armed ||
        !adjusted_input || stuck_rotor_inhibit || drive_safety_limit_inhibit) {
        __set_PRIMASK(mask);
        return 0u;
    }
    /* Three sine CCRs differ. Restore a common six-step PWM and its CH4
     * sample trigger before exposing the driven vector, with the bridge off.
     * Current/temperature caps retain priority over the ADC sampling floor. */
    if (!driveSafetyLimitAllowsZcd()) {
        fdBenchStop(FD_BENCH_SAFETY_CAP);
        __set_PRIMASK(mask);
        return 0u;
    }
    uint16_t duty = min_startup_duty;
    uint16_t cap = driveEffectiveDutyLimit();
    if (duty > cap) duty = cap;
    uint16_t ticks = (uint16_t)((uint32_t)duty * tim1_arr / 2000u + 1u);
    if (ticks < ZCD_MINIMUM_ON_TICKS) ticks = ZCD_MINIMUM_ON_TICKS;
    fdAsyncCancel();
    allOff();
    SET_DUTY_CYCLE_ALL(ticks);
    generatePwmTimerEvent();
    adcZcdResetAcquisition(ADC_ZCD_RESET_START);
    stepper_sine = 0;
    running = 1;
    old_routine = 1;
    commutation_interval = average_interval = last_average_interval = 9000u;
    for (unsigned i = 0u; i < 6u; i++) commutation_intervals[i] = 9000u;
    SET_INTERVAL_TIMER_COUNT(9000u);
    zero_crosses = 20u; /* native sine handover seed; NOT twenty measured ZCs */
    prop_brake_active = 0u;
    step = changeover_step;
    duty_cycle_setpoint = duty_cycle = last_duty_cycle = duty;
    adjusted_duty_cycle = ticks;
    fd_native_sine.phase = 2u;
    fd_native_sine.sixstep_at = DWT->CYCCNT - fd_native_sine.started;
    fd_native_sine.input_at_handoff = adjusted_input;
    commutate();
    fd_native_sine.last_step = step;
    __set_PRIMASK(mask);
    return !fd_power_bench.inhibit;
}

static void fdNativeSineEdge(void)
{
    if (!fd_power_bench.active_id || fd_native_sine.phase < 2u) return;
    uint32_t age = DWT->CYCCNT - fd_native_sine.started;
    fd_native_sine.edges++;
    if (fd_native_sine.edges == 1u) fd_native_sine.first_edge_at = age;
    if (fd_native_sine.edges == 6u) {
        fd_native_sine.sixth_edge_at = age;
        fd_native_sine.phase = 3u;
    }
}

static uint8_t fdBenchDeadlineExpired(void)
{
    return fd_power_bench.inhibit || (fd_power_bench.active_id &&
        (uint32_t)(DWT->CYCCNT - fd_native_sine.started) >= 3000000u * 160u);
}
