// Stopped-board register inventory only. No GPIO/ADC configuration writes,
// no halt/reset/flash; this is NOT a running waveform/analog voltage capture.
const fs=require('node:fs'),assert=require('node:assert/strict');
const {connect}=require('./fd6288_startup_capture.cjs');
const candidates={A:[3,4,7,15],B:[0,3,4,7],F:[1]};
function decode(port,r){
 return candidates[port].map(pin=>{
  const mode=(r.MODER>>>(2*pin))&3;
  return {pin:`P${port}${pin}`,mode:['input','output','alternate','analog'][mode],
   pull:['none','up','down','reserved'][(r.PUPDR>>>(2*pin))&3],
   alternate:mode===2?((r[pin<8?'AFRL':'AFRH']>>>((pin%8)*4))&15):null,
   digital_level:mode===3?null:((r.IDR>>>pin)&1),
   status:mode===1||mode===2?'occupied_or_needs_review':'candidate_not_proven_unused'};
 });
}
async function capture(out,stopped,connection=connect){
 assert(stopped,'Fresh operator zero-throttle/stopped confirmation required');
 assert(out&&!fs.existsSync(out),'Use a new output JSON path');
 const c=await connection();
 try{
  async function words(a,n){
   const v=(await c.rpc(`read_memory 0x${a.toString(16)} 32 ${n}`)).trim().split(/\s+/).map(Number);
   assert.equal(v.length,n);assert(v.every(x=>Number.isInteger(x)&&x>=0&&x<=0xffffffff));return v;
  }
  async function gate(){
   assert.equal((await words(0xe000ed08,1))[0],0x08001000,'Main VTOR mismatch');
   assert.equal((await words(0x48000010,1))[0]&0x1f00,0,'Bridge A high');
   assert.equal((await words(0x48001410,1))[0]&1,0,'Bridge F high');
   assert((await words(0x40012c34,4)).every(x=>x===0),'PWM not zero');
  }
  const clocks=(await words(0x4002104c,1))[0];
  assert.equal(clocks&0x23,0x23,'GPIO A/B/F clocks must already be enabled; never enable them here');
  await gate();
  const ports={},pins=[];
  for(const [port,address]of [['A',0x48000000],['B',0x48000400],['F',0x48001400]]){
   const v=await words(address,11),r={MODER:v[0],OTYPER:v[1],OSPEEDR:v[2],PUPDR:v[3],IDR:v[4],ODR:v[5],AFRL:v[8],AFRH:v[9]};
   ports[port]=r;pins.push(...decode(port,r));
  }
  await gate();
  const result={capturedAt:new Date().toISOString(),clocks,ports,pins,
   excluded:['PA13/SWDIO','PA14/SWCLK','PA0/1/2 BEMF','PA5 voltage','PA6 current','PA8..12/PF0 bridge','PB5 DShot','PB6 telemetry','NRST/BOOT'],
   caveats:['Stopped static configuration only; no analog voltage or edge timing measured.',
    'Analog-mode IDR is not a usable signal measurement. Input mode does not prove unused hardware.',
    'PA15/PB3/PB4 may retain JTAG/SWO alternate functions: never reconfigure automatically.',
    'PF1 clock/board connection needs review before any analog scan.']};
  fs.writeFileSync(out,JSON.stringify(result,null,2)+'\n',{flag:'wx'});
  return result;
 }finally{c.close()}
}
module.exports={decode,capture};
if(require.main===module)capture(process.argv[2],process.argv.includes('--stopped'))
 .then(r=>console.log(JSON.stringify(r,null,2))).catch(e=>{console.error(e.message);process.exitCode=1});
