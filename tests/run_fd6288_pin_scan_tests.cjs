const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict');
const {spawnSync}=require('node:child_process');
const {decode}=require('../tools/scripts/fd6288_pin_scan_capture.cjs');
const root=path.resolve(__dirname,'..'),exe=path.join(fs.mkdtempSync(path.join(os.tmpdir(),'fd-pin-policy-')),'test');
for(const [cmd,args]of [[process.env.CC||'cc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','tests/fd6288_pin_scan_tests.c','-o',exe]],[exe,[]]]){
 const r=spawnSync(cmd,args,{cwd:root,stdio:'inherit'});assert.equal(r.status,0);
}
const b=Buffer.alloc(12328);b.writeUInt32LE(0x46504931,0);b.writeUInt32LE(1,4);b.writeUInt32LE(1,8);b.writeUInt32LE(1,12);
for(let j=0;j<6;j++)b.writeUInt16LE(j+10,44+2*j);
assert.equal(decode(b).summary.PA3.mean,10);assert.equal(decode(b).summary.PA6_current.mean,15);
b.writeUInt32LE(0,12);assert.throws(()=>decode(b));b.writeUInt32LE(1,12);
b.writeUInt32LE(513,8);assert.throws(()=>decode(b));
console.log('PASS pin scan decoder bounds, frozen requirement, channel ordering');
