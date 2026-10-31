#pragma once
/* Same observer and hooks, bounded chronological windows. No control reads. */
typedef struct {
 uint32_t magic,abi,trial,started,frozen,sectors,active,first_samples;
 uint32_t seen,count,max_cycles,calls,start0,end0,start1,end1,post_samples;
 FdEdgeSample samples[514];
} FdEdgeObserve;
_Static_assert(sizeof(FdEdgeObserve)==12404,"raw observer ABI");
extern volatile FdEdgeObserve fd_edge_observe;
static inline void fdEdgeBegin(uint32_t trial,uint32_t now) {
 /* Clear header only: old slots are invalidated by present ranges/counters.
  * Avoid a 12KB memset in the masked startup path. */
 volatile uint8_t *h=(volatile uint8_t *)&fd_edge_observe;
 for(unsigned i=0;i<68;i++)h[i]=0;
 fd_edge_observe.magic=0x46454f31u;fd_edge_observe.abi=2;
#ifdef TARGET_FD_ALIGN_SAMPLE_PAIR
 fd_edge_observe.abi=3;
#endif
 fd_edge_observe.trial=trial;fd_edge_observe.started=now;
 fd_edge_observe.active=UINT32_MAX;
 fd_edge_observe.samples[0].present=fd_edge_observe.samples[1].present=0;
}
static inline uint8_t fdEdgeActive(void) {
#ifdef TARGET_FD_ALIGN_SAMPLE_PAIR
 return 0;
#else
 return !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#endif
}
static inline void fdEdgeClose(uint32_t now) {
 if(fdEdgeActive() && fd_edge_observe.active==0)fd_edge_observe.end0=now-fd_edge_observe.started;
 fd_edge_observe.active=UINT32_MAX;
}
static inline void fdEdgeFinish(uint32_t now) {
 fdEdgeClose(now);fd_edge_observe.end1=now-fd_edge_observe.started;fd_edge_observe.frozen=1;
}
static inline void fdEdgeStep(uint32_t now,uint8_t stage) {
#ifdef TARGET_FD_ALIGN_SAMPLE_PAIR
 (void)now;(void)stage;return;
#endif
 if(fd_edge_observe.frozen || stage!=2u)return;
 fd_edge_observe.active=fd_edge_observe.sectors++;
 if(fd_edge_observe.active==0)fd_edge_observe.start0=now-fd_edge_observe.started;
}
static inline void fdEdgeStore(uint32_t slot,FdEdgeSample v) {
 v.at-=fd_edge_observe.started;v.present=1;
 fd_edge_observe.samples[slot]=v;fd_edge_observe.count++;
}
static inline void fdEdgeCost(uint32_t cycles) {
 fd_edge_observe.calls++;
 if(cycles>fd_edge_observe.max_cycles)fd_edge_observe.max_cycles=cycles;
}
static inline uint8_t fdEdgeNeeded(int16_t margin,uint8_t reason,uint8_t flags) {
 (void)margin;(void)reason;(void)flags;return fdEdgeActive();
}
static inline void fdEdgeSample(FdEdgeSample v) {
 if(!fdEdgeActive())return;
 v.reserved=(uint16_t)fd_edge_observe.sectors;
 if(fd_edge_observe.active==0 && fd_edge_observe.first_samples<256u)
  fdEdgeStore(2u+fd_edge_observe.first_samples++,v);
 if(!fd_edge_observe.start1) {
  fdEdgeStore(258u+(fd_edge_observe.seen++ & 63u),v);
  if(v.reason==7u)fd_edge_observe.start1=v.at-fd_edge_observe.started;
 } else {
  fdEdgeStore(322u+fd_edge_observe.post_samples++,v);
  if(fd_edge_observe.post_samples==192u)fdEdgeFinish(v.at);
 }
}
static inline unsigned fdEdgeAlignSlot(uint8_t left,uint32_t age) {
 if(fd_edge_observe.frozen || left!=1u)return 22u;
 if(age>=59800u*160u && !fd_edge_observe.samples[1].present)return 1u;
 if(age>=59000u*160u && !fd_edge_observe.samples[0].present)return 0u;
 return 22u;
}
