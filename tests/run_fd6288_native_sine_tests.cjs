// Execute the actual native sine loop, output formula and trial adapter with
// simulated registers/time. No motor model, SWD or synthetic automatic ZCs.
const fs=require('fs'),path=require('path'),os=require('os'),cp=require('child_process'),assert=require('assert');
const root=path.resolve(__dirname,'..'),main=fs.readFileSync(path.join(root,'Src/main.c'),'utf8');
function block(s,start){const p=s.indexOf(start);assert(p>=0,start);const b=s.indexOf('{',p);let d=0;for(let i=b;i<s.length;i++){if(s[i]==='{')d++;if(s[i]==='}'&&!--d)return s.slice(p,i+1);}throw Error(start);}
const settingStart=main.indexOf('    /* Trial-only RAM overrides;');
assert(settingStart>=0);
const settings=main.slice(settingStart,main.indexOf('#endif',settingStart));
const table=main.slice(main.indexOf('int16_t pwmSin[] = {'),main.indexOf('};',main.indexOf('int16_t pwmSin[] = {'))+2);
const sine=main.slice(main.indexOf('            if (input > 48 && armed) {',main.indexOf('} else { // stepper sine')),main.indexOf('\n#endif // gimbal mode',main.indexOf('} else { // stepper sine')));
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
static struct {uint32_t CYCCNT;} dwt;
#define DWT (&dwt)
static uint32_t mask,ccr[4],cap=610,bridge,commutations,compares;
static unsigned armed=1,adjusted_input,input,running,stepper_sine,do_once_sinemode;
static unsigned stuck_rotor_inhibit,drive_safety_limit_inhibit,heli_coast_restart_inhibit,heli_coast_active;
static unsigned minimum_duty_cycle=210,min_startup_duty=360,startup_max_duty_cycle=610,use_speed_control_loop;
static unsigned duty_cycle_setpoint,duty_cycle,last_duty_cycle,prop_brake_active,old_routine;
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
static void settingsOverride(void){${settings}}
static void mapInput(void){${block(main,'if (eepromBuffer.use_sine_start) {')}}
static void nativeStep(void){for(unsigned once=0;once<1;once++){${sine}}}
static void zero(void){adjusted_input=0;fdBenchGuardInput();assert(!bridge&&!fd_power_bench.inhibit);}
static void begin(unsigned throttle){zero();adjusted_input=throttle;assert(!fdBenchGuardInput());mapInput();assert(fd_native_sine.phase==1);}
int main(void){
 eepromBuffer.use_sine_start=0;eepromBuffer.advance_level=34;eepromBuffer.auto_advance=1;
 settingsOverride();assert(fdBenchConfigOk());assert(eepromBuffer.minimum_duty_cycle==1);
 assert(eepromBuffer.advance_level==34&&eepromBuffer.auto_advance==1);
 eepromBuffer.advance_level=18;eepromBuffer.auto_advance=0;
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
static void fdAsyncSchedule(uint32_t at,uint32_t ticks){(void)at;schedules++;scheduled_ticks=ticks;}
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

const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-native-sine-host-')),c=path.join(dir,'test.c'),exe=path.join(dir,'test');fs.writeFileSync(c,program);
for(const [cmd,args] of [['cc',['-std=c11','-O1','-fsanitize=address,undefined','-I'+path.join(root,'Inc'),c,'-o',exe]],[exe,[]]]){const r=cp.spawnSync(cmd,args,{encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);}
