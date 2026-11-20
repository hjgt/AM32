#ifdef TARGET_STARTUP_SEQUENCE
static void sequenceReset(void)
{
    fd_rev_acquiring = fd_trace_failure_pinned = 0u;
    fd_rev = (FdRevPolicy){0};
    memset((void *)&fd_startup_trace, 0, sizeof(fd_startup_trace));
    benchReset();
    old_routine = 1; forward = 1; step = 0;
}
static void sequenceTickAt(uint32_t now)
{
    DWT->CYCCNT = now;
    fdRevTick();
}
static void sequenceComplete(void)
{
#ifdef TARGET_STARTUP_SEQUENCE_V2
    if (fd_rev.stage == FD_REV_CLOSED) {
        DWT->CYCCNT = fd_rev.closed_at + FD_REV_OBSERVE_TICKS;
        fdBenchService();
        return;
    }
#endif
    benchAt(300000);
}
static void sequenceRealEdge(void)
{
    uint32_t age = fd_rev.period / 2u;
    DWT->CYCCNT = fd_rev.sector_at + age;
    pre(age / 80u); pre(age / 80u); pre(age / 80u);
    post(age / 80u); post(age / 80u);
    CHECK(bemfcounter != 0u);
    zcfound = 1; zcfoundroutine();
}
static void sequenceLock(void)
{
    sequenceTickAt(fd_rev.sector_at + FD_REV_ALIGN_TICKS);
    CHECK(fd_rev.stage == FD_REV_OPEN);
    for (unsigned n = 1u; n <= 6u; n++) {
        sequenceRealEdge();
        CHECK(fd_rev.confidence == n && zero_crosses == 0u);
        if (n < 6u) sequenceTickAt(fd_rev.sector_at + fd_rev.period);
    }
    CHECK(fd_rev.stage == FD_REV_PENDING && fd_rev_acquiring);
    unsigned writes = gate_writes;
    sequenceTickAt(fd_rev.edge_at + fd_rev.delay - 1u);
    CHECK(fd_rev.stage == FD_REV_PENDING && gate_writes == writes);
    sequenceTickAt(fd_rev.edge_at + fd_rev.delay);
    CHECK(fd_rev.stage == FD_REV_CLOSED && !fd_rev_acquiring);
    CHECK(zero_crosses == 6u && adcZcdDriveCrossings() == 5u);
    CHECK(commutation_interval == fd_rev.measured / 80u);
    CHECK(interval_timer == fd_rev.delay / 80u);
    CHECK(adc_zcd_earliest == commutation_interval / 2u);
    for (unsigned i = 0; i < 6u; i++) CHECK(commutation_intervals[i] == commutation_interval);
}
static void test_sequence(void)
{
    // Existing acquisition shape/freshness requirements are not relaxed.
    test_all_phases(); test_confirmation(); test_failed_start_edge_shape();
    test_time_gate_and_resets(); test_coast_unchanged();
    for (unsigned dir = 0u; dir <= 1u; dir++) {
        sequenceReset(); forward = dir; step = dir ? 0 : 2;
        benchStart(); CHECK(step == 1u && fd_rev.stage == FD_REV_ALIGN);
        duty_cycle_setpoint = 2000; actualStartupFloor(); CHECK(duty_cycle_setpoint == 360);
        unsigned writes = gate_writes;
        sequenceTickAt(fd_rev.sector_at + FD_REV_ALIGN_TICKS - 1u);
        CHECK(gate_writes == writes && !zero_crosses);
        sequenceLock();
        CHECK(!bemf_timeout_happened && adc_zcd_diag.resets[ADC_ZCD_RESET_TIMEOUT] == 0u);
        sequenceComplete();
        CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW && !fd_trace_failure_pinned);
        CHECK((fd_power_bench.records[0].reserved & 15u) == FD_REV_CLOSED);
        CHECK(!running && !gates && !pwm);
    }
    // Repeat failures with protection OFF and ON; no blind retry at positive
    // command, planned steps never count as real ZC or native missing edges.
    for (unsigned protection = 0; protection <= 1; protection++) {
        sequenceReset(); stuck_enabled = protection; benchStart();
        for (unsigned us = 0; us < 300000u; us += 50u) {
            sequenceTickAt(fd_power_policy.started + us * FD_REV_US);
            interval_timer = 50000; fd6288TimeoutStep();
            CHECK(!zero_crosses && !bemf_timeout_happened && !fd_power_bench.inhibit);
        }
        benchAt(300000);
        CHECK(fd_power_bench.last_reason == FD_BENCH_NO_LOCK && fd_trace_failure_pinned);
        CHECK(!running && !gates && !pwm && !fd_rev_acquiring);
        CHECK(fd_power_bench.records[0].reserved == FD_REV_OPEN);
        unsigned writes = gate_writes;
        fdRevTick(); commutate(); allpwm(); comStep(3); fdBenchWritePwm(2000);
        CHECK(!gates && !pwm && gate_writes == writes);
        CHECK(fdBenchGuardInput() && !fdBenchBegin());
        CHECK(fd_power_bench.trials == 1);
        unsigned capture = fd_startup_trace.capture_id, sequence = fd_startup_trace.sample_sequence;
        adjusted_input = newinput = 0; CHECK(!fdBenchGuardInput());
        benchStart(); sequenceLock(); sequenceComplete();
        CHECK(fd_startup_trace.capture_id == capture && fd_startup_trace.sample_sequence == sequence);
        CHECK(fd_power_bench.trials == 2 && (fd_power_bench.records[1].reserved & 15u) == FD_REV_CLOSED);
    }
    // A successful trial is replaceable; the first following failure is pinned.
    sequenceReset(); benchStart(); sequenceLock(); sequenceComplete();
    adjusted_input = newinput = 0; CHECK(!fdBenchGuardInput());
    benchStart(); CHECK(fd_startup_trace.capture_id == 2u);
    benchAt(300000); CHECK(fd_trace_failure_pinned);
    // Native switch/latch still trips after handoff and recovers only at zero.
    sequenceReset(); benchStart(); sequenceLock(); stuck_enabled = 1;
    bemf_timeout_happened = 11; CHECK(stuckRotorGuardInput()); fdBenchService();
    CHECK(stuck_rotor_inhibit && stuck_rotor_trip_count == 1 && bemf_timeout_happened == 102);
    CHECK(fd_power_bench.last_reason == FD_BENCH_NATIVE_STUCK && !gates && !pwm);
    adjusted_input = 0; CHECK(!fdBenchGuardInput()); CHECK(!stuckRotorGuardInput());
    sequenceReset(); benchStart(); sequenceLock();
    interval_timer = 45001; fd6288TimeoutStep();
    CHECK(bemf_timeout_happened == 1u);
