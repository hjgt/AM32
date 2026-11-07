const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const source=fs.readFileSync('Mcu/g431/Src/phaseouts.c','utf8');
function fn(sig){const a=source.indexOf(sig),o=source.indexOf('{',a);assert(a>=0);let n=0;for(let i=o;i<source.length;i++){if(source[i]==='{')n++;if(source[i]==='}'&&!--n)return source.slice(a,i+1);}throw Error(sig);}
const c=`#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_POWER_BENCH
#define TARGET_FD_HIZ_OBSERVE
static unsigned mask,calls,fd_hiz_active;
static struct { unsigned inhibit; } fd_power_bench;
static unsigned __get_PRIMASK(void){return mask;}
static void __disable_irq(void){mask=1;}
static void __set_PRIMASK(unsigned v){mask=v;}
static struct {unsigned BSRR,BRR;} ports[6];
#define PHASE_A_GPIO_PORT_HIGH (&ports[0])
#define PHASE_B_GPIO_PORT_HIGH (&ports[1])
#define PHASE_C_GPIO_PORT_HIGH (&ports[2])
#define PHASE_A_GPIO_PORT_LOW (&ports[3])
#define PHASE_B_GPIO_PORT_LOW (&ports[4])
#define PHASE_C_GPIO_PORT_LOW (&ports[5])
#define PHASE_A_GPIO_HIGH 1
#define PHASE_B_GPIO_HIGH 2
#define PHASE_C_GPIO_HIGH 4
#define PHASE_A_GPIO_LOW 8
#define PHASE_B_GPIO_LOW 16
#define PHASE_C_GPIO_LOW 32
#define HIGH_BITREG_OFF BRR
#define LL_GPIO_MODE_OUTPUT 1
#define LL_GPIO_MODE_ALTERNATE 2
#define LL_GPIO_SetPinMode(a,b,c) (calls++)
#define LEG(name) static void name(void){assert(mask);calls++;}
LEG(phaseAPWM) LEG(phaseBPWM) LEG(phaseCPWM)
LEG(phaseALOW) LEG(phaseBLOW) LEG(phaseCLOW)
LEG(phaseAFLOAT) LEG(phaseBFLOAT) LEG(phaseCFLOAT)
${['void comStep(int newStep)','void allpwm()','void proportionalBrake()','void fullBrake()','void twoChannelForward()','void twoChannelReverse()'].map(fn).join('\n')}
int main(void){
 for(unsigned irq=0;irq<2;irq++)for(unsigned lock=0;lock<3;lock++){
  mask=irq;fd_hiz_active=lock==1;fd_power_bench.inhibit=lock==2;
  for(unsigned step=1;step<=6;step++){calls=0;comStep(step);assert(mask==irq);assert(calls==(lock?0u:3u));}
  void (*entry[])(void)={allpwm,proportionalBrake,fullBrake,twoChannelForward,twoChannelReverse};
  for(unsigned i=0;i<5;i++){calls=0;entry[i]();assert(mask==irq);assert(lock?!calls:calls>0);}
 }
 puts("PASS actual bridge entrypoints: six comStep sectors, allpwm, proportional/full brake, brushed directions blocked under Hi-Z/inhibit, IRQ mask restored");
}
`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-hiz-bridge-')),exe=path.join(dir,'test');
for(const [cmd,args,input] of [[process.env.CC||'cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-x','c','-','-o',exe],c],[exe,[],undefined]]){
 const r=spawnSync(cmd,args,{input,encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);
}
