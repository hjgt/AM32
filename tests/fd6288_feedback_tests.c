/* S3 actual ADC + bench adapter + commutation, not a physical motor model. */
#ifdef TARGET_STARTUP_FEEDBACK
static void feedbackReset(void)
{
    fd_rev_acquiring = fd_trace_failure_pinned = 0u;
    fd_rev = (FdRevPolicy){0};
    memset((void *)&fd_startup_trace, 0, sizeof(fd_startup_trace));
    benchReset();
    old_routine = forward = 1; step = 0; bemf_timeout = 10;
    eepromBuffer.auto_advance = 0; temp_advance = 16;
}
static void feedbackEdge(unsigned us)
{
    DWT->CYCCNT = fd_rev.sector_at + us * FD_REV_US;
    unsigned accepted = adc_zcd_diag.accepted;
    pre(2u * us); pre(2u * us); pre(2u * us);
    post(2u * us); post(2u * us);
    CHECK(adc_zcd_diag.accepted == accepted + 1u && bemfcounter);
    zcfound = 1; zcfoundroutine();
    CHECK(primask == 0u);
}
static void feedbackLock(void)
{
    for (unsigned n = 1; n <= 6u; n++) {
        char old_step = step;
        feedbackEdge(2000u);
        CHECK(fd_rev.confidence == n && zero_crosses == 0u);
        if (n < 6u) {
            CHECK(fd_rev.stage == FD_REV_OPEN && step != old_step);
            CHECK(interval_timer == 0u && fd_rev_acquiring);
            CHECK(fd_rev.sector_at == DWT->CYCCNT);
        } else CHECK(fd_rev.stage == FD_REV_PENDING && step == old_step);
    }
    CHECK(fd_rev.measured == 2000u * FD_REV_US);
    unsigned writes = gate_writes;
    sequenceTickAt(fd_rev.edge_at + fd_rev.delay - 1u);
    CHECK(gate_writes == writes && fd_rev_acquiring);
    sequenceTickAt(fd_rev.edge_at + fd_rev.delay);
    CHECK(fd_rev.stage == FD_REV_CLOSED && !fd_rev_acquiring);
    CHECK(zero_crosses == 6u && commutation_interval == 4000u);
    CHECK(adcZcdDriveCrossings() == 5u && adc_zcd_earliest == 2000u);
    CHECK(interval_timer == fd_rev.delay / 80u);
    for (unsigned i = 0; i < 6u; i++) CHECK(commutation_intervals[i] == 4000u);
}
static void feedbackRetry(void)
{
    char old_step = step;
    sequenceTickAt(fd_rev.sector_at + FD_REV_RETRY_TICKS - 1u);
    CHECK(step == old_step);
    sequenceTickAt(fd_rev.sector_at + FD_REV_RETRY_TICKS);
}
static void test_feedback(void)
{
    test_all_phases(); test_confirmation(); test_failed_start_edge_shape();
    test_time_gate_and_resets(); test_coast_unchanged();
    for (unsigned dir = 0; dir <= 1; dir++) {
        feedbackReset(); forward = dir; benchStart();
        CHECK(step == 1 && fd_rev.stage == FD_REV_OPEN);
        duty_cycle_setpoint = 2000; actualStartupFloor(); CHECK(duty_cycle_setpoint == 360);
        feedbackLock();
        CHECK(!bemf_timeout_happened && !adc_zcd_diag.resets[ADC_ZCD_RESET_TIMEOUT]);
        sequenceComplete();
        CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW);
        CHECK(fd_power_bench.records[0].reserved == 0x64u);
        CHECK(!running && !gates && !pwm);
    }
    // An isolated first edge never seeds a measured period. A timeout clears
    // its streak and private ADC history, but does not feed a fake ZC to IIR.
    feedbackReset(); benchStart(); feedbackEdge(14000);
    CHECK(fd_rev.confidence == 1u && fd_rev.measured == 0u);
    unsigned interval = commutation_interval;
    feedbackRetry();
    CHECK(fd_power_bench.floor == 400u); /* now >30 ms, same S2 envelope */
    CHECK(!fd_rev.confidence && !adcZcdDriveCrossings() && !zero_crosses);
    CHECK(bemf_timeout_happened == 1u && commutation_interval == interval);
    CHECK(adc_zcd_diag.resets[ADC_ZCD_RESET_TIMEOUT] == 1u);
    // Recovery after an acquisition retry may still earn an observation.
    feedbackLock(); sequenceComplete();
    CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW);
    CHECK(fd_power_bench.records[0].timeouts == 1u);

    // Actual native threshold/latch is serviced on retries. No synthetic
    // timeout for an accepted-edge step; disabling the switch keeps it off.
    for (unsigned on = 0; on <= 1; on++) {
        feedbackReset(); stuck_enabled = on; benchStart();
        for (unsigned n = 1; n <= 10; n++) {
            feedbackRetry();
            CHECK(bemf_timeout_happened == n && !stuck_rotor_inhibit);
            CHECK(!zero_crosses && !fd_rev.confidence);
        }
        feedbackRetry();
        if (on) {
            CHECK(stuck_rotor_trip_count == 1u && stuck_rotor_inhibit);
            CHECK(fd_power_bench.last_reason == FD_BENCH_NATIVE_STUCK);
        } else {
            CHECK(!stuck_rotor_trip_count && bemf_timeout_happened == 11u);
            benchAt(300000);
            CHECK(fd_power_bench.last_reason == FD_BENCH_NO_LOCK);
        }
        CHECK(!gates && !pwm && !running);
        unsigned writes = gate_writes;
        fdRevTick(); commutate(); comStep(3); allpwm(); fdBenchWritePwm(2000);
        CHECK(gate_writes == writes && !gates && !pwm);
        CHECK(fdBenchGuardInput() && !fdBenchBegin());
        adjusted_input = newinput = 0;
        CHECK(!fdBenchGuardInput() && !stuckRotorGuardInput());
        benchStart(); CHECK(fd_power_bench.trials == 2u);
    }
    // A delayed service cannot make burst retries or extend the deadline.
    feedbackReset(); benchStart();
    sequenceTickAt(fd_rev.sector_at + 3u * FD_REV_RETRY_TICKS);
    CHECK(bemf_timeout_happened == 1u);
    unsigned writes = gate_writes;
    fdRevTick(); CHECK(gate_writes == writes);
    sequenceTickAt(fd_power_policy.started + FD_REV_ACQUIRE_TICKS);
    CHECK(gate_writes == writes && fd_power_bench.inhibit);
    // Partial/pending acquisition never earns an extra second.
    for (unsigned n = 0; n <= 6; n++) {
        feedbackReset(); benchStart();
        fd_rev.confidence = n; fd_rev.stage = FD_REV_PENDING;
        benchAt(300000);
        CHECK(fd_power_bench.last_reason == FD_BENCH_NO_LOCK && !gates && !pwm);
    }
    // Explicit zero and safety caps beat a pending sixth-edge commutation.
    for (unsigned stop = 0; stop < 3; stop++) {
        feedbackReset(); benchStart();
        for (unsigned n = 0; n < 6; n++) feedbackEdge(2000);
        if (stop == 0) adjusted_input = newinput = 0;
        if (stop == 1) drive_safety_limit_inhibit = 1;
        if (stop == 2) { stuck_enabled = 1; bemf_timeout_happened = 11; CHECK(stuckRotorGuardInput()); }
        char old_step = step;
        sequenceTickAt(fd_rev.edge_at + fd_rev.delay);
        fdBenchWritePwm(2000);
        CHECK(step == old_step && !fd_rev_acquiring);
        if (stop) CHECK(!running && !gates && !pwm);
        else CHECK(!fd_power_bench.active_id && !fd_power_bench.floor);
    }
    // Wrap-safe handoff/observation; first CLOSED timeout aborts (no retry).
    feedbackReset(); DWT->CYCCNT = UINT32_MAX - 200u; benchStart();
    feedbackLock();
    interval_timer = 45001; writes = gate_writes; fd6288TimeoutStep();
    CHECK(fd_power_bench.last_reason == FD_BENCH_LOST_FEEDBACK);
    CHECK(gate_writes == writes && !gates && !pwm && !stuck_rotor_trip_count);
    feedbackReset(); benchStart(); feedbackLock();
    DWT->CYCCNT = fd_rev.closed_at + FD_REV_OBSERVE_TICKS - 1;
    fdBenchService(); CHECK(!fd_power_bench.inhibit);
    DWT->CYCCNT++; fdBenchService(); CHECK(fd_power_bench.inhibit);
    // Pure policy: duplicates, late IRQ, counter wrap, second-edge seed and
    // advance clamp. Acquisition no longer uses S2's arbitrary phase window.
    FdRevPolicy p; fdRevBeginPolicy(&p, UINT32_MAX - 500u);
    CHECK(!fdRevEdgePolicy(&p, p.sector_at + FD_REV_RETRY_TICKS, 16));
    CHECK(!p.confidence);
    uint32_t first = p.sector_at + 1000u * FD_REV_US;
    CHECK(!fdRevEdgePolicy(&p, first, 16) && p.confidence == 1);
    CHECK(!fdRevEdgePolicy(&p, first + 1u, 16) && p.confidence == 1);
    CHECK(fdRevAdvancePolicy(&p, first) == FD_REV_STEP);
    for (unsigned n = 2; n <= 6; n++) {
        uint32_t now = p.sector_at + 1000u * FD_REV_US;
        fdRevEdgePolicy(&p, now, 255);
        CHECK(p.measured == 1000u * FD_REV_US && p.confidence == n);
        if (n < 6) CHECK(fdRevAdvancePolicy(&p, now) == FD_REV_STEP);
    }
    CHECK(p.delay == p.measured / 8u);
    CHECK(fdRevAdvancePolicy(&p, p.edge_at + p.delay) == FD_REV_HANDOFF);
}
#endif
