// Execute the actual native sine loop, output formula and trial adapter with
// simulated registers/time. No motor model, SWD or synthetic automatic ZCs.
const fs=require('fs'),path=require('path'),os=require('os'),cp=require('child_process'),assert=require('assert');
const m2Step4Extended=process.argv.includes('--m2-step4-extended');
const m2Step4LateAdc=m2Step4Extended||process.argv.includes('--m2-step4-late-adc');
const m2Step4Coast=m2Step4LateAdc||process.argv.includes('--m2-step4-coast');
const m2Step5Advance=process.argv.includes('--m2-runup-step5-advance');
const m2Rpm600=process.argv.includes('--m2-openloop-600');
const m2Rpm700=process.argv.includes('--m2-openloop-700');
const m2Pure=process.argv.includes('--m2-pure');
const m2NativeRamp=process.argv.includes('--m2-native-ramp');
const m2StablePre=m2Pure||m2NativeRamp||process.argv.includes('--m2-stable-pre');
const m2Heli=!m2NativeRamp&&(m2StablePre||process.argv.includes('--m2-heli-start'));
const m2RunFix=m2StablePre||m2Heli||process.argv.includes('--m2-run-fix');
const m2AdcCapture=!m2Pure&&(m2RunFix||process.argv.includes('--m2-adc-capture'));
const m2RealAdc=m2Pure||m2AdcCapture||process.argv.includes('--m2-handoff-real-adc');
const m2Continuous=m2Pure||m2AdcCapture||process.argv.includes('--m2-continuous');
const m2HandoffHold=m2Continuous||m2RealAdc||process.argv.includes('--m2-handoff-hold200');
const m2OpenloopOnly=m2Rpm600||m2Rpm700||process.argv.includes('--m2-openloop-hold200')||process.argv.includes('--m2-openloop-only');
const m2Hold200=m2HandoffHold||m2Rpm600||m2Rpm700||process.argv.includes('--m2-openloop-hold200');
const m2Adaptive=m2HandoffHold||m2OpenloopOnly||process.argv.includes('--m2-adaptive');
const m2RunupLateAdc=m2Adaptive||m2Step5Advance||process.argv.includes('--m2-runup-step4-late-adc');
const m2Runup=m2RunupLateAdc||process.argv.includes('--m2-runup');
const m2Sixstep=m2Runup||m2Step4Coast||process.argv.includes('--m2-sixstep');
if(m2Sixstep)process.argv.push('--m2-coast');
const clampB=process.argv.includes('--clamp-b');if(clampB)process.argv.push('--fixed-12');
const ab=process.argv.includes('--sixstep-ab');if(ab)process.argv.push('--sixstep-control');
const fixed12=process.argv.includes('--fixed-12'),sixstep=process.argv.includes('--sixstep-control');if(fixed12||sixstep)process.argv.push('--fixed');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
function block(s,start){const p=s.indexOf(start);assert(p>=0,start);const b=s.indexOf('{',p);let d=0;for(let i=b;i<s.length;i++){if(s[i]==='{')d++;if(s[i]==='}'&&!--d)return s.slice(p,i+1);}throw Error(start);}
const initAt=main.indexOf('    fdNativeSineClockInit();',main.indexOf('int main(void)'));
assert(initAt>main.indexOf('int main(void)')&&initAt<main.indexOf('    enableCorePeripherals();',main.indexOf('int main(void)')),'Control clock must initialize before interrupts, independently of trace');
const settingStart=main.indexOf('    /* Trial-only RAM overrides;');
assert(settingStart>=0);
const settingsEnd=main.indexOf('\n#endif\n    // eepromBuffer.advance_level',settingStart);
assert(settingsEnd>settingStart);
const settings=main.slice(settingStart,settingsEnd);
const table=main.slice(main.indexOf('int16_t pwmSin[] = {'),main.indexOf('};',main.indexOf('int16_t pwmSin[] = {'))+2);
const sine=main.slice(main.indexOf('#ifdef TARGET_FD_NATIVE_SINE_ONLY',main.indexOf('} else { // stepper sine')),main.indexOf('\n#endif // gimbal mode',main.indexOf('} else { // stepper sine')));
const reasons=fs.readFileSync(path.join(root,'Inc/fd6288_startup_power.h'),'utf8').match(/enum \{[\s\S]*?\};/)[0];
const src=`
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define TARGET_FD_NATIVE_SINE
#define TARGET_FD_NO_DIAGNOSTICS
#define TARGET_STARTUP_POWER_BENCH
#define FD_POWER_BENCH_MAGIC 0x46504231u
#define ADC_ZCD_RESET_COUNT 0
#define ADC_ZCD_RESET_START 1
#define SINE_DIVIDER 2
#define DEAD_TIME 70
#define ZCD_MINIMUM_ON_TICKS 704u
#define EFFECTIVE_STALL_PROTECTION 0
#define EFFECTIVE_BRAKE_ON_STOP 0
${reasons}
typedef struct {uint32_t magic,abi,extra,trials,completed,stops,inhibit,last_reason,active_id,floor;} FdPowerDiagnostics;
static struct {uint32_t CTRL,CYCCNT;} dwt;
static struct {uint32_t DEMCR;} core_debug;
#define CoreDebug (&core_debug)
#define CoreDebug_DEMCR_TRCENA_Msk (1u<<24)
#define DWT_CTRL_CYCCNTENA_Msk 1u
#define __DSB() ((void)0)
#define __ISB() ((void)0)
static unsigned clock_advances=1;
#define __NOP() ((void)(clock_advances && (dwt.CTRL&1u) && (core_debug.DEMCR&(1u<<24)) ? ++dwt.CYCCNT : 0u))
#define DWT (&dwt)
static uint32_t mask,ccr[4],cap=610,bridge,commutations,compares;
static unsigned armed=1,adjusted_input,input,running,stepper_sine,do_once_sinemode;
static unsigned stuck_rotor_inhibit,drive_safety_limit_inhibit,heli_coast_restart_inhibit,heli_coast_active;
static unsigned minimum_duty_cycle=210,min_startup_duty=360,startup_max_duty_cycle=610,use_speed_control_loop;
static unsigned duty_cycle_setpoint,duty_cycle,last_duty_cycle,prop_brake_active,old_routine;
static uint16_t one_khz_loop_counter;
static uint8_t PROCESS_ADC_FLAG;
#define PID_LOOP_DIVIDER 20u
static unsigned average_interval,last_average_interval,commutation_interval,commutation_intervals[6];
static unsigned step=1,changeover_step=5,step_delay=100,adjusted_duty_cycle,tim1_arr=6665,TIMER1_MAX_ARR=6665,gate_drive_offset=70;
static unsigned prop_brake_duty_cycle,e_rpm,forward=1;
static int16_t phase_A_position,phase_B_position,phase_C_position;
static unsigned zero_crosses;
static struct {unsigned minimum_duty_cycle,use_sine_start,startup_power,motor_kv,motor_poles,sine_mode_changeover_thottle_level,sine_mode_power,advance_level,auto_advance,comp_pwm,variable_pwm,pwm_frequency,bi_direction,rc_car_reverse,drag_brake_strength,active_brake_power;} eepromBuffer={1,1,150,22,14,15,6,18,0,1,0,24,0,0,0,0};
static uint32_t __get_PRIMASK(void){return mask;}
static void __disable_irq(void){mask=1;}
static void __set_PRIMASK(uint32_t x){mask=x;}
static void allOff(void){bridge=0;}
static void setall(uint32_t x){ccr[0]=ccr[1]=ccr[2]=x;ccr[3]=x>=704?x/2:0;}
#define SET_DUTY_CYCLE_ALL(x) setall(x)
#define SET_INTERVAL_TIMER_COUNT(x) ((void)(x))
#define DISABLE_COM_TIMER_INT() ((void)0)
static void maskPhaseInterrupts(void){}
void fdAsyncCancel(void){}
static void heliCoastTrackingLost(unsigned x){running=0;bridge=0;setall(0);duty_cycle_setpoint=duty_cycle=last_duty_cycle=0;heli_coast_restart_inhibit=x;heli_coast_active=0;}
static void adcZcdResetAcquisition(unsigned x){(void)x;}
static unsigned driveSafetyLimitAllowsZcd(void){return cap>=211;}
static unsigned driveEffectiveDutyLimit(void){return cap;}
static void generatePwmTimerEvent(void){}
void commutate(void){assert(!bridge);assert(ccr[0]==ccr[1]&&ccr[1]==ccr[2]);assert(ccr[3]);step=step==6?1:step+1;commutations++;bridge=2;}
`;
const prefix=src;
const tickSource=block(main,'void tenKhzRoutine()');
assert(tickSource.indexOf('fdNativeSineSensorTick();')>tickSource.indexOf('one_khz_loop_counter++;'));
assert(tickSource.indexOf('fdNativeSineSensorTick();')<tickSource.indexOf('if (!stepper_sine) {'));
let program=prefix+`\n#include "${path.join(root,'Src/fd6288_native_sine.inc')}"\n`+`
static void phaseAPWM(void){bridge=1;}
static void phaseBPWM(void){bridge=1;}
static void phaseCPWM(void){bridge=1;}
${block(fs.readFileSync(path.join(root,'Mcu/g431/Src/phaseouts.c'),'utf8'),'void allpwm()')}
static void fullBrake(void){}
static void proportionalBrake(void){}
static void comStep(unsigned x){(void)x;}
static void setPWMCompare1(unsigned x){ccr[0]=x;compares++;}
static void setPWMCompare2(unsigned x){ccr[1]=x;compares++;}
static void setPWMCompare3(unsigned x){ccr[2]=x;compares++;}
static void delayMicros(unsigned x){DWT->CYCCNT+=x*160u;}
${block(fs.readFileSync(path.join(root,'Src/functions.c'),'utf8'),'long map(long x, long in_min, long in_max, long out_min, long out_max)\n{')}
${table}
${block(main,'void advanceincrement()')}
static void checkSensorTick(void){
 unsigned old_sine=stepper_sine,old_flag=PROCESS_ADC_FLAG,old_counter=one_khz_loop_counter;
 unsigned old_pwm[4]={ccr[0],ccr[1],ccr[2],ccr[3]},old_bridge=bridge;
 stepper_sine=1;PROCESS_ADC_FLAG=0;one_khz_loop_counter=0;
 unsigned requests=0;
 for(unsigned tick=1;tick<=60000;tick++){
  one_khz_loop_counter++;fdNativeSineSensorTick();
  if(PROCESS_ADC_FLAG){requests++;PROCESS_ADC_FLAG=0;}
 }
 assert(requests==60000/21);
 // An outstanding foreground request remains pending, including while idle.
 one_khz_loop_counter=20;PROCESS_ADC_FLAG=1;fdNativeSineSensorTick();assert(PROCESS_ADC_FLAG==1&&one_khz_loop_counter==20);
 one_khz_loop_counter=21;fdNativeSineSensorTick();assert(PROCESS_ADC_FLAG==1&&one_khz_loop_counter==0);
 // Six-step continues to own its existing PID/sensor schedule.
 stepper_sine=0;PROCESS_ADC_FLAG=0;one_khz_loop_counter=21;fdNativeSineSensorTick();assert(!PROCESS_ADC_FLAG&&one_khz_loop_counter==21);
 for(unsigned i=0;i<4;i++)assert(ccr[i]==old_pwm[i]);assert(bridge==old_bridge);
 stepper_sine=old_sine;PROCESS_ADC_FLAG=old_flag;one_khz_loop_counter=old_counter;
 puts("PASS sine sensor service requests across 3s, pending work retained, six-step schedule and PWM unchanged");
}
static void settingsOverride(void){${settings}
}
static void mapInput(void){${block(main,'if (eepromBuffer.use_sine_start) {')}}
static void nativeStep(void){for(unsigned once=0;once<1;once++){
${sine}
}}
static void zero(void){adjusted_input=0;fdBenchGuardInput();assert(!bridge&&!fd_power_bench.inhibit);}
static void begin(unsigned throttle){zero();adjusted_input=throttle;assert(!fdBenchGuardInput());mapInput();assert(fd_native_sine.phase==1);}
int main(void){
 checkSensorTick();
 eepromBuffer.use_sine_start=0;eepromBuffer.advance_level=34;eepromBuffer.auto_advance=1;
 settingsOverride();assert(fdBenchConfigOk());assert(eepromBuffer.minimum_duty_cycle==1);
 assert(eepromBuffer.advance_level==34&&eepromBuffer.auto_advance==1);
 eepromBuffer.advance_level=18;eepromBuffer.auto_advance=0;
 assert(!fdNativeSineClockReady());clock_advances=0;fdNativeSineClockInit();assert(!fdNativeSineClockReady());
 adjusted_input=150;assert(fdBenchGuardInput());assert(!bridge&&fd_power_bench.last_reason==15);
 clock_advances=1;fdNativeSineClockInit();assert(fdNativeSineClockReady());
 // A held positive input after boot never acquires permission by itself.
 adjusted_input=215;assert(fdBenchGuardInput());assert(!fd_power_bench.active_id);
 begin(215);nativeStep();assert(stepper_sine&&bridge==1);assert(input==123&&step_delay==57);assert(fd_native_sine.sine_steps==1);assert(ccr[0]!=ccr[1]);
 // 150% is a valid setting, but changing settings mid-trial is rejected.
 eepromBuffer.sine_mode_power=5;assert(fdBenchGuardInput());assert(!bridge);eepromBuffer.sine_mode_power=6;
 assert(fdBenchGuardInput());assert(!fd_power_bench.active_id);
 // Natural phase-boundary handover on a slow rising throttle, not a fixed dwell.
 zero();uint32_t start=DWT->CYCCNT;unsigned iterations=0;
 while(DWT->CYCCNT-start<1000000u*160u){unsigned us=(DWT->CYCCNT-start)/160u;adjusted_input=us*400u/1000000u;if(fdBenchGuardInput()){delayMicros(50);continue;}mapInput();if(input<=48){delayMicros(50);continue;}nativeStep();iterations++;if(!stepper_sine)break;}
 assert(!stepper_sine&&bridge==2&&commutations==1);assert(iterations>360);assert(fd_native_sine.phase==2&&fd_native_sine.edges==0&&zero_crosses==20);assert(step==6);assert(ccr[0]==1200&&ccr[0]==ccr[1]&&ccr[3]);
 // The native seed of 20 does not count as measured feedback in the summary.
 for(unsigned i=0;i<6;i++){DWT->CYCCNT+=160000;fdNativeSineEdge();}
 assert(fd_native_sine.phase==3&&fd_native_sine.edges==6);
 DWT->CYCCNT=fd_native_sine.started+fd_native_sine.sixth_edge_at+1000000u*160u;fdBenchService();assert(fd_power_bench.inhibit&&!bridge&&fd_native_sine.reason==FD_BENCH_WINDOW);
 adjusted_input=215;assert(fdBenchGuardInput());
 // Operator zero and disarm between the last sine step and handover stop output.
 begin(150);nativeStep();adjusted_input=0;unsigned writes=compares;advanceincrement();assert(!bridge&&compares==writes);allpwm();assert(!bridge);assert(!fdNativeSineHandoff());
 begin(150);nativeStep();armed=0;advanceincrement();assert(!bridge&&!fdNativeSineHandoff());armed=1;
 // Losing the control clock also gates every output and latches the trial.
 begin(150);nativeStep();DWT->CTRL&=~1u;advanceincrement();assert(!bridge&&fd_native_sine.reason==15);allpwm();assert(!bridge);fdNativeSineClockInit();assert(fdBenchGuardInput());
 // A hard protection ceiling below the valid ADC window cannot be overridden.
 begin(150);nativeStep();cap=100;assert(!fdNativeSineHandoff()&&!bridge);cap=610;
 // A stalled/no-feedback trial is bounded, including DWT wraparound.
 zero();DWT->CYCCNT=0xfffffff0u;adjusted_input=215;assert(!fdBenchGuardInput());mapInput();nativeStep();DWT->CYCCNT=fd_native_sine.started+3000000u*160u;fdBenchService();assert(!bridge&&fd_power_bench.inhibit);
 unsigned n=fd_power_bench.trials;assert(fdBenchGuardInput());assert(fd_power_bench.trials==n);
 zero();assert(mask==0);assert(!fdBenchDeadlineExpired());
 // Force references to adapter entry points also used by linked firmware.
 (void)fdBenchBegin;fdBenchWritePwm(1200);assert(!bridge);
 puts("PASS native sine ramp/handover, fresh feedback count, zero/disarm races, cap priority, bounded stop and restart latch");return 0;}
`;


