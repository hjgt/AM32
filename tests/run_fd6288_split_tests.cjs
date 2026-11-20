const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),assert=require('node:assert/strict');
const {spawnSync}=require('node:child_process');
const root=path.resolve(__dirname,'..'),read=p=>fs.readFileSync(path.join(root,p),'utf8');
const strip=s=>s.replace(/^#include[^\n]*$/gm,'').replace(/^#pragma once$/gm,'');
const impl=['Inc/fd6288_startup_wave.h','Inc/fd6288_startup_trace.h','Src/fd6288_startup_wave.c'].map(p=>strip(read(p))).join('\n');
const harness=read('tests/fd6288_wave_tests.c').split('int main(int argc')[0];
const source='#define TARGET_STARTUP_SPLIT_LOG\n'+harness.replace('/* WAVE_IMPLEMENTATION */',impl)+read('tests/fd6288_split_tests.c');
const variant=Number(process.argv[3]||23);assert([23,24,25,26].includes(variant));
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd6288-split-')),exe=path.join(dir,'test'),fixture=exe+'.bin';
function run(cmd,args,input) {
 const p=spawnSync(cmd,args,{input,encoding:'utf8'});
 process.stdout.write(p.stdout||'');process.stderr.write(p.stderr||'');
 assert.equal(p.status,0);
}
run(process.env.CC||'cc',['-DFD6288_TRACE_VARIANT='+variant,'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-x','c','-','-o',exe],source);
run(exe,[fixture]);
const {decode}=require('../tools/scripts/fd6288_wave_capture.cjs'),raw=fs.readFileSync(fixture),d=decode(raw);
assert.equal(d.header.abi,2);assert.equal(d.header.variant,variant);
assert.deepEqual(d.banks.map(b=>[b.header.trial_id,b.classification]),[[1,'no_lock'],[4,'post_handoff_loss'],[6,'success']]);
const b=d.banks[0];
assert.equal(b.startup_prefix.records.length,125);assert.equal(b.startup_prefix.overwritten,0);
assert.equal(b.startup_prefix.records[0].at_us,50);
assert.equal(b.startup_prefix.records.at(-1).at_us,49650);
assert.equal(b.first_edge.at_us,170050);
assert.equal(b.first_edge_window.records.length,128);
assert(b.first_edge_window.records[0].at_us<b.first_edge.at_us);
assert.equal(b.first_edge_window.records.at(-1).at_us,176050);
assert.equal(b.events.records.length,96);assert.equal(b.events.omitted_suffix,26);
assert.equal(d.banks[1].startup_prefix.records.length,125); // survives early handoff
for(const [offset,value] of [[4,1],[60,192000],[56,0],[64+32,129],[64+32,4000<<16],[64+128,8000001],[64+124,0xff020301]]) {
 const bad=Buffer.from(raw);bad.writeUInt32LE(value,offset);assert.throws(()=>decode(bad),`corruption at${offset}`);
}
assert.throws(()=>decode(raw.subarray(0,raw.length-1)));
console.log('PASS split C->JS: separate windows, exact trigger, suffix omissions, ABI/corrupt-count rejection');
if(process.argv[2]) run(process.execPath,[path.join(__dirname,'fd6288_pair_capture_tests.cjs'),process.argv[2],fixture,'wave']);
