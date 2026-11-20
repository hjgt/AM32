const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'),read=p=>fs.readFileSync(path.join(root,p),'utf8');
const strip=s=>s.replace(/^#include[^\n]*$/gm,'').replace(/^#pragma once$/gm,'');
const harness=read('tests/fd6288_wave_tests.c').split('/* WAVE_IMPLEMENTATION */')[0];
const impl=['Inc/fd6288_startup_wave.h','Inc/fd6288_startup_trace.h','Src/fd6288_startup_dense.inc'].map(p=>strip(read(p))).join('\n');
const source='#define TARGET_FD_DENSE_LOG\n#define FD6288_TRACE_VARIANT 27u\n'+harness+impl+read('tests/fd6288_dense_tests.c');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-dense-')),exe=path.join(dir,'test'),fixture=exe+'.bin';
function run(cmd,args,input){const p=spawnSync(cmd,args,{input,encoding:'utf8'});process.stdout.write(p.stdout||'');process.stderr.write(p.stderr||'');assert.equal(p.status,0);}
run(process.env.CC||'cc',['-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-x','c','-','-o',exe],source);run(exe,[fixture]);
const {decode}=require('../tools/scripts/fd6288_wave_capture.cjs'),raw=fs.readFileSync(fixture),d=decode(raw),b=d.banks[0];
assert.equal(d.header.abi,3);assert.equal(d.header.trials_seen,7);assert.equal(b.samples.length,1280);
assert.equal(b.samples[720].step,2);assert.equal(b.samples[0].reason,'invalid');assert.equal(b.samples[1].reason,'blank');
assert.equal(b.samples[0].trigger,667);assert.equal(b.samples[0].adc[0],1500);
for(const [o,v] of [[4,9],[8,26],[24,0],[52,1290],[64+32,1281],[128,0xffffffff]]) {const x=Buffer.from(raw);x.writeUInt32LE(v,o);assert.throws(()=>decode(x));}
assert.throws(()=>decode(raw.subarray(0,raw.length-1)));
console.log('PASS dense actual C: full rate, capacity, wrap, event overflow, latest trial, packed metadata, decoder corrupt rejection; no drive writes');
if(process.argv[2])run(process.execPath,[path.join(__dirname,'fd6288_pair_capture_tests.cjs'),process.argv[2],fixture,'wave']);
