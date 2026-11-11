const fs=require('node:fs'),os=require('node:os'),path=require('node:path'),assert=require('node:assert/strict'),{spawnSync}=require('node:child_process');
const c=`#include <assert.h>
#include <stdio.h>
#define TARGET_FD_EDGE_RAW
#define TARGET_FD_EDGE_OBSERVE
#define TARGET_FD_ALIGN_SAMPLE_PAIR
#define TARGET_FD_LAUNCH_RAW
#define TARGET_FD_ALIGN_TAIL
#define TARGET_FD_EARLY_REKICK
#define FD6288_SECOND_ALIGN_MS 300
#include "fd6288_edge_observe.h"
volatile FdEdgeObserve fd_edge_observe;
static void window(uint32_t from,unsigned us,unsigned accept,uint8_t step){
 fdEdgeStep(fd_edge_observe.started+from*160u,2);
 for(unsigned i=40;i<us;i+=40)fdEdgeSample((FdEdgeSample){.at=fd_edge_observe.started+(from+i)*160u,.a=i,.b=i+1,.c=i+2,.margin=i>accept?9:-9,.cnt=900,.duty=1334,.trigger=667,.reason=i==accept?7:2,.step=step,.flags=step==3?3:1});
 fdEdgeClose(fd_edge_observe.started+(from+us)*160u);
}
int main(void){
 for(unsigned wrap=0;wrap<2;wrap++){
  uint32_t start=wrap?0xffff0000u:0u;fdEdgeBegin(7+wrap,start);
  fdEdgeStore(0,(FdEdgeSample){.at=start+327000u*160u,.a=1500,.b=700,.c=10,.margin=-55,.reason=255,.step=6,.flags=5});
  fdEdgeStore(1,(FdEdgeSample){.at=start+329800u*160u,.a=1501,.b=701,.c=10,.margin=-54,.reason=255,.step=6,.flags=5});
  fd_edge_observe.max_cycles=160;
  for(unsigned k=0;k<3;k++)window(330000u+k*21000u,1000,480,2);
  window(393000u,25500,25000,2);window(418500u,20500,20000,3);
  fdEdgeFinish(start+439000u*160u);
  assert(fd_edge_observe.abi==9&&fd_edge_observe.frozen&&fd_edge_observe.active==UINT32_MAX);
  assert(fd_edge_observe.sectors==5&&fd_edge_observe.count<512&&fd_edge_observe.count>350);
  assert(fd_edge_observe.samples[0].b==700&&fd_edge_observe.samples[1].b==701&&fd_edge_observe.samples[0].reserved==160);
  assert(fd_edge_observe.seen==fd_edge_observe.count+fd_edge_observe.post_samples);
  assert(fd_edge_observe.start1==418500u*160u);
  assert(fwrite((const void*)&fd_edge_observe,sizeof(fd_edge_observe),1,stdout)==1);
 }
 fdEdgeBegin(9,0);fdEdgeStep(330000u*160u,2);
 for(unsigned i=0;i<600;i++)fdEdgeSample((FdEdgeSample){.at=330000u*160u+i,.step=2,.reason=7});
 assert(fd_edge_observe.frozen&&fd_edge_observe.count==512);
 uint32_t end=fd_edge_observe.end1;fdEdgeFinish(500000u*160u);assert(fd_edge_observe.end1==end);
 fdEdgeBegin(10,0);fdEdgeStep(330000u*160u,2);fdEdgeSample((FdEdgeSample){.at=454000u*160u,.step=2});
 assert(fd_edge_observe.frozen&&!fd_edge_observe.count);
}
`;
const dir=fs.mkdtempSync(path.join(os.tmpdir(),'fd-rk-recorder-')),exe=path.join(dir,'test');
let r=spawnSync('cc',['-std=c11','-O1','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-IInc','-x','c','-','-o',exe],{input:c,encoding:'utf8'});assert.equal(r.status,0,r.stderr);
r=spawnSync(exe);assert.equal(r.status,0,r.stderr.toString());assert.equal(r.stdout.length,12404*2);
for(let wrap=0;wrap<2;wrap++){
 const d=require('../tools/scripts/fd6288_edge_decode.cjs').decode(r.stdout.subarray(wrap*12404,(wrap+1)*12404));
 assert.equal(d.header.abi,9);assert.equal(d.header.sectors,5);assert.equal(d.alignment_tail.length,2);assert.equal(d.records.filter(x=>x.reason===7).length,5);
 for(let sec=1;sec<=5;sec++){
  const z=d.records.filter(x=>x.sector===sec);assert(z.length);const begin=z[0].from_launch_us;const dense=z.filter(x=>x.from_launch_us<begin+1900);
  for(let i=1;i<dense.length;i++)assert.equal(dense[i].from_launch_us-dense[i-1].from_launch_us,40);
 }
 assert(d.records.at(-1).from_launch_us>108000&&d.records.at(-1).from_launch_us<124000);
}
console.log('PASS ABI9 actual C recorder→JS: all4 pulse windows+feedback continuation,20ms gaps, dense prefixes, ACCEPT always kept, original tail preserved,512/124ms limits, freeze and wrap');
const b=Buffer.alloc(160,255);[0x46524b31,1,8,3,0,0,8,0].forEach((v,i)=>b.writeUInt32LE(v,4*i));
for(let i=0;i<4;i++){const o=32+i*32;[330000+i*20500,330500+i*20500,500,i===3?3:1,331000+i*20500,331000+i*20500,0,0].forEach((v,j)=>b.writeUInt32LE(j===3||j===6||j===7?v:v*160,o+j*4));}
const q=require('../tools/scripts/fd6288_rekick_decode.cjs').decode(b);assert.equal(q.rekicks_applied,3);assert.equal(q.pulses.length,4);assert.equal(q.pulses[3].classification,3);assert.equal(q.pulses[0].event_age_us,500);
for(const offset of [0,4,12,32+12]){const bad=Buffer.from(b);bad.writeUInt32LE(99,offset);assert.throws(()=>require('../tools/scripts/fd6288_rekick_decode.cjs').decode(bad));}
console.log('PASS re-kick metadata: initial+3, ignored/normal/exhausted classification, software times, corruption rejection');
