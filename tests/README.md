# FD6288/G431 host checks

Run from the repository root:

```sh
node tests/run_fd6288_host_tests.cjs
```

Requires Node.js and a host C compiler with AddressSanitizer and
UndefinedBehaviorSanitizer (macOS `cc`/Clang works). The runner uses a unique
temporary directory, reads the actual ADC branch, declarations, and selected
main control functions, substitutes registers/GPIO with stubs, and executes
the harness. It does not access a debugger or compile another board target.

Coverage: all six phases/both polarities, static post-cross plateaus, blanking,
fresh-only reads, hysteresis, invalid samples, low/high-speed confirmation,
earliest-event boundary, artificial startup-period bounds, acquisition resets,
unchanged Coast observation, trace wrap/freeze, native stuck threshold/switch,
latched shutdown, explicit operator-zero recovery, pending commutation, and
interrupt-mask preservation.

The runner now tests both startup variants (A: original timing; B: seed from
the second actual DRIVE crossing and separate atomic timeout stepping), plus
the real RAM logger, wraparound, freeze/rearm, unchanged Coast period filtering,
the stale-timeout race, and the native stuck-rotor threshold after forced steps.
Current totals: A 474 assertions; B 488 assertions. These include the earlier
362 assertions and 72 checks prompted by the captured failed-start edge shape:
post-side plateau followed by the wrong-direction transition must not be
accepted, while a later properly confirmed edge still works in all six phases
and both polarities. This is a synthetic shape regression, not a replay of
overwritten samples or a motor model. The earlier A/B totals were 402/416.

Capture-tool tests use synthetic memory and a mock RPC provider only:

```sh
node tests/fd6288_capture_tests.cjs obj-startup-timing-B/AM32_CUSTOM_FD6288_G431_2.20.elf
```

See [the evening procedure](../NIGHT-TEST-2026-09-20.md) for explicit single-board
build commands and capture use. Trace and timing changes are opt-in build flags;
normal builds default to both disabled. Use separate OBJ directories for each
flag combination; if reusing an existing directory with different flags, force
a rebuild (`make -B` with the same explicit board target). Make does not track
command-line compiler flag changes as source dependencies.

These are function-level tests, **not** a motor simulation, full-firmware
interrupt scheduler, or evidence of safe loaded operation. ADC sample age,
analog ringing/demagnetization, real startup torque, filter delay, missed
crossings, ISR execution time, and power-stage operation require bench tests.
The harness extracts functions by balanced braces; fail/review it if source
layout or preprocessor boundaries change. Target feature assumptions are
checked before compiling.

Only build the requested firmware target:

```sh
make -j1 OBJ=obj-loaded-zcd-candidate CUSTOM_FD6288_G431 ARM_SDK_PREFIX=arm-none-eabi-
node tests/check_fd6288_image.cjs obj-loaded-zcd-candidate/AM32_CUSTOM_FD6288_G431_2.20.bin obj-loaded-zcd-candidate/AM32_CUSTOM_FD6288_G431_2.20.hex
```

The image check verifies HEX checksums/addresses, main-only flash coverage,
initial stack/reset vectors, `.file_name` coverage, and BIN/HEX consistency.
It does **not** program flash or validate the settings currently in the ESC.

## Startup-power policy and integrated P0/P1/S1 bench adapters

```sh
node tests/run_fd6288_power_policy_tests.cjs
```

This builds only `experiments/fd6288-startup-power/policy_test.c` on the host,
with ASan/UBSan. It checks the shared one-shot floor boost, release, caps,
zero-command rearming, raw-cycle wrap and shared P0/P1 bench-stop request.
It does NOT build firmware, connect hardware, implement a bridge shutdown, or
certify safe startup/current. The firmware adapter is independently exercised
by `run_fd6288_host_tests.cjs`: A 474, B 488, P0/P1 226 each, S1 12626 assertions
(including repeated deadline/timeout service checks, not 12626 independent scenarios).

S1 additionally tests actual ADC acceptance into the rev-up policy, both
directions, the 30 ms alignment, adjacent phase/period-qualified edges,
delayed handoff with a real-edge-based interval seed, no fabricated ZCs/native
timeouts, no-edge shutdown with protection Off/On, output guards after stop,
no positive-command retrigger, native fault priority/recovery, first-failure
trace retention, manual stopped rearm, raw-counter wrap and delayed service.
S1 is opt-in, requires trace and timing fix, excludes P0/P1, and still stops
every trial at 300 ms. It is NOT a continuous-operation or flight image.

```sh
make -j1 CUSTOM_FD6288_G431 OBJ=obj-startup-sequence-S1 ARM_SDK_PREFIX=arm-none-eabi- FD6288_STARTUP_TRACE=1 FD6288_STARTUP_TIMING_FIX=1 FD6288_STARTUP_SEQUENCE=1
node tests/check_fd6288_image.cjs obj-startup-sequence-S1/AM32_CUSTOM_FD6288_G431_2.20.bin obj-startup-sequence-S1/AM32_CUSTOM_FD6288_G431_2.20.hex
node tests/fd6288_capture_tests.cjs obj-startup-sequence-S1/AM32_CUSTOM_FD6288_G431_2.20.elf
```

The last command uses only mock RPC/memory, not the attached debugger. The
capture decoder understands S1 event kinds, trial stages and failure reasons;
`capture_id` identifies the retained trial, which need not be the latest trial.

### S2: slower ramp, late-edge settling, separate observation deadline

S2 uses `FD6288_STARTUP_SEQUENCE=2`, trace variant 5; S1 stays at option 1.
The common host runner now checks S2 (12804 assertions), including six-step
settling without weaker handoff qualification, no extension for partial
streaks, 1 s from actual handoff, blocking-wait deadline shutdown and immediate
bench stop on the first runtime timeout with native protection Off/On.
The shared S1 regression now has 12627 assertions (one existing combined
assertion was split). No motor simulation or flight-readiness claim.

```sh
node tests/fd6288_s1_replay.cjs
node tests/fd6288_capture_tests.cjs obj-startup-sequence-S2/AM32_CUSTOM_FD6288_G431_2.20.elf 5
```

Replay uses the real, retained S1 binary trace with all 123 events, verifies
every recorded policy confidence result, and reproduces no handoff. It must
not be used to predict S2 motor behavior using unchanged S1 edge timestamps.
Capture tests use mock RPC only; pass explicit variant 5 for S2. Its summary
reserved byte has stage in the low nibble and maximum streak in the high
nibble. Earlier capture ABIs remain readable.

### S3: event-led acquisition, paired variant 7

`FD6288_STARTUP_SEQUENCE=3` requires trace=1, timing_fix=1 and pair_trace=1.
The host runner now adds 773 assertions for the actual ADC/adapter/commutation
path: first five qualified edges act immediately, second-edge period seeding,
sixth-edge delayed handoff, retries without fake ZCs, recovery after retry,
native threshold Off/On, stop/rearm/output interlocks, wrap and deadlines.
The first 30 ms power envelope is preserved without an alignment dwell.
Six consecutive edges no longer mean S2's fixed-sector phase qualification.

The paired runner tests both variant 6 and 7 through the real C implementation
and JS decoder, including reused-record initialization, retention, overflow,
and read-only mock RPC. S3 may exceed the 72-record capacity after repeated
partial streaks; explicit overflow is not silently treated as complete evidence.
The decoder exposes `retry_deadline_us` for S3, not a commanded sector period.
No tests here simulate a physical motor or validate flight/protection behavior.
