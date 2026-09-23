/* S3 actual ADC + bench adapter + commutation, not a physical motor model. */
#ifdef TARGET_STARTUP_FEEDBACK
#ifdef TARGET_STARTUP_S3_POWER
enum { TEST_INITIAL_REQUEST = 540, TEST_ACQUIRE_REQUEST = 600 };
#else
enum { TEST_INITIAL_REQUEST = 360, TEST_ACQUIRE_REQUEST = 400 };
#endif
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
    sequenceTickAt(fd_rev.sector_at + fd_rev.period - 1u);
    CHECK(step == old_step);
    sequenceTickAt(fd_rev.sector_at + fd_rev.period);
}
#ifdef TARGET_STARTUP_S4
static void test_s4(void)
{
    /* Real retry adapter: six shorter NO-FEEDBACK sectors, never synthetic
     * ADC edges, and only once per operator start in either direction. */
    for (unsigned dir = 0; dir <= 1; dir++) {
        feedbackReset(); forward = dir; benchStart();
        const uint32_t began = fd_power_policy.started;
        for (unsigned n = 1; n <= 11; n++) {
            char previous = step;
            feedbackRetry();
            CHECK(step == (dir ? (previous == 6 ? 1 : previous + 1) :
                (previous == 1 ? 6 : previous - 1)));
            CHECK(fd_rev.period == (n >= 2 && n < 8 ? FD_S4_RESCUE_TICKS : FD_REV_RETRY_TICKS));
            CHECK(!fd_rev.confidence && !zero_crosses && !adc_zcd_diag.accepted);
            CHECK(bemf_timeout_happened == n);
        }
        CHECK((uint32_t)(DWT->CYCCNT - began) == 160500u * FD_REV_US);
        CHECK(fd_rev.rescue_used && !fd_rev.rescue_left);
        benchAt(300000); CHECK(fd_power_bench.last_reason == FD_BENCH_NO_LOCK);
    }
    /* A qualified edge before/during the attempt returns to ordinary S3
     * event-led acquisition, even if a later edge is missing. */
    for (unsigned retry = 0; retry <= 7; retry++) {
        feedbackReset(); benchStart();
        for (unsigned n = 0; n < retry; n++) feedbackRetry();
        feedbackEdge(2000);
        CHECK(fd_rev.confidence == 1);
#ifdef TARGET_STARTUP_RESCUE_BRIDGE
        unsigned left = retry >= 2 ? 7 - retry : 0;
        CHECK(fd_rev.rescue_left == left);
        CHECK(fd_rev.period == (left ? FD_S4_RESCUE_TICKS : FD_REV_RETRY_TICKS));
#else
        CHECK(!fd_rev.rescue_left);
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS);
#endif
#ifdef TARGET_STARTUP_RESCUE_RETAIN
        CHECK(fd_rev.rescue_used == (retry >= 2) && !fd_rev.empty_retries);
#else
        CHECK(fd_rev.rescue_used);
#endif
        feedbackRetry(); feedbackRetry();
#ifdef TARGET_STARTUP_RESCUE_BRIDGE
        left = retry < 2 ? 6 : (left > 2 ? left - 2 : 0);
        CHECK(fd_rev.rescue_left == left);
        CHECK(fd_rev.period == (left ? FD_S4_RESCUE_TICKS : FD_REV_RETRY_TICKS));
#else
#ifdef TARGET_STARTUP_RESCUE_RETAIN
        CHECK(fd_rev.period == (retry >= 2 ? FD_REV_RETRY_TICKS : FD_S4_RESCUE_TICKS));
        CHECK(fd_rev.rescue_left == (retry >= 2 ? 0 : 6));
#else
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS && !fd_rev.rescue_left);
#endif
#endif
        feedbackLock(); CHECK(fd_rev.stage == FD_REV_CLOSED);
    }
    // Late edge cannot rescue an expired 8 ms sector; wrap-safe and no bursts.
    FdRevPolicy p; fdRevBeginPolicy(&p, UINT32_MAX - 100u);
    for (unsigned n = 0; n < 2; n++)
        CHECK(fdRevAdvancePolicy(&p, p.sector_at + p.period) == FD_REV_RETRY);
    CHECK(p.period == FD_S4_RESCUE_TICKS);
    CHECK(!fdRevEdgePolicy(&p, p.sector_at + p.period, 16) && !p.confidence);
    const uint32_t delayed = p.sector_at + 3u * p.period;
    CHECK(fdRevAdvancePolicy(&p, delayed) == FD_REV_RETRY && p.rescue_left == 5);
    CHECK(fdRevAdvancePolicy(&p, delayed) == FD_REV_NONE && p.rescue_left == 5);
    CHECK(!fdRevEdgePolicy(&p, p.sector_at + p.period - 1u, 16) && p.confidence == 1);
