#include <assert.h>
#include <stdio.h>
#define TARGET_FD_EDGE_OBSERVE
#include "../Inc/fd6288_edge_observe.h"
volatile FdEdgeObserve fd_edge_observe;
static FdEdgeSample sample(uint32_t at,int16_t margin,unsigned reason,unsigned flags){
 return (FdEdgeSample){.at=at,.a=100,.b=200,.c=300,.margin=margin,.reason=reason,.flags=flags,.step=2,.cnt=500,.duty=800,.trigger=300};
}
int main(void){
 assert(fdEdgeMask(-8,3,1)==2 && fdEdgeMask(8,3,1)==2);
 assert(fdEdgeMask(-9,2,1)==1 && fdEdgeMask(9,5,1)==4);
 assert(fdEdgeMask(-19,1,0)==0); /* blank can never establish an observed side */
 for(unsigned wrap=0;wrap<2;wrap++){
  uint32_t start=wrap?UINT32_MAX-1000u:0;
  fdEdgeBegin(7,start);assert(sizeof(fd_edge_observe)==592);
  assert(fdEdgeAlignSlot(2,60000u*160)==22);
  assert(fdEdgeAlignSlot(1,58999u*160)==22);
  unsigned slot=fdEdgeAlignSlot(1,59000u*160);assert(slot==0);
  fdEdgeStore(slot,sample(start+89000u*160,0,255,5));
  slot=fdEdgeAlignSlot(1,59800u*160);assert(slot==1);
  fdEdgeStore(slot,sample(start+89800u*160,0,255,5));
  assert(fdEdgeAlignSlot(1,60000u*160)==22);
  fdEdgeStep(start+100,1);assert(!fdEdgeActive());
  for(unsigned sector=0;sector<2;sector++){
   uint32_t t=start+(90000+sector*20000)*160u;
   fdEdgeStep(t,2);assert(fdEdgeActive());
   assert(fdEdgeNeeded(-10,1,0));fdEdgeSample(sample(t+100,-10,1,0)); /* blank does not mark pre */
   assert(!fd_edge_observe.seen);
   fdEdgeSample(sample(t+200,150,5,1)); /* observed post but unarmed */
   fdEdgeSample(sample(t+300,0,3,1));
   fdEdgeSample(sample(t+400,-19,2,1));
   fdEdgeSample(sample(t+500,9,6,1));
   fdEdgeSample(sample(t+600,9,7,1));
   fdEdgeSample(sample(t+700,0,0,0));
   assert(fd_edge_observe.seen==63);
   assert(!fdEdgeNeeded(150,5,1));
   uint32_t count=fd_edge_observe.count;
   for(unsigned i=0;i<1000;i++)fdEdgeSample(sample(t+800,150,5,1));
   assert(count==fd_edge_observe.count);
   fdEdgeCost(100);fdEdgeCost(80);assert(fd_edge_observe.max_cycles==100);
   fdEdgeClose(t+1000);
  }
  assert(fd_edge_observe.frozen && fd_edge_observe.count==22);
  fdEdgeStep(start+150000u*160,2);assert(fd_edge_observe.sectors==2);
  assert(!fdEdgeNeeded(-19,2,1));
 }
 FdEdgeObserve output=fd_edge_observe;
 fdEdgeBegin(8,0);fdEdgeStep(10,2);fdEdgeFinish(20);
 assert(fd_edge_observe.count==0 && fd_edge_observe.frozen && fd_edge_observe.end0==20);
 fdEdgeBegin(9,0);fdEdgeFinish(1);
 assert(fd_edge_observe.active==UINT32_MAX && fd_edge_observe.frozen && !fd_edge_observe.sectors);
 assert(fwrite(&output,sizeof(output),1,stdout)==1);
 return 0;
}