if(process.argv.includes('--sine-only')||process.argv.includes('--centered')||process.argv.includes('--fixed')){
 assert(!process.argv.includes('--adc'),'Select one test mode');
 program=program.replace('#define TARGET_FD_NATIVE_SINE','#define TARGET_FD_NATIVE_SINE\n#define TARGET_FD_NATIVE_SINE_ONLY');
 const i=program.indexOf('int main(void){');program=program.slice(0,i)+`
int main(void){
 checkSensorTick();
 fdNativeSineClockInit();settingsOverride();assert(fdNativeSineClockReady());
 for(unsigned dir=0;dir<2;dir++)for(unsigned th=48;th<=2047;th=th==48?100:th==100?215:th==215?256:th==256?400:th==400?2047:2048){
  forward=dir;begin(th);unsigned n=fd_native_sine.sine_steps;
  for(unsigned i=0;i<720;i++){nativeStep();assert(stepper_sine&&bridge==1);assert(fd_native_sine.phase==1&&fd_native_sine.edges==0);assert(fd_native_sine.sixstep_at==0xffffffffu);assert(commutations==0);assert(step_delay>=57&&step_delay<=500);}
  assert(fd_native_sine.sine_steps==n+720);if(th>=215)assert(step_delay==57);zero();
 }
 // Deadline, operator zero and clock failures also stop the isolated path.
 begin(2047);nativeStep();DWT->CYCCNT=fd_native_sine.started+3000000u*160u;nativeStep();assert(!bridge&&fd_native_sine.reason==FD_BENCH_WINDOW&&stepper_sine);assert(commutations==0);assert(fd_native_sine.sixstep_at==0xffffffffu);
 assert(fdBenchGuardInput());begin(400);nativeStep();adjusted_input=0;nativeStep();fdBenchService();assert(!bridge);
 begin(400);nativeStep();armed=0;nativeStep();fdBenchService();assert(!bridge);armed=1;
 begin(400);nativeStep();DWT->CTRL&=~1u;nativeStep();assert(!bridge&&fd_native_sine.reason==15&&commutations==0);
 puts("PASS sine-only: low-to-full throttle in both directions, no six-step/feedback handover, native speed cap, 3s cutoff and zero/fault stop");return 0;}
`;
}


if(process.argv.includes('--centered')||process.argv.includes('--fixed')){
 program=program.replace('#define TARGET_FD_NATIVE_SINE_ONLY','#define TARGET_FD_NATIVE_SINE_ONLY\n#define TARGET_FD_SINE_CENTERED_PWM');
 const regs=`
#define TIM_CR1_CKD_Msk 0x300u
#define TIM_CR1_UDIS 2u
#define TIM_BDTR_DTG_Msk 255u
#define TARGET_FIXED_DEAD_TIME_CODE 0xceu
#define ZCD_SAMPLE_FLOOR 460u
static struct {uint32_t CR1,PSC,ARR,CCER,BDTR,CCR1,CCR2,CCR3,CCR4,CNT;} fake_tim={.CR1=0x81,.ARR=6665,.CCER=0x1555,.BDTR=0x200a0ce};
#define TIM1 (&fake_tim)
static struct {uint32_t MODER,AFR[2],BRR,BSRR,IDR;} fake_pa={.AFR={0,0x66666}},fake_pf={.AFR={6,0}};
#define GPIOA (&fake_pa)
#define GPIOF (&fake_pf)
#define PHASE_A_GPIO_HIGH (1u<<8)
#define PHASE_B_GPIO_HIGH (1u<<9)
#define PHASE_C_GPIO_HIGH (1u<<10)
#define PHASE_A_GPIO_LOW (1u<<11)
#define PHASE_B_GPIO_LOW (1u<<12)
#define PHASE_C_GPIO_LOW 1u
#define PHASE_A_GPIO_PORT_HIGH GPIOA
#define PHASE_B_GPIO_PORT_HIGH GPIOA
#define PHASE_C_GPIO_PORT_HIGH GPIOA
#define PHASE_A_GPIO_PORT_LOW GPIOA
#define PHASE_B_GPIO_PORT_LOW GPIOA
#define PHASE_C_GPIO_PORT_LOW GPIOF
#define LL_GPIO_MODE_OUTPUT 1u
#define LL_GPIO_MODE_ALTERNATE 2u
#define LOW_BITREG_OFF BRR
`;
 program=program.replace('#define DEAD_TIME 70','#define DEAD_TIME 70\n'+regs);
 program=program.replace('static void allOff(void){bridge=0;}','static void allOff(void){bridge=0;GPIOA->MODER=0x1550000;GPIOF->MODER=1;}');
 program=program.replace('ccr[3]=x>=704?x/2:0;}','ccr[3]=x>=704?x/2:0;TIM1->CCR1=TIM1->CCR2=TIM1->CCR3=x;TIM1->CCR4=ccr[3];}');
 const phase=fs.readFileSync(path.join(root,'Mcu/g431/Src/phaseouts.c'),'utf8');
 for(const c of ['A','B','C'])program=program.replace(`static void phase${c}PWM(void){bridge=1;}`,block(phase,`void phase${c}PWM()`));
 program=program.replace('void phaseAPWM()',`#define LL_GPIO_SetPinMode(port,pin,mode) ((port)->MODER=((port)->MODER&~(3u<<(2*__builtin_ctz(pin))))|((mode)<<(2*__builtin_ctz(pin))),bridge=(mode)==2?1:bridge)
void phaseAPWM()`);
 for(const [n,idx] of [[1,0],[2,1],[3,2]])program=program.replace(`ccr[${idx}]=x;compares++;}`,`ccr[${idx}]=x;TIM1->CCR${n}=x;compares++;}`);
 const i=program.indexOf('int main(void){');
 const raw=block(main,'void advanceincrement()').replace('void advanceincrement()','static void rawAdvanceincrement()');
 program=program.slice(0,i)+'\n#undef TARGET_FD_SINE_CENTERED_PWM\n'+raw+'\n#define TARGET_FD_SINE_CENTERED_PWM\n'+`
int main(void){
 checkSensorTick();
 fdNativeSineClockInit();settingsOverride();assert(fdNativeSineClockReady());
 unsigned clipped=0,min_out=65535,max_out=0;
 for(unsigned dir=0;dir<2;dir++){
  forward=dir;begin(215);nativeStep();assert(bridge==1);
  for(unsigned angle=0;angle<360;angle++){
   int16_t a=phase_A_position,b=phase_B_position,c=phase_C_position;
   rawAdvanceincrement();unsigned raw[3]={ccr[0],ccr[1],ccr[2]};if(raw[0]<=368)clipped++;
   phase_A_position=a;phase_B_position=b;phase_C_position=c;advanceincrement();
   assert(TIM1->CR1==0x81&&TIM1->BDTR==0x200a0ce&&bridge==1);
   for(unsigned x=0;x<3;x++){assert(ccr[x]>=460&&ccr[x]<=6666-460);if(ccr[x]<min_out)min_out=ccr[x];if(ccr[x]>max_out)max_out=ccr[x];for(unsigned y=0;y<3;y++)assert((int)ccr[x]-(int)ccr[y]==(int)raw[x]-(int)raw[y]);}
  }
  unsigned last[3]={ccr[0],ccr[1],ccr[2]};zero();assert(!bridge&&ccr[0]==0);
  assert(fd_sine_pwm_snapshot.steps>0&&fd_sine_pwm_snapshot.ccr1==last[0]&&fd_sine_pwm_snapshot.ccr2==last[1]&&fd_sine_pwm_snapshot.ccr3==last[2]);
  for(unsigned pin=8;pin<=12;pin++)assert(((fd_sine_pwm_snapshot.gpioa_moder>>(2*pin))&3u)==2u);
  assert((fd_sine_pwm_snapshot.gpiof_moder&3u)==2u);
 }
 assert(clipped==274);assert(commutations==0);assert(min_out>368&&6666-max_out>368);
 begin(215);TIM1->PSC=1;nativeStep();assert(!bridge&&fd_native_sine.reason==FD_BENCH_BAD_CONFIG);TIM1->PSC=0;
 begin(215);TIM1->ARR=600;nativeStep();assert(!bridge);TIM1->ARR=6665;
 begin(215);TIM1->BDTR=70;nativeStep();assert(!bridge);TIM1->BDTR=0x200a0ce;
 begin(215);TIM1->CR1|=2;nativeStep();assert(!bridge);TIM1->CR1=0x81;
 printf("PASS actual GPIO/PWM mapping: all360angles/bothdirections, all pair differences preserved, native clipped137/360, centered range%u..%u, live-state frozen before off, invalid timer refused\\n",min_out,max_out);
 return 0;}
`;
}


if(process.argv.includes('--fixed')){
 program=program.replace('#define TARGET_FD_SINE_CENTERED_PWM','#define TARGET_FD_SINE_CENTERED_PWM\n#define TARGET_FD_SINE_FIXED_PROBE');
 const i=program.indexOf('int main(void){');program=program.slice(0,i)+`
int main(void){
 checkSensorTick();fdNativeSineClockInit();settingsOverride();
 begin(215);nativeStep();assert(bridge&&fd_native_sine.sine_steps==1);
 assert(phase_A_position==150&&phase_B_position==269&&phase_C_position==29);
 assert(ccr[0]==3603&&ccr[1]==3063&&ccr[2]==3597);
 assert(fd_sine_pins.count==0&&!fd_sine_pins.frozen);
 unsigned prior[3]={ccr[0],ccr[1],ccr[2]};
 DWT->CYCCNT=fd_sine_pins.driven_at+159999;nativeStep();assert(fd_sine_pins.count==0);
 for(unsigned n=0;n<256;n++){
  DWT->CYCCNT=fd_sine_pins.driven_at+160000+n*1600;
  TIM1->CNT=(n*31)%6666;unsigned t=TIM1->CNT,a=0,f=0;
  for(unsigned c=0;c<3;c++){if(t>=368&&t<prior[c])a|=1u<<(8+c);if(t>=prior[c]+368){if(c<2)a|=1u<<(11+c);else f|=1;}}
  GPIOA->IDR=a;GPIOF->IDR=f;nativeStep();
  assert(fd_sine_pins.count==n+1&&fd_sine_pins.samples[n].before==t&&fd_sine_pins.samples[n].after==t);
  assert(fd_sine_pins.samples[n].gpioa==a&&fd_sine_pins.samples[n].gpiof==f);
  assert(fd_native_sine.sine_steps==1&&mask==0&&TIM1->CR1==0x81);
  for(unsigned c=0;c<3;c++)assert(ccr[c]==prior[c]);
 }
 assert(fd_sine_pins.frozen);nativeStep();assert(fd_sine_pins.count==256);
 DWT->CYCCNT=fd_native_sine.started+100000*160u-1;fdBenchService();assert(bridge);
 DWT->CYCCNT++;fdBenchService();assert(!bridge&&fd_native_sine.reason==FD_BENCH_WINDOW&&fd_sine_pins.frozen);
 assert(fdBenchGuardInput());assert(fd_sine_pins.count==256); // no restart while held
 begin(215);nativeStep();assert(fd_sine_pins.count==0&&!fd_sine_pins.frozen);zero();assert(!bridge&&fd_sine_pins.frozen);
 begin(215);nativeStep();armed=0;fdBenchService();assert(!bridge&&fd_sine_pins.frozen);armed=1;
 begin(215);nativeStep();DWT->CTRL&=~1u;fdBenchService();assert(!bridge&&fd_sine_pins.frozen);
 assert(!commutations);puts("PASS fixed-vector100ms: exact CCRs, single command, passive pad sampling,1ms preload wait, capacity freeze, no held-input restart, zero/disarm/clock stop");return 0;}
`;
}