#ifdef TARGET_STARTUP_SEQUENCE_V2
    CHECK(fd_power_bench.inhibit && !running && !gates && !pwm);
#else
    CHECK(!zero_crosses);
#endif
    CHECK(adc_zcd_diag.resets[ADC_ZCD_RESET_TIMEOUT] == 1u);
    benchAt(300000);
    CHECK(fd_power_bench.last_reason == FD_BENCH_LOST_FEEDBACK && fd_trace_failure_pinned);
    // C06 takes priority in all rev-up stages, including pending handoff.
    for (unsigned stage = FD_REV_ALIGN; stage <= FD_REV_PENDING; stage++) {
        sequenceReset(); benchStart(); fd_rev.stage = stage;
        drive_safety_limit_inhibit = 1; fdRevTick(); fdBenchWritePwm(2000);
        CHECK(fd_power_bench.last_reason == FD_BENCH_SAFETY_CAP && !running && !gates && !pwm);
    }
    sequenceReset(); benchStart(); adjusted_input = newinput = 0;
    CHECK(!fdBenchGuardInput() && !fd_rev_acquiring && !fd_power_bench.active_id);
    // Raw-counter wrap and delayed service must not emit a catch-up burst.
    sequenceReset(); DWT->CYCCNT = UINT32_MAX - 300u; benchStart();
    sequenceLock(); sequenceComplete(); CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW);
    sequenceReset(); benchStart(); unsigned writes = gate_writes;
    sequenceTickAt(fd_power_policy.started + FD_POWER_TRIAL_TICKS + 1000);
    CHECK(gate_writes == writes && fd_power_bench.last_reason == FD_BENCH_NO_LOCK);
    // Policy rejects early/late/duplicate and non-adjacent/inconsistent edges.
    FdRevPolicy p; fdRevBeginPolicy(&p, UINT32_MAX - 15u);
    CHECK(fdRevAdvancePolicy(&p, p.sector_at + FD_REV_ALIGN_TICKS) == FD_REV_STEP);
    CHECK(!fdRevEdgePolicy(&p, p.sector_at + p.period / 16u, 16u) && !p.confidence);
    CHECK(!fdRevEdgePolicy(&p, p.sector_at + p.period / 2u, 16u) && !p.confidence);
    fdRevAdvancePolicy(&p, p.sector_at + p.period);
    fdRevEdgePolicy(&p, p.sector_at + p.period / 2u, 16u); CHECK(p.confidence == 1u);
    fdRevAdvancePolicy(&p, p.sector_at + p.period);
    fdRevAdvancePolicy(&p, p.sector_at + p.period); CHECK(!p.confidence);
    fdRevEdgePolicy(&p, p.sector_at + p.period * 15u / 16u, 16u); CHECK(!p.confidence);
    for (unsigned i = 0; i < 100u; i++) fdRevAdvancePolicy(&p, p.sector_at + p.period);
    CHECK(p.period == FD_REV_MIN_PERIOD && p.stage == FD_REV_OPEN);
    // A pending real-edge delay cannot extend the bridge-on window.
    sequenceReset(); benchStart(); fd_rev.stage = FD_REV_PENDING;
    fd_rev.edge_at = fd_power_policy.started + FD_POWER_TRIAL_TICKS - 100u;
    fd_rev.delay = 10000u; sequenceTickAt(fd_power_policy.started + FD_POWER_TRIAL_TICKS);
    CHECK(fd_power_bench.last_reason == FD_BENCH_NO_LOCK && !pwm && !gates);
    // Explicit stopped ARM releases the failure pin without energizing.
    adjusted_input = newinput = input = 0; fdBenchGuardInput();
    fd_startup_trace.command = FD_TRACE_ARM_COMMAND; fdTraceControlTick();
    CHECK(!fd_trace_failure_pinned && fd_startup_trace.state == FD_TRACE_ARMED);
    CHECK(!gates && !pwm);
