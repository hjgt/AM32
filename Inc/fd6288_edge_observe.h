#pragma once
#include <stdint.h>
/* Sparse software-consumed snapshots, NOT a waveform or hardware timestamps.
 * Slots0/1: second alignment at>=59/59.8ms. Each of first2 drive sectors:
 * first4 consumed triplets; first pre/hyst/post/accepted/invalid/confirmation.
 * No control path reads these fields. All callers already serialized. */
typedef struct {
    uint32_t at;
    uint16_t a,b,c;
    int16_t margin;
    uint16_t cnt,duty,trigger;
    uint8_t reason,step,flags,present;
    uint16_t reserved;
} FdEdgeSample;
#ifdef TARGET_FD_EDGE_RAW
#include "fd6288_edge_raw.h"
#else
typedef struct {
    uint32_t magic,abi,trial,started,frozen,sectors,active,first_samples;
    uint32_t seen,count,max_cycles,calls,start0,end0,start1,end1;
    FdEdgeSample samples[22];
} FdEdgeObserve;
_Static_assert(sizeof(FdEdgeSample)==24,"edge sample ABI");
_Static_assert(sizeof(FdEdgeObserve)==592,"edge observer ABI");
#ifdef TARGET_FD_EDGE_OBSERVE
extern volatile FdEdgeObserve fd_edge_observe;
static inline void fdEdgeBegin(uint32_t trial,uint32_t now)
{
    fd_edge_observe=(FdEdgeObserve){.magic=0x46454f31u,.abi=1,.trial=trial,
        .started=now,.active=UINT32_MAX};
}
static inline uint8_t fdEdgeActive(void)
{return !fd_edge_observe.frozen && fd_edge_observe.active<2u;}
static inline void fdEdgeClose(uint32_t now)
{
    if(!fdEdgeActive())return;
    if(fd_edge_observe.active==0)fd_edge_observe.end0=now-fd_edge_observe.started;
    else {fd_edge_observe.end1=now-fd_edge_observe.started;fd_edge_observe.frozen=1;}
    fd_edge_observe.active=UINT32_MAX;
}
static inline void fdEdgeFinish(uint32_t now)
{fdEdgeClose(now);fd_edge_observe.frozen=1;}
static inline void fdEdgeStep(uint32_t now,uint8_t stage)
{
    if(fd_edge_observe.frozen || stage!=2u || fd_edge_observe.sectors>=2u)return;
    uint32_t n=fd_edge_observe.sectors++;
    fd_edge_observe.active=n;fd_edge_observe.first_samples=fd_edge_observe.seen=0;
    if(n==0)fd_edge_observe.start0=now-fd_edge_observe.started;
    else fd_edge_observe.start1=now-fd_edge_observe.started;
}
static inline void fdEdgeStore(uint32_t slot,FdEdgeSample v)
{
    v.at-=fd_edge_observe.started;v.present=1;
    fd_edge_observe.samples[slot]=v;fd_edge_observe.count++;
}
static inline void fdEdgeCost(uint32_t cycles)
{fd_edge_observe.calls++;if(cycles>fd_edge_observe.max_cycles)fd_edge_observe.max_cycles=cycles;}
static inline uint32_t fdEdgeMask(int16_t margin,uint8_t reason,uint8_t flags)
{
    uint32_t mask=0;
    if(flags&1u)mask|=margin < -8 ? 1u : margin > 8 ? 4u : 2u;
    if(reason==7u)mask|=8u;
    if(reason==0u)mask|=16u;
    if(reason==6u)mask|=32u;
    return mask;
}
static inline uint8_t fdEdgeNeeded(int16_t margin,uint8_t reason,uint8_t flags)
{
    return fdEdgeActive() && (fd_edge_observe.first_samples<4u ||
        (fdEdgeMask(margin,reason,flags)&~fd_edge_observe.seen));
}
static inline void fdEdgeSample(FdEdgeSample v)
{
    if(!fdEdgeActive())return;
    uint32_t base=2u+10u*fd_edge_observe.active;
    if(fd_edge_observe.first_samples<4u)fdEdgeStore(base+fd_edge_observe.first_samples++,v);
    /* flags bit0 means software-valid, non-blank. bit1 is expected rising.
     * Side is observed voltage sign; reason is detector disposition. */
    uint32_t mask=fdEdgeMask(v.margin,v.reason,v.flags)&~fd_edge_observe.seen;
    for(unsigned i=0;i<6;i++)if(mask&(1u<<i))fdEdgeStore(base+4u+i,v);
    fd_edge_observe.seen |= mask;
}
static inline unsigned fdEdgeAlignSlot(uint8_t left,uint32_t age)
{
    if(fd_edge_observe.frozen || left!=1u)return 22u;
    if(age>=59800u*160u && !fd_edge_observe.samples[1].present)return 1u;
    if(age>=59000u*160u && !fd_edge_observe.samples[0].present)return 0u;
    return 22u;
}
#else
#define fdEdgeBegin(...) ((void)0)
#define fdEdgeClose(...) ((void)0)
#define fdEdgeFinish(...) ((void)0)
#define fdEdgeStep(...) ((void)0)
#endif

#endif /* raw / legacy */
