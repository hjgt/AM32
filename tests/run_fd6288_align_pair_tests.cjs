const fs=require('fs'),path=require('path'),os=require('os'),assert=require('assert/strict'),{spawnSync}=require('child_process');
const blue=process.argv.includes('--blue-start'),two=process.argv.includes('--two-vector')||blue,tail=process.argv.includes('--align-tail')||two,launch=process.argv.includes('--launch-raw')||tail;
const s=fs.readFileSync('Src/main.c','utf8'),start=s.indexOf('volatile uint8_t fd_align_sample_late;'),end=s.indexOf('\n#endif\n#ifdef TARGET_FD_ALIGN_SETTLE',start);assert(start>0&&end>start);
const h=fs.readFileSync('Mcu/g431/Inc/peripherals.h','utf8'),hs=h.indexOf('static inline uint16_t zcd_ccr4_from_duty'),he=h.indexOf('\n#define SET_DUTY_CYCLE_ALL',hs);
const c=`#include <assert.h>
#include <stdio.h>
#define TARGET_FD_EDGE_RAW
#define TARGET_FD_EDGE_OBSERVE
#define TARGET_FD_ALIGN_SAMPLE_PAIR
${launch?'#define TARGET_FD_LAUNCH_RAW':''}
${tail?'#define TARGET_FD_ALIGN_TAIL\n#define FD6288_SECOND_ALIGN_MS 300':''}
${blue?'#define TARGET_FD_BLUE_START':two?'#define TARGET_FD_LAUNCH_TWO':''}
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
${launch?`
 for(unsigned wrap=0;wrap<2;wrap++) {
  fdAlignmentPairReset();fdEdgeBegin(5+wrap,wrap?0xffff0000u:0u);
  fd_edge_observe.first_samples=512;fd_edge_observe.count=514;
  fd_edge_observe.samples[500].a=1234; /* no12KB clear on buffer reuse */
  uint32_t at=fd_edge_observe.started+330000u*160u;
${tail?`
  assert(fdEdgeAlignSlot(2,297000u*160u)==22);
  assert(fdEdgeAlignSlot(1,296999u*160u)==22);
  assert(fdEdgeAlignSlot(1,297000u*160u)==0);
  fdEdgeStore(0,(FdEdgeSample){.at=at-3000u*160u,.a=1500,.b=730,.c=10,.margin=-25,.step=6,.reason=255,.flags=5});
  assert(fdEdgeAlignSlot(1,297001u*160u)==22);
  assert(fdEdgeAlignSlot(1,299799u*160u)==22);
  assert(fdEdgeAlignSlot(1,299800u*160u)==1);
  fdEdgeStore(1,(FdEdgeSample){.at=at-200u*160u,.a=1501,.b=740,.c=11,.margin=-16,.step=6,.reason=255,.flags=5});
  assert(fdEdgeAlignSlot(1,299900u*160u)==22);
  fd_edge_observe.max_cycles=160;
`:''}
  fdEdgeStep(at,1);assert(fd_edge_observe.abi==3 && !fdEdgeActive());
  fdEdgeStep(at,2);assert(fd_edge_observe.abi==${blue?8:two?6:tail?5:4} && fdEdgeActive());
  assert(!fd_edge_observe.count && fd_edge_observe.samples[500].a==1234);
  for(unsigned n=1;n<${two?1600:1100};n++) {
   uint32_t now=at+n*42u*160u;
   if(n==21){fdEdgeClose(now);fdEdgeStep(now,2);}
   fdEdgeSample((FdEdgeSample){.at=now,.a=n,.b=n+1,.c=n+2,.step=n<21?2:3,.reason=n==8||n==60${two?'||n==1000':''}?7:n==1?1:n==2?0:2});
  }
  assert(fd_edge_observe.frozen && fd_edge_observe.active==UINT32_MAX);
  assert(fd_edge_observe.first_samples<512 && fd_edge_observe.first_samples>${two?350:470});
  assert(fd_edge_observe.seen==fd_edge_observe.count+fd_edge_observe.post_samples);
  assert(fd_edge_observe.start1==fd_edge_observe.start0+8u*42u*160u);
${tail?`
  assert(fd_edge_observe.samples[0].present && fd_edge_observe.samples[1].present);
  assert(fd_edge_observe.samples[0].reserved==160);
  assert(fd_edge_observe.samples[0].b==730 && fd_edge_observe.samples[1].b==740);
  assert(fdEdgeAlignSlot(1,299800u*160u)==22);
`:''}
  for(unsigned i=0;i<47;i++)assert(fd_edge_observe.samples[2+i].a==i+1);
  uint32_t count=fd_edge_observe.count,end=fd_edge_observe.end1;
  fdEdgeFinish(at+100000u*160u);assert(fd_edge_observe.end1==end);
  fd_rev.align_left=1;fd_rev.sector_at=at+500000u*160u;timer.CCR4=600;
  sample(10000,999,600);assert(fd_edge_observe.count==count && timer.CCR4==600 && !fd_pair_started && !fd_align_sample_late);
  output=fd_edge_observe;
 }
 fdEdgeBegin(7,0);assert(fd_edge_observe.abi==3 && !fd_edge_observe.count && !fdEdgeActive());
 fdEdgeStep(330000u*160u,2);fdEdgeSample((FdEdgeSample){.at=330001u*160u,.step=2});
 fdEdgeFinish(330002u*160u);assert(fd_edge_observe.count==1);
 fdEdgeBegin(8,0);fdEdgeStep(330000u*160u,2);
 for(unsigned n=0;n<600;n++)fdEdgeSample((FdEdgeSample){.at=(330001u+n*42u)*160u,.step=2,.reason=7});
 assert(fd_edge_observe.frozen && fd_edge_observe.count==512 && fd_edge_observe.first_samples==512);