#ifdef TARGET_STARTUP_RESCUE_BRIDGE
    CHECK(p.rescue_left == 4 && p.period == FD_S4_RESCUE_TICKS);
#else
    CHECK(!p.rescue_left && p.period == FD_REV_RETRY_TICKS);
#endif

    // Actual floor integration: 30-edge boundary no longer causes 360->277.
    feedbackReset(); benchStart(); feedbackLock();
    const uint32_t closed = fd_rev.closed_at;
    DWT->CYCCNT = closed + 100000u * FD_REV_US;
    for (unsigned z = 29; z <= 31; z++) {
        zero_crosses = z; duty_cycle_setpoint = 277;
        actualStartupFloor(); CHECK(duty_cycle_setpoint == 360);
    }
    unsigned previous = 360;
    for (unsigned us = 100000; us <= 500000; us += 1000) {
        DWT->CYCCNT = closed + us * FD_REV_US;
        duty_cycle_setpoint = 277; actualStartupFloor();
        CHECK(duty_cycle_setpoint <= previous && duty_cycle_setpoint >= 277);
        CHECK(previous - duty_cycle_setpoint <= 1);
        previous = duty_cycle_setpoint;
        if (us <= 150000) CHECK(duty_cycle_setpoint == 360);
        if (us == 300000) CHECK(duty_cycle_setpoint == 285);
        if (us >= 450000) CHECK(duty_cycle_setpoint == 277 && !fd_power_bench.floor);
    }
    // Floor only: do not impose the 610 startup ceiling beyond original
    // eligibility, or hold a high throttle request down for 450 ms.
    DWT->CYCCNT = closed + 100000u * FD_REV_US;
    duty_cycle_setpoint = 800; actualStartupFloor(); CHECK(duty_cycle_setpoint == 800);
    zero_crosses = 29; duty_cycle_setpoint = 800; actualStartupFloor();
    CHECK(duty_cycle_setpoint == 610);
    zero_crosses = 100;
    hard_cap = 250; duty_cycle_setpoint = 200;
    actualStartupFloor(); CHECK(duty_cycle_setpoint == 250);
    input = 0; duty_cycle_setpoint = 0;
    actualStartupFloor(); CHECK(!duty_cycle_setpoint);
    adjusted_input = newinput = 0; fdBenchService();
    CHECK(!fd_power_bench.active_id && !fd_power_bench.floor);
    // Arithmetic independently checked with a 64-bit reference; no overflow
    // at normal floor delta 150 or the entire duty domain (0..2000).
    for (unsigned base = 0; base <= 2000; base += 40) {
        for (unsigned min = 0; min <= base; min += 20) {
            for (unsigned us = 150000; us <= 450000; us += 10000) {
                unsigned expected = us == 450000 ? 0 : min +
                    (uint64_t)(base - min) * (450000 - us) / 300000;
                CHECK(fdS4ClosedFloor(us * FD_REV_US, base, min) == expected);
            }
        }
    }
    CHECK(fdS4ClosedFloor(FD_S4_HOLD_TICKS, 200, 210) == 200);
    // Safety/zero/native latch beat the next fast commutation.
    for (unsigned reason = 0; reason < 3; reason++) {
        feedbackReset(); benchStart(); feedbackRetry(); feedbackRetry();
        CHECK(fd_rev.rescue_left == 6);
        unsigned writes = gate_writes;
        if (!reason) adjusted_input = newinput = 0;
        if (reason == 1) drive_safety_limit_inhibit = 1;
        if (reason == 2) { stuck_enabled = 1; bemf_timeout_happened = 11; CHECK(stuckRotorGuardInput()); }
        sequenceTickAt(fd_rev.sector_at + fd_rev.period);
        CHECK(gate_writes == writes && !fd_rev_acquiring);
        if (reason) CHECK(!gates && !pwm && !running);
    }
    feedbackReset(); DWT->CYCCNT = UINT32_MAX - 100u; benchStart();
    feedbackRetry(); feedbackRetry(); feedbackLock();
    DWT->CYCCNT = fd_rev.closed_at + 300000u * FD_REV_US;
    zero_crosses = 30; duty_cycle_setpoint = 277; actualStartupFloor();
    CHECK(duty_cycle_setpoint == 285);
}
#endif
#ifdef TARGET_STARTUP_S4
static void test_handoff_real_interval(void)
{
    // Reproduce #8/#17's accelerating edge intervals (rounded to us) through
    // the actual acquisition, handoff and ADC guard, not a copied gate model.
    const unsigned periods[2][6] = {
        {2000,22084,10454,8262,7243,6732},
        {2000,16473,10608,8620,7548,6631}
    };
    const unsigned crossing_us[2] = {5961,5298};
    for (unsigned k = 0; k < 2; k++) {
        for (unsigned dir = 0; dir <= 1; dir++) {
            feedbackReset(); forward = dir; benchStart();
            for (unsigned n = 0; n < 6; n++) feedbackEdge(periods[k][n]);
            uint32_t smooth = fd_rev.measured, delay = fd_rev.delay;
            CHECK(smooth / 80u > 2u * crossing_us[k]);
            CHECK(delay == smooth / 2u - smooth * temp_advance / 64u);
            sequenceTickAt(fd_rev.edge_at + delay);
            CHECK(fd_rev.stage == FD_REV_CLOSED && commutation_interval == smooth / 80u);
            CHECK(fd_rev.delay == delay); // leave first commutation timing alone
#ifdef TARGET_STARTUP_HANDOFF_REAL
            CHECK(fd_rev.last_real_period == periods[k][5] * FD_REV_US);
            CHECK(adc_zcd_last_real_interval == 2u * periods[k][5]);
            CHECK(adc_zcd_earliest == periods[k][5]);
#else
            CHECK(adc_zcd_earliest == (smooth / 80u) / 2u);
#endif
            unsigned accepted = adc_zcd_diag.accepted;
            pre(delay / 80u + 200); pre(delay / 80u + 300); pre(delay / 80u + 400);
            post(2u * crossing_us[k]); post(2u * crossing_us[k] + 104u);
#ifdef TARGET_STARTUP_HANDOFF_REAL
            CHECK(adc_zcd_diag.accepted == accepted + 1u && bemfcounter);
#else
            CHECK(adc_zcd_diag.accepted == accepted && !bemfcounter);
            post(adc_zcd_earliest + 500u);
            CHECK(adc_zcd_diag.accepted == accepted && !adc_zcd_before_seen);
#endif
        }
    }
#ifdef TARGET_STARTUP_HANDOFF_REAL
    // Reset/first edge cannot carry an old trial's interval into handoff.
    FdRevPolicy p; fdRevBeginPolicy(&p, UINT32_MAX - 100u);
    CHECK(!p.last_real_period);
    fdRevEdgePolicy(&p, p.sector_at + 1000u * FD_REV_US, 16);
    CHECK(!p.last_real_period);
    fdRevAdvancePolicy(&p, p.edge_at);
    fdRevEdgePolicy(&p, p.sector_at + 2000u * FD_REV_US, 16);
    CHECK(p.last_real_period == 2000u * FD_REV_US);
    fdRevAdvancePolicy(&p, p.edge_at);
    fdRevAdvancePolicy(&p, p.sector_at + p.period);
    CHECK(!p.last_real_period && !p.confidence);
    // The fix is initialization, NOT accepting an early pulse later when
    // the clock passes the gate. That pulse still disarms the detector.
    feedbackReset(); benchStart(); feedbackLock();
    pre(500); pre(600); pre(700);
    unsigned accepted = adc_zcd_diag.accepted;
    post(adc_zcd_earliest); post(adc_zcd_earliest + 100);
    CHECK(adc_zcd_diag.accepted == accepted && !adc_zcd_before_seen);
#endif
}
#endif
#ifdef TARGET_STARTUP_S4
static void test_retained_rescue(void)
{
    // #3 shape: two early real edges, a timeout, one isolated real edge,
    // then two missing sectors. Exercise the actual ADC and bridge adapter.
    for (unsigned dir = 0; dir <= 1; dir++) {
        feedbackReset(); forward = dir; benchStart();
        feedbackEdge(19827); feedbackEdge(9282); feedbackRetry();
        CHECK(bemf_timeout_happened == 1 && !fd_rev.confidence);
        feedbackEdge(10466); CHECK(fd_rev.confidence == 1);
        feedbackRetry(); CHECK(fd_rev.period == FD_REV_RETRY_TICKS);
        feedbackRetry(); CHECK(!zero_crosses && adc_zcd_diag.accepted == 3);
        CHECK(bemf_timeout_happened == 3);
#ifdef TARGET_STARTUP_RESCUE_RETAIN
        CHECK(fd_rev.rescue_used && fd_rev.rescue_left == 6);
        CHECK(fd_rev.period == FD_S4_RESCUE_TICKS);
        // First real edge interrupts the rescue, but never refunds budget.
        feedbackEdge(2000);
#ifdef TARGET_STARTUP_RESCUE_BRIDGE
        CHECK(fd_rev.rescue_used && fd_rev.rescue_left == 5);
        feedbackRetry(); feedbackRetry();
        CHECK(fd_rev.rescue_left == 3 && fd_rev.period == FD_S4_RESCUE_TICKS);
#else
        CHECK(fd_rev.rescue_used && !fd_rev.rescue_left);
        feedbackRetry(); feedbackRetry();
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS && !fd_rev.rescue_left);
#endif
#else
        CHECK(fd_rev.rescue_used && !fd_rev.rescue_left);
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS);
#endif
        feedbackLock(); CHECK(fd_rev.stage == FD_REV_CLOSED);
        sequenceComplete(); CHECK(fd_power_bench.last_reason == FD_BENCH_WINDOW);
    }
