// Offline arithmetic check of the actual trigger-position helper.
// An update between CCR1..3 and CCR4 leaves some NEW power compares with OLD CCR4.
const fs=require('fs'),os=require('os'),path=require('path'),assert=require('assert/strict'),{spawnSync}=require('child_process');
const s=fs.readFileSync('Mcu/g431/Inc/peripherals.h','utf8');
const a=s.indexOf('static inline uint16_t zcd_ccr4_from_duty'),b=s.indexOf('\n#define SET_DUTY_CYCLE_ALL',a);assert(a>=0&&b>a);
const c=`#include <stdint.h>\n#include <assert.h>\n#include <stdio.h>\n#define ZCD_SAMPLE_FLOOR 460u\n#define ZCD_ADC_WINDOW_TICKS 244u\n#define ZCD_MINIMUM_ON_TICKS 704u\n${s.slice(a,b)}
static unsigned cmp(unsigned duty){unsigned v=duty*6665u/2000u+1u;return v<704u?704u:v;}
int main(void){unsigned n=0;
 for(unsigned old=1;old<=2000;old++)for(int d=-2;d<=2;d++){
  int next=(int)old+d;if(next<1||next>2000)continue;
  unsigned oc=cmp(old),nc=cmp((unsigned)next),trigger=zcd_ccr4_from_duty(oc);
  assert(trigger>=460u);assert(trigger+244u<=(oc<nc?oc:nc));n++;
 }
 printf("PASS %u +/-2 duty transitions: old trigger +244 fits both old/new power compares.\\n",n);
 for(unsigned old=210;old<=400;old++)for(unsigned next=old;next<=400;next++){
  assert(zcd_ccr4_from_duty(cmp(old))+244u<=cmp(next));
 }
 puts("PASS all monotonic 210..400 positioning/acquisition transitions.");
 assert(zcd_ccr4_from_duty(cmp(400))+244u>cmp(210));
 puts("Counterexample only for a larger drop: request400->210, old CCR4=667, new compare704, 667+244>704. This does not establish such a driven transition in trial18.");
}
`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-pwm-audit-')),file=path.join(dir,'window.c'),exe=path.join(dir,'window');fs.writeFileSync(file,c);
for(const [cmd,args] of [['cc',['-std=c11','-Wall','-Wextra','-Werror',file,'-o',exe]],[exe,[]]]){const r=spawnSync(cmd,args,{encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);}
