// Offline audit only: uses actual detector functions and synthetic register states.
const fs=require('fs'),path=require('path'),os=require('os'),assert=require('assert/strict'),{spawnSync}=require('child_process');
const root=process.cwd(), upstream=process.argv[2];
if(!upstream)throw Error('Pass the pinned upstream v2.21 Src/main.c path');
function body(s,sig){let a=s.indexOf(sig),p=s.indexOf('{',a),n=1,i=p+1;assert(a>=0&&p>=0);for(;n&&i<s.length;i++){if(s[i]==='{')n++;if(s[i]==='}')n--;}assert(!n);return s.slice(a,i);}
let c=fs.readFileSync(path.join(root,'Mcu/g431/Src/comparator.c'),'utf8');
c=c.slice(c.indexOf('#ifdef USE_ADC_ZCD\n'),c.indexOf('#else /* !USE_ADC_ZCD')).replace(/^#include[^\n]*$/gm,'')+'\n#endif\n';
const up=body(fs.readFileSync(upstream,'utf8'),'void getBemfState()').replace('void getBemfState()','void upstreamBemfState()').replace('getCompOutputLevel()','upstreamRaw()');
const prefix=`
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#define USE_ADC_ZCD
#define TARGET_ADC_ZCD_GUARD
#define TARGET_FD_ADC_JEOS
#define TARGET_FD_NO_DIAGNOSTICS
#define FD_DIAG(...) ((void)0)
#define fdTraceNoFresh() ((void)0)
#define ADC1 0
#define LL_ADC_INJ_RANK_1 0
#define LL_ADC_INJ_RANK_2 1
#define LL_ADC_INJ_RANK_3 2
#define INTERVAL_TIMER_COUNT (timer)
#define FD6288_STARTUP_TIMING_FIX 1
static uint32_t timer;
static uint16_t jdr[3];
static uint8_t fresh;
static unsigned mask;
static unsigned __get_PRIMASK(void){return mask;}
static void __disable_irq(void){mask=1;}
static void __set_PRIMASK(unsigned p){mask=p;}
static unsigned LL_ADC_IsActiveFlag_JEOS(int a){(void)a;return fresh;}
static void LL_ADC_ClearFlag_JEOS(int a){(void)a;fresh=0;}
static unsigned LL_ADC_INJ_ReadConversionData12(int a,unsigned r){(void)a;return jdr[r];}
char step,rising,old_routine;
volatile uint32_t commutation_interval=10000,polling_mode_changeover=2000;
static unsigned bemfcounter,bad_count,bad_count_threshold=6;
static unsigned upstreamRaw(void){return 0;} /* physical post side, rising=1 */
`;
const suffix=`
static unsigned feed(unsigned a,unsigned b,unsigned cc){uint8_t level; jdr[0]=a;jdr[1]=b;jdr[2]=cc;fresh=1;timer+=83;return getCompOutputSample(&level);}
static void reset(void){adcZcdResetAcquisition(0);step=3;rising=1;commutation_interval=10000;changeCompInput();timer=0;}
int main(void){
 reset();unsigned accepted=0;for(unsigned i=0;i<540;i++)accepted+=feed(0,1200,1600);assert(accepted==0);assert(adc_zcd_before_seen==0);
 bemfcounter=bad_count=0;unsigned polls=0;while(bemfcounter<=6){upstreamBemfState();polls++;}assert(polls==7);
 printf("Post-only: ADC guard accepted=%u over 540 fresh groups (one blank); upstream steady post qualifies after %u polls.\\n",accepted,polls);
 reset();feed(0,600,1600);feed(0,600,1600);assert(adc_zcd_before_seen);
 assert(feed(0,1000,2200)==0);assert(feed(0,1400,3000)==0);
 reset();feed(0,600,1600);feed(0,600,1600);
 // n completed but its JEOS was not serviced until n+1 A/B updated.
 // n+1 C is still old. Clear/read/recheck JEOS sees no new completion.
 assert(feed(0,1000,1600)==0);assert(adc_zcd_confirmations==1);
 unsigned mixed=feed(0,1400,2200);assert(mixed==1);
 puts("Partial-next-sequence counterexample: all complete frames pre; two mixed JDR snapshots produce an accepted edge. Hardware delay prerequisite is NOT established by this host test.");
}
`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-chain-audit-')),file=path.join(dir,'replay.c'),exe=path.join(dir,'replay');fs.writeFileSync(file,prefix+c+up+suffix);
for(const [cmd,args] of [['cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',file,'-o',exe]],[exe,[]]]){const r=spawnSync(cmd,args,{encoding:'utf8'});process.stdout.write(r.stdout||'');process.stderr.write(r.stderr||'');assert.equal(r.status,0);}
