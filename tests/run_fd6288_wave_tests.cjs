const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),assert=require('node:assert/strict');
const {spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'), read=p=>fs.readFileSync(path.join(root,p),'utf8');
const strip=s=>s.replace(/^#include[^\n]*$/gm,'').replace(/^#pragma once$/gm,'');
const impl=['Inc/fd6288_startup_wave.h','Inc/fd6288_startup_trace.h','Src/fd6288_startup_wave.c'].map(p=>strip(read(p))).join('\n');
const source=read('tests/fd6288_wave_tests.c').replace('/* WAVE_IMPLEMENTATION */',impl);
const variant=Number(process.argv[3]||9);assert([9,10].includes(variant));
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd6288-wave-')), exe=path.join(dir,'test'), fixture=exe+'.bin';
for(const [cmd,args,input] of [
  [process.env.CC||'cc',[`-DFD6288_TRACE_VARIANT=${variant}`,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-x','c','-','-o',exe],source],
  [exe,[fixture]],
]){const p=spawnSync(cmd,args,{input,encoding:'utf8'});process.stdout.write(p.stdout||'');process.stderr.write(p.stderr||'');assert.equal(p.status,0);}
const {decode,SIZE}=require('../tools/scripts/fd6288_wave_capture.cjs'), raw=fs.readFileSync(fixture),d=decode(raw);
assert.equal(raw.length,SIZE);
assert.deepEqual(d.banks.map(b=>[b.header.trial_id,b.classification]),[[1,'no_lock'],[4,'post_handoff_loss'],[6,'success']]);
assert.equal(d.banks[0].samples.overwritten,344);assert.equal(d.banks[0].events.overwritten,25);
assert.equal(d.banks[1].samples.records.at(-1).stage,4);
assert.equal(d.banks[2].events.records.at(-1).adc[2],780);
for(const [offset,value] of [[0,0],[4,2],[8,8],[24,0],[28,3],[32,0],[64,1],[64+20,7],[64+28,0],[64+32,1000001]]) {
  const b=Buffer.from(raw);b.writeUInt32LE(value,offset);assert.throws(()=>decode(b),`invalid field ${offset}`);
}
assert.throws(()=>decode(raw.subarray(0,SIZE-1)));
console.log('PASS S3-D C -> JS layout, tail ordering, coverage, class metadata and corrupt input rejection');
if(process.argv[2]) {
 const p=spawnSync(process.execPath,[path.join(__dirname,'fd6288_pair_capture_tests.cjs'),process.argv[2],fixture,'wave'],{encoding:'utf8'});
 process.stdout.write(p.stdout||'');process.stderr.write(p.stderr||'');assert.equal(p.status,0);
}
