// Replay the recorded S1 policy inputs only. This is NOT a motor model and
// cannot predict S2 samples: changing the commanded steps changes the plant.
const fs = require('node:fs');
const path = require('node:path');
const os = require('node:os');
const assert = require('node:assert/strict');
const {spawnSync} = require('node:child_process');
const {decode} = require('../tools/scripts/fd6288_startup_capture.cjs');
const root = path.resolve(__dirname, '..');
const file = process.argv[2] || path.join(root, 'hardware-backups/startup-S1-20260921/trial-first-read.bin');
const d = decode(fs.readFileSync(file));
assert.equal(d.header.variant, 4);
assert.equal(d.header.capture_id, 1);
assert.deepEqual(d.events.gaps, []);
const statements = [];
for (const e of d.events.records) {
  const t = (e.cycles - d.header.start_cycles) >>> 0;
  if (e.kind === 'rev_step') statements.push(`assert(fdRevAdvancePolicy(&s, ${t}u) == FD_REV_STEP); assert(s.period / 160u == ${e.commanded_sector_us}u); steps++;`);
  if (e.kind === 'rev_edge') statements.push(`assert(!fdRevEdgePolicy(&s, ${t}u, 16u)); assert(s.confidence == ${e.confidence}u); edges++; if (s.confidence > maximum) maximum = s.confidence;`);
}
const source = `#include <assert.h>
#include <stdio.h>
#include "fd6288_startup_sequence.h"
int main(void) {
FdRevPolicy s; unsigned steps=0, edges=0, maximum=0;
fdRevBeginPolicy(&s, 0u);
${statements.join('\n')}
assert(steps==53u && edges==13u && maximum==2u && s.stage==FD_REV_OPEN);
puts("PASS: recorded S1 replay: 53 planned steps, 13 ADC edges, maximum qualification 2, no handoff.");
puts("This reproduces the S1 decision, NOT S2 motor behavior or mechanical rotation.");
}`;
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-s1-replay-'));
const binary = path.join(directory, 'replay');
const build = spawnSync(process.env.CC || 'cc', ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
  '-fsanitize=address,undefined', '-I', path.join(root, 'Inc'), '-x', 'c', '-', '-o', binary], {input: source, encoding: 'utf8'});
process.stderr.write(build.stderr || '');
assert.equal(build.status, 0, 'Replay compile failed');
const result = spawnSync(binary, [], {encoding: 'utf8'});
process.stdout.write(result.stdout || ''); process.stderr.write(result.stderr || '');
assert.equal(result.status, 0, 'Recorded S1 decisions no longer reproduce');
