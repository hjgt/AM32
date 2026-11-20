"""Add the M2 waveform observer to an extracted November 11 Release tree."""
import pathlib
import sys

tree = pathlib.Path(sys.argv[1])
main = tree / "Src/main.c"
source = main.read_text()
patches = [
    ("static void fdRevTick(void)\n{",
     '#include "fd6288_release_closed_capture.inc"\n\nstatic void fdRevTick(void)\n{'),
    ("        fdTraceEvent(FD_EVENT_REV_HANDOFF, fd_rev.confidence);\n        commutate();",
     "        fdTraceEvent(FD_EVENT_REV_HANDOFF, fd_rev.confidence);\n"
     "        fdReleaseCaptureBegin(now);\n        commutate();"),
    ("        getBemfState(); /* sole JEOS consumer in this build, includes coast */",
     "        getBemfState(); /* sole JEOS consumer in this build, includes coast */\n"
     "        fdReleaseCaptureSample();"),
    ("        comStep(step);\n#ifdef TARGET_FD_SCOPE_PB7",
     "        comStep(step);\n        fdReleaseCaptureSwitch();\n#ifdef TARGET_FD_SCOPE_PB7"),
    ("    uint32_t accepted_at = DWT->CYCCNT;\n#endif\n#ifdef TARGET_FD_NATIVE_SINE",
     "    uint32_t accepted_at = DWT->CYCCNT;\n    fdReleaseCaptureEdge(accepted_at);\n"
     "#endif\n#ifdef TARGET_FD_NATIVE_SINE"),
    ("    save_flash_nolib(eepromBuffer.buffer, sizeof(eepromBuffer.buffer), eeprom_address);",
     "    /* Keep the board settings page intact in this capture image. */"),
    ("    read_flash_bin(eepromBuffer.buffer, eeprom_address, sizeof(eepromBuffer.buffer));",
     "    read_flash_bin(eepromBuffer.buffer, eeprom_address, sizeof(eepromBuffer.buffer));\n"
     "    /* Release admission profile. Apply in RAM for the M2 comparison. */\n"
     "    eepromBuffer.use_sine_start = 0u;\n"
     "    eepromBuffer.startup_power = 150u;\n"
     "    eepromBuffer.minimum_duty_cycle = 1u;\n"
     "    eepromBuffer.pwm_frequency = 24u;\n"
     "    eepromBuffer.comp_pwm = 1u;\n"
     "    eepromBuffer.variable_pwm = 0u;\n"
     "    eepromBuffer.bi_direction = 0u;\n"
     "    eepromBuffer.rc_car_reverse = 0u;"),
]
for old, new in patches:
    if source.count(old) != 1:
        raise SystemExit(f"Wrong Release tree or existing patch at {old!r}")
    source = source.replace(old, new, 1)
adapter = pathlib.Path(__file__).resolve().parents[2] / "Src/fd6288_release_closed_capture.inc"
(tree / "Src" / adapter.name).write_bytes(adapter.read_bytes())
main.write_text(source)
