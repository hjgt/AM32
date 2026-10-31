const fs=require('fs'),os=require('os'),path=require('path'),assert=require('assert/strict'),{spawnSync}=require('child_process');
const exe=path.join(fs.mkdtempSync(path.join(os.tmpdir(),'fd-raw-')),'test');
let r=spawnSync('cc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','tests/fd6288_edge_raw_tests.c','-o',exe]);assert.equal(r.status,0,r.stderr.toString());
r=spawnSync(exe);assert.equal(r.status,0,r.stderr.toString());
const {decode}=require('../tools/scripts/fd6288_edge_decode.cjs'),d=decode(r.stdout);
assert.equal(d.records.length,512);assert.equal(d.records[0].reason,1);
const pre=d.records.filter(x=>x.window==='before_accept_or_tail');assert.equal(pre.length,64);assert.equal(pre[0].a,339);assert.equal(pre.at(-1).reason,7);assert.equal(pre.at(-1).sector,2);
assert.equal(d.records.at(-1).at_us,596/160);
for(const offset of [0,4,16,28,64]){let bad=Buffer.from(r.stdout);bad.writeUInt32LE(0xffffffff,offset);assert.throws(()=>decode(bad));}
console.log('PASS raw observer: chronological ring, late first edge, blank retention, bounded post window, freeze, reset, wrap, decoder ABI');

// A new short/empty trial must not expose stale slots from a long prior trial.
let short=Buffer.from(r.stdout);short.writeUInt32LE(1,28);short.writeUInt32LE(1,32);short.writeUInt32LE(0,56);short.writeUInt32LE(0,64);assert.equal(decode(short).records.length,2);
short.writeUInt32LE(0,28);short.writeUInt32LE(0,32);assert.equal(decode(short).records.length,0);
