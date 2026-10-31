const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const {decode}=require('../tools/scripts/fd6288_edge_decode.cjs');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-edge-')),exe=path.join(dir,'test');
const cc=spawnSync(process.env.CC||'cc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',path.join(__dirname,'fd6288_edge_observe_tests.c'),'-o',exe],{encoding:'utf8'});
assert.equal(cc.status,0,cc.stderr);const r=spawnSync(exe);assert.equal(r.status,0,r.stderr.toString());
const d=decode(r.stdout);assert.equal(d.records.length,22);assert.equal(d.records[0].at_us,89000);
assert.equal(d.records[6].event,'first_pre');assert.equal(d.records[6].directed_margin,-19);
assert.equal(d.records[8].event,'first_post');assert.equal(d.records[8].reason,5);
for(const [offset,value] of [[0,0],[4,9],[16,0],[36,23]]){const bad=Buffer.from(r.stdout);bad.writeUInt32LE(value,offset);assert.throws(()=>decode(bad));}
console.log('PASS sparse observer: checkpoint/event slots, blank exclusion, both sectors, freeze, zero/reset, wrap, C/JS ABI and invalid capture rejection');
