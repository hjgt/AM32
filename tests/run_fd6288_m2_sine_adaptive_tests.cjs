const assert = require('node:assert/strict');
const childProcess = require('node:child_process');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');

const root = path.resolve(__dirname, '..');
const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'fd-m2-adaptive-host-'));
const executable = path.join(directory, 'test');
const compile = childProcess.spawnSync('cc', [
  '-std=c11', '-O1', '-Wall', '-Wextra', '-Werror',
  '-fsanitize=address,undefined',
  '-I' + path.join(root, 'Inc'),
  path.join(__dirname, 'fd6288_m2_sine_adaptive_tests.c'),
  '-o', executable,
], {encoding: 'utf8'});
process.stdout.write(compile.stdout || '');
process.stderr.write(compile.stderr || '');
assert.equal(compile.status, 0, 'Compile failed');

const test = childProcess.spawnSync(executable, [], {encoding: 'utf8'});
process.stdout.write(test.stdout || '');
process.stderr.write(test.stderr || '');
assert.equal(test.status, 0, 'Adaptive replay failed');
