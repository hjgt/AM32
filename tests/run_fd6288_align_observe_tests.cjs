const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{execFileSync}=require('node:child_process');
const d=fs.mkdtempSync(path.join(os.tmpdir(),'fd-align-')),exe=path.join(d,'test'),fixture=path.join(d,'fixture.bin');
execFileSync('cc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',path.join(__dirname,'fd6288_align_observe_tests.c'),'-o',exe],{stdio:'inherit'});
execFileSync(exe,[fixture],{stdio:'inherit'});
const {decode}=require('../tools/scripts/fd6288_align_decode.cjs');const b=fs.readFileSync(fixture),r=decode(b);
assert.equal(r.trial,7);assert.equal(r.bins[0].phases[0].mean,95);assert.equal(r.bins[17].start_ms,85);
const bad=Buffer.from(b);bad.writeUInt32LE(0,12);assert.throws(()=>decode(bad));
console.log('PASS alignment C/JS ABI');