` : ''}
 assert(fwrite(&output,sizeof(output),1,stdout)==1);
}
`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-pairs-'));fs.writeFileSync(path.join(dir,'test.c'),c);
let r=spawnSync('cc',['-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-IInc',path.join(dir,'test.c'),'-o',path.join(dir,'test')]);assert.equal(r.status,0,r.stderr.toString());r=spawnSync(path.join(dir,'test'));assert.equal(r.status,0,r.stderr.toString());
const d=require('../tools/scripts/fd6288_edge_decode.cjs').decode(r.stdout);
if(launch){
 assert.equal(d.header.abi,blue?8:two?6:tail?5:4);assert(d.records.length>(two?350:470));assert.equal(d.records[0].reason,1);assert.equal(d.records[1].reason,0);
 if(tail){
  assert.equal(d.alignment_tail.length,2);assert.equal(d.alignment_tail[0].before_launch_us,3000);assert.equal(d.alignment_tail[1].before_launch_us,200);
  assert.equal(d.alignment_tail[0].mid_error,-25);assert.equal(d.alignment_hook_max_us,1);
  let short=Buffer.from(r.stdout);short[68+24+21]=0;assert.equal(require('../tools/scripts/fd6288_edge_decode.cjs').decode(short).alignment_tail.length,1);
  let bad=Buffer.from(r.stdout);bad[68+18]=7;assert.throws(()=>require('../tools/scripts/fd6288_edge_decode.cjs').decode(bad));
  console.log('PASS tail checkpoints: exact297/299.8ms boundaries, one store per slot, survives drive/retry/freeze, missing slot explicit, wrap, decoder');
 }
 assert.equal(d.records.filter(x=>x.reason===7).length,two?3:2);assert.equal(d.records.at(-1).sector,2);
 assert(d.records.every(x=>x.from_launch_us<(two?61000:40000)));
 if(two){
  assert(d.records.at(-1).from_launch_us>60000);assert(d.records.some(x=>x.reason===7&&x.from_launch_us===42000));
  const dense2=d.records.filter(x=>x.sector===2&&x.from_launch_us<21*42+2000);
  assert(dense2.length>=47);for(let i=1;i<dense2.length;i++)assert.equal(dense2[i].from_launch_us-dense2[i-1].from_launch_us,42);
  console.log('PASS ABI6 full61ms coverage, second-vector dense prefix, late ACCEPT retained past old40ms cap');
 }
 for(let i=1;i<47;i++)assert.equal(d.records[i].from_launch_us-d.records[i-1].from_launch_us,42);
 for(const off of [0,4,16,28,32]){let bad=Buffer.from(r.stdout);bad.writeUInt32LE(0xffffffff,off);assert.throws(()=>require('../tools/scripts/fd6288_edge_decode.cjs').decode(bad));}
 console.log('PASS launch recorder: preserves actual pair sampling, header-only reuse, full-rate2ms, decimation, ACCEPT retention, first/second sector, bounds/freeze, retry non-overwrite, wrap, new trial, ABI4 decoder');
}else{assert.equal(d.records.length,512);assert.equal(d.pairs.length,256);assert.equal(d.pairs[0].delta_mid,100);}
console.log('PASS actual alignment adapter: preload discard, early/late tags, bounds, wrap, stop reset, stale trigger rejection, duty window enumeration, ABI3 decoding');
