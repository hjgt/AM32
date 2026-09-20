/* Included by the host harness, against actual adapter and gate functions. */
#ifdef TARGET_STARTUP_POWER_BENCH
static void benchReset(void)
{
    memset((void *)&fd_power_bench, 0, sizeof(fd_power_bench));
    memset((void *)&adc_zcd_diag, 0, sizeof(adc_zcd_diag));
    fdPowerInit(&fd_power_policy);
    fd_power_starting = 0;
    fdTraceInit();
    primask = 0; wait_cycle_increment = 0;
    eepromBuffer.pwm_frequency = 24; eepromBuffer.comp_pwm = 1;
    eepromBuffer.variable_pwm = eepromBuffer.use_sine_start = 0;
    eepromBuffer.bi_direction = eepromBuffer.rc_car_reverse = 0;
    min_startup_duty = 360; minimum_duty_cycle = 210; startup_max_duty_cycle = 610;
    stepper_sine = use_speed_control_loop = 0;
    armed = 1; hard_cap = 2000; stuck_enabled = 0;
    stuck_rotor_inhibit = drive_safety_limit_inhibit = 0;
    stuck_rotor_trip_count = bemf_timeout_happened = 0;
    heli_coast_active = heli_coast_restart_inhibit = 0;
    heli_coast_bailout_requested = heli_coast_valid_crossings = 0;
    running = prop_brake_active = 0;
    input = adjusted_input = newinput = 0;
    duty_cycle = duty_cycle_setpoint = last_duty_cycle = adjusted_duty_cycle = 0;
    pwm = gates = gate_writes = 0;
    battery_voltage = 780; actual_current = 0;
    zero_crosses = 0; adcZcdResetAcquisition(ADC_ZCD_RESET_COUNT);
    CHECK(!fdBenchGuardInput()); // actual decoded zero, not just internal input
}

static void benchStart(void)
{
    newinput = adjusted_input = input = 447;
    CHECK(!fdBenchGuardInput());
    CHECK(fdBenchBegin());
    fdTraceBegin();
    adcZcdResetAcquisition(ADC_ZCD_RESET_START);
    commutate();
    running = 1; fd_power_starting = 0;
    last_duty_cycle = duty_cycle = 360;
    fdBenchService();
    CHECK(fd_power_bench.active_id && !fd_power_bench.inhibit);
}

static void benchAt(unsigned us)
{
    DWT->CYCCNT = fd_power_policy.started + us * FD_POWER_TICKS_PER_US;
    fdBenchService();
}

