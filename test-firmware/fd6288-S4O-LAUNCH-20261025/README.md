# S4O-LAUNCH — compiled, not flashed

Main HEX: AM32_CUSTOM_FD6288_G431_S4O_LAUNCH.hex
Symbols: S4O_LAUNCH.elf
HEX SHA256: 209cf69a0e201a355a8c0edabf20e6123cc9f4073a7c418382390d5fb8600c7c
36620 data bytes, 0x08001000..0x0800F7FF only. No BL/EEPROM data.

Based on S4N-STATS. First acquisition vector advances one additional sector
after unchanged double alignment; one-time5us all-off gap guards the jump.
Forward alignment5,6→launch2; reverse alignment3,2→launch6. Subsequent sequence,
power limits, deadline, ADC decisions, running timing and stop guards unchanged.
Light history and latest-trial sector statistics retained. No runtime recovery.

Build exactlyCUSTOM_FD6288_G431:
`FD6288_LAUNCH_ADVANCE=1 FD6288_ACQ_STATS=1 FD6288_ALIGN_HOLD=1 FD6288_DIAGNOSTICS=0 FD6288_PIN_SCAN=0 FD6288_PIN_SYNC=0 node tools/scripts/build_fd6288_s4j.cjs alignnative`

Host policy/vector, adapter structural, scheduler, flag and sector ABI tests
passed; image bounds checked. Not physical timing/torque validation. Existing
newlib-stub/RWX linker warnings remain. See docs/FD6288-S4O-LAUNCH-20261025.md.
Not flight-ready. Do not keep testing if the launch kick or heating worsens.
