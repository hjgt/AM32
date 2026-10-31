# Finite running recovery: offline policy prototype

## Status — NOT a firmware feature yet

`Inc/fd6288_recovery_policy.h` implements a pure, host-tested state machine.
It is deliberately not included or called by the MCU firmware. No build switch
can currently enable it on the aircraft. Existing 22.5ms/50us stops remain.
This prevents an unreviewed phase-prediction adapter from reaching hardware.
The startup-statistics HEX does NOT provide airborne recovery.

## Implemented policy boundaries (experimental)

- Eligibility requires 48 consecutive plausible real feedback edges, at least
  20ms of history, and sector periods within 200–2000us. It never handles cold
  startup or stationary alignment. Gate must preempt tick/edge processing.
- A missing edge at 1.5 trusted periods requests recovery. At 2 periods or later,
  the estimate is rejected as stale; this is not a 22.5ms blind-restart scheme.
- Entry requests a duty cap of min(entry duty/2,400), never exceeding current
  demand. If below the adapter's valid ADC-window floor, refuse recovery.
  This is a duty limit, NOT measured power/current/torque regulation.
- Search budget min(12 trusted periods,10ms), at most two predicted-step
  requests. Requests do not create feedback edges or update measured speed.
  A delayed probe beyond one quarter period is rejected, with no catch-up loop.
- Six consecutive physical feedback edges with plausible intervals earn a
  20ms capped release toward entry duty. The first is a seed, not a real period.
  Loss during release stops. A second recovery in the same nonzero command
  is prohibited; only operator zero resets the budget. Fault/direction/coast
  incompatibility must preempt via the gate; persistent faults cannot be cleared
  by simply supplying zero while the fault remains asserted.

These thresholds are explicit test values, not justified motor-specific limits.
The allowance is intentionally finite; whether two predicted steps can safely
recapture this hardware remains unknown. Nothing here proves rotor direction.

## MCU adapter still required before an experimental recovery image

1. Track accepted feedback timestamps plus actual commutation epoch/phase. A
   requested asynchronous commutation may not have executed; cancel/synchronize
   ownership before creating a probe, never double-advance a sector.
2. Apply the cap through the common duty/safety path BEFORE bridge changes.
   Check the valid ADC aperture at that cap and PWM frequency. Resolve the
   startup floor so it cannot override the recovery cap.
3. Choose and validate probe phase from trustworthy rotor history. The prototype
   requests a probe but intentionally does not select a physical vector.
4. Clear only the appropriate ADC qualification state. Never seed a synthetic
   crossing or IIR period. Cancel old timer epochs and make fault/zero gating
   atomic with bridge action. Do not invoke double alignment while rotating.
5. Map recovered real intervals back to asynchronous scheduling without counting
   prediction as measurement; handle ramp termination and native stuck counters.
6. Add adapter-level tests (including actual ISR overlap/epoch, direction, coast,
   zero, fault, already-pending timer, ADC invalidity and stalled service), then
   build a separately named, opt-in bench image. Normal startup should be frozen
   for this comparison. No flight testing until bench validation is sufficient.

Host runner: `node tests/run_fd6288_recovery_tests.cjs`.
Coverage: eligibility, DWT-style unsigned wrap, duty floor/cap/ramp, finite
time/probes, six real edges vs duplicates, late/stale rejection, fault/zero
preemption, inability to replenish the recovery budget.
