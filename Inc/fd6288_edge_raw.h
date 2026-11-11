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
#ifdef TARGET_FD_LAUNCH_RAW
#ifdef TARGET_FD_ALIGN_TAIL
#ifdef TARGET_FD_HIZ_OBSERVE
 return fd_edge_observe.abi==7u && !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#elif defined(TARGET_FD_EARLY_REKICK)
 return fd_edge_observe.abi==9u && !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#elif defined(TARGET_FD_BLUE_START)
 return fd_edge_observe.abi==8u && !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#elif defined(TARGET_FD_LAUNCH_TWO)
 return fd_edge_observe.abi==6u && !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#else
 return fd_edge_observe.abi==5u && !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#endif
#else
 return fd_edge_observe.abi==4u && !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#endif
#else
 return 0;
#endif
#else
 return !fd_edge_observe.frozen && fd_edge_observe.active!=UINT32_MAX;
#endif
}
static inline void fdEdgeClose(uint32_t now) {
 if(fdEdgeActive() && fd_edge_observe.active==0)fd_edge_observe.end0=now-fd_edge_observe.started;
 fd_edge_observe.active=UINT32_MAX;
}
static inline void fdEdgeFinish(uint32_t now) {
#ifdef TARGET_FD_LAUNCH_RAW
 if(fd_edge_observe.frozen)return; /* retain the capture-window end */
#endif
 fdEdgeClose(now);fd_edge_observe.end1=now-fd_edge_observe.started;fd_edge_observe.frozen=1;
}
static inline void fdEdgeStep(uint32_t now,uint8_t stage) {
#ifdef TARGET_FD_ALIGN_SAMPLE_PAIR
#ifdef TARGET_FD_LAUNCH_RAW
 if(stage!=2u || fd_edge_observe.frozen)return;
 if(fd_edge_observe.abi==3u) {
  /* Pair sampling/control already ended in commutate(). Reuse its slots;
   * clear counters only, never memset12KB in the masked launch path. */
#ifdef TARGET_FD_ALIGN_TAIL
#ifdef TARGET_FD_EARLY_REKICK
  fd_edge_observe.abi=9u;
#elif defined(TARGET_FD_BLUE_START)
  fd_edge_observe.abi=8u;
#elif defined(TARGET_FD_LAUNCH_TWO)
  fd_edge_observe.abi=6u;
#else
  fd_edge_observe.abi=5u;
#endif
  /* Preserve slots0/1. Their payloads precede launch, unlike drive margins.
   * Retain the alignment observer's selected-hook maximum before reset. */
  if(fd_edge_observe.samples[0].present)
   fd_edge_observe.samples[0].reserved=fd_edge_observe.max_cycles>65535u?65535u:fd_edge_observe.max_cycles;
#else
  fd_edge_observe.abi=4u;
#endif
  fd_edge_observe.sectors=0u;
  fd_edge_observe.first_samples=fd_edge_observe.seen=fd_edge_observe.count=0u;
  fd_edge_observe.max_cycles=fd_edge_observe.calls=0u;
  fd_edge_observe.start0=fd_edge_observe.end0=0u;
  fd_edge_observe.start1=fd_edge_observe.end1=fd_edge_observe.post_samples=0u;
#ifndef TARGET_FD_ALIGN_TAIL
  fd_edge_observe.samples[0].present=fd_edge_observe.samples[1].present=0u;
#endif
 }
#else
 (void)now;(void)stage;return;
#endif
#endif
 if(fd_edge_observe.frozen || stage!=2u)return;
 fd_edge_observe.active=fd_edge_observe.sectors++;
 if(fd_edge_observe.active==0)fd_edge_observe.start0=now-fd_edge_observe.started;
#ifdef TARGET_FD_EARLY_REKICK
 fd_edge_observe.start1=now-fd_edge_observe.started; /* latest vector boundary */
#endif
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
#ifdef TARGET_FD_LAUNCH_RAW
 /* Already-consumed ADC triplets only. First2ms at full consumed rate,
  * then alternate groups, always retain ACCEPT. At most one24B store. */
 uint32_t age=(uint32_t)(v.at-fd_edge_observe.started)-fd_edge_observe.start0;
#ifdef TARGET_FD_EARLY_REKICK
 if(age>=124000u*160u) {fdEdgeFinish(v.at);return;}
#elif defined(TARGET_FD_LAUNCH_TWO) || defined(TARGET_FD_BLUE_START)
 if(age>=61000u*160u) {fdEdgeFinish(v.at);return;}
#else
 if(age>=40000u*160u) {fdEdgeFinish(v.at);return;}
#endif
 uint32_t n=fd_edge_observe.seen++;
#ifndef TARGET_FD_EARLY_REKICK
 if(v.reason==7u && !fd_edge_observe.start1)
  fd_edge_observe.start1=v.at-fd_edge_observe.started;
#endif
#ifdef TARGET_FD_EARLY_REKICK
 uint8_t dense=(uint32_t)((v.at-fd_edge_observe.started)-fd_edge_observe.start1)<2000u*160u;
 if(!dense && (n&3u) && v.reason!=7u) {
#elif defined(TARGET_FD_LAUNCH_TWO) || defined(TARGET_FD_BLUE_START)
 uint8_t dense=age<2000u*160u || (fd_edge_observe.active==1u &&
    (uint32_t)((v.at-fd_edge_observe.started)-fd_edge_observe.end0)<2000u*160u);
 if(!dense && (n&3u) && v.reason!=7u) {
#else
 if(age>=2000u*160u && (n&1u) && v.reason!=7u) {
#endif
  fd_edge_observe.post_samples++;return;
 }
 fdEdgeStore(2u+fd_edge_observe.first_samples++,v);
 if(fd_edge_observe.first_samples==512u)fdEdgeFinish(v.at);
 return;
#endif
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
#ifdef TARGET_FD_ALIGN_TAIL
 if(fd_edge_observe.abi!=3u)return 22u; /* never overwrite on retry */
 if(age>=(FD6288_SECOND_ALIGN_MS*1000u-200u)*160u && !fd_edge_observe.samples[1].present)return 1u;
 if(age>=(FD6288_SECOND_ALIGN_MS*1000u-3000u)*160u && !fd_edge_observe.samples[0].present)return 0u;
#else
 if(age>=59800u*160u && !fd_edge_observe.samples[1].present)return 1u;
 if(age>=59000u*160u && !fd_edge_observe.samples[0].present)return 0u;
#endif
 return 22u;
}

#ifdef TARGET_FD_HIZ_OBSERVE
static inline void fdEdgeHiZBegin(uint32_t off_at, uint32_t trigger_at) {
 if(fd_edge_observe.samples[0].present)
  fd_edge_observe.samples[0].reserved=fd_edge_observe.max_cycles>65535u?65535u:fd_edge_observe.max_cycles;
 fd_edge_observe.abi=7u; fd_edge_observe.sectors=0u; fd_edge_observe.active=0u;
 fd_edge_observe.first_samples=fd_edge_observe.seen=fd_edge_observe.count=0u;
 fd_edge_observe.max_cycles=fd_edge_observe.calls=0u;
 fd_edge_observe.start0=off_at-fd_edge_observe.started;
 fd_edge_observe.start1=trigger_at-fd_edge_observe.started;
 fd_edge_observe.end0=fd_edge_observe.end1=fd_edge_observe.post_samples=0u;
}
static inline void fdEdgeHiZSample(FdEdgeSample v) {
 if(!fdEdgeActive())return;
 uint32_t age=(uint32_t)(v.at-fd_edge_observe.started)-fd_edge_observe.start0;
 if(age>=5000u*160u) {fdEdgeFinish(v.at);return;}
 /* Queue/CCR preload boundary is deliberately visible, never called BEMF.
  * First2 groups OR first150us are marked, not discarded. Remaining groups
  * may still include demagnetization/clamping; this is not a validity gate. */
 if(fd_edge_observe.seen<2u || age<150u*160u)v.flags|=16u;
 v.reserved=(uint16_t)fd_edge_observe.seen++;
 fdEdgeStore(2u+fd_edge_observe.first_samples++,v);
 if(fd_edge_observe.first_samples==512u)fdEdgeFinish(v.at);
}
#endif
