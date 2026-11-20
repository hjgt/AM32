// Check the actual target machine code, not just the C source spelling.
const assert = require('node:assert/strict');
const {spawnSync} = require('node:child_process');
const elf = process.argv[2];
assert(elf, 'Provide S2D2 ELF');
const p = spawnSync('arm-none-eabi-objdump', ['-d', '--disassemble=fdTraceEvent', elf], {encoding: 'utf8'});
assert.equal(p.status, 0, p.stderr);
assert(p.stdout.includes('<fdTraceEvent>:'));
assert(!/\t(?:bl|blx)(?:\.w)?\s/.test(p.stdout), 'Boundary logger must not call helpers');
assert(!/<(?:memcpy|memset|__aeabi_mem\w*)>/.test(p.stdout), 'Bulk memory helper remains');
assert(!/\tsub(?:s|\.w)?\s+sp\b/.test(p.stdout), 'Unexpected stack temporary in event logger');
console.log('PASS ARM codegen: fdTraceEvent has no function calls, bulk-memory helpers or stack temporary allocation. Not a cycle-time measurement.');
