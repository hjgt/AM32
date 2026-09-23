// Compile the actual ADC branch and selected actual main.c functions against
// register stubs. No source changes, MCU connection, or firmware build here.
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { spawnSync } = require('node:child_process');
const assert = require('node:assert/strict');
const root = path.resolve(__dirname, '..');
const read = (name) => fs.readFileSync(path.join(root, name), 'utf8');
const targets = read('Inc/targets.h');
const targetStart = targets.indexOf('#ifdef CUSTOM_FD6288_G431');
const target = targets.slice(targetStart, targets.indexOf('#ifndef FIRMWARE_NAME', targetStart));
for (const flag of ['TARGET_ADC_ZCD_GUARD', 'TARGET_STUCK_ROTOR_LATCH', 'HELI_COAST_ON_ZERO', 'HELI_COAST_BAILOUT']) {
  assert(new RegExp(`^#define ${flag}\\s*$`, 'm').test(target), `Target disabled ${flag}`);
}
assert(/^#define TARGET_MIN_BEMF_COUNTS 0$/m.test(target));
assert(/^#define HELI_COAST_ADC_MIN_SPAN 64u$/m.test(target));
const stripIncludes = (source) => source.replace(/^#include[^\n]*$/gm, '');
function functionBody(source, signature) {
  const start = source.indexOf(signature);
  if (start < 0) throw new Error(`Missing function: ${signature}`);
  const open = source.indexOf('{', start);
  let depth = 0;
  for (let i = open; i < source.length; ++i) {
    if (source[i] === '{') ++depth;
    if (source[i] === '}' && --depth === 0) return source.slice(start, i + 1);
  }
  throw new Error(`Unterminated function: ${signature}`);
}
const comparator = read('Mcu/g431/Src/comparator.c');
const start = comparator.indexOf('#ifdef USE_ADC_ZCD\n');
const end = comparator.indexOf('#else /* !USE_ADC_ZCD');
if (start < 0 || end < start) throw new Error('ADC branch markers changed');
const adc = stripIncludes(comparator.slice(start, end)) + '\n#endif\n';
const header = stripIncludes(read('Mcu/g431/Inc/comparator.h'));
const main = read('Src/main.c');
const signatures = [
  'static void heliCoastTrackingLost(uint8_t inhibit_restart)',
  'static void stuckRotorStop(void)',
  'static uint8_t stuckRotorGuardInput(void)',
  'void getBemfState()',
  'void commutate()',
  'void zcfoundroutine()',
];
const adapter = main.slice(main.indexOf('/* FD_POWER_BENCH_ADAPTER_BEGIN'),
  main.indexOf('/* FD_POWER_BENCH_ADAPTER_END */'));
assert(adapter.length > 1000);
const functions = signatures.map((s, i) => functionBody(main, s) + (i === 2 ?
  `\n#ifdef TARGET_STARTUP_POWER_BENCH\n${adapter}\n#endif\n` : '')).join('\n');
const harness = read('tests/fd6288_host_tests.c');
const marker = '/* FIRMWARE_FUNCTIONS */';
if (!harness.includes(marker)) throw new Error('Missing harness marker');
const traceHeader = stripIncludes(read('Inc/fd6288_startup_trace.h')).replace('#pragma once', '');
const trace = stripIncludes(read('Src/fd6288_startup_trace.c'));
const timeout = functionBody(main, 'static void fd6288TimeoutStep(void)');
const powerHeader = stripIncludes(read('Inc/fd6288_startup_sequence.h')) + '\n' +
  stripIncludes(read('Inc/fd6288_startup_power_policy.h')) + '\n' +
  stripIncludes(read('Inc/fd6288_startup_power.h')).replace('#pragma once', '');
const phase = read('Mcu/g431/Src/phaseouts.c');
const phaseFunctions = ['void comStep(int newStep)', 'void allpwm()']
  .map(s => functionBody(phase, s)).join('\n');
const startupFloor = main.slice(main.indexOf('/* FD_STARTUP_FLOOR_BEGIN */'), main.indexOf('/* FD_STARTUP_FLOOR_END */'));
assert(startupFloor.includes('uint16_t startup_floor = fd_power_bench.floor'));
const ramp = functionBody(main, 'if (ramp_count > ramp_divider)');
const source = harness.replace(marker, `${powerHeader}\n${traceHeader}\n${header}\n${trace}\n${adc}\n${functions}\n#ifdef TARGET_STARTUP_POWER_BENCH\n${phaseFunctions}\nstatic void actualStartupFloor(void) { ${startupFloor} }\nstatic void actualRamp(void) { ${ramp} }\n#endif\n#if FD6288_STARTUP_TIMING_FIX\n${timeout}\n#endif`)
  .replace('/* POWER_BENCH_TESTS */', read('tests/fd6288_power_adapter_tests.c') + '\n' + read('tests/fd6288_sequence_tests.c') + '\n' + read('tests/fd6288_feedback_tests.c'));
// Ensure the tested adapter is hooked into all output/timing entry points.
for (const [signature, token] of [
  ['void setInput()', 'fdBenchGuardInput()'], ['void setInput()', 'fdBenchBegin()'],
  ['void setInput()', 'uint16_t startup_floor = fd_power_bench.floor'],
  ['void tenKhzRoutine()', 'fdBenchWritePwm(adjusted_duty_cycle)'],
  ['void zcfoundroutine()', 'fdBenchDeadlineExpired()'],
  ['void PeriodElapsedCallback()', 'if (fd_power_bench.inhibit) return;'],
  ['void setInput()', 'fdRevStartDrive()'],
  ['void tenKhzRoutine()', 'fdRevTick()'],
  ['void zcfoundroutine()', 'fdRevObserveEdge()'],
  ['void PeriodElapsedCallback()', 'if (fd_rev_acquiring) return;'],
]) assert(functionBody(main, signature).includes(token), `Missing hook: ${signature}: ${token}`);
const output = fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-host-tests-'));
for (const variant of [0, 1, 2, 3, 4, 5, 7, 8, 11, 12, 13]) {
const binary = path.join(output, `test-${variant}`);
const compile = spawnSync(process.env.CC || 'cc', [
  `-DFD6288_STARTUP_TIMING_FIX=${[1, 4, 5, 7, 8, 11, 12, 13].includes(variant) ? 1 : 0}`,
  ...(variant >= 4 ? ['-DTARGET_STARTUP_SEQUENCE'] : []),
  ...(variant >= 5 ? ['-DTARGET_STARTUP_SEQUENCE_V2'] : []),
  ...(variant >= 7 ? ['-DTARGET_STARTUP_FEEDBACK'] : []),
  ...(variant === 8 ? ['-DTARGET_STARTUP_S3_POWER'] : []),
  ...(variant >= 11 ? ['-DTARGET_STARTUP_S4'] : []),
  ...(variant >= 12 ? ['-DTARGET_STARTUP_HANDOFF_REAL'] : []),
  ...(variant === 13 ? ['-DTARGET_STARTUP_RESCUE_RETAIN'] : []),
  ...(variant >= 2 ? ['-DTARGET_STARTUP_POWER_BENCH',
    `-DFD6288_STARTUP_POWER_EXTRA=${variant === 2 ? 0 : 40}`,
    `-DFD6288_TRACE_VARIANT=${variant}`] : []),
  '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
  '-Wno-unused-function', '-fsanitize=address,undefined',
  '-x', 'c', '-', '-o', binary,
], { input: source, encoding: 'utf8' });
process.stdout.write(compile.stdout || '');
process.stderr.write(compile.stderr || '');
if (compile.status !== 0) process.exit(compile.status || 1);
const result = spawnSync(binary, [], { encoding: 'utf8' });
process.stdout.write(result.stdout || '');
process.stderr.write(result.stderr || '');
console.log(`Host test executable: ${binary}`);
if (result.status !== 0) process.exit(result.status || 1);
}
