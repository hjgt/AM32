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
