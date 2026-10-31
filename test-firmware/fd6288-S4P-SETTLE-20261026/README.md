# S4P-SETTLE — compiled, not flashed

Main: AM32_CUSTOM_FD6288_G431_S4P_SETTLE.hex
Symbols: S4P_SETTLE.elf
HEX SHA256:5cf6e42815518e6a048f4f206702c6743c09f16613072d63b4c78c5e5168b431
37256 data bytes, only0x08001000..0x0800F7FF. No BL/settings data.

Second positioning hold30→60ms, hard startup deadline360→390ms; power requests
unchanged, S4O launch kept. Adds808byte passive alignment statistics. Alignment
samples never participate in zero-cross qualification. All other protections
remain, running recovery is NOT enabled. Longer hold adds thermal exposure.

Read using tools/scripts/fd6288_sector_capture.cjs with this ELF; output includes
alignment bins plus latest acquisition sectors and32 light result summaries.
Return zero/stop and preserve power before capture; next start overwrites bins.
See docs/FD6288-S4P-SETTLE-20261026.md for tests and limitations.
