// Offline image preparation only; never connects to the target or writes flash.
const fs=require('fs'),path=require('path'),assert=require('assert/strict'),crypto=require('crypto');
const {parseHex:parseMainHex}=require('./fd6288_startup_capture.cjs');
const sha=b=>crypto.createHash('sha256').update(b).digest('hex');
function record(type,offset,data){const a=[data.length,offset>>8,offset&255,type,...data];a.push((-a.reduce((s,x)=>s+x,0))&255);return ':'+Buffer.from(a).toString('hex').toUpperCase();}
function encode(map){const addresses=[...map.keys()].sort((a,b)=>a-b),lines=[];let upper=-1;for(let i=0;i<addresses.length;){const address=addresses[i],high=address>>>16;if(high!==upper){upper=high;lines.push(record(4,0,[high>>8,high&255]));}const data=[];let j=i;while(j<addresses.length&&addresses[j]===address+data.length&&addresses[j]>>>16===upper&&data.length<16){data.push(map.get(addresses[j]));j++;}lines.push(record(0,address&65535,data));i=j;}lines.push(record(1,0,[]));return lines.join('\n')+'\n';}
function parseHex(text){const map=new Map();let base=0,eof=false;for(const line of text.trim().split(/\r?\n/)){assert(!eof);assert(/^:[0-9a-f]+$/i.test(line));const b=Buffer.from(line.slice(1),'hex');assert.equal(b.length,b[0]+5);assert.equal(b.reduce((a,x)=>a+x,0)&255,0);const offset=b.readUInt16BE(1),type=b[3];if(type===4){assert.equal(b[0],2);base=b.readUInt16BE(4)*65536;}else if(type===0){for(let i=0;i<b[0];i++){const a=base+offset+i;assert(a>=0x08001000&&a<0x08010000);assert(!map.has(a));map.set(a,b[4+i]);}}else if(type===1){assert.equal(b[0],0);eof=true;}else assert.fail('Unexpected HEX record');}assert(eof);return map;}
function settingsMap(page){return new Map([...page].map((b,i)=>[0x0800f800+i,b]));}
function decoded(b){return {sine_start:b[19],startup_power:b[25],motor_kv:b[26]*40+20,motor_poles:b[27],advance_level_raw:b[23],auto_advance:b[47],manual_advance_deg:b[23]<4?b[23]*7.5:(b[23]-10)*0.9375,sine_range:b[40],sine_power:b[45],pwm_khz:b[24]};}
function prepare(beforeFile,mainFile,out){
 const before=fs.readFileSync(beforeFile);assert.equal(before.length,2048);assert.equal(before[0],1);assert(before[23]<43&&before[47]<=1,'Unexpected timing encoding');
 const header=fs.readFileSync(path.join(__dirname,'../../Inc/fd6288_native_sine_config.h'),'utf8');
 function value(name){const m=header.match(new RegExp('#define '+name+' (\\d+)u'));assert(m,name);return Number(m[1]);}
 const values={6:1,18:0,19:1,20:1,21:0,24:24,25:value('FD_NATIVE_SINE_STARTUP_POWER'),26:value('FD_NATIVE_SINE_MOTOR_KV_CODE'),27:value('FD_NATIVE_SINE_MOTOR_POLES'),38:0,40:value('FD_NATIVE_SINE_RANGE'),45:value('FD_NATIVE_SINE_POWER')};
 const after=Buffer.from(before);for(const [offset,v] of Object.entries(values))after[Number(offset)]=v;
 assert.equal(after[23],before[23]);assert.equal(after[47],before[47]);
 const changes=[];for(let i=0;i<2048;i++){if(before[i]!==after[i]){assert(Object.hasOwn(values,i));changes.push({offset:i,before:before[i],after:after[i]});}}
 const main=parseMainHex(fs.readFileSync(mainFile,'utf8'));assert([...main.keys()].every(a=>a>=0x08001000&&a<0x0800f800));
 const settings=settingsMap(after),restore=settingsMap(before),combined=new Map([...main,...settings]);
 assert.equal(combined.size,main.size+2048);assert([...combined.keys()].every(a=>a>=0x08001000&&a<=0x0800ffff));
 fs.mkdirSync(out,{recursive:true});
 const files={'NATIVE-SINE-SETTINGS.bin':after,'NATIVE-SINE-SETTINGS.hex':encode(settings),'NATIVE-SINE-COMBINED.hex':encode(combined),'RESTORE-PREVIOUS-SETTINGS.hex':encode(restore)};
 for(const [name,data] of Object.entries(files))fs.writeFileSync(path.join(out,name),data,{flag:'wx'});
 assert.deepEqual(parseHex(fs.readFileSync(path.join(out,'NATIVE-SINE-SETTINGS.hex'),'utf8')),settings);
 assert.deepEqual(parseHex(fs.readFileSync(path.join(out,'NATIVE-SINE-COMBINED.hex'),'utf8')),combined);
 assert.deepEqual(parseHex(fs.readFileSync(path.join(out,'RESTORE-PREVIOUS-SETTINGS.hex'),'utf8')),restore);
 const report={timing_policy:'inherit existing stored advance and auto/manual selection',before_backup:path.relative(path.resolve(__dirname,'../..'),path.resolve(beforeFile)),before_backup_root:'repository',before_sha256:sha(before),before:decoded(before),after:decoded(after),changes,combined_regions:['0x08001000..0x0800f7ff main','0x0800f800..0x0800ffff settings'],bootloader_included:false,checks:{untouched_bytes_preserved:true,timing_bytes_unchanged:true,settings_roundtrip:true,combined_roundtrip:true,restore_roundtrip:true},files:Object.fromEntries(Object.keys(files).map(n=>[n,sha(fs.readFileSync(path.join(out,n)))]))};
 fs.writeFileSync(path.join(out,'settings-profile.json'),JSON.stringify(report,null,2)+'\n',{flag:'wx'});return report;
}
if(require.main===module){const [before,main,out]=process.argv.slice(2);assert(before&&main&&out,'Usage: node prepare_fd6288_native_sine_settings.cjs BACKUP-2048.bin MAIN.hex NEW-DIR');console.log(JSON.stringify(prepare(before,main,out),null,2));}
module.exports={prepare,encode,decoded};
