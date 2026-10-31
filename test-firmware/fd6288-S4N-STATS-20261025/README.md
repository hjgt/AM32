# S4N-STATS — not flashed

Control baseline: S4N ALIGN_HOLD; adds acquisition-sector statistics only.
Main image: AM32_CUSTOM_FD6288_G431_S4N_STATS.hex
Matching symbols: S4N_STATS.elf
HEX SHA256: fe1a183f0edfeaa9659680c051499cb59036c8bc17170fe6257da5c0335b5a0b
36528 HEX data bytes, all in 0x08001000..0x0800F7FF. No BL/settings payload.
RAM6976bytes, flash code36024bytes. Existing newlib-stub/RWX linker warnings remain.

FD6288_ACQ_STATS=1 FD6288_ALIGN_HOLD=1 FD6288_DIAGNOSTICS=0
FD6288_PIN_SCAN=0 FD6288_PIN_SYNC=0
node tools/scripts/build_fd6288_s4j.cjs alignnative

Large waveform logging is absent. Latest trial's first24 sector statistics
freeze at handoff/stop; light history retains32 outcomes. Read after a normal
trial, then stop/read immediately after a failed trial. Another start overwrites
sector statistics; power loss clears all RAM records.

No running-recovery MCU adapter is included. Stop guards are NOT relaxed.
Host recovery policy tests do not establish hardware safety or flight readiness.
