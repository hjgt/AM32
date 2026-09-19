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