#ifdef TARGET_STARTUP_RESCUE_RETAIN
    // Non-consecutive timeouts cannot accumulate into rescue. Qualified
    // feedback resets the missing-sector counter, not native fault count.
    feedbackReset(); benchStart();
    for (unsigned n = 0; n < 5; n++) {
        feedbackEdge(1000); CHECK(!fd_rev.empty_retries && !fd_rev.rescue_used);
        feedbackRetry(); CHECK(fd_rev.empty_retries == 1 && !fd_rev.rescue_used);
        CHECK(bemf_timeout_happened == n + 1);
    }
    feedbackRetry(); CHECK(fd_rev.rescue_used && fd_rev.rescue_left == 6);
    feedbackLock(); CHECK(fd_rev.stage == FD_REV_CLOSED && !fd_rev.rescue_left);
    // Closed-loop loss must stop, never fall back into another attempt.
    unsigned writes = gate_writes;
    interval_timer = 45001; fd6288TimeoutStep();
    CHECK(fd_power_bench.last_reason == FD_BENCH_LOST_FEEDBACK);
    fdRevTick(); CHECK(gate_writes == writes && !gates && !pwm);
    // Real operator zero/new trial renews the budget; forced internal zero
    // or positive throttle cannot do so after the stopped trial.
    CHECK(fdBenchGuardInput() && !fdBenchBegin());
    adjusted_input = newinput = 0; CHECK(!fdBenchGuardInput());
    benchStart(); CHECK(!fd_rev.rescue_used && !fd_rev.empty_retries);

    // Native threshold wins before a prospective rescue drives a new step.
    feedbackReset(); stuck_enabled = 1; bemf_timeout = 2; benchStart();
    feedbackEdge(1000); feedbackRetry(); feedbackEdge(1000); feedbackRetry();
    writes = gate_writes; feedbackRetry();
    CHECK(stuck_rotor_inhibit && fd_power_bench.last_reason == FD_BENCH_NATIVE_STUCK);
    CHECK(gate_writes == writes && !gates && !pwm);

    // Delayed service near the deadline may start a rescue, but cannot
    // extend 300 ms to complete its six sectors. Test across DWT wrap too.
    feedbackReset(); DWT->CYCCNT = UINT32_MAX - 100u; benchStart();
    feedbackEdge(1000);
    sequenceTickAt(fd_power_policy.started + 272000u * FD_REV_US);
    CHECK(fd_rev.empty_retries == 1 && !fd_rev.rescue_used);
    feedbackRetry(); CHECK(fd_rev.rescue_left == 6);
    writes = gate_writes;
    sequenceTickAt(fd_power_policy.started + FD_REV_ACQUIRE_TICKS);
    CHECK(fd_power_bench.last_reason == FD_BENCH_NO_LOCK && !gates && !pwm);
    CHECK(gate_writes == writes);
    // Genuine zero cancels an unused future opportunity without drive.
    feedbackReset(); benchStart(); feedbackEdge(1000); feedbackRetry();
    writes = gate_writes; adjusted_input = newinput = 0;
    sequenceTickAt(fd_rev.sector_at + fd_rev.period);
    CHECK(gate_writes == writes && !fd_rev_acquiring && !fd_power_bench.active_id);