#ifdef TARGET_STARTUP_SEQUENCE_V2
    // Slower ramp and a one-electrical-cycle hold on coherent late edges.
    fdRevBeginPolicy(&p, 0u);
    fdRevAdvancePolicy(&p, FD_REV_ALIGN_TICKS);
    uint32_t first = p.period;
    fdRevEdgePolicy(&p, p.sector_at + p.period * 4u / 5u, 16u);
    CHECK(p.confidence == 1u && !p.settle_steps && p.decision == FD_REV_EDGE_SEED);
    fdRevAdvancePolicy(&p, p.sector_at + p.period);
    CHECK(p.period == first * 31u / 32u);
    fdRevEdgePolicy(&p, p.sector_at + p.period * 4u / 5u, 16u);
    CHECK(p.confidence == 2u && p.settle_steps == 6u && p.max_confidence == 2u);
    uint32_t held = p.period;
    for (unsigned i = 0u; i < 6u; i++) {
        fdRevAdvancePolicy(&p, p.sector_at + p.period);
        CHECK(p.period == held && p.settle_steps == 5u - i);
    }
    CHECK(!p.confidence && p.stage == FD_REV_OPEN); // missing edges never earn lock
    fdRevAdvancePolicy(&p, p.sector_at + p.period);
    CHECK(p.period == held * 31u / 32u);
    fdRevEdgePolicy(&p, p.sector_at + p.period * 15u / 16u, 16u);
    CHECK(p.decision == FD_REV_EDGE_LATE && !p.confidence && !p.settle_steps);
    fdRevAdvancePolicy(&p, p.sector_at + p.period);
    fdRevEdgePolicy(&p, p.sector_at + p.period / 16u, 16u);
    CHECK(p.decision == FD_REV_EDGE_EARLY && !p.confidence && !p.settle_steps);

    // Partial qualification must never earn a longer powered interval.
    for (unsigned count = 0; count < 6; count++) {
        sequenceReset(); benchStart(); fd_rev.confidence = count;
        fd_rev.settle_steps = 6; benchAt(300000);
        CHECK(fd_power_bench.last_reason == FD_BENCH_NO_LOCK && !gates && !pwm);
    }
    // Successful handoff gets 1 s from the actual handoff, not from throttle.
    sequenceReset(); benchStart(); sequenceLock();
    benchAt(300000); CHECK(!fd_power_bench.inhibit && running);
    DWT->CYCCNT = fd_rev.closed_at + FD_REV_OBSERVE_TICKS - 1u;
    fdBenchService(); CHECK(!fdBenchDeadlineExpired());
    fdTraceControlTick(); CHECK(fd_startup_trace.state == FD_TRACE_ACTIVE);
    DWT->CYCCNT++; fdBenchService();
    CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW && !running && !gates && !pwm);
    CHECK((fd_power_bench.records[0].reserved >> 4) == 6u);

    // Polling ISR's blocking wait must use the extended deadline as well.
    sequenceReset(); benchStart(); sequenceLock();
    DWT->CYCCNT = fd_rev.closed_at + FD_REV_OBSERVE_TICKS - 100u;
    interval_timer = 1000; wait_cycle_increment = 40;
    zcfoundroutine(); wait_cycle_increment = 0;
    CHECK(fd_power_bench.inhibit && !pwm && !gates && !running);

    // First runtime missing-feedback timeout aborts without a forced step,
    // even with native protection Off; this is separate from its >10 latch.
    for (unsigned on = 0u; on <= 1u; on++) {
        sequenceReset(); benchStart(); sequenceLock(); stuck_enabled = on;
        unsigned writes = gate_writes;
        interval_timer = 45001u; fd6288TimeoutStep();
        CHECK(fd_power_bench.last_reason == FD_BENCH_LOST_FEEDBACK);
        CHECK(!stuck_rotor_trip_count && bemf_timeout_happened == 1u);
        CHECK(gate_writes == writes && !gates && !pwm && !running);
        CHECK(fdBenchGuardInput() && !fdBenchBegin());
    }
    // Current/duty limits, zero recovery, and Coast restrictions remain common.
#endif
}
#endif