if(process.argv.includes('--adc')){
 const adc=fs.readFileSync(path.join(root,'Mcu/g431/Src/comparator.c'),'utf8');
 assert(/#define TARGET_MIN_BEMF_COUNTS 0/.test(fs.readFileSync(path.join(root,'Inc/targets.h'),'utf8')));
 program=program.replace('#define TARGET_FD_NATIVE_SINE',`#define TARGET_FD_NATIVE_SINE
#define TARGET_ADC_ZCD_GUARD
#define TARGET_FD_ADC_JEOS
#define TARGET_FD_ADC_COHERENT
#define TARGET_FD_ASYNC_COMM
#define USE_ADC_ZCD
#define FD6288_STARTUP_TIMING_FIX 1
#define FD_DIAG(...) ((void)0)
#define FD_DECISION(...) ((void)0)
#define fdTraceNoFresh() ((void)0)
#define fdTraceEvent(...) ((void)0)
#define ADC_ZCD_BLANK_SAMPLES 1
#define ADC_ZCD_HYSTERESIS_COUNTS 8
#define ADC_ZCD_MIN_DRIVE_DELTA 64
#define FD_EVENT_ZC 0
#define ADC1 0
#define LL_ADC_INJ_RANK_1 0
#define LL_ADC_INJ_RANK_2 1
#define LL_ADC_INJ_RANK_3 2`);
 program=program.replace('static unsigned average_interval,last_average_interval,commutation_interval,commutation_intervals[6];','static unsigned average_interval,last_average_interval,commutation_intervals[6];\nvolatile uint32_t commutation_interval,polling_mode_changeover=2000;');
 program=program.replace('static void adcZcdResetAcquisition(unsigned x){(void)x;}','void adcZcdResetAcquisition(uint8_t x);\nvoid changeCompInput(void);\nstatic volatile char rising;');
 program=program.replace('commutations++;bridge=2;}','commutations++;bridge=2;rising=step%2;changeCompInput();}');
 const real=`
static uint8_t adc_zcd_phase,adc_zcd_blank,adc_zcd_last_out,adc_zcd_before_seen,adc_zcd_confirmations,adc_zcd_reported,adc_zcd_drive_crossings,adc_zcd_required_samples;
static uint32_t adc_zcd_earliest,adc_zcd_last_real_interval;
static unsigned fresh,read_count,rearmed,schedules,scheduled_ticks;
static unsigned interval_timer,thiszctime,waitTime,advance,temp_advance=8,auto_advance_level=8,bemfcounter,bad_count,bad_count_threshold=6,zcfound,min_bemf_counts_up,min_bemf_counts_down;
static uint16_t samples[3];
#undef SET_INTERVAL_TIMER_COUNT
#define SET_INTERVAL_TIMER_COUNT(x) (interval_timer=(x))
#define INTERVAL_TIMER_COUNT interval_timer
static unsigned LL_ADC_IsActiveFlag_JEOS(unsigned x){(void)x;return fresh;}
static void LL_ADC_ClearFlag_JEOS(unsigned x){(void)x;fresh=0;}
static unsigned LL_ADC_INJ_ReadConversionData12(unsigned x,unsigned rank){(void)x;read_count++;return samples[rank];}
static void adcZcdRearmInjected(void){rearmed++;}
static void heliCoastRecordZeroCross(void){}
static void enableCompInterrupts(void){old_routine=1;}
static void fdAsyncSchedule(uint32_t at,uint32_t ticks){scheduled_at=at;schedules++;scheduled_ticks=ticks;}
${block(adc,'void adcZcdResetAcquisition(uint8_t reason)')}
${block(adc,'uint32_t adcZcdDrivePeriod(uint32_t elapsed, uint32_t previous)')}
${block(adc,'static uint8_t adcZcdQualifyDrive(int32_t margin)')}
${block(adc,'uint8_t getCompOutputSample(uint8_t *level)')}
${block(adc,'void changeCompInput(void)')}
${block(main,'void getBemfState()')}
${block(main,'void zcfoundroutine()')}
${block(main,'void fdAdcJeosService(void)')}
static void sample(unsigned b,unsigned ticks){samples[0]=1600;samples[1]=b;samples[2]=0;interval_timer=ticks;fresh=1;DWT->CYCCNT+=6667u;fdAdcJeosService();}
`;
 const i=program.indexOf('int main(void){');program=program.slice(0,i)+real+`
int main(void){
 checkSensorTick();
 fdNativeSineClockInit();assert(fdNativeSineClockReady());
 begin(150);nativeStep();assert(stepper_sine&&bridge==1);
 sample(400,100);assert(!fresh&&read_count==0&&schedules==0); // no ZC-driven commutation in sine
 phase_A_position=0;assert(fdNativeSineHandoff());assert(step==6&&rising==0&&adc_zcd_phase==6);
 assert(adc_zcd_drive_crossings==0&&adc_zcd_earliest==0&&adc_zcd_required_samples==2); // seed20 is not20 acquired ADC edges
 sample(400,200);assert(schedules==0); // boundary group blanked
 sample(400,300);sample(400,400);assert(schedules==0); // POST level alone cannot accept
 sample(1200,500);assert(schedules==0); // actual PRE arms the falling edge
 sample(400,600);assert(schedules==0); // first fresh POST confirmation
 unsigned reads=read_count;fdAdcJeosService();assert(schedules==0&&read_count==reads); // no duplicate vote
 sample(400,700);assert(schedules==1&&fd_native_sine.edges==1);
 assert(commutation_interval==9000&&scheduled_ticks==3375); // native seed period, inherited7.5deg timing
 sample(400,800);assert(schedules==1&&fd_native_sine.edges==1); // once per sector
 assert(rearmed>=7&&mask==0&&old_routine==1);
 puts("PASS actual native handover -> ADC fresh PRE/POST qualification -> JEOS event -> inherited timing -> async schedule; no hardware comparator dependency");return 0;}
`;
}


if(fixed12){
 assert(main.indexOf('fdSineRecordSensors();')>main.indexOf('if (PROCESS_ADC_FLAG == 1)'));
 assert(main.indexOf('fdSineRecordSensors();')<main.indexOf('PROCESS_ADC_FLAG = 0;',main.indexOf('if (PROCESS_ADC_FLAG == 1)')));
 program=program.replace('#define TARGET_FD_SINE_FIXED_PROBE','#define TARGET_FD_SINE_FIXED_PROBE\n#define TARGET_FD_SINE_FIXED_12KHZ');
 program=program.replace('static unsigned zero_crosses;','static unsigned zero_crosses;\nstatic int16_t actual_current=3;\nstatic uint16_t battery_voltage=820,ADC_raw_current=10;');
 program=program.replace('static void settingsOverride(void)', '#define TIM1_AUTORELOAD 6665u\n#define SET_AUTO_RELOAD_PWM(v) (TIM1->ARR=(v))\nstatic void applyCarrier(void){'+block(main,'if (eepromBuffer.pwm_frequency < 145')+'}\nstatic void settingsOverride(void)');
 program=program.replace('checkSensorTick();fdNativeSineClockInit();settingsOverride();','checkSensorTick();fdNativeSineClockInit();settingsOverride();\n applyCarrier();tim1_arr=TIMER1_MAX_ARR;assert(TIM1->ARR==13330&&TIMER1_MAX_ARR==13330&&eepromBuffer.pwm_frequency==12);');
 program=program.replace('begin(215);nativeStep();assert(bridge&&fd_native_sine.sine_steps==1);',`begin(215);fdSineRecordSensors();assert(fd_sine_sensors.updates==0);nativeStep();assert(bridge&&fd_native_sine.sine_steps==1);
 assert(fd_sine_sensors.baseline_current==3&&fd_sine_sensors.baseline_voltage==820&&fd_sine_sensors.baseline_raw_current==10);
 unsigned c0=ccr[0],c1=ccr[1],c2=ccr[2];
 actual_current=53;battery_voltage=805;ADC_raw_current=30;fdSineRecordSensors();
 actual_current=20;battery_voltage=815;ADC_raw_current=20;fdSineRecordSensors();
 assert(fd_sine_sensors.updates==2&&fd_sine_sensors.peak_current==53&&fd_sine_sensors.minimum_voltage==805&&fd_sine_sensors.peak_raw_current==30);
 assert(ccr[0]==c0&&ccr[1]==c1&&ccr[2]==c2&&fd_native_sine.sine_steps==1);`);
 program=program.replace('ccr[0]==3603&&ccr[1]==3063&&ccr[2]==3597','ccr[0]==7205&&ccr[1]==6125&&ccr[2]==7193');
 program=program.replace('(n*31)%6666','(n*61)%13331');
 program=program.replace('assert(fdBenchGuardInput());assert(fd_sine_pins.count==256);',`actual_current=999;battery_voltage=0;ADC_raw_current=999;fdSineRecordSensors();
 assert(fd_sine_sensors.updates==2&&fd_sine_sensors.peak_current==53&&fd_sine_sensors.minimum_voltage==805);
 assert(fdBenchGuardInput());assert(fd_sine_pins.count==256);`);
 program=program.replace('begin(215);nativeStep();assert(fd_sine_pins.count==0',`begin(215);nativeStep();assert(fd_sine_sensors.updates==0&&fd_sine_sensors.baseline_current==999);eepromBuffer.pwm_frequency=24;assert(fdBenchGuardInput()&&!bridge);eepromBuffer.pwm_frequency=12;
 begin(215);nativeStep();assert(fd_sine_pins.count==0`);
 program=program.replace('PASS fixed-vector100ms:', 'PASS 12k fixed-vector100ms + sensor peaks/freeze and wrong-frequency interlock:');
}


if(sixstep){
 const outs=fs.readFileSync(path.join(root,'Mcu/g431/Src/phaseouts.c'),'utf8');
 program=program.replace('#define TARGET_FD_SINE_FIXED_PROBE','#define TARGET_FD_SINE_FIXED_PROBE\n#define TARGET_FD_SIXSTEP_ALIGN_CONTROL');
 program=program.replace('static unsigned zero_crosses;','static unsigned zero_crosses;\nstatic int16_t actual_current=3;\nstatic uint16_t battery_voltage=820,ADC_raw_current=10;\nstatic unsigned pwm_updates;');
 program=program.replace('#define LOW_BITREG_OFF BRR','#define LOW_BITREG_OFF BRR\n#define HIGH_BITREG_OFF BRR\n#define LOW_BITREG_ON BSRR');
 program=program.replace('static void generatePwmTimerEvent(void){}','static void generatePwmTimerEvent(void){assert(!bridge&&TIM1->CCR4==0);pwm_updates++;}');
 const functions=['phaseAFLOAT','phaseBFLOAT','phaseCFLOAT','phaseALOW','phaseBLOW','phaseCLOW'].map(n=>block(outs,'void '+n+'()')).join('\n');
 program=program.replace('void phaseAPWM()',functions+'\nvoid phaseAPWM()');
 program=program.replace('static void comStep(unsigned x){(void)x;}',block(outs,'void comStep(int newStep)'));
 const i=program.indexOf('int main(void){');program=program.slice(0,i)+`
int main(void){
 checkSensorTick();fdNativeSineClockInit();settingsOverride();
 begin(215);nativeStep();assert(bridge&&fd_native_sine.sine_steps==1&&fd_native_sine.last_step==6&&pwm_updates==1);
 assert(eepromBuffer.pwm_frequency==24&&ccr[0]==1200&&ccr[1]==1200&&ccr[2]==1200&&TIM1->CCR4==0);
 for(unsigned pin=8;pin<=12;pin++)assert(((GPIOA->MODER>>(2*pin))&3u)==((pin==8||pin==11)?2u:1u));
 assert((GPIOF->MODER&3u)==1u&&GPIOF->BSRR==1u);
 actual_current=53;battery_voltage=805;ADC_raw_current=30;fdSineRecordSensors();assert(fd_sine_sensors.updates==1&&fd_sine_sensors.peak_current==53);
 for(unsigned n=0;n<256;n++){
  DWT->CYCCNT=fd_sine_pins.driven_at+160000+n*1600;TIM1->CNT=(n*31)%6666;unsigned t=TIM1->CNT;
  GPIOA->IDR=(t>=368&&t<1200?1u<<8:0)|(t>=1568?1u<<11:0);GPIOF->IDR=1;nativeStep();
  assert(fd_sine_pins.count==n+1&&fd_native_sine.sine_steps==1&&pwm_updates==1);
  assert(TIM1->CCR1==1200&&TIM1->CCR2==1200&&TIM1->CCR3==1200&&TIM1->CCR4==0&&mask==0);
 }
 assert(fd_sine_pins.frozen);DWT->CYCCNT=fd_native_sine.started+16000000-1;fdBenchService();assert(bridge);DWT->CYCCNT++;fdBenchService();assert(!bridge&&fd_native_sine.reason==FD_BENCH_WINDOW);
 actual_current=999;fdSineRecordSensors();assert(fd_sine_sensors.updates==1&&fd_sine_sensors.peak_current==53);assert(fdBenchGuardInput());
 begin(215);nativeStep();assert(bridge&&fd_sine_sensors.updates==0);zero();assert(!bridge&&fd_sine_pins.frozen);
 unsigned updates=pwm_updates;cap=100;begin(215);nativeStep();assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP&&pwm_updates==updates);cap=610;
 begin(215);TIM1->ARR=13330;nativeStep();assert(!bridge&&fd_native_sine.reason==FD_BENCH_BAD_CONFIG);TIM1->ARR=6665;
 begin(215);nativeStep();armed=0;fdBenchService();assert(!bridge);armed=1;
 begin(215);nativeStep();DWT->CTRL&=~1u;fdBenchService();assert(!bridge&&fd_native_sine.reason==15);
 assert(!commutations);puts("PASS historical step6 control: actual GPIO functions,1200ticks/24k, one apply while off, no ADC trigger,100ms stop, sensor peaks and all inhibit paths");return 0;}
`;
}


if(ab){
 program=program.replace('#define TARGET_FD_SIXSTEP_ALIGN_CONTROL','#define TARGET_FD_SIXSTEP_ALIGN_CONTROL\n#define TARGET_FD_SIXSTEP_AB_CONTROL');
 program=program.replace('fd_native_sine.last_step==6','fd_native_sine.last_step==1');
 program=program.replace('(GPIOF->MODER&3u)==1u&&GPIOF->BSRR==1u','(GPIOF->MODER&3u)==1u&&GPIOF->BRR==1u&&GPIOA->BSRR==(1u<<12)');
 program=program.replace('GPIOA->IDR=(t>=368&&t<1200?1u<<8:0)|(t>=1568?1u<<11:0);GPIOF->IDR=1;', 'GPIOA->IDR=(t>=368&&t<1200?1u<<8:0)|(t>=1568?1u<<11:0)|(1u<<12);GPIOF->IDR=0;');
 program=program.replace('PASS historical step6 control:', 'PASS A-to-B step1 control:');
}


if(clampB){
 const outs=fs.readFileSync(path.join(root,'Mcu/g431/Src/phaseouts.c'),'utf8');
 program=program.replace('#define TARGET_FD_SINE_FIXED_12KHZ','#define TARGET_FD_SINE_FIXED_12KHZ\n#define TARGET_FD_SINE_CLAMP_B');
 program=program.replace('#define LOW_BITREG_OFF BRR','#define LOW_BITREG_OFF BRR\n#define HIGH_BITREG_OFF BRR\n#define LOW_BITREG_ON BSRR');
 program=program.replace('void phaseAPWM()',block(outs,'void phaseBLOW()')+'\nvoid phaseAPWM()');
 program=program.replace('static unsigned zero_crosses;','static unsigned zero_crosses;\nstatic unsigned pwm_updates;');
 program=program.replace('static void generatePwmTimerEvent(void){}','static void generatePwmTimerEvent(void){assert(!bridge&&TIM1->CCR4==0);pwm_updates++;}');
 const i=program.indexOf('int main(void){');program=program.slice(0,i)+`
int main(void){
 checkSensorTick();fdNativeSineClockInit();settingsOverride();applyCarrier();tim1_arr=TIMER1_MAX_ARR;
 begin(215);nativeStep();assert(bridge&&fd_native_sine.sine_steps==1&&pwm_updates==1);
 assert(TIM1->ARR==13330&&TIM1->CCR1==1080&&TIM1->CCR2==0&&TIM1->CCR3==1068&&TIM1->CCR4==0);
 unsigned centered[3]={ccr[0],ccr[1],ccr[2]},actual[3]={TIM1->CCR1,TIM1->CCR2,TIM1->CCR3};
 assert(centered[0]==7205&&centered[1]==6125&&centered[2]==7193);
 for(unsigned a=0;a<3;a++)for(unsigned b=0;b<3;b++)assert((int)centered[a]-(int)centered[b]==(int)actual[a]-(int)actual[b]);
 for(unsigned pin=8;pin<=12;pin++)assert(((GPIOA->MODER>>(2*pin))&3u)==((pin==9||pin==12)?1u:2u));
 assert((GPIOF->MODER&3u)==2u&&GPIOA->BSRR==(1u<<12));
 actual_current=53;battery_voltage=805;ADC_raw_current=30;fdSineRecordSensors();assert(fd_sine_sensors.updates==1&&fd_sine_sensors.peak_current==53);
 for(unsigned n=0;n<256;n++){
  DWT->CYCCNT=fd_sine_pins.driven_at+160000+n*1600;TIM1->CNT=(n*61)%13331;unsigned t=TIM1->CNT;
  GPIOA->IDR=(1u<<12)|(t>=368&&t<1080?1u<<8:0)|(t>=1448?1u<<11:0)|(t>=368&&t<1068?1u<<10:0);GPIOF->IDR=t>=1436;nativeStep();
  assert(fd_sine_pins.count==n+1&&fd_native_sine.sine_steps==1&&pwm_updates==1);
  assert(TIM1->CCR1==1080&&TIM1->CCR2==0&&TIM1->CCR3==1068&&TIM1->CCR4==0&&mask==0);
 }
 assert(fd_sine_pins.frozen);DWT->CYCCNT=fd_native_sine.started+16000000-1;fdBenchService();assert(bridge);DWT->CYCCNT++;fdBenchService();assert(!bridge&&fd_native_sine.reason==FD_BENCH_WINDOW);
 actual_current=999;fdSineRecordSensors();assert(fd_sine_sensors.updates==1&&fd_sine_sensors.peak_current==53);assert(fdBenchGuardInput());
 begin(215);nativeStep();zero();assert(!bridge&&fd_sine_pins.frozen);
 unsigned updates=pwm_updates;cap=100;begin(215);nativeStep();assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP&&pwm_updates==updates);cap=610;
 begin(215);TIM1->ARR=6665;nativeStep();assert(!bridge&&fd_native_sine.reason==FD_BENCH_BAD_CONFIG);TIM1->ARR=13330;
 begin(215);nativeStep();armed=0;fdBenchService();assert(!bridge);armed=1;
 begin(215);nativeStep();DWT->CTRL&=~1u;fdBenchService();assert(!bridge&&fd_native_sine.reason==15);
 assert(!commutations);puts("PASS fixed B-clamp: exact all-pair differences preserved, actual A/C PWM and B low, one off-state update,100ms and fault stops, passive records");return 0;}
`;
}

if (process.argv.includes('--m2-coast')) {
 program=program.replace('#define TARGET_FD_NATIVE_SINE',`#define TARGET_FD_NATIVE_SINE
#define TARGET_FD_NATIVE_SINE_ONLY
#define TARGET_FD_M2_SINE_COAST`);
 program=program.replace('active_brake_power;}','active_brake_power, input_type, stuck_rotor_protection; struct {unsigned current, temperature;} limits;}');
 const includeAt=program.indexOf('\n#include "');
 program=program.slice(0,includeAt)+`
#define DSHOT_IN 1u
#define ZCD_SAMPLE_FLOOR 460u
#define ZCD_ADC_WINDOW_TICKS 244u
#define TARGET_FIXED_DEAD_TIME_CODE 0xceu
#define TIM_CR1_CKD_Msk (3u<<8)
#define TIM_CR1_CMS_Msk (3u<<5)
#define TIM_CR1_UDIS 2u
#define TIM_BDTR_DTG_Msk 0xffu
static struct {uint32_t PSC, ARR, CR1, BDTR, CCR1, CCR2, CCR3, CCR4, CNT;} mockTim={.ARR=6665,.CR1=0x81,.BDTR=0x80ce};
static struct {uint32_t BSRR, BRR;} mockGpio;
#define TIM1 (&mockTim)
#define GPIOB (&mockGpio)
static uint16_t degrees_celsius=25, ADC_raw_current=8, battery_voltage=1140;
void allpwm(void);
static int16_t actual_current;
static unsigned use_current_limit;
static unsigned adc_rearmed, adc_cleared, adc_fresh;
#define ADC1 1
#define LL_ADC_INJ_RANK_1 1
#define LL_ADC_INJ_RANK_2 2
#define LL_ADC_INJ_RANK_3 3
#define LL_ADC_IsActiveFlag_JEOS(adc) (adc_fresh)
#define LL_ADC_ClearFlag_JEOS(adc) do { adc_cleared++; adc_fresh=0; } while (0)
#define LL_ADC_INJ_ReadConversionData12(adc,rank) ((rank)*100u)
#define SET_HELI_COAST_HIGH_Z_SAMPLE(period) do { TIM1->CCR1=TIM1->CCR2=TIM1->CCR3=0; TIM1->CCR4=(period)/2; } while (0)
#undef SET_DUTY_CYCLE_ALL
#define SET_DUTY_CYCLE_ALL(x) do { setall(x); TIM1->CCR1=TIM1->CCR2=TIM1->CCR3=(x); TIM1->CCR4=zcd_ccr4_from_duty((x)); } while (0)
#define TARGET_FD_ADC_COHERENT
static void adcZcdRearmInjected(void){adc_rearmed++;}
static void getBemfState(void){assert(0 && "sine trial cannot enter the BEMF controller");}
static unsigned zcfound, rising, bemfcounter, min_bemf_counts_up, min_bemf_counts_down;
static void heliCoastRecordZeroCross(void){assert(0);}
static void zcfoundroutine(void){assert(0);}
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
extern volatile uint8_t fd_m2_sine_closed_late;
#endif
${block(fs.readFileSync(path.join(root,'Mcu/g431/Inc/peripherals.h'),'utf8'),'static inline uint16_t zcd_ccr4_from_duty(uint16_t duty)')}
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
static void fdM2SineClosedEvent(uint8_t kind);
static void fdMockClosedSample(void){if(fd_m2_sine_closed_late)TIM1->CCR4=zcd_ccr4_from_duty(TIM1->CCR1);fdM2SineClosedEvent(0u);}
#endif
`+program.slice(includeAt);
 const i=program.indexOf('int main(void){');
 program=program.slice(0,i)+`
${block(main,'void fdAdcJeosService(void)')}
static void checkCompares(void)
{
 unsigned span_min=2000,span_max=0,power10_clamps=0,max_model_error=0;
 for(unsigned fade=0;fade<=150;fade++){
  for(unsigned angle=0;angle<360;angle++){
   uint16_t raw[3]={fdM2SineRaw(pwmSin[angle],fade),
     fdM2SineRaw(pwmSin[(angle+119)%360],fade),fdM2SineRaw(pwmSin[(angle+239)%360],fade)};
   uint16_t c[3];
   fdM2SineCompares(raw,460,c);
   unsigned lo=6666,hi=0,clamps=0;
   for(unsigned a=0;a<3;a++){
    assert(c[a]==0||c[a]>=460);
    assert(c[a]<6666-460);
    if(!c[a])clamps++;
    if(c[a]<lo)lo=c[a];
    if(c[a]>hi)hi=c[a];
    for(unsigned b=0;b<3;b++){
     assert((int)c[a]-c[b]==(int)raw[a]-raw[b]);
     /* Timing-only model with both legs freewheeling low during dead time. */
     unsigned ah=c[a]?c[a]-368:0, bh=c[b]?c[b]-368:0;
     int error=(int)ah-(int)bh-((int)raw[a]-raw[b]);
     unsigned magnitude=error<0?-error:error;
     if(magnitude>max_model_error)max_model_error=magnitude;
    }
   }
   assert(clamps<=1);
   if(fade==150){
    power10_clamps+=clamps;
    if(hi-lo<span_min)span_min=hi-lo;
    if(hi-lo>span_max)span_max=hi-lo;
   }
  }
 }
 assert(span_min>=890&&span_max<=1060&&power10_clamps>0&&power10_clamps<360);
 printf("Power-10 full circle span %u..%u ticks, clamp at %u/360 angles, timing-model worst difference %u ticks\\n",
  span_min,span_max,power10_clamps,max_model_error);
}
static void drive(unsigned us){delayMicros(us);fdBenchService();nativeStep();}
static unsigned drivenSector(void)
{
 const unsigned c[3]={TIM1->CCR1,TIM1->CCR2,TIM1->CCR3};
 if(c[0]==c[1]||c[1]==c[2]||c[0]==c[2]) return 0;
 unsigned hi=0,lo=0;
 for(unsigned i=1;i<3;i++){
  if(c[i]>c[hi])hi=i;
  if(c[i]<c[lo])lo=i;
 }
 const unsigned step_by_phase[3][3]={{0,1,6},{4,0,5},{3,2,0}};
 return step_by_phase[hi][lo];
}

static void checkDriveDirection(unsigned direction)
{
 forward=direction;
 begin(100);nativeStep();
 for(unsigned n=0;n<1500;n++)drive(100);
 unsigned last=0,transitions=0;
 while(fd_m2_sine_run.slow_travel<720u*FD_M2_SINE_DEGREE) {
  drive(100);
  unsigned sector=drivenSector();
  if(!sector||sector==last)continue;
  if(last){
   unsigned expected=forward?(last==6?1:last+1):(last==1?6:last-1);
   assert(sector==expected);
   transitions++;
  }
  last=sector;
 }
 assert(transitions>=11);
 zero();
}

int main(void){
 settingsOverride();
 assert(eepromBuffer.startup_power==50&&eepromBuffer.motor_poles==24&&eepromBuffer.motor_kv==12);
 assert(eepromBuffer.sine_mode_power==10&&eepromBuffer.sine_mode_changeover_thottle_level==5);
 assert(eepromBuffer.advance_level==18&&eepromBuffer.auto_advance==0&&eepromBuffer.input_type==DSHOT_IN);
 eepromBuffer.limits.temperature=120;min_startup_duty=260;
 assert(fdBenchConfigOk());
 checkCompares();
 fdNativeSineClockInit();
 DWT->CYCCNT=0xffff0000u;
 begin(100);nativeStep();assert(bridge&&phase_A_position==180);
 for(unsigned n=0;n<1490;n++)drive(100);
 assert(bridge&&phase_A_position==180&&fd_m2_sine_coast.max_span_ticks>800);
 while(fd_m2_sine_run.slow_travel<720u*FD_M2_SINE_DEGREE) {
  drive(42);assert(fd_m2_sine_run.rate==fdM2SineRequestedRate(48));
 }
 uint32_t started=fd_native_sine.started;
 while(!fd_m2_sine_run.coast && DWT->CYCCNT-started<2100000u*160u){
  unsigned before=fd_m2_sine_run.rate;
  drive(42);
  assert(fd_m2_sine_run.rate>=before&&fd_m2_sine_run.rate-before<=24u);
 }
 assert(fd_m2_sine_run.coast&&!bridge&&TIM1->CCR4==3333);
 assert(fd_m2_sine_coast.off_at>1100000u*160u&&fd_m2_sine_coast.off_at<1500000u*160u);
 assert(fd_m2_sine_coast.rate==fdM2SineRequestedRate(120)&&fd_m2_sine_coast.clamp_frames>0);
 for(unsigned n=0;n<960;n++){
  DWT->CYCCNT=fd_m2_sine_run.coast_at+n*6666u+3600u;
  adc_fresh=1;fdAdcJeosService();
 }
 assert(adc_cleared==960&&adc_rearmed==960&&fd_m2_sine_coast.seen==960);
 assert(fd_m2_sine_coast.count==492&&fd_m2_sine_coast.samples[491].c==300);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;
 fdBenchService();assert(!bridge&&!fd_m2_sine_run.coast&&fd_native_sine.reason==FD_BENCH_HIZ_DONE);
 assert(fdBenchGuardInput()&&fd_power_bench.trials==1);
 begin(36);nativeStep();
 for(unsigned n=0;n<20000;n++)drive(100);
 assert(!bridge&&fd_m2_sine_coast.off_at>=2000000u*160u&&fd_m2_sine_coast.rate<fdM2SineRequestedRate(120));
 begin(100);nativeStep();actual_current=400;drive(42);
 assert(fd_native_sine.reason==FD_BENCH_SAFETY_CAP&&!bridge);actual_current=0;
 begin(100);nativeStep();drive(251);assert(!bridge&&fd_native_sine.reason==FD_BENCH_COMM_LATE);
 begin(100);nativeStep();armed=0;drive(42);assert(!bridge);armed=1;
 begin(100);nativeStep();zero();assert(!bridge&&!fd_m2_sine_run.coast);
 checkDriveDirection(0);checkDriveDirection(1);
 assert(!commutations && !fd_native_sine.edges && mask==0);
 puts("PASS M2 profile, slow travel, slew, high-Z JEOS capture, timeout, current/late abort and zero re-arm");
 return 0;
}
`;
}

if (m2Sixstep) {
 const pickupCapture=m2Adaptive?'capture-m2-sine-500-adaptive-20261118.json':'capture-m2-sine-coast-direction-20261117.json';
 const capture=JSON.parse(fs.readFileSync(path.join(root,'docs/m2-startup',pickupCapture),'utf8'));
 const earlyCapture=m2Runup?JSON.parse(fs.readFileSync(path.join(root,'docs/m2-startup/capture-m2-sine-runup-420-20261117.json'),'utf8')):null;
 const earlySectors=earlyCapture?.handoff.sectors.slice(0,4);
 const earlyReplay=earlySectors?.map(s=>`{${Math.round((s.at_ms-earlySectors[0].at_ms)*160000)}u,${Math.round((s.edge_ms-earlySectors[0].at_ms)*160000)}u}`).join(',\n');
 const replay=capture.coast.samples.filter(s=>s.us<20000).map(s=>`{${Math.round(s.us*160)}u,${s.a},${s.b},${s.c}}`).join(',\n');
 let runupFlag=m2Runup?'\n#define TARGET_FD_M2_SINE_RUNUP':'';
 if(m2RunupLateAdc)runupFlag+='\n#define TARGET_FD_M2_SINE_STEP4_LATE_ADC\n#define FD6288_M2_SINE_STEP4_ADC_TICKS 960u';
 if(m2Step5Advance)runupFlag+='\n#define TARGET_FD_M2_SINE_STEP5_ADVANCE\n#define FD6288_M2_SINE_STEP5_ADVANCE_US 350u';
 if(m2RunFix)runupFlag+='\n#define TARGET_FD_M2_RUN_FIX';
 if(m2StablePre)runupFlag+='\n#define TARGET_FD_M2_STABLE_PRE';
 if(m2Pure)runupFlag+='\n#define TARGET_FD_M2_PURE';
 if(m2Heli)runupFlag+='\n#define TARGET_FD_M2_HELI_START\n#define FD6288_M2_HELI_RISE_MS 3000u';
 if(m2AdcCapture)runupFlag+='\n#define TARGET_FD_M2_SINE_ADC_CAPTURE';
 if(m2Continuous)runupFlag+='\n#define TARGET_FD_M2_SINE_CONTINUOUS';
 if(m2Adaptive)runupFlag+='\n#define TARGET_FD_M2_SINE_ADAPTIVE';
 if(m2OpenloopOnly)runupFlag+='\n#define TARGET_FD_M2_SINE_RUNUP_COAST_ONLY';
 if(m2Hold200)runupFlag+='\n#define FD6288_M2_SINE_ALIGN_HOLD_US 200000u';
 if(m2Rpm600)runupFlag+='\n#define FD6288_M2_SINE_RUNUP_RPM 600u';
 if(m2Rpm700)runupFlag+='\n#define FD6288_M2_SINE_RUNUP_RPM 700u';
 program=program.replace('#define TARGET_FD_M2_SINE_COAST','#define TARGET_FD_M2_SINE_COAST\n#define TARGET_FD_M2_SINE_SIXSTEP'+runupFlag)
  .replace('void commutate(void){assert(!bridge);assert(ccr[0]==ccr[1]&&ccr[1]==ccr[2]);assert(ccr[3]);step=step==6?1:step+1;commutations++;bridge=2;}',
   'void commutate(void){assert(ccr[0]==ccr[1]&&ccr[1]==ccr[2]);assert(ccr[3]);step=forward?(step==6?1:step+1):(step==1?6:step-1);commutations++;bridge=2;}')
  .replace('#define SET_INTERVAL_TIMER_COUNT(x) ((void)(x))','static unsigned interval;\n#define INTERVAL_TIMER_COUNT interval\n#define SET_INTERVAL_TIMER_COUNT(x) (interval=(x))')
  .replace('static unsigned adc_rearmed, adc_cleared, adc_fresh;',
   'static unsigned adc_rearmed, adc_cleared, adc_fresh, seed_period, accepted, adc_reads, adc_collision;\nstatic uint16_t adc_phase[3],thiszctime,lastzctime;\nstatic uint16_t fdMockRankRead(unsigned rank){adc_reads++;if(adc_collision&&rank==3u)adc_fresh=1u;return adc_phase[rank-1u];}')
  .replace('((rank)*100u)','adc_phase[(rank)-1]')
  .replace('static void adcZcdRearmInjected(void){adc_rearmed++;}',`static void adcZcdRearmInjected(void){adc_rearmed++;}
static void adcZcdSeedHandoff(uint32_t period){seed_period=period;}
static uint8_t getCompOutputSample(uint8_t *level){*level=0;LL_ADC_ClearFlag_JEOS(ADC1);return accepted;}`);
 if(m2Adaptive)program=program.replace('void commutate(void){assert(ccr[0]==ccr[1]&&ccr[1]==ccr[2]);assert(ccr[3]);step=forward?',
  'static void fdMockClosedSample(void);\nvoid commutate(void){assert(ccr[0]==ccr[1]&&ccr[1]==ccr[2]);assert(ccr[3]);step=forward?')
  .replace('commutations++;bridge=2;}','commutations++;bridge=2;fdMockClosedSample();}');
 const i=program.lastIndexOf('int main(void){');
 program=program.slice(0,i)+`
static const FdM2SineCoastSample replay[]={
${replay}
};

static void inject(void)
{
 adc_fresh=1;
 fdAdcJeosService();
}

#ifdef TARGET_FD_M2_SINE_RUNUP
/* Use the archived shape at another speed, resampled at the ADC carrier. */
static void replayCoastScaled(unsigned percent)
{
 unsigned i=0;
 for(uint32_t at=3600u; !fd_m2_sine_pickup_ready; at+=6666u){
  uint32_t archived_at=at*percent/100u;
  while(i+1u<sizeof(replay)/sizeof(replay[0]) && archived_at>=replay[i+1u].cycles) i++;
  if(i+1u==sizeof(replay)/sizeof(replay[0])) break;
  if(archived_at<replay[i].cycles) continue;
  const FdM2SineCoastSample a=replay[i], b=replay[i+1u];
  const int32_t age=archived_at-a.cycles, gap=b.cycles-a.cycles;
  adc_phase[0]=a.a+((int32_t)b.a-a.a)*age/gap;
  adc_phase[1]=a.b+((int32_t)b.b-a.b)*age/gap;
  adc_phase[2]=a.c+((int32_t)b.c-a.c)*age/gap;
  DWT->CYCCNT=fd_m2_sine_run.coast_at+at;
  inject();
 }
}

static void replayCoastAtPeriod(unsigned period_us)
{
 for(uint32_t at=3600u; at<40000u*160u && !fd_m2_sine_pickup_ready; at+=6666u){
  unsigned angle=(at*60u/(period_us*160u))%360u;
  adc_phase[0]=pwmSin[angle];
  adc_phase[1]=pwmSin[(angle+119u)%360u];
  adc_phase[2]=pwmSin[(angle+239u)%360u];
  DWT->CYCCNT=fd_m2_sine_run.coast_at+at;
  inject();
 }
}

static void checkRecordedEarlyEdges(void)
{
 const uint32_t period=${earlyCapture?Math.round(earlyCapture.handoff.coast_period_us*160):0}u;
 const uint32_t events[][2]={${earlyReplay||'{0u,0u}'}};
 const uint32_t shift=period*3u/8u;
 FdM2SineLock original={.period=period}, advanced={.period=period};
 for(unsigned i=0;i<4u;i++){
  uint32_t boundary=1000000u+events[i][0], edge=1000000u+events[i][1];
  original.sector_at=boundary;
  original.due=boundary+original.period;
  original.observed=0u;
  assert(!fdM2SineLockObserve(&original,edge));
  advanced.sector_at=boundary-shift;
  advanced.due=advanced.sector_at+advanced.period;
  advanced.observed=0u;
  assert(fdM2SineLockObserve(&advanced,edge));
 }
 assert(original.streak==0u&&advanced.streak==4u);
 assert(!fdM2SineLockReady(&advanced,advanced.due));
 puts("PASS recorded early events enter the phase window after advancing the timed boundaries");
}
#else
static void replayCoast(void)
{
 for(unsigned i=0;i<sizeof(replay)/sizeof(replay[0]);i++){
  adc_phase[0]=replay[i].a;adc_phase[1]=replay[i].b;adc_phase[2]=replay[i].c;
  DWT->CYCCNT=fd_m2_sine_run.coast_at+replay[i].cycles;
  inject();
  if(replay[i].cycles>=160000u && !fd_m2_sine_pickup_ready){
   DWT->CYCCNT+=6666u;
   inject(); /* archived decimated samples stand in for the missing carrier */
  }
  if(fd_m2_sine_pickup_ready)break;
 }
}
#endif

static void startCoast(void)
{
 begin(100);nativeStep();
 uint32_t start=DWT->CYCCNT;
#ifdef TARGET_FD_M2_SINE_RUNUP
 while(!fd_m2_sine_run.coast && DWT->CYCCNT-start<
       (1900000u+FD6288_M2_SINE_ALIGN_HOLD_US+FD_M2_SINE_RUNUP_EXTRA_US)*160u){
  uint32_t before=fd_m2_sine_run.rate;
  drive(42);
  assert(fd_m2_sine_run.rate>=before&&fd_m2_sine_run.rate-before<=24u);
  if(fd_m2_sine_run.runup)
   assert(fd_m2_sine_run.platform_travel==FD_M2_SINE_PLATFORM_TRAVEL*FD_M2_SINE_DEGREE);
 }
 assert(fd_m2_sine_coast.rate==FD_M2_SINE_RUNUP_RATE&&
        fd_m2_sine_run.runup_hold_us>=FD_M2_SINE_RUNUP_HOLD_US);
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
 assert(fd_m2_sine_coast.off_at>(1750000u+FD6288_M2_SINE_ALIGN_HOLD_US+FD_M2_SINE_RUNUP_EXTRA_US)*160u&&
        fd_m2_sine_coast.off_at<(1830000u+FD6288_M2_SINE_ALIGN_HOLD_US+FD_M2_SINE_RUNUP_EXTRA_US)*160u);
#else
 assert(fd_m2_sine_coast.off_at>1500000u*160u&&fd_m2_sine_coast.off_at<1600000u*160u);
#endif
#else
 while(!fd_m2_sine_run.coast && DWT->CYCCNT-start<1400000u*160u)drive(42);
#endif
 assert(fd_m2_sine_run.coast && !bridge && !fd_m2_sine_timed);
}

static void reachPlatform(void)
{
 startCoast();
#ifdef TARGET_FD_M2_SINE_RUNUP
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
 replayCoastScaled(100u);
#else
 replayCoastScaled(126u);
#endif
#else
 replayCoast();
#endif
}

static void finishSector(uint8_t edge, unsigned fraction)
{
 if(edge){
  DWT->CYCCNT=fd_m2_sine_lock.sector_at+fd_m2_sine_lock.period*fraction/8u;
#ifdef TARGET_FD_M2_SINE_STEP5_ADVANCE
  FdM2SineLock expected=fd_m2_sine_lock;
  uint8_t qualified=fdM2SineLockObserve(&expected,DWT->CYCCNT);
  uint32_t due=expected.due;
  if(qualified&&forward&&step==4u&&
     (int32_t)(due-DWT->CYCCNT)>(int32_t)((350u+250u)*160u))
   due-=350u*160u;
#endif
  accepted=1;inject();accepted=0;
#ifdef TARGET_FD_M2_SINE_STEP5_ADVANCE
  assert(fd_m2_sine_lock.due==due&&fd_m2_sine_lock.streak==expected.streak);
  if(qualified&&step==4u){
   accepted=1;inject();accepted=0;
   assert(fd_m2_sine_lock.due==due); /* one advance per sector */
  }
#endif
 }
 DWT->CYCCNT=fd_m2_sine_lock.due;
 fdBenchService();nativeStep();
#ifdef TARGET_FD_M2_SINE_STEP4_LATE_ADC
 if(fd_m2_sine_timed){
  assert(TIM1->CCR1==1334u&&TIM1->CCR2==1334u&&TIM1->CCR3==1334u);
  assert(TIM1->CCR4==(forward&&step==4u?960u:667u));
 }else if(running){
  assert(TIM1->CCR4==667u);
 }
#endif
}

static void startAtPredictedBoundary(void)
{
 assert(fd_m2_sine_pickup_ready&&!bridge);
 unsigned next=fdM2SineNextSector(fd_m2_sine_pickup.stable,forward);
 uint32_t period=fd_m2_sine_pickup.period;
#ifdef TARGET_FD_M2_SINE_ADAPTIVE
 uint32_t due=fd_m2_sine_pickup.stable_at+period-period/8u;
#elif defined(TARGET_FD_M2_SINE_RUNUP)
 uint32_t due=fd_m2_sine_pickup.stable_at+period*3u/8u;
#else
 uint32_t due=fd_m2_sine_pickup.stable_at+period-period/4u;
#endif
 assert((int32_t)(due-DWT->CYCCNT)>0);
 DWT->CYCCNT=due-1u;
 nativeStep();assert(!bridge);
 DWT->CYCCNT=due;
 nativeStep();assert(fd_m2_sine_timed&&bridge==2&&step==next);
}

int main(void)
{
 settingsOverride();eepromBuffer.limits.temperature=120;min_startup_duty=260;
 fdNativeSineClockInit();
 DWT->CYCCNT=0xffff0000u;
 forward=0;reachPlatform();assert(!fd_m2_sine_pickup_ready && !commutations);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;
 fdBenchService();assert(!bridge&&fd_native_sine.reason==FD_BENCH_NO_LOCK);

#ifdef TARGET_FD_M2_SINE_RUNUP
 checkRecordedEarlyEdges();
 forward=1;
 startCoast();replayCoastAtPeriod(2300u);
 assert(!fd_m2_sine_pickup_ready&&!bridge&&fd_m2_sine_handoff.coast_period>FD_M2_SINE_RUNUP_MAX_SECTOR);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;
 fdBenchService();assert(!bridge&&fd_native_sine.reason==FD_BENCH_NO_LOCK);
 startCoast();replayCoastAtPeriod(1700u);
 assert(!fd_m2_sine_pickup_ready&&!bridge&&fd_m2_sine_handoff.coast_period &&
        fd_m2_sine_handoff.coast_period<FD_M2_SINE_RUNUP_MIN_SECTOR);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;
 fdBenchService();assert(!bridge&&fd_native_sine.reason==FD_BENCH_NO_LOCK);
 begin(100);nativeStep();
 uint32_t start=DWT->CYCCNT;
 while(!fd_m2_sine_run.runup&&DWT->CYCCNT-start<1400000u*160u)drive(42);
 assert(fd_m2_sine_run.runup&&bridge);
 input=FD_M2_SINE_EXIT_INPUT-1u;drive(42);
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_OPERATOR_ZERO);
 startCoast();
 input=FD_M2_SINE_EXIT_INPUT-1u;fdBenchService();
 assert(!bridge&&!fd_m2_sine_run.coast&&fd_native_sine.reason==FD_BENCH_OPERATOR_ZERO);
 reachPlatform();assert(fd_m2_sine_pickup_ready);
 actual_current=400;nativeStep();actual_current=0;
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP);
 puts("PASS M2 run-up slew and hold, coast speed band, and throttle drop");
#ifdef TARGET_FD_M2_SINE_STEP5_ADVANCE
 /* Trial 1 after step 4. Values are DWT cycles on a relative timeline. */
 FdM2SineLock fifth={.period=304791u,.last_edge=82138u,
     .last_period=246811u,.sector_at=285153u,.due=589944u,.streak=4u};
 uint32_t fifth_edge=321821u;
 assert(!fdM2SineLockObserve(&fifth,fifth_edge));
 fifth.sector_at-=350u*160u;
 assert(fdM2SineLockObserve(&fifth,fifth_edge)&&fifth.streak==5u);
 assert(!fdM2SineLockReady(&fifth,fifth.due));
 puts("PASS recorded step-5 early event qualifies after a 350 us boundary advance");
#endif
#endif

 forward=1;reachPlatform();startAtPredictedBoundary();
 assert(fd_m2_sine_timed&&stepper_sine&&!running&&bridge==2);
#ifdef TARGET_FD_M2_SINE_RUNUP
 assert(fd_m2_sine_pickup.period>=FD_M2_SINE_RUNUP_MIN_SECTOR&&fd_m2_sine_pickup.period<=FD_M2_SINE_RUNUP_MAX_SECTOR);
#else
 assert(fd_m2_sine_pickup.period>=2100u*160u&&fd_m2_sine_pickup.period<=2800u*160u);
#endif
 assert(TIM1->CCR1==1334);
#ifdef TARGET_FD_M2_SINE_STEP4_LATE_ADC
 assert(TIM1->CCR4==(forward&&step==4u?960u:667u));
#else
 assert(TIM1->CCR4==667);
#endif
 finishSector(1,1); /* early crossing cannot move the timed boundary or earn lock */
 assert(!fd_m2_sine_lock.streak);
 assert(fd_m2_sine_powered.counts[0]==1u&&fd_m2_sine_powered.samples[0][0].flags==1u);
 for(unsigned n=0;n<5u;n++)finishSector(1,4);
 assert(fd_m2_sine_timed&&!running&&fd_m2_sine_lock.streak==5u);
 finishSector(1,4);
 assert(!fd_m2_sine_timed&&running&&!stepper_sine&&zero_crosses==6);
 assert(seed_period&&commutation_interval==seed_period&&interval>=seed_period/5);
 assert(fd_m2_sine_handoff.sectors==7&&fd_m2_sine_handoff.closed_at!=UINT32_MAX);
 assert(fd_native_sine.edges==0);
 zero();

 reachPlatform();startAtPredictedBoundary();
 for(unsigned n=0;n<12u;n++)finishSector(0,0);
 assert(!running&&!bridge&&fd_native_sine.reason==FD_BENCH_NO_LOCK);
 assert(fd_m2_sine_handoff.sectors==12&&fd_m2_sine_handoff.closed_at==UINT32_MAX);
 reachPlatform();startAtPredictedBoundary();
 actual_current=400;fdBenchService();
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP);
#if defined(TARGET_FD_M2_SINE_RUNUP) && defined(TARGET_FD_M2_SINE_STEP4_LATE_ADC)
 actual_current=0;
 reachPlatform();startAtPredictedBoundary();
 for(unsigned n=0;step!=4u&&n<6u;n++)finishSector(0,0);
 assert(step==4u&&fd_m2_sine_timed&&TIM1->CCR4==960u);
 unsigned i=fd_m2_sine_handoff.sectors-1u;
 for(unsigned n=0;n<3u;n++){
  DWT->CYCCNT=fd_m2_sine_lock.sector_at+2000u+n*6666u;inject();
  assert((fd_m2_sine_powered.samples[i][n].flags>>3)==(n<2u?0u:960u));
 }
 finishSector(0,0);
 assert(step==5u&&fd_m2_sine_timed&&TIM1->CCR4==667u);
 for(unsigned n=0;n<6u;n++)finishSector(1,4);
 assert(!fd_m2_sine_timed&&running&&step==5u&&TIM1->CCR4==667u);
 zero();
 reachPlatform();startAtPredictedBoundary();
 for(unsigned n=0;step!=3u&&n<6u;n++)finishSector(0,0);
 assert(step==3u&&fd_m2_sine_timed);
 TIM1->CCR1=960u+ZCD_ADC_WINDOW_TICKS-1u;
 finishSector(0,0);
 assert(!bridge&&!fd_m2_sine_timed&&fd_native_sine.reason==FD_BENCH_BAD_CONFIG);
 puts("PASS step-4 late aperture, boundary sample labels, restoration on exit and closed loop, and ON-window rejection");
#endif
 puts("PASS M2 archived phase playback, opposite direction rejection, timed six-step, six-edge handoff, missed-feedback and current stops");
 return 0;
}
`;
}

if(m2Adaptive) {
 program=program.slice(0,program.lastIndexOf('int main(void)'))+`
static void checkCyclePickup(void)
{
 for(unsigned direction=0;direction<2u;direction++){
  FdM2SinePickup valid={0},opposite={0};
  uint8_t ready=0,wrong_ready=0;
  uint32_t origin=0xffff0000u;
  for(uint32_t us=0;us<30000u&&!ready;us+=42u){
   unsigned angle=(us%10000u)*360u/10000u;
   if(!direction)angle=(360u-angle)%360u;
   uint16_t phase[3]={pwmSin[angle],pwmSin[(angle+119u)%360u],pwmSin[(angle+239u)%360u]};
   ready=fdM2SinePickupSample(&valid,origin+us*160u,phase,10000u*160u/6u,direction);
   wrong_ready|=fdM2SinePickupSample(&opposite,origin+us*160u,phase,10000u*160u/6u,!direction);
  }
  assert(ready&&!wrong_ready&&valid.cycle_mask==7u);
  assert(valid.period>1640u*160u&&valid.period<1695u*160u);
  assert(valid.stable==(direction?(valid.last_marker==0u?6u:valid.last_marker==1u?2u:4u):
                                 (valid.last_marker==0u?5u:valid.last_marker==1u?1u:3u)));
  uint16_t flat[3]={100,100,100};
  assert(!fdM2SinePickupSample(&valid,valid.previous_at+1000u*160u,flat,10000u*160u/6u,direction));
  assert(!valid.cycle_mask&&!valid.have_marker);
 }
 FdM2SinePickup quiet={0};
 uint16_t flat[3]={100,100,100};
 for(uint32_t us=0;us<40000u;us+=42u)
  assert(!fdM2SinePickupSample(&quiet,us*160u,flat,10000u*160u/6u,1));
 assert(!quiet.cycle_mask&&!quiet.period);
 puts("PASS full-turn pickup direction, interpolated phase mapping, clock wrap, stale samples and flat signal");
}

static void adaptiveSample(uint32_t age, uint8_t before, uint8_t edge)
{
 static const uint8_t high[6]={0,2,2,1,1,0};
 static const uint8_t low[6]={1,1,0,0,2,2};
 static const uint8_t floating[6]={2,0,1,2,0,1};
 unsigned sector=step-1u;
 rising=step%2u;
 adc_phase[high[sector]]=2000u;
 adc_phase[low[sector]]=10u;
 int32_t displacement=before?-60:60;
 if(!rising)displacement=-displacement;
 adc_phase[floating[sector]]=1005+displacement;
 DWT->CYCCNT=fd_m2_sine_lock.sector_at+age;
 accepted=edge;inject();accepted=0;
}

static void adaptiveBoundary(void)
{
 DWT->CYCCNT=fd_m2_sine_lock.due;
 fdBenchService();nativeStep();
 if(fd_m2_sine_timed){
  assert(bridge==2&&TIM1->CCR1==1334u&&TIM1->CCR2==1334u&&TIM1->CCR3==1334u);
  uint16_t expected=(fd_m2_sine_sampling.late_mask&(1u<<(step-1u)))?960u:667u;
  assert(TIM1->CCR4==expected);
 }else if(running){
  assert(TIM1->CCR4==((fd_m2_sine_closed_late&(1u<<(step-1u)))?960u:667u));
 }
}

static void checkTorqueRequest(void)
{
 unsigned previous=0u;
 for(unsigned value=137u;value<=2047u;value++){
  input=value;
 ${block(main,'if (eepromBuffer.use_sine_start) {\n                duty_cycle_setpoint')}
 ${main.slice(main.indexOf('/* FD_M2_SINE_TORQUE_BEGIN */'),main.indexOf('/* FD_M2_SINE_TORQUE_END */'))}
#ifdef TARGET_FD_M2_SINE_CONTINUOUS
  assert(duty_cycle_setpoint>=previous);
  if(value==137u)assert(duty_cycle_setpoint==minimum_duty_cycle+40u);
  if(value==2047u)assert(duty_cycle_setpoint==2000u);
#else
  assert(duty_cycle_setpoint==400u);
#endif
  previous=duty_cycle_setpoint;
 }
 input=200u;
 puts("PASS actual throttle map over all inputs: continuous follows throttle, limited bench keeps fixed torque");
}

static void adaptiveMidpoint(void)
{
 adaptiveSample(120u*160u,1,0);
 adaptiveSample(165u*160u,1,0);
 adaptiveSample(fd_m2_sine_lock.period/2u,0,1);
 assert(fd_m2_sine_lock.observed&&fd_m2_sine_sampling.front_samples>=2u);
 adaptiveBoundary();
}

static void startAdaptiveDrive(void)
{
 reachPlatform();startAtPredictedBoundary();
 assert(fd_m2_sine_timed&&bridge==2);
 assert(fd_m2_sine_handoff.abi==2u&&fd_m2_sine_powered.abi==2u);
 assert(fd_m2_sine_pickup.period>=FD_M2_SINE_RUNUP_MIN_SECTOR);
 assert(fd_m2_sine_pickup.period<=FD_M2_SINE_RUNUP_MAX_SECTOR);
 assert(fd_m2_sine_pickup.period>=1600u*160u&&fd_m2_sine_pickup.period<=1680u*160u);
 assert(fd_m2_sine_pickup.stable==4u&&step==5u);
 uint32_t marker=fd_m2_sine_pickup.stable_at-fd_m2_sine_run.coast_at;
 assert(marker>18800u*160u&&marker<19000u*160u);
 uint32_t start=fd_m2_sine_handoff.pickup_at-fd_m2_sine_coast.off_at;
 assert(start>20100u*160u&&start<20600u*160u);
}

int main(void)
{
 settingsOverride();eepromBuffer.limits.temperature=120;min_startup_duty=260;
 checkCyclePickup();
 fdNativeSineClockInit();DWT->CYCCNT=0xffff0000u;
 forward=0;reachPlatform();assert(!fd_m2_sine_pickup_ready&&!commutations);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;
 fdBenchService();assert(fd_native_sine.reason==FD_BENCH_NO_LOCK&&!bridge);
 forward=1;
 startCoast();assert(fd_m2_sine_coast.rate==500u*72u);
 replayCoastAtPeriod(2000u);
 assert(!fd_m2_sine_pickup_ready&&!bridge);
 assert(fd_m2_sine_handoff.coast_period>FD_M2_SINE_RUNUP_MAX_SECTOR);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;
 fdBenchService();assert(fd_native_sine.reason==FD_BENCH_NO_LOCK);
 startCoast();replayCoastAtPeriod(1450u);
 assert(!fd_m2_sine_pickup_ready&&!bridge);
 assert(fd_m2_sine_handoff.coast_period&&fd_m2_sine_handoff.coast_period<FD_M2_SINE_RUNUP_MIN_SECTOR);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;
 fdBenchService();assert(fd_native_sine.reason==FD_BENCH_NO_LOCK);
 puts("PASS 500 RPM command platform, archived coast pickup, reverse and measured-speed rejection");

 startAdaptiveDrive();
 /* Exercise closed-loop restoration after every phase has learned late sampling. */
 printf("Archived full-turn pickup: sector %u, period %u cycles, phase marker %u us\\n",
  step,fd_m2_sine_pickup.period,
  (fd_m2_sine_pickup.stable_at-fd_m2_sine_run.coast_at)/160u);
 fd_m2_sine_sampling.late_mask=0x3fu;
 TIM1->CCR4=960u;
 uint32_t coast_period=fd_m2_sine_handoff.coast_period;
 for(unsigned n=0;n<6u;n++){
  adaptiveMidpoint();
  assert(fd_m2_sine_timed&&!running);
 }
 assert(!fdM2SineAdaptiveCyclePeriod(&fd_m2_sine_lock));
 for(unsigned n=0;n<2u;n++){
  adaptiveMidpoint();
  assert(fd_m2_sine_timed&&!running);
  assert(!fdM2SineAdaptiveCyclePeriod(&fd_m2_sine_lock));
 }
 adaptiveMidpoint();
 assert(!fd_m2_sine_timed&&running&&!stepper_sine&&zero_crosses>=6u);
 assert(fd_m2_sine_handoff.sectors==9u&&fd_m2_sine_handoff.closed_at!=UINT32_MAX);
 assert(fd_m2_sine_handoff.closed_period>=coast_period-coast_period/32u);
 assert(fd_m2_sine_handoff.closed_period<=coast_period+coast_period/32u);
 assert(seed_period==commutation_interval&&seed_period==fd_m2_sine_handoff.closed_period/80u);
 assert(TIM1->CCR4==960u&&fd_m2_sine_closed_late==0x3fu);
 checkTorqueRequest();
#ifdef TARGET_FD_M2_SINE_CONTINUOUS
 fd_native_sine.edges=6u;fd_native_sine.sixth_edge_at=DWT->CYCCNT-fd_native_sine.started;
 for(unsigned i=0;i<80u;i++){
  DWT->CYCCNT+=1000000u*160u;fdBenchService();
  assert(running&&bridge&&!fd_power_bench.inhibit&&!fdBenchDeadlineExpired());
 }
 actual_current=404;fdBenchService();assert(running&&bridge&&!fd_power_bench.inhibit);
 actual_current=1200;fdBenchService();assert(running&&bridge&&!fd_power_bench.inhibit);
 use_current_limit=1u;eepromBuffer.limits.current=10u;
 actual_current=1999;fdBenchService();assert(running&&bridge&&!fd_power_bench.inhibit);
 actual_current=2000;fdBenchService();actual_current=0;use_current_limit=0u;
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP);
 puts("PASS continuous deadline logic across 80 seconds and DWT wraps; no extra 4 A trip, configured current threshold retained");
 startAdaptiveDrive();
 for(unsigned n=0;n<9u;n++)adaptiveMidpoint();
 zero();assert(!bridge&&fd_native_sine.reason==FD_BENCH_OPERATOR_ZERO);
 puts("PASS continuous mode operator-zero stop");
 return 0;
#endif

 assert(fd_m2_sine_closed.count==1u&&fd_m2_sine_closed.events[0].kind==0u);
 for(unsigned sector=1u;sector<=6u;sector++){
  step=sector;assert(zcd_ccr4_from_duty(1334u)==960u);
  assert(zcd_ccr4_from_duty(2000u)==1000u); /* Normal aperture overtakes the learned floor. */
  assert(zcd_ccr4_from_duty(1000u)==500u); /* A short ON pulse never takes an unsafe late point. */
  assert(!zcd_ccr4_from_duty(703u));
 }
 for(unsigned i=0;i<40u;i++){
  DWT->CYCCNT+=100u*160u;fdNativeSineEdge();
 }
 assert(fd_m2_sine_closed.count==41u);
 zero();
 assert(!fd_m2_sine_closed_late);
 assert(fd_m2_sine_closed.count==42u&&fd_m2_sine_closed.events[9].kind==2u);
 assert(fd_m2_sine_closed.events[9].trigger==960u);
 puts("PASS repeated full-turn period and three matching phase estimates before real entry-point handoff");

 startAdaptiveDrive();
 for(unsigned n=0;n<6u;n++)adaptiveMidpoint();
 adaptiveBoundary(); /* A missing sector breaks the consecutive run. */
 assert(fd_m2_sine_lock.streak==0u);
 for(unsigned n=0;n<3u;n++)adaptiveMidpoint();
 assert(fd_m2_sine_timed&&!running&&fd_m2_sine_lock.streak==3u);
 assert(fdM2SineAdaptiveCyclePeriod(&fd_m2_sine_lock));
 for(unsigned n=0;n<3u;n++)adaptiveMidpoint();
 assert(fd_m2_sine_timed&&!running&&fd_m2_sine_lock.streak==6u);
 adaptiveMidpoint(); /* Supply the remaining adjacent gap after the miss. */
 assert(!fd_m2_sine_timed&&running&&fd_m2_sine_lock.streak==7u);
 zero();
 puts("PASS full-turn estimates alone cannot hand off after a missed sector or an incomplete adjacent-gap set");

 startAdaptiveDrive();
 unsigned record=fd_m2_sine_handoff.sectors-1u;
 adaptiveSample(120u*160u,1,0);
 assert(fd_m2_sine_sampling.front_samples==1u);
 adaptiveSample(179u*160u,0,1);
 assert(!fd_m2_sine_lock.observed&&!fd_m2_sine_lock.streak);
 assert(TIM1->CCR4==960u&&fd_m2_sine_sampling.retried);
 assert(fd_m2_sine_sampling.front_samples==0u);
 assert(fd_m2_sine_adaptation.sector[record].decision==3u);
 adaptiveSample(220u*160u,0,1); /* Stale aperture cannot latch a detector edge. */
 adaptiveSample(261u*160u,1,0);
 assert(fd_m2_sine_sampling.front_samples==0u);
 assert((fd_m2_sine_powered.samples[record][2].flags>>3)==0u);
 assert((fd_m2_sine_powered.samples[record][3].flags>>3)==0u);
 adaptiveSample(279u*160u,1,0);
 adaptiveSample(324u*160u,1,0);
 assert(fd_m2_sine_sampling.front_samples==2u);
 assert((fd_m2_sine_powered.samples[record][4].flags>>3)==960u);
 adaptiveSample(fd_m2_sine_lock.period/2u,0,1);
 assert(fd_m2_sine_lock.observed&&fd_m2_sine_lock.streak==1u);
 adaptiveBoundary();
 for(unsigned n=0;step!=4u&&n<6u;n++)adaptiveMidpoint();
 assert(fd_m2_sine_timed&&step==4u&&TIM1->CCR4==960u);
 adaptiveBoundary();
 assert(step==5u&&TIM1->CCR4==((fd_m2_sine_sampling.late_mask&(1u<<4u))?960u:667u));
 zero();
 puts("PASS single-front early event retries late ADC, settling samples excluded, front requalified, aperture restored");

 startAdaptiveDrive();
 assert(TIM1->CCR4==667u);
 uint32_t previous_due=fd_m2_sine_lock.due;
 uint32_t previous_period=fd_m2_sine_lock.period;
 adaptiveSample(120u*160u,1,0);
 adaptiveSample(165u*160u,1,0);
 adaptiveSample(fd_m2_sine_lock.period*3u/16u,0,1);
 assert(fd_m2_sine_lock.observed&&fd_m2_sine_lock.streak==1u);
 assert(!fd_m2_sine_sampling.retried&&fd_m2_sine_lock.period==previous_period);
 assert(fd_m2_sine_lock.due==previous_due-previous_period/4u);
 assert(fd_m2_sine_lock.nominal_due==fd_m2_sine_lock.due);
 adaptiveBoundary();
 assert(fd_m2_sine_lock.sector_at==previous_due-previous_period/4u);
 zero();
 puts("PASS trusted standard-aperture early event advances the immediate boundary without resetting acquisition or inventing speed");

 startAdaptiveDrive();
 uint32_t original_off=fd_m2_sine_coast.off_at,original_count=fd_m2_sine_coast.count;
 uint16_t original_a=fd_m2_sine_coast.samples[0].a;
 adaptiveBoundary();assert(fd_m2_sine_timed&&bridge);
 adaptiveBoundary();
 assert(!bridge&&!running&&!fd_m2_sine_timed&&fd_m2_sine_run.coast);
 assert(fd_m2_sine_recovery.attempts==1u&&fd_m2_sine_handoff.sectors==2u);
 assert(TIM1->CCR1==0u&&TIM1->CCR2==0u&&TIM1->CCR3==0u&&TIM1->CCR4==3333u);
 assert(fd_m2_sine_recovery.first_sector==2u&&fd_m2_sine_recovery.resumed_at==UINT32_MAX);
 replayCoastScaled(100u);assert(fd_m2_sine_pickup_ready&&!bridge);
 assert(fd_m2_sine_recovery.count&&fd_m2_sine_recovery.measured_period);
 startAtPredictedBoundary();
 assert(fd_m2_sine_recovery.resumed_at!=UINT32_MAX&&fd_m2_sine_handoff.sectors==3u);
 assert(fd_m2_sine_coast.off_at==original_off&&fd_m2_sine_coast.count==original_count&&
        fd_m2_sine_coast.samples[0].a==original_a);
 for(unsigned n=0;n<9u;n++)adaptiveMidpoint();
 assert(running&&!fd_m2_sine_timed&&fd_m2_sine_handoff.closed_at!=UINT32_MAX);
 zero();
 puts("PASS two missing sectors enter high-Z, one full-turn reacquisition resumes drive and reaches handoff, first capture retained");

 startAdaptiveDrive();adaptiveBoundary();adaptiveBoundary();
 replayCoastScaled(100u);startAtPredictedBoundary();
 adaptiveBoundary();adaptiveBoundary();
 assert(!bridge&&!running&&fd_native_sine.reason==FD_BENCH_NO_LOCK);
 assert(fd_m2_sine_recovery.attempts==1u&&fd_m2_sine_handoff.sectors==4u);
 assert(fd_m2_sine_handoff.closed_at==UINT32_MAX);
 startAdaptiveDrive();adaptiveBoundary();adaptiveBoundary();
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;fdBenchService();
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_NO_LOCK);
 startAdaptiveDrive();adaptiveBoundary();adaptiveBoundary();zero();
 assert(!bridge&&!fd_m2_sine_run.coast&&!fd_m2_sine_timed);
 startAdaptiveDrive();actual_current=400;fdBenchService();actual_current=0;
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP);
 startAdaptiveDrive();zero();
 assert(!bridge&&!fd_m2_sine_timed&&!fd_m2_sine_run.coast);
 puts("PASS bounded recovery, no reacquisition timeout, current stop and operator zero during recovery");
 return 0;
}
`;
}

if(m2OpenloopOnly) {
 program=program.slice(0,program.lastIndexOf('int main(void)'))+`
int main(void)
{
 settingsOverride();eepromBuffer.limits.temperature=120;min_startup_duty=260;
 fdNativeSineClockInit();DWT->CYCCNT=0xffff0000u;forward=1;
#if FD6288_M2_SINE_ALIGN_HOLD_US
 begin(100);nativeStep();
 while(DWT->CYCCNT-fd_m2_sine_run.drive_at<160000u*160u)drive(42);
 assert(fd_m2_sine_run.angle==0u&&phase_A_position==180&&bridge);
 uint32_t held[3]={TIM1->CCR1,TIM1->CCR2,TIM1->CCR3};
 assert(fd_m2_sine_coast.max_span_ticks>=890u);
 while(DWT->CYCCNT-fd_m2_sine_run.drive_at<(FD_M2_SINE_MOTION_AT_US-42u)*160u){
  drive(42);
  assert(fd_m2_sine_run.angle==0u&&fd_m2_sine_run.slow_travel==0u);
  assert(TIM1->CCR1==held[0]&&TIM1->CCR2==held[1]&&TIM1->CCR3==held[2]);
 }
 while(DWT->CYCCNT-fd_m2_sine_run.drive_at<FD_M2_SINE_MOTION_AT_US*160u)drive(42);
 assert(fd_m2_sine_run.angle>0u&&fd_m2_sine_run.slow_travel>0u);
 zero();assert(!bridge);
 begin(100);nativeStep();
 while(DWT->CYCCNT-fd_m2_sine_run.drive_at<160000u*160u)drive(42);
 actual_current=400;drive(42);actual_current=0;
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP);
 puts("PASS full-amplitude fixed-angle hold to 350 ms, motion starts afterwards, current stop during hold");
#endif
 startCoast();
 assert(fd_m2_sine_coast.rate==FD6288_M2_SINE_RUNUP_RPM*72u);
 assert(!fd_m2_sine_pickup_requested&&!fd_m2_sine_timed&&!bridge);
 assert(TIM1->CCR1==0u&&TIM1->CCR2==0u&&TIM1->CCR3==0u&&TIM1->CCR4==3333u);
 for(unsigned n=0;n<960u;n++){
  DWT->CYCCNT=fd_m2_sine_run.coast_at+3600u+n*6666u;
  adc_phase[0]=100u;adc_phase[1]=300u;adc_phase[2]=200u;
  inject();nativeStep();
  assert(!bridge&&!running&&!commutations&&!fd_m2_sine_pickup_ready);
 }
 assert(fd_m2_sine_coast.count==492u);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;fdBenchService();
 assert(fd_native_sine.reason==FD_BENCH_HIZ_DONE&&!bridge);
 assert(fd_m2_sine_handoff.sectors==0u&&fd_m2_sine_handoff.pickup_at==UINT32_MAX);
 assert(fdBenchGuardInput());
 startCoast();zero();assert(!bridge&&!fd_m2_sine_run.coast);
 begin(100);nativeStep();actual_current=400;drive(42);
 assert(!bridge&&fd_native_sine.reason==FD_BENCH_SAFETY_CAP);
 printf("PASS %u RPM open-loop drive, full high-Z coast, zero six-step commutations, one-shot latch and stops\\n",
  FD6288_M2_SINE_RUNUP_RPM);
 return 0;
}
`;
}

if(m2Step4Coast) {
 program=program.replace('#define TARGET_FD_M2_SINE_SIXSTEP',
  '#define TARGET_FD_M2_SINE_SIXSTEP\n#define TARGET_FD_M2_SINE_STEP4_COAST\n#define FD6288_M2_SINE_STEP4_HOLD_US '+(m2Step4Extended?1600:500));
 if(m2Step4LateAdc)program=program.replace('#define TARGET_FD_M2_SINE_STEP4_COAST',`
#define TARGET_FD_M2_SINE_STEP4_COAST
#define TARGET_FD_M2_SINE_STEP4_LATE_ADC
#define FD6288_M2_SINE_STEP4_ADC_TICKS 960u`);
 program=program.slice(0,program.lastIndexOf('int main(void)'))+`
int main(void)
{
 settingsOverride();eepromBuffer.limits.temperature=120;min_startup_duty=260;
 fdNativeSineClockInit();forward=1;
 DWT->CYCCNT=0xffff0000u;
 reachPlatform();startAtPredictedBoundary();
 assert(step==3u);
 assert(TIM1->CCR4==667u);
 finishSector(1,4u);
 assert(step==4u&&fd_m2_sine_timed&&fd_m2_sine_handoff.sector[0].streak==1u);
 uint32_t sector_at=fd_m2_sine_lock.sector_at;
#ifdef TARGET_FD_M2_SINE_STEP4_LATE_ADC
 assert(TIM1->CCR1==1334u&&TIM1->CCR2==1334u&&TIM1->CCR3==1334u&&TIM1->CCR4==960u);
 DWT->CYCCNT=sector_at+2000u;inject();
 DWT->CYCCNT=sector_at+8666u;inject();
 DWT->CYCCNT=sector_at+15332u;inject();
 assert(fd_m2_sine_powered.counts[1]==3u);
 assert((fd_m2_sine_powered.samples[1][0].flags>>3)==0u);
 assert((fd_m2_sine_powered.samples[1][1].flags>>3)==0u);
 assert((fd_m2_sine_powered.samples[1][2].flags>>3)==960u);
#endif
 const uint32_t hold=FD6288_M2_SINE_STEP4_HOLD_US*160u;
#if FD6288_M2_SINE_STEP4_HOLD_US > 500
 for(unsigned n=3;n<=38u;n++){
  DWT->CYCCNT=sector_at+2000u+n*6666u;
  accepted=n==29u;inject();accepted=0;nativeStep();
  assert(step==4u&&bridge==2);
 }
 assert(fd_m2_sine_powered.counts[1]==24u);
 assert(fd_m2_sine_powered.samples[1][23].cycles>1500u*160u);
 assert(fd_m2_sine_handoff.sector[1].streak==2u);
 fd_m2_sine_lock.due=sector_at+hold-2u;
#endif
 DWT->CYCCNT=sector_at+hold-1u;nativeStep();
 assert(bridge==2&&fd_m2_sine_timed);
 DWT->CYCCNT++;nativeStep();
 assert(!bridge&&!fd_m2_sine_timed&&fd_m2_sine_run.coast&&TIM1->CCR4==3333u);
 assert(fd_m2_sine_step4_coast.off_at-fd_m2_sine_step4_coast.sector_at==hold);
 assert(fd_m2_sine_step4_coast.prior_edge==fd_m2_sine_handoff.sector[0].edge);
 unsigned comms=commutations, initial_count=fd_m2_sine_coast.count;
 for(unsigned n=0;n<960;n++){
  DWT->CYCCNT=fd_m2_sine_run.coast_at+n*6666u+3600u;
  inject();nativeStep();
  assert(!bridge&&commutations==comms);
 }
 assert(fd_m2_sine_coast.count==initial_count&&fd_m2_sine_step4_coast.count==492u);
 DWT->CYCCNT=fd_m2_sine_run.coast_at+40000u*160u;fdBenchService();
 assert(fd_native_sine.reason==FD_BENCH_HIZ_DONE&&!running&&!bridge);
 assert(fd_m2_sine_handoff.closed_at==UINT32_MAX);
 printf("PASS powered step 4 pulse, cutoff at %u us, 40ms independent coast capture, no restart\\n",FD6288_M2_SINE_STEP4_HOLD_US);
 return 0;
}
`;
}

if(m2RealAdc) {
 const adc=fs.readFileSync(path.join(root,'Mcu/g431/Src/comparator.c'),'utf8');
 const irq=block(main,'void fdAdcJeosService(void)');
 assert(irq.indexOf('zcfoundroutine();')<irq.indexOf('adcZcdRearmInjected();'));
 assert(irq.indexOf('adcZcdRearmInjected();')<irq.indexOf('fdM2SineAdcFinishIrq('));
 assert(!block(adc,'static inline void fdAdcCaptureSnapshot').includes('LL_ADC_'));
 const boundaryCapture=JSON.parse(fs.readFileSync(path.join(root,'docs/m2-startup/capture-m2-sine-500-heli-softstart-87pct-20261119.json'),'utf8'));
 const boundarySamples=boundaryCapture.adc_capture.samples.filter(r=>r.step===5&&r.at_ms>=1588&&r.at_ms<1588.26);
 assert.equal(boundarySamples.length,7);
 const boundaryReplay=boundarySamples.map(r=>`{${Math.round((r.at_ms-boundarySamples[0].at_ms)*160000)}u,${Math.round(r.elapsed_us*2)}u,${r.a}u,${r.b}u,${r.c}u}`).join(',\n');
 const stableCapture=JSON.parse(fs.readFileSync(path.join(root,'docs/m2-startup/capture-m2-sine-500-heli-boundary-89pct-20261119.json'),'utf8'));
 const stableSwitch=stableCapture.closed.events.find(r=>r.step===3&&r.kind===0&&r.at_ms>19040.9).at_ms;
 const stableSamples=stableCapture.adc_capture.samples.filter(r=>r.step===3&&r.at_ms>=stableSwitch&&r.at_ms<19041.07);
 assert.equal(stableSamples.length,4);
 const stableReplay=stableSamples.map(r=>`{${Math.round((r.at_ms-stableSwitch)*160000)}u,${Math.round(r.elapsed_us*2)}u,${r.a}u,${r.b}u,${r.c}u,${r.read_counter}u}`).join(',\n');
 const fifth=JSON.parse(fs.readFileSync(path.join(root,'docs/m2-startup/capture-m2-sine-500-cycle-pickup-20261118.json'),'utf8'))
  .handoff.sectors[4].samples.filter(s=>s.us<=326)
  .map(s=>`{${Math.round(s.us*160)}u,${s.a},${s.b},${s.c}}`).join(',\n');
 program=program.replace('#define TARGET_FD_NATIVE_SINE',`#define TARGET_FD_NATIVE_SINE
#define TARGET_ADC_ZCD_GUARD
#define TARGET_FD_ADC_JEOS
#define TARGET_FD_ASYNC_COMM
#define USE_ADC_ZCD
#define FD6288_STARTUP_TIMING_FIX 1
#define FD_DIAG(...) ((void)0)
#define FD_DECISION(...) ((void)0)
#define fdTraceNoFresh() ((void)0)
#define fdTraceEvent(...) ((void)0)
#define ADC_ZCD_BLANK_SAMPLES 1u
#define ADC_ZCD_HYSTERESIS_COUNTS 8
#define ADC_ZCD_MIN_DRIVE_DELTA 64`)
 .replace('static void adcZcdResetAcquisition(unsigned x){(void)x;}',
          'void adcZcdResetAcquisition(uint8_t reason);\nvoid changeCompInput(void);')
 .replace('static void adcZcdSeedHandoff(uint32_t period){seed_period=period;}',
          'void adcZcdSeedHandoff(uint32_t period);')
 .replace('static uint8_t getCompOutputSample(uint8_t *level){*level=0;LL_ADC_ClearFlag_JEOS(ADC1);return accepted;}',
          'uint8_t getCompOutputSample(uint8_t *level);')
 .replace('static void getBemfState(void){assert(0 && "sine trial cannot enter the BEMF controller");}',
          'void getBemfState(void);')
 .replace('static void zcfoundroutine(void){assert(0);}','void zcfoundroutine(void);')
 .replace('static void heliCoastRecordZeroCross(void){assert(0);}','static void heliCoastRecordZeroCross(void){}')
 .replace('adc_phase[(rank)-1]','fdMockRankRead(rank)')
 .replace('static void fdMockClosedSample(void){',
          'static void fdMockClosedSample(void){rising=forward?step%2u:!(step%2u);changeCompInput();bemfcounter=zcfound=0u;');
 const definitions=`
${m2AdcCapture?fs.readFileSync(path.join(root,'Inc/fd6288_startup_trace.h'),'utf8').match(/enum \{\n    FD_SAMPLE_INVALID[\s\S]*?\};/)[0]+'\nstatic uint8_t adc_zcd_decision;':''}
static uint8_t adc_zcd_phase,adc_zcd_blank,adc_zcd_last_out,adc_zcd_before_seen,adc_zcd_confirmations,adc_zcd_reported,adc_zcd_drive_crossings,adc_zcd_required_samples;
static uint32_t adc_zcd_earliest,adc_zcd_last_real_interval;
static unsigned polling_mode_changeover=2000u;
static unsigned waitTime,advance,temp_advance=8u,auto_advance_level=8u,bad_count,bad_count_threshold=6u,schedules,scheduled_ticks,scheduled_at;
static void enableCompInterrupts(void){old_routine=1u;}
static void fdAsyncSchedule(uint32_t at,uint32_t ticks){scheduled_at=at;schedules++;scheduled_ticks=ticks;}
${m2RunFix?'#include \"fd6288_m2_run.h\"\nstatic FdM2CrossingTime fd_m2_crossing_time;\n'+block(adc,'uint32_t adcZcdDelayCorrection'):''}
${m2StablePre?'static uint32_t fd_m2_switch_at,fd_m2_last_group_at;static uint8_t fd_m2_pre_allowed;':''}
${block(adc,'void adcZcdResetAcquisition(uint8_t reason)')}
${block(adc,'void adcZcdSeedHandoff(uint32_t period)')}
${block(adc,'uint32_t adcZcdDrivePeriod(uint32_t elapsed, uint32_t previous)')}
${block(adc,'static uint8_t adcZcdQualifyDrive(int32_t margin)')}
${m2AdcCapture?block(adc,'static inline void fdAdcCaptureSnapshot'):''}
${block(adc,'uint8_t getCompOutputSample(uint8_t *level)')}
${block(adc,'void changeCompInput(void)')}
${block(main,'void getBemfState()')}
${block(main,'void zcfoundroutine()')}
`;
 if(m2AdcCapture)program=program.replace('#define FD_DECISION(...) ((void)0)','#define FD_DECISION(reason) (adc_zcd_decision=(reason))');
 program=program.replace('static void inject(void)',definitions+'\nstatic void inject(void)');
 program=program.slice(0,program.lastIndexOf('int main(void)'))+`
static const FdM2SineCoastSample recordedFifth[]={
${fifth}
};
static void closedSample(uint32_t elapsed, uint8_t before)
{
 static const uint8_t high[6]={0,2,2,1,1,0},low[6]={1,1,0,0,2,2},floating[6]={2,0,1,2,0,1};
 unsigned sector=step-1u;
 adc_phase[high[sector]]=2000u;adc_phase[low[sector]]=10u;
 int32_t displacement=before?-60:60;
 if(!rising)displacement=-displacement;
 adc_phase[floating[sector]]=1005+displacement;
 DWT->CYCCNT+=(elapsed-interval)*80u;interval=elapsed;unsigned before_reads=adc_reads;inject();assert(adc_reads==before_reads+3u);
}
int main(void)
{
 settingsOverride();eepromBuffer.limits.temperature=120;min_startup_duty=260;
 fdNativeSineClockInit();DWT->CYCCNT=0xffff0000u;forward=1;
 startAdaptiveDrive();
 uint32_t old_due=fd_m2_sine_lock.due,old_period=fd_m2_sine_lock.period;
 for(unsigned i=0;i<sizeof(recordedFifth)/sizeof(recordedFifth[0]);i++){
  DWT->CYCCNT=fd_m2_sine_lock.sector_at+recordedFifth[i].cycles;
  adc_phase[0]=recordedFifth[i].a;adc_phase[1]=recordedFifth[i].b;adc_phase[2]=recordedFifth[i].c;inject();
 }
 assert(fd_m2_sine_lock.observed&&!fd_m2_sine_sampling.retried&&TIM1->CCR4==667u);
 assert(fd_m2_sine_lock.due==old_due-old_period/4u&&fd_m2_sine_lock.period==old_period);
 assert(fd_m2_sine_handoff.sector[0].edge!=UINT32_MAX);
 zero();
 puts("PASS archived step-5 raw ADC samples accept the 325 us crossing and advance the next boundary without a late-aperture reset");
 startAdaptiveDrive();
 for(unsigned i=0;i<9u;i++){
  adaptiveSample(120u*160u,1u,0u);
  adaptiveSample(165u*160u,1u,0u);
  adaptiveSample(210u*160u,1u,0u);
  adaptiveSample(fd_m2_sine_lock.period/2u-42u*160u,0u,0u);
  assert(!fd_m2_sine_lock.observed); /* First POST is not an event. */
  adaptiveSample(fd_m2_sine_lock.period/2u,0u,0u);
  assert(fd_m2_sine_lock.observed&&fd_m2_sine_lock.streak==i+1u);
  adaptiveBoundary();
 }
 assert(running&&!stepper_sine&&!fd_m2_sine_timed);
 assert(fd_m2_sine_closed_late==(1u<<3u)&&adc_zcd_drive_crossings==5u);
#if defined(TARGET_FD_M2_RUN_FIX) && !defined(TARGET_FD_M2_PURE)
 uint32_t save_now=DWT->CYCCNT;
 fdM2WireRecord(1250u,(2u<<9)|312u);fdM2WireRecord(1250u,(2u<<9)|312u);
 assert(fd_m2_wire.count==1u&&fd_m2_wire.samples[0].period==1250u);
 assert(fd_m2_wire.samples[0].payload==((2u<<9)|312u));
 DWT->CYCCNT=save_now;
 fd_m2_wire.count=fd_m2_wire.last_at=0u;
 puts("PASS wire logging decimates actual DShot period payload");
#endif

#ifdef TARGET_FD_M2_HELI_START
 assert(fd_m2_heli.active&&fd_m2_heli.healthy&&fd_m2_heli.output==400u);
 uint32_t heli_now=DWT->CYCCNT;FdM2HeliStart heli_save=fd_m2_heli;
 unsigned save_last=last_duty_cycle,save_duty=duty_cycle,native_ramp_calls=0u,ramp_count=0u;
 last_duty_cycle=400u;duty_cycle=1500u;
 ${main.slice(main.indexOf('/* FD_M2_HELI_OUTPUT_BEGIN */'),main.indexOf('/* FD_M2_HELI_OUTPUT_END */'))}
 {native_ramp_calls++;}
 assert(duty_cycle==400u&&!native_ramp_calls);
 duty_cycle=300u;
 ${main.slice(main.indexOf('/* FD_M2_HELI_OUTPUT_BEGIN */'),main.indexOf('/* FD_M2_HELI_OUTPUT_END */'))}
 {native_ramp_calls++;}
 assert(duty_cycle==300u&&!native_ramp_calls);
 fd_m2_heli=heli_save;DWT->CYCCNT=heli_now;last_duty_cycle=save_last;duty_cycle=save_duty;
 (void)ramp_count;
 puts("PASS actual handoff initializes heli feedback; output hook replaces native ramp, decreases immediately");
#endif
 assert(adc_zcd_blank==1u&&adc_zcd_earliest&&fd_native_sine.edges==0u);
 for(unsigned i=0;i<10u;i++){
  unsigned previous=schedules;
  uint32_t period=commutation_interval;
  closedSample(interval+84u,1u);closedSample(interval+84u,1u);closedSample(interval+84u,1u);
  assert(schedules==previous&&adc_zcd_before_seen);
  closedSample(period-84u,0u);assert(schedules==previous);
  fdAdcJeosService();assert(schedules==previous); /* No fresh group, no extra confirmation. */
  closedSample(period,0u);
  assert(schedules==previous+1u&&scheduled_ticks>period/5u&&scheduled_ticks<period/2u);
  assert(fd_native_sine.edges==i+1u&&interval==0u);
  DWT->CYCCNT+=scheduled_ticks*80u;interval=scheduled_ticks;
  uint16_t former_trigger=TIM1->CCR4;
  commutate();
  assert(TIM1->CCR4==(step==4u?960u:667u));
  assert(adc_zcd_blank==(former_trigger==TIM1->CCR4?1u:2u)&&adc_zcd_drive_crossings==5u);
 }
 assert(fd_native_sine.phase==3u&&fd_m2_sine_closed.count==21u);
 /* Compare the effective sampling aperture BEFORE writing its preload. */
 static const struct {uint16_t duty,old_point;uint8_t step,blank;uint16_t next;} aperture[]={
  {1334u,667u,4u,2u,960u},{1334u,960u,5u,2u,667u},
  {1334u,667u,2u,1u,667u},{3803u,1901u,4u,1u,1901u},
  {3803u,1901u,2u,1u,1901u}
 };
 for(unsigned i=0;i<sizeof(aperture)/sizeof(aperture[0]);i++){
  step=aperture[i].step;rising=step%2u;TIM1->CCR1=aperture[i].duty;TIM1->CCR4=aperture[i].old_point;
  changeCompInput();assert(adc_zcd_blank==aperture[i].blank);
  TIM1->CCR4=zcd_ccr4_from_duty(TIM1->CCR1);assert(TIM1->CCR4==aperture[i].next);
 }
 puts("PASS actual 667/960 aperture changes drain two groups; equal low/high midpoints drain one");
 /* A fast sector with exactly one PRE in its second carrier group. */
 for(unsigned old_blank=2u;old_blank>=1u;old_blank--){
  adcZcdResetAcquisition(ADC_ZCD_RESET_START);commutation_interval=580u;adcZcdSeedHandoff(580u);
  step=2u;rising=0u;TIM1->CCR1=TIM1->CCR2=TIM1->CCR3=3803u;TIM1->CCR4=1901u;
  changeCompInput();assert(adc_zcd_blank==1u&&adc_zcd_required_samples==1u);
  if(old_blank==2u)adc_zcd_blank=2u; /* Replay the former blanket rule. */
  interval=200u;bemfcounter=zcfound=0u;
  unsigned before_schedule=schedules;
  closedSample(250u,0u);closedSample(334u,1u);closedSample(418u,0u);
  assert(schedules==before_schedule+(old_blank==1u));
#ifdef TARGET_FD_M2_RUN_FIX
  if(old_blank==1u){
   assert(DWT->CYCCNT-scheduled_at>3000u&&DWT->CYCCNT-scheduled_at<4000u);
   assert(scheduled_at+scheduled_ticks*80u-DWT->CYCCNT>=1600u);
  }
#endif
#ifdef TARGET_FD_M2_SINE_ADC_CAPTURE
  unsigned tail=(fd_m2_sine_adc_capture.header.count-1u)&127u;
  assert(fd_m2_sine_adc_capture.samples[tail].elapsed==418u);
  assert(fd_m2_sine_adc_capture.samples[tail].period==580u);
  assert(((fd_m2_sine_adc_capture.samples[tail].flags>>4)&1u)==(old_blank==1u));
  unsigned before_count=fd_m2_sine_adc_capture.header.count,before_reads=adc_reads;
  fdAdcJeosService();
  assert(fd_m2_sine_adc_capture.header.count==before_count&&adc_reads==before_reads);
#endif
 }
 puts("PASS fast sector: second-group-only PRE is lost with blanket two-group blanking, accepted with actual-aperture rule");
#ifdef TARGET_FD_M2_RUN_FIX
 /* Actual trial2 step5 failure: invalid boundary, sole PRE, then POST. */
 static const struct {uint32_t at,timer;uint16_t a,b,c;} boundary[]={
 ${boundaryReplay}
 };
 for(unsigned legacy=0u;legacy<=1u;legacy++){
  adcZcdResetAcquisition(ADC_ZCD_RESET_START);commutation_interval=318u;adcZcdSeedHandoff(318u);
  step=5u;rising=1u;forward=1u;
  TIM1->CCR1=TIM1->CCR2=TIM1->CCR3=5876u;TIM1->CCR4=2938u;
  changeCompInput();assert(adc_zcd_blank==1u);
  uint32_t origin=DWT->CYCCNT+16000u;
  unsigned previous=schedules;
  adc_fresh=0u;uint8_t level=0u;
  assert(!getCompOutputSample(&level)&&adc_zcd_blank==1u); /* no poll credit */
  for(unsigned i=0u;i<7u;i++){
   DWT->CYCCNT=origin+boundary[i].at;interval=boundary[i].timer;
   adc_phase[0]=boundary[i].a;adc_phase[1]=boundary[i].b;adc_phase[2]=boundary[i].c;
   bemfcounter=zcfound=0u;inject();
   if(i==0u){
    assert(adc_zcd_blank==0u&&!adc_zcd_before_seen&&schedules==previous);
    if(legacy)adc_zcd_blank=1u; /* old invalid sample left blank unchanged */
   }
   if(i==1u)assert(adc_zcd_before_seen==!legacy);
   if(i==2u)assert(schedules==previous+!legacy);
  }
  assert(schedules==previous+!legacy);
 }
 /* A genuine two-group aperture drain still consumes both, never accepts
  * the boundary triplet; a POST without PRE remains rejected. */
 uint8_t level=0u;
 adcZcdResetAcquisition(ADC_ZCD_RESET_START);commutation_interval=318u;adcZcdSeedHandoff(318u);
 step=5u;rising=1u;changeCompInput();adc_zcd_blank=2u;
 interval=78u;adc_phase[0]=1708u;adc_phase[1]=2461u;adc_phase[2]=2593u;
 adc_fresh=1u;assert(!getCompOutputSample(&level)&&adc_zcd_blank==1u&&!adc_zcd_before_seen);
 adc_phase[0]=554u;adc_phase[1]=2391u;adc_phase[2]=20u;
 adc_fresh=1u;assert(!getCompOutputSample(&level)&&!adc_zcd_blank&&!adc_zcd_before_seen);
 interval=242u;adc_phase[0]=1265u;adc_phase[1]=2422u;
 adc_fresh=1u;assert(!getCompOutputSample(&level)&&!adc_zcd_reported);
 puts("PASS actual 87pct step5 capture: invalid boundary consumes drain, sole PRE retained, 85us crossing schedules; old rule misses; two-group/no-fresh/PRE guards retained");
#endif
#ifdef TARGET_FD_M2_STABLE_PRE
 static const struct {uint32_t at,timer;uint16_t a,b,c,cnt;} stable[]={
 ${stableReplay}
 };
 /* Simulate the same-carrier captured sequence with old and repaired rules;
  * the repaired first PRE must NOT itself schedule a commutation. */
 for(unsigned legacy=0u;legacy<=1u;legacy++){
  adcZcdResetAcquisition(ADC_ZCD_RESET_START);commutation_interval=312u;adcZcdSeedHandoff(312u);
  step=3u;rising=1u;forward=1u;TIM1->CCR1=TIM1->CCR2=TIM1->CCR3=5996u;TIM1->CCR4=2998u;
  uint32_t origin=0xfffff000u;DWT->CYCCNT=origin;adc_fresh=0u;changeCompInput();
  assert(fd_m2_pre_allowed);if(legacy)fd_m2_pre_allowed=0u;
  fd_m2_last_group_at=origin+stable[0].at-6501u;
  unsigned previous=schedules;
  for(unsigned i=0u;i<4u;i++){
   DWT->CYCCNT=origin+stable[i].at;interval=stable[i].timer;TIM1->CNT=stable[i].cnt;
   adc_phase[0]=stable[i].a;adc_phase[1]=stable[i].b;adc_phase[2]=stable[i].c;
   bemfcounter=zcfound=0u;inject();
   if(i==0u)assert(adc_zcd_before_seen==!legacy&&schedules==previous&&adc_zcd_last_out==1u);
   if(i==1u){assert(schedules==previous);if(!legacy)assert(fd_m2_crossing_time.crossing);}
   if(i==2u){
    assert(schedules==previous+!legacy);
    if(!legacy)assert(scheduled_at+scheduled_ticks*80u-DWT->CYCCNT>=1600u);
   }
  }
  assert(schedules==previous+!legacy);
 }
 /* Pending old group, real aperture change and low-speed startup are ineligible. */
 adc_fresh=1u;changeCompInput();assert(!fd_m2_pre_allowed);adc_fresh=0u;
 commutation_interval=4000u;changeCompInput();assert(!fd_m2_pre_allowed);
 commutation_interval=312u;step=4u;TIM1->CCR1=1334u;TIM1->CCR4=667u;
 fd_m2_sine_closed_late=1u<<3u;changeCompInput();assert(adc_zcd_blank==2u&&!fd_m2_pre_allowed);
 fd_m2_sine_closed_late=0u;
 /* Even with the rescue enabled, transient/rail/late groups cannot arm it. */
 for(unsigned reject=0u;reject<5u;reject++){
  adcZcdResetAcquisition(ADC_ZCD_RESET_START);commutation_interval=312u;adcZcdSeedHandoff(312u);
  step=3u;rising=1u;TIM1->CCR1=5996u;TIM1->CCR4=2998u;adc_fresh=0u;
  DWT->CYCCNT=1000000u;changeCompInput();assert(fd_m2_pre_allowed);
  uint32_t age=reject==0u?3000u:4555u;
  fd_m2_last_group_at=DWT->CYCCNT+age-(reject==3u?10000u:6501u);
  DWT->CYCCNT+=age;TIM1->CNT=reject==4u?3200u:3663u;
  adc_phase[0]=4u;adc_phase[1]=reject==1u?1856u:(reject==2u?4u:509u);adc_phase[2]=2332u;
  interval=156u;adc_fresh=1u;assert(!getCompOutputSample(&level)&&!adc_zcd_before_seen&&adc_zcd_last_out==1u);
 }
 /* A rank collision must return before spending blank credit or arming PRE. */
 adcZcdResetAcquisition(ADC_ZCD_RESET_START);commutation_interval=312u;adcZcdSeedHandoff(312u);
 step=3u;rising=1u;TIM1->CCR1=5996u;TIM1->CCR4=2998u;adc_fresh=0u;
 DWT->CYCCNT=1000000u;changeCompInput();fd_m2_last_group_at=998054u;
 DWT->CYCCNT+=4555u;TIM1->CNT=3663u;
 adc_phase[0]=4u;adc_phase[1]=509u;adc_phase[2]=2332u;
 adc_collision=1u;adc_fresh=1u;assert(!getCompOutputSample(&level));
 assert(adc_zcd_blank==1u&&!adc_zcd_before_seen&&fd_m2_last_group_at==998054u);
 adc_collision=0u;adc_fresh=0u;
 /* Falling-direction PRE must also leave polarity alone until a real POST. */
 rising=0u;forward=0u;DWT->CYCCNT=1000000u;changeCompInput();
 fd_m2_last_group_at=998054u;DWT->CYCCNT+=4555u;adc_phase[1]=1856u;
 adc_fresh=1u;assert(!getCompOutputSample(&level)&&adc_zcd_before_seen&&!adc_zcd_last_out);
 DWT->CYCCNT+=6500u;interval=325u;adc_phase[1]=509u;
 adc_fresh=1u;assert(getCompOutputSample(&level)&&level==1u);
 forward=1u;
 puts("PASS actual 89pct step3 capture: settled PRE retained without an edge, hysteresis brackets zero, POST schedules with service lead; old rule misses; pending/aperture/startup guards retained");
#endif
#ifdef TARGET_FD_M2_SINE_ADC_CAPTURE
 unsigned adc_state_before_stop=fd_m2_sine_adc_capture.header.state;
#endif
 unsigned before_stop=fd_m2_sine_closed.count;
 zero();assert(!bridge&&!fd_m2_sine_closed_late&&fd_m2_sine_closed.count==before_stop+1u);
#ifdef TARGET_FD_M2_SINE_ADC_CAPTURE
 assert(!fd_m2_sine_adc_enabled&&fd_m2_sine_adc_capture.header.state==(adc_state_before_stop>=FD_M2_ADC_FROZEN?adc_state_before_stop:FD_M2_ADC_STOPPED));
#endif
 puts("PASS actual ADC PRE/two-POST qualification through full-turn lock, real JEOS controller handoff, ten closed-loop schedules, learned step-4 aperture and stop trace");
 return 0;
}
`;
}

if(m2Pure){
 program=program.replace('static void fdM2SineClosedEvent(uint8_t kind);','static void fdM2SineClosedEvent(uint8_t kind){(void)kind;}');
 // Remove observation-only assertions from the test harness, never from the
 // included firmware. All control/safety/raw-waveform checks still execute.
 const diagnostic=/fd_m2_sine_(?:coast|handoff|powered|adaptation|closed)|fd_power_bench\.(?:trials|completed|last_reason)|fd_native_sine\.(?:reason|sixstep_at|first_edge_at|sixth_edge_at|stopped_at|sine_steps)/;
 let next='',cursor=0;
 for(let a=program.indexOf('assert(');a>=0;a=program.indexOf('assert(',cursor)){
  let depth=1,j=a+7;
  for(;j<program.length&&depth;j++){if(program[j]==='(')depth++;else if(program[j]===')')depth--;}
  const assertion=program.slice(a,j);next+=program.slice(cursor,a)+(diagnostic.test(assertion)?'((void)0)':assertion);cursor=j;
 }
 program=next+program.slice(cursor);
 program=program.replace(block(program,'static void startAdaptiveDrive(void)'),`static void startAdaptiveDrive(void){
  reachPlatform();startAtPredictedBoundary();assert(fd_m2_sine_timed&&bridge==2u);
  assert(fd_m2_sine_pickup.period>=1600u*160u&&fd_m2_sine_pickup.period<=1680u*160u);
 }`)
 .replace('unsigned before_stop=fd_m2_sine_closed.count;','unsigned before_stop=0u;(void)before_stop;')
 .replace('fd_native_sine.edges==i+1u','fd_native_sine.edges==(i<6u?i+1u:6u)');
 program=program.replace(' unsigned before_stop=0u;(void)before_stop;',`
 /* Pure still enforces configured limits and never reintroduces4A bench trip. */
 use_current_limit=0u;actual_current=1200;degrees_celsius=40u;
 fdBenchService();assert(!fd_power_bench.inhibit);
 use_current_limit=1u;eepromBuffer.limits.current=10u;actual_current=1999;
 fdBenchService();assert(!fd_power_bench.inhibit);
 actual_current=2000;fdBenchService();assert(fd_power_bench.inhibit&&!bridge&&!running);
 zero();use_current_limit=0u;actual_current=0;
 startAdaptiveDrive();degrees_celsius=120u;fdBenchService();assert(fd_power_bench.inhibit&&!bridge);
 zero();degrees_celsius=40u;startAdaptiveDrive();running=1u;fd_native_sine.phase=2u;
 running=0u;fdBenchService();assert(fd_power_bench.inhibit&&!bridge);
 zero();startAdaptiveDrive();
 puts("PASS pure current/temperature/running-state stops and zero rearm retain control protections");
 unsigned before_stop=0u;(void)before_stop;`);

}

const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-native-sine-host-')),c=path.join(dir,'test.c'),exe=path.join(dir,'test');fs.writeFileSync(c,program);
for(const [cmd,args] of [['cc',['-std=c11','-O1','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),c,'-o',exe]],[exe,[]]]){const r=cp.spawnSync(cmd,args,{encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);}