#endif
}
#endif
static void test_feedback(void)
{
#ifdef TARGET_STARTUP_RESCUE_BRIDGE
    for (unsigned dir = 0; dir <= 1; dir++) {
        // #7: five fast sectors entered, isolated edge consumes fifth;
        // only ONE short sector remains. Timeout cannot renew that budget.
        feedbackReset(); forward = dir; benchStart();
        for (unsigned i = 0; i < 6; i++) feedbackRetry();
        CHECK(fd_rev.rescue_left == 2);
        feedbackEdge(2243); CHECK(fd_rev.rescue_left == 1);
        CHECK(fd_rev.confidence == 1 && fd_rev.period == FD_S4_RESCUE_TICKS);
        feedbackRetry(); CHECK(!fd_rev.rescue_left && !fd_rev.confidence);
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS);
        feedbackRetry(); CHECK(!fd_rev.rescue_left && fd_rev.rescue_used);
        // #10: edge in the LAST short sector has no remaining budget.
        feedbackReset(); forward = dir; benchStart();
        for (unsigned i = 0; i < 7; i++) feedbackRetry();
        CHECK(fd_rev.rescue_left == 1);
        feedbackEdge(6691); CHECK(!fd_rev.rescue_left);
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS);
        // Two consecutive edges give feedback sole control immediately.
        feedbackReset(); forward = dir; benchStart();
        feedbackRetry(); feedbackRetry(); feedbackEdge(2000);
        CHECK(fd_rev.rescue_left == 5);
        feedbackEdge(2000); CHECK(fd_rev.confidence == 2 && !fd_rev.rescue_left);
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS && fd_rev.rescue_used);
        // Alternating solitary edges and timeouts consume, never replenish.
        feedbackReset(); forward = dir; benchStart();
        feedbackRetry(); feedbackRetry();
        for (unsigned i = 0; i < 3; i++) {
            feedbackEdge(1000); CHECK(fd_rev.rescue_left == 5 - 2 * i);
            feedbackRetry(); CHECK(fd_rev.rescue_left == 4 - 2 * i);
        }
        CHECK(fd_rev.period == FD_REV_RETRY_TICKS && fd_rev.rescue_used);
        // Native protection can stop the retained final sector before drive.
        feedbackReset(); forward = dir; benchStart();
        feedbackRetry(); feedbackRetry(); feedbackEdge(2000);
        stuck_enabled = 1; bemf_timeout = bemf_timeout_happened;
        unsigned writes = gate_writes; feedbackRetry();
        CHECK(stuck_rotor_inhibit && !gates && !pwm && gate_writes == writes);
    }
