#pragma once
#include <stdint.h>
enum { FD_SYNC_CAPACITY=512, FD_SYNC_WINDOW=1, FD_SYNC_ADC2_FRESH=2,
 FD_SYNC_NO_COLLISION=8, FD_SYNC_CADENCE=16, FD_SYNC_POST_BLANK=32,
 FD_SYNC_DRIVE_VALID=64 };
typedef struct {
 uint32_t cycles;
 uint16_t a,b,c,pa3,pa7,cnt,arr,duty,trigger;
 uint8_t step,flags;
} FdPinSyncRecord;
typedef struct {
 uint32_t magic,abi,count,frozen,trial,closed_seen,closed_at,previous_at;
 FdPinSyncRecord records[FD_SYNC_CAPACITY];
} FdPinSync;
/* Each observation is at IRQ consumption time, not an ADC aperture timestamp.
 * 200ms settling wait is a capture gate only; it never controls the motor. */
static inline uint8_t fdPinSyncReady(volatile FdPinSync *s,uint32_t now,uint8_t drive,uint8_t acquiring)
{
 if(!drive||acquiring){s->closed_seen=0;return 0;}
 if(!s->closed_seen){s->closed_seen=1;s->closed_at=now;s->previous_at=now;
  s->count=0;s->frozen=0;s->trial++;return 0;}
 return !s->frozen&&(uint32_t)(now-s->closed_at)>=32000000u;
}
static inline void fdPinSyncPush(volatile FdPinSync *s,const FdPinSyncRecord *r,uint8_t drive,uint8_t acquiring)
{
 if(!fdPinSyncReady(s,r->cycles,drive,acquiring))return;
 uint32_t gap=r->cycles-s->previous_at;s->previous_at=r->cycles;
 uint32_t n=s->count;if(n>=FD_SYNC_CAPACITY){s->frozen=1;return;}
 FdPinSyncRecord v=*r;
 /* Reject pairing inference across missed/late service or a long pause. */
 uint32_t period=(uint32_t)r->arr+1u;
 if(gap>period/2u&&gap<period+period/2u)v.flags|=FD_SYNC_CADENCE;
 s->records[n]=v;s->count=n+1u;
 if(n+1u==FD_SYNC_CAPACITY)s->frozen=1;
}
