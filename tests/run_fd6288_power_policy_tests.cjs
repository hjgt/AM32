// Build only the isolated HOST prototype, never a firmware target or debugger.
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const {spawnSync} = require('node:child_process');
const root = path.resolve(__dirname, '..');
const output = fs.mkdtempSync(path.join(os.tmpdir(), 'fd6288-power-policy-'));
const binary = path.join(output, 'policy-test');
const compile = spawnSync(process.env.CC || 'cc', [
  '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
  '-fsanitize=address,undefined',
  path.join(root, 'experiments/fd6288-startup-power/policy_test.c'), '-o', binary,
], {encoding: 'utf8'});
process.stdout.write(compile.stdout || '');
process.stderr.write(compile.stderr || '');
if (compile.status !== 0) process.exit(compile.status || 1);
const result = spawnSync(binary, [], {encoding: 'utf8'});
process.stdout.write(result.stdout || '');
process.stderr.write(result.stderr || '');
console.log(`Host-only prototype test: ${binary}`);
if (result.status !== 0) process.exit(result.status || 1);
