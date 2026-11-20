// Execute actual peripherals.h window function and actual C06 cap functions.
const fs=require('node:fs'),path=require('node:path'),os=require('node:os'),assert=require('node:assert/strict');
const {spawnSync}=require('node:child_process');
const read=p=>fs.readFileSync(path.join(__dirname,'..',p),'utf8');
const h=read('Mcu/g431/Inc/peripherals.h'),m=read('Src/main.c');
const window=h.slice(h.indexOf('#define ZCD_DTG_TICKS'),h.indexOf('#define SET_DUTY_CYCLE_ALL'));
const limits=m.slice(m.indexOf('static uint16_t driveZcdMinimumDuty'),m.indexOf('/* Publish the inhibit before touching timers/GPIO'));
assert(read('Mcu/g431/Src/peripherals.c').includes('TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;'));
assert(read('Mcu/g431/Src/peripherals.c').includes('TIM_InitStruct.Prescaler = 0;'));
const source=`#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define DEAD_TIME 70u
#define TARGET_FIXED_DEAD_TIME_CODE 0xCEu
${window}
static uint32_t tim1_arr=6665;
static uint16_t duty_cycle_maximum=2000;
static uint8_t use_current_limit;
static int16_t use_current_limit_adjust=2000;
${limits}
int main(void) {
  for(unsigned dtg=0;dtg<256;dtg++) {
    unsigned expected;
    switch(dtg>>5){case 0:case 1:case 2:case 3:expected=dtg;break;
    case 4:case 5:expected=(64+(dtg&63))*2;break;
    case 6:expected=(32+(dtg&31))*8;break;default:expected=(32+(dtg&31))*16;}
    assert(ZCD_DTG_TICKS(dtg)==expected);
  }
  assert(ZCD_DTG_TICKS(0xCE)==368);
#ifdef TARGET_ADC_ZCD_ACTUAL_DEADTIME
  assert(ZCD_SAMPLE_FLOOR==460 && ZCD_MINIMUM_ON_TICKS==704);
  assert(zcd_ccr4_from_duty(700)==0 && zcd_ccr4_from_duty(704)==460);
  assert(zcd_ccr4_from_duty(800)==460 && driveZcdMinimumDuty()==211);
#else
  assert(ZCD_SAMPLE_FLOOR==162 && ZCD_MINIMUM_ON_TICKS==406);
  assert(zcd_ccr4_from_duty(700)==350 && driveZcdMinimumDuty()==122);
#endif
  assert(zcd_ccr4_from_duty(1200)==600); // S3 initial 360 request unchanged
  assert(zcd_ccr4_from_duty(1334)==667); // S3 400 request unchanged
  for(unsigned duty=0;duty<=65535;duty++) {
    unsigned c=zcd_ccr4_from_duty(duty);
    if(duty<ZCD_MINIMUM_ON_TICKS) assert(c==0);
    else assert(c>=ZCD_SAMPLE_FLOOR && c+ZCD_ADC_WINDOW_TICKS<=duty);
  }
  for(unsigned arr=1000;arr<=10000;arr++) {
    tim1_arr=arr; unsigned minimum=driveZcdMinimumDuty();
    assert(minimum*arr/2000+1>=ZCD_MINIMUM_ON_TICKS);
    assert((minimum-1)*arr/2000+1<ZCD_MINIMUM_ON_TICKS);
    duty_cycle_maximum=minimum-1; use_current_limit=0;
    assert(!driveSafetyLimitAllowsZcd());
    duty_cycle_maximum=minimum; assert(driveSafetyLimitAllowsZcd());
    use_current_limit=1; use_current_limit_adjust=minimum-1;
    assert(!driveSafetyLimitAllowsZcd());
    use_current_limit_adjust=minimum; assert(driveSafetyLimitAllowsZcd());
    use_current_limit_adjust=-1; assert(!driveSafetyLimitAllowsZcd());
  }
  puts("PASS actual ADC window + C06 caps: all DTG codes, 65536 pulse widths, 9001 ARR/cap boundaries");
}`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd6288-window-'));
for(const fixed of [false,true]) {
 const exe=path.join(dir,fixed?'fixed':'baseline');
 for(const [cmd,args,input] of [
  [process.env.CC||'cc',[...(fixed?['-DTARGET_ADC_ZCD_ACTUAL_DEADTIME']:[]),'-std=c11','-O1','-g','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-x','c','-','-o',exe],source],
  [exe,[]],
 ]){const p=spawnSync(cmd,args,{input,encoding:'utf8'});process.stdout.write(p.stdout||'');process.stderr.write(p.stderr||'');assert.equal(p.status,0);}
}
