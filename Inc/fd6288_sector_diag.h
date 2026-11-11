#pragma once
#include <stdint.h>
#include "fd6288_edge_observe.h"
/* Latest trial, first 24 commanded sectors, not an ADC waveform. Callers
 * serialize boundaries in existing critical sections; JEOS has sole sample
 * ownership. No additional interrupt mask, allocation, or peripheral reads. */
#define FD_SECTOR_CAPACITY 24u
typedef struct {
    uint32_t start, end, first_accept, last_accept;
    uint16_t reasons[10], samples, usable;
    int16_t minimum, maximum, first, last;
    uint16_t pre_seen, confirmations;
    uint8_t step, stage, rising, forward;
    uint32_t exit_kind;
} FdSectorRecord;
typedef struct {
    uint32_t magic, abi, trial, started, active, sectors, frozen, finish_reason;
    FdSectorRecord records[FD_SECTOR_CAPACITY];
} FdSectorDiagnostics;
_Static_assert(sizeof(FdSectorRecord)==60u,"sector record ABI");
_Static_assert(sizeof(FdSectorDiagnostics)==1472u,"sector diagnostic ABI");
#ifdef TARGET_FD_SECTOR_DIAG
extern volatile FdSectorDiagnostics fd_sector_diag;
static inline void fdSectorBegin(uint32_t id,uint32_t now)
{
    fd_sector_diag.magic=0x46534331u; fd_sector_diag.abi=1u;
    fd_sector_diag.trial=id; fd_sector_diag.started=now;
    fd_sector_diag.active=UINT32_MAX; fd_sector_diag.sectors=0;
    fd_sector_diag.frozen=0; fd_sector_diag.finish_reason=0;
}
static inline void fdSectorClose(uint32_t now,uint32_t why)
{
    uint32_t n=fd_sector_diag.active;
    if(fd_sector_diag.frozen || n>=FD_SECTOR_CAPACITY)return;
    fd_sector_diag.records[n].end=now-fd_sector_diag.started;
    fd_sector_diag.records[n].exit_kind=why;
    fd_sector_diag.active=UINT32_MAX;
}
static inline void fdSectorStep(uint32_t now,uint8_t step,uint8_t stage,uint8_t rising,uint8_t forward)
{
    if(fd_sector_diag.frozen)return;
    uint32_t n=fd_sector_diag.sectors++;
    fd_sector_diag.active=UINT32_MAX;
    if(n>=FD_SECTOR_CAPACITY)return;
    fd_sector_diag.records[n]=(FdSectorRecord){.start=now-fd_sector_diag.started,
        .first_accept=UINT32_MAX,.last_accept=UINT32_MAX,
        .minimum=INT16_MAX,.maximum=INT16_MIN,
        .step=step,.stage=stage,.rising=rising,.forward=forward};
    fd_sector_diag.active=n;
}
static inline void fdSectorFinish(uint32_t now,uint32_t why)
{
    if(fd_sector_diag.frozen)return;
    fdSectorClose(now,5u);
    fd_sector_diag.finish_reason=why; fd_sector_diag.frozen=1u;
}
static inline __attribute__((always_inline)) void fdSectorSample(int16_t margin,
    uint8_t reason,uint8_t usable,uint8_t pre,uint8_t confirms,uint32_t accepted_at)
{
    uint32_t n=fd_sector_diag.active;
    if(n>=FD_SECTOR_CAPACITY || fd_sector_diag.frozen || reason>=10u)return;
    volatile FdSectorRecord *r=&fd_sector_diag.records[n];
    /* Bounded 360ms acquisition at24kHz is <8640 samples, no16-bit overflow. */
    r->samples++; r->reasons[reason]++;
    if(usable){
        if(!r->usable)r->first=margin;
        r->usable++; r->last=margin;
        if(margin<r->minimum)r->minimum=margin;
        if(margin>r->maximum)r->maximum=margin;
    }
    if(pre)r->pre_seen=1;
    if(confirms>r->confirmations)r->confirmations=confirms;
    if(reason==7u){
        uint32_t at=accepted_at-fd_sector_diag.started;
        if(r->first_accept==UINT32_MAX)r->first_accept=at;
        r->last_accept=at;
    }
}
#else
#define fdSectorBegin(id,now) ((void)0)
#define fdSectorClose(now,why) ((void)0)
#define fdSectorStep(now,step,stage,rising,forward) ((void)0)
#define fdSectorFinish(now,why) ((void)0)
#endif
