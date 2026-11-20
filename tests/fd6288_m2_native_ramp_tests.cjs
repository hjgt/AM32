// Check the real EEPROM decode and native output ramp at the M2 20 kHz tick.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),cp=require('node:child_process'),assert=require('node:assert/strict');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
const ca=main.indexOf('        if(eepromBuffer.max_ramp < 10){'),cb=main.indexOf('        if (motor_kv < 300)',ca);
assert(ca>0&&cb>ca);const config=main.slice(ca,cb);
const ra=main.indexOf('        if (ramp_count > ramp_divider) {'),rb=main.indexOf('\n#ifdef DRIVE_SAFETY_LIMIT_PRIORITY',ra);
assert(ra>0&&rb>ra);const ramp=main.slice(ra,rb);
const a=main.indexOf('/* FD_M2_HELI_OUTPUT_BEGIN */'),b=main.indexOf('/* FD_M2_HELI_OUTPUT_END */',a);assert(a>0&&b>a);
const hook=main.slice(a,b);
const code=`
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
static struct {uint8_t max_ramp;} eepromBuffer;
static uint16_t duty_cycle,last_duty_cycle;
static uint32_t zero_crosses,average_interval;
static uint16_t ramp_count;
static uint8_t ramp_divider;
static uint8_t max_ramp_startup,max_ramp_low_rpm,max_ramp_high_rpm,max_duty_cycle_change;
static void configure(unsigned value){
 eepromBuffer.max_ramp=value;max_ramp_startup=2;max_ramp_low_rpm=6;max_ramp_high_rpm=16;
 ${config}
}
static void tick(unsigned requested){
 ramp_count++;duty_cycle=requested;
 ${hook}
 ${ramp}
 last_duty_cycle=duty_cycle;
}
static void check(unsigned setting,unsigned ticks,unsigned stage){
 configure(setting);ramp_count=0;duty_cycle=last_duty_cycle=400;
 zero_crosses=stage?200:9;average_interval=stage==2?300:1000;
 assert(ramp_divider==9&&max_ramp_startup==setting&&max_ramp_low_rpm==setting&&max_ramp_high_rpm==setting);
 for(unsigned i=1;i<=ticks;i++){
  unsigned previous=last_duty_cycle;tick(2000);
  assert(last_duty_cycle>=previous&&last_duty_cycle-previous<=setting);
  if(i<ticks)assert(last_duty_cycle<2000);
 }
 assert(last_duty_cycle==2000);
 /* The original ramp also slews a nonzero decrease. */
 tick(400);assert(last_duty_cycle>400);
 for(unsigned i=1;i<=ticks;i++)tick(400);
 assert(last_duty_cycle==400);
}
int main(void){
 for(unsigned stage=0;stage<3;stage++){check(1,16000,stage);check(2,8000,stage);}
 configure(160);assert(!ramp_divider&&max_ramp_startup==2&&max_ramp_low_rpm==6&&max_ramp_high_rpm==16);
 puts("PASS actual native Ramp:0.1pct/ms=800ms and0.2pct/ms=400ms for20->100; startup/low/high stages, single active ramp and nonzero downward slew");
}
`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'m2-native-ramp-')),src=path.join(dir,'test.c'),exe=path.join(dir,'test');fs.writeFileSync(src,code);
for(const [cmd,args] of [['cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',src,'-o',exe]],[exe,[]]]){
 const r=cp.spawnSync(cmd,args,{encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}
