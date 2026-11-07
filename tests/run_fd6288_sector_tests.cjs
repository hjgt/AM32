const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{execFileSync}=require('node:child_process');
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-sector-')),exe=path.join(dir,'test'),fixture=path.join(dir,'fixture.bin');
execFileSync(process.env.CC||'cc',['-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',path.join(__dirname,'fd6288_sector_diag_tests.c'),'-o',exe],{stdio:'inherit'});
execFileSync(exe,[fixture],{stdio:'inherit'});
const {decode}=require('../tools/scripts/fd6288_sector_capture.cjs'),raw=fs.readFileSync(fixture),d=decode(raw);
assert.equal(d.header.trial,6);assert.equal(d.records.length,1);
assert.equal(d.records[0].minimum,-15);assert.equal(d.records[0].maximum,20);
assert.equal(d.records[0].first_accept_us,100/160);assert.equal(d.records[0].exit_kind,5);
assert.equal(d.records[0].reasons.accepted,1);
const extended=Buffer.from(raw);extended.writeUInt32LE(820000*160,32+4);
assert.equal(decode(extended).records[0].end_us,820000);
extended.writeUInt32LE(1300000*160,32+4);
assert.equal(decode(extended).records[0].end_us,1300000);
extended.writeUInt32LE(2090000*160,32+4);assert.equal(decode(extended).records[0].end_us,2090000);
for(const [at,value] of [[0,0],[4,2],[16,0],[24,0],[32+4,480000001]]){
 const bad=Buffer.from(raw);bad.writeUInt32LE(value,at);assert.throws(()=>decode(bad));
}
assert.throws(()=>decode(raw.subarray(0,raw.length-1)));
console.log('PASS C->JS sector ABI and corrupt/unfinished capture rejection');
