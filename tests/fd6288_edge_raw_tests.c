#include <assert.h>
#include <stdio.h>
#define TARGET_FD_EDGE_OBSERVE
#define TARGET_FD_EDGE_RAW
#include "../Inc/fd6288_edge_observe.h"
volatile FdEdgeObserve fd_edge_observe;
static void feed(unsigned n,unsigned reason){fdEdgeSample((FdEdgeSample){.at=fd_edge_observe.started+n,.a=n,.b=n+1,.c=n+2,.step=2,.reason=reason});}
int main(void){
 for(unsigned wrap=0;wrap<2;wrap++) {
  fdEdgeBegin(1,wrap?0xffffff00u:0);fdEdgeStep(fd_edge_observe.started+1,2);
  feed(2,1);assert(fd_edge_observe.samples[2].reason==1);
  for(unsigned i=3;i<402;i++)feed(i,2);
  assert(fd_edge_observe.first_samples==256 && fd_edge_observe.seen==400);
  fdEdgeClose(fd_edge_observe.started+402);fdEdgeStep(fd_edge_observe.started+403,2);
  feed(404,7);assert(fd_edge_observe.start1==404);
  for(unsigned i=0;i<192;i++)feed(405+i,2);
  assert(fd_edge_observe.frozen && fd_edge_observe.post_samples==192);
  unsigned count=fd_edge_observe.count;feed(1000,7);assert(fd_edge_observe.count==count);
  assert(fd_edge_observe.samples[258+((401-64)%64)].a==339);
 }
 FdEdgeObserve snapshot=fd_edge_observe;
 fdEdgeBegin(2,0);fdEdgeStep(1,2);feed(2,1);fdEdgeFinish(3);
 assert(fd_edge_observe.first_samples==1 && fd_edge_observe.seen==1 && !fd_edge_observe.post_samples && !fd_edge_observe.start1);
 assert(!fd_edge_observe.samples[0].present);
 assert(fwrite(&snapshot,sizeof(snapshot),1,stdout)==1);
}