#endif
    test_all_phases(); test_confirmation(); test_failed_start_edge_shape();
    test_time_gate_and_resets(); test_coast_unchanged();
#ifdef TARGET_STARTUP_S4
    test_s4();
    test_handoff_real_interval();
    test_retained_rescue();
#endif
#ifdef TARGET_STARTUP_S3_POWER
    // Higher REQUEST, same native initial output seed and ramp. No direct
    // jump to 540/600, no EEPROM edits, no extension of the powered window.
    feedbackReset(); benchStart();
    CHECK(min_startup_duty == 360 && startup_max_duty_cycle == 610);
    CHECK(last_duty_cycle == 360 && fd_power_bench.floor == 540);
    duty_cycle_setpoint = 2000; actualStartupFloor();
    for (unsigned i = 1; i <= 100; i++) {
        duty_cycle = duty_cycle_setpoint; ramp_count = 10; actualRamp();
        CHECK(duty_cycle == (i < 90 ? 360 + 2 * i : 540));
        last_duty_cycle = duty_cycle;
    }
    benchAt(29999); CHECK(fd_power_bench.floor == 540);
    benchAt(30000); CHECK(fd_power_bench.floor == 600);
    actualStartupFloor();
    for (unsigned i = 1; i <= 40; i++) {
        duty_cycle = duty_cycle_setpoint; ramp_count = 10; actualRamp();
        CHECK(duty_cycle == (i < 30 ? 540 + 2 * i : 600));
        last_duty_cycle = duty_cycle;
    }
    hard_cap = 490; fdBenchService(); CHECK(fd_power_bench.floor == 490);
    actualStartupFloor(); CHECK(duty_cycle_setpoint == 490);
    hard_cap = 2000;
    // Post-handoff request releases 600 -> 360 within the SAME 20 ms from
    // the sixth edge as S3. Early handoff never exceeds the initial 540 cap.
    for (unsigned early = 0; early <= 1; early++) {
        feedbackReset(); benchStart(); benchAt(early ? 5000 : 60000);
        fd_rev.stage = FD_REV_CLOSED;
        fd_rev.closed_at = fd_rev.edge_at = DWT->CYCCNT;
        const uint32_t edge = fd_rev.edge_at;
        fdBenchService(); CHECK(fd_power_bench.floor == (early ? 540 : 600));
        DWT->CYCCNT = edge + 10000u * FD_REV_US;
        fdBenchService(); CHECK(fd_power_bench.floor == 480);
        DWT->CYCCNT = edge + 20000u * FD_REV_US;
        fdBenchService(); CHECK(fd_power_bench.floor == 360);
        DWT->CYCCNT = edge + 30000u * FD_REV_US;
        fdBenchService(); CHECK(fd_power_bench.floor == 360);
    }
    feedbackReset(); benchStart(); hard_cap = 0; fdBenchService();
    CHECK(fd_power_bench.last_reason == FD_BENCH_SAFETY_CAP && !gates && !pwm);
    CHECK(fd_power_bench.floor == 0);
#endif
    for (unsigned dir = 0; dir <= 1; dir++) {
        feedbackReset(); forward = dir; benchStart();
        CHECK(step == 1 && fd_rev.stage == FD_REV_OPEN);
        duty_cycle_setpoint = 2000; actualStartupFloor(); CHECK(duty_cycle_setpoint == TEST_INITIAL_REQUEST);
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
    CHECK(fd_power_bench.floor == TEST_ACQUIRE_REQUEST); /* now >30 ms */
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
