const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict');
const {spawnSync}=require('node:child_process'),{decode}=require('../tools/scripts/fd6288_pin_sync_capture.cjs');
const root=path.resolve(__dirname,'..'),exe=path.join(fs.mkdtempSync(path.join(os.tmpdir(),'fd-sync-test-')),'test');
for(const [cmd,args]of [[process.env.CC||'cc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','tests/fd6288_pin_sync_tests.c','-o',exe]],[exe,[]]]){
 const r=spawnSync(cmd,args,{cwd:root,stdio:'inherit'});assert.equal(r.status,0);
}
const b=Buffer.alloc(12320);b.writeUInt32LE(0x46505331,0);b.writeUInt32LE(1,4);b.writeUInt32LE(1,8);b.writeUInt32LE(1,12);
b[54]=1;b[55]=123;for(let j=0;j<5;j++)b.writeUInt16LE(100+j,36+2*j);
assert.equal(decode(b).eligible,1);assert.equal(decode(b).stats.neutral.mean,101);
b[55]=1;assert.equal(decode(b).eligible,0);assert.equal(decode(b).stats.pa3,null);
b.writeUInt32LE(513,8);assert.throws(()=>decode(b));b.writeUInt32LE(1,8);
b.writeUInt32LE(0,12);assert.throws(()=>decode(b));
console.log('PASS sync decoder: ABI, bounds, validity filters, phase mean, frozen requirement');
