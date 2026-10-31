const fs=require('fs'),path=require('path'),os=require('os'),assert=require('assert/strict'),{spawnSync}=require('child_process');
const s=fs.readFileSync('Src/main.c','utf8'),start=s.indexOf('volatile uint8_t fd_align_sample_late;'),end=s.indexOf('\n#endif\n#ifdef TARGET_FD_ALIGN_SETTLE',start);assert(start>0&&end>start);
const h=fs.readFileSync('Mcu/g431/Inc/peripherals.h','utf8'),hs=h.indexOf('static inline uint16_t zcd_ccr4_from_duty'),he=h.indexOf('\n#define SET_DUTY_CYCLE_ALL',hs);
const c=`#include <assert.h>
#include <stdio.h>
#define TARGET_FD_EDGE_RAW
#define TARGET_FD_EDGE_OBSERVE
#define TARGET_FD_ALIGN_SAMPLE_PAIR
#include "fd6288_edge_observe.h"
volatile FdEdgeObserve fd_edge_observe;
static struct {uint32_t CCR1,CCR4;} timer;
#define TIM1 (&timer)
static struct {uint32_t sector_at;unsigned align_left;} fd_rev;
extern volatile uint8_t fd_align_sample_late;
#define ZCD_MINIMUM_ON_TICKS 704u
#define ZCD_SAMPLE_FLOOR 460u
#define ZCD_ADC_WINDOW_TICKS 244u
${h.slice(hs,he)}
static unsigned step=6;
${s.slice(start,end)}
static void sample(unsigned us,unsigned value,unsigned trigger) {uint16_t v[3]={1500,value,10};fdAlignmentPairSample(v,fd_rev.sector_at+us*160u,1100,1200,trigger);}
int main(void) {
 for(unsigned wrap=0;wrap<2;wrap++) {
 fdEdgeBegin(wrap+1,wrap?0xffff0000u:0u);fd_rev.sector_at=fd_edge_observe.started;fd_rev.align_left=1;timer.CCR1=1200;
 sample(9000,1,600);assert(!fd_pair_started);
 sample(10000,1,600);sample(10042,2,600);assert(fd_edge_observe.first_samples==0);
 sample(10084,750,600);assert(fd_edge_observe.first_samples==1&&fd_align_sample_late&&timer.CCR4==900);
 /* Even an old early sample after a missed UEV is discarded. */
 sample(10126,999,600);assert(fd_edge_observe.first_samples==1);
 sample(10168,850,900);assert(fd_edge_observe.first_samples==2&&!fd_align_sample_late&&timer.CCR4==600);
 assert(fd_edge_observe.samples[2].b==750&&fd_edge_observe.samples[3].b==850);
 for(unsigned i=2;i<512;i++) {unsigned t=10200+i*84;sample(t,999,timer.CCR4);sample(t+42,750+(i%2)*100,timer.CCR4);}
 assert(fd_edge_observe.first_samples==512&&!fd_pair_started&&!fd_align_sample_late);
 sample(54000,999,600);assert(fd_edge_observe.first_samples==512);
 fdEdgeFinish(fd_rev.sector_at+56000*160u);
 }
 FdEdgeObserve output=fd_edge_observe;
 fdEdgeBegin(3,0);fd_rev.sector_at=0;sample(10000,1,600);sample(10042,1,600);sample(10084,1,600);assert(fd_align_sample_late);
 fdAlignmentPairReset();assert(timer.CCR4==600&&!fd_align_sample_late);
 fdEdgeBegin(4,0);sample(10000,1,600);sample(10042,1,600);sample(10084,1,900);assert(!fd_edge_observe.first_samples&&!fd_pair_started);
 for(unsigned d=0;d<=6665;d++){fd_align_sample_late=1;unsigned v=zcd_ccr4_from_duty(d);if(d<704)assert(v==0);else assert(v>=460&&v+244<=d);}
 assert(fwrite(&output,sizeof(output),1,stdout)==1);
}
`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-pairs-'));fs.writeFileSync(path.join(dir,'test.c'),c);
let r=spawnSync('cc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-IInc',path.join(dir,'test.c'),'-o',path.join(dir,'test')]);assert.equal(r.status,0,r.stderr.toString());r=spawnSync(path.join(dir,'test'));assert.equal(r.status,0,r.stderr.toString());
const d=require('../tools/scripts/fd6288_edge_decode.cjs').decode(r.stdout);assert.equal(d.records.length,512);assert.equal(d.pairs.length,256);assert.equal(d.pairs[0].delta_mid,100);
console.log('PASS actual alignment adapter: preload discard, early/late tags, bounds, wrap, stop reset, stale trigger rejection, duty window enumeration, ABI3 decoding');