static void test_power_adapter(void)
{
    benchReset();
    fdPowerInit(&fd_power_policy); // a positive boot command cannot start
    newinput = adjusted_input = input = 447;
    CHECK(!fdBenchBegin());
    CHECK(fd_power_bench.inhibit && !gates && !pwm);
    input = 0; CHECK(fdBenchGuardInput()); // forced internal zero is not recovery
    CHECK(!fd_power_policy.ready);
    adjusted_input = newinput = 0; CHECK(!fdBenchGuardInput());
    benchStart();
    CHECK(fd_power_bench.floor == 360 + FD6288_STARTUP_POWER_EXTRA);
    // Actual setInput startup branch and the existing ramp, not a copied
    // idealized formula: the +40 request rises gradually from native seed 360.
    input = 69; duty_cycle_setpoint = 230; actualStartupFloor();
    CHECK(duty_cycle_setpoint == 360 + FD6288_STARTUP_POWER_EXTRA);
    for (unsigned i = 1; i <= 20; i++) {
        duty_cycle = duty_cycle_setpoint; ramp_count = 10; actualRamp();
        CHECK(duty_cycle == (FD6288_STARTUP_POWER_EXTRA ? 360 + 2 * i : 360));
        last_duty_cycle = duty_cycle;
    }
    duty_cycle_setpoint = 900; actualStartupFloor(); CHECK(duty_cycle_setpoint == 610);
    hard_cap = 375; fdBenchService();
    CHECK(fd_power_bench.floor == (FD6288_STARTUP_POWER_EXTRA ? 375 : 360));
    hard_cap = 2000;
    benchAt(100000); CHECK(fd_power_bench.floor == 360 + FD6288_STARTUP_POWER_EXTRA);
    benchAt(110000); CHECK(fd_power_bench.floor == 360 + FD6288_STARTUP_POWER_EXTRA / 2);
    benchAt(120000); CHECK(fd_power_bench.floor == 360);
    // Deadline must remain active after the native startup-floor branch ends.
    zero_crosses = 1000; actual_current = 251; battery_voltage = 745; duty_cycle = 600;
    adc_zcd_diag.accepted = 12; adc_zcd_diag.resets[ADC_ZCD_RESET_TIMEOUT] = 3;
    com_irq = 1; fdBenchWritePwm(2000); CHECK(pwm == 2000);
    benchAt(299999); CHECK(!fd_power_bench.inhibit);
    benchAt(300000);
    CHECK(fd_power_bench.inhibit && !running && !gates && !pwm && !com_irq);
    CHECK(!fd_power_bench.active_id && fd_power_bench.completed == 1);
    CHECK(fd_power_bench.records[0].reason == FD_BENCH_WINDOW);
    CHECK(fd_power_bench.records[0].elapsed_cycles == FD_POWER_TRIAL_TICKS);
    CHECK(fd_power_bench.records[0].accepted == 12 && fd_power_bench.records[0].timeouts == 3);
    CHECK(fd_power_bench.records[0].peak_filtered_current == 251);
    CHECK(fd_power_bench.records[0].min_voltage == 745 && fd_power_bench.records[0].max_duty == 600);
    CHECK(!stuck_rotor_trip_count && !stuck_rotor_inhibit);
    unsigned writes = gate_writes;
    comStep(1); allpwm(); commutate(); zcfoundroutine(); fdBenchWritePwm(2000);
    CHECK(!gates && !pwm && gate_writes == writes && !primask);
    primask = 1; comStep(1); allpwm(); commutate(); fdBenchWritePwm(2000);
    CHECK(primask == 1 && !gates && !pwm); primask = 0;
    CHECK(fdBenchGuardInput()); CHECK(!fdBenchBegin());
    CHECK(fd_power_bench.trials == 1); // no automatic repeat on held throttle
    adjusted_input = 0; CHECK(!fdBenchGuardInput());
    CHECK(!heli_coast_restart_inhibit);
    benchStart(); CHECK(fd_power_bench.trials == 2);

    // Early release uses the private real-edge streak, never zero_crosses.
    benchReset(); benchStart(); zero_crosses = 999;
    benchAt(20000); CHECK(fd_power_bench.floor == 360 + FD6288_STARTUP_POWER_EXTRA);
    adc_zcd_drive_crossings = 5; adc_zcd_diag.accepted = 5;
    benchAt(40000); benchAt(50000);
    CHECK(fd_power_bench.floor == 360 + FD6288_STARTUP_POWER_EXTRA / 2);
    benchAt(60000); CHECK(fd_power_bench.floor == 360);
    CHECK(fd_power_work.fifth_observed_cycles == 40000u * FD_POWER_TICKS_PER_US);
    adjusted_input = newinput = 0; CHECK(!fdBenchGuardInput());
    CHECK(fd_power_bench.records[0].reason == FD_BENCH_OPERATOR_ZERO);
    CHECK(!fd_power_bench.inhibit && !fd_power_bench.active_id);
    // Existing zero-throttle Coast can observe, but positive rotating pickup
    // is intentionally excluded from the bench image.
    heli_coast_active = 1; running = 1; fdBenchWritePwm(2000); CHECK(!pwm);
    adjusted_input = newinput = 447; CHECK(fdBenchGuardInput());
    CHECK(!gates && !running && fd_power_bench.last_reason == FD_BENCH_NOT_RUNNING);

    benchReset(); benchStart(); stuck_enabled = 1;
    bemf_timeout_happened = 11; CHECK(stuckRotorGuardInput());
    fdBenchService();
    CHECK(stuck_rotor_trip_count == 1 && bemf_timeout_happened == 102);
    CHECK(fd_power_bench.last_reason == FD_BENCH_NATIVE_STUCK);
    CHECK(!gates && !pwm && !running);
    adjusted_input = 0; CHECK(!fdBenchGuardInput()); CHECK(!stuckRotorGuardInput());
    CHECK(stuck_rotor_trip_count == 1);

    benchReset(); benchStart(); drive_safety_limit_inhibit = 1;
    fdBenchWritePwm(2000);
    CHECK(fd_power_bench.last_reason == FD_BENCH_SAFETY_CAP && !gates && !pwm);
    CHECK(!stuck_rotor_trip_count);
    benchReset(); benchStart(); hard_cap = 0; fdBenchService();
    CHECK(fd_power_bench.inhibit && !gates && !pwm);

    benchReset(); DWT->CYCCNT = UINT32_MAX - 10000u; benchStart();
    benchAt(110000); CHECK(fd_power_bench.floor == 360 + FD6288_STARTUP_POWER_EXTRA / 2);
    benchAt(300000); CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW);

    // In the blocking ZC wait, TIM6 cannot preempt itself. The real polling
    // loop must notice the raw-cycle deadline without another IRQ.
    benchReset(); benchStart(); zero_crosses = 5; commutation_interval = 10000;
    interval_timer = 1000; DWT->CYCCNT = fd_power_policy.started + FD_POWER_TRIAL_TICKS - 100;
    wait_cycle_increment = 40; zcfoundroutine(); wait_cycle_increment = 0;
    CHECK(fd_power_bench.inhibit && !gates && !pwm && !running);
    CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW);

    benchReset(); eepromBuffer.pwm_frequency = 48;
    CHECK(fdBenchGuardInput()); CHECK(eepromBuffer.pwm_frequency == 48);
    CHECK(fd_power_bench.last_reason == FD_BENCH_BAD_CONFIG);
    benchReset(); min_startup_duty = 210; CHECK(fdBenchGuardInput());
    benchReset(); eepromBuffer.use_sine_start = 1; CHECK(fdBenchGuardInput());
    benchReset(); DWT->CTRL = 0; adjusted_input = 447;
    CHECK(!fdBenchBegin() && !gates && !pwm);

    benchReset();
    for (unsigned i = 1; i <= 18; ++i) {
        adjusted_input = newinput = 0; CHECK(!fdBenchGuardInput());
        benchStart(); benchAt(300000);
        CHECK(fd_power_bench.records[(i - 1) % 16].id == i);
        CHECK(fd_power_bench.completed == i && fd_power_bench.trials == i);
    }
    CHECK(fd_power_bench.records[0].id == 17 && fd_power_bench.records[1].id == 18);
    CHECK(fd_power_bench.records[2].id == 3 && fd_power_bench.records[15].id == 16);
}
#endif
