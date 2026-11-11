#pragma once
#include <stdint.h>
/* Fixed5ms bins, first30ms + second60ms. Consumed PWM-triggered ADC triplets,
 * NOT rotor position, speed, or hardware sample timestamps. No control reads. */
typedef struct {
    uint32_t total, valid, sum[3];
    uint16_t minimum[3], maximum[3], first[3], last[3];
} FdAlignBin;
typedef struct {
    uint32_t magic, abi, trial, frozen;
    FdAlignBin bins[18];
} FdAlignObserve;
_Static_assert(sizeof(FdAlignBin)==44u,"alignment bin ABI");
_Static_assert(sizeof(FdAlignObserve)==808u,"alignment logger ABI");
static inline void fdAlignBegin(volatile FdAlignObserve *s,uint32_t trial)
{ *s=(FdAlignObserve){.magic=0x46414c31u,.abi=1u,.trial=trial}; }
static inline void fdAlignSample(volatile FdAlignObserve *s,uint8_t left,
    uint32_t age_cycles,const uint16_t values[3],uint8_t usable)
{
    if(s->frozen || (left!=1u && left!=2u))return;
    uint32_t bin=age_cycles/800000u;
    if(bin>=(left==2u?6u:12u))return;
    if(left==1u)bin+=6u;
    volatile FdAlignBin *b=&s->bins[bin]; b->total++;
    if(!usable)return;
    for(unsigned i=0;i<3;i++){
        uint16_t v=values[i];
        if(!b->valid)b->minimum[i]=b->maximum[i]=b->first[i]=v;
        if(v<b->minimum[i])b->minimum[i]=v;
        if(v>b->maximum[i])b->maximum[i]=v;
        b->last[i]=v;b->sum[i]+=v;
    }
    b->valid++;
}
