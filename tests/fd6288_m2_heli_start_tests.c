#include <assert.h>
#include <stdio.h>
#include "fd6288_m2_heli_start.h"
static uint32_t nextStep(uint32_t step,uint32_t forward)
{return forward?(step==6u?1u:step+1u):(step==1u?6u:step-1u);}
static void steadyRise(uint32_t origin,uint32_t forward)
{
    FdM2HeliStart s;uint32_t step=3u,edge=origin;
    fdM2HeliBegin(&s,42u,origin,origin,256000u,step,400u);
    uint16_t duty=400u;
    for(unsigned us=50u;us<=3000000u;us+=50u){
        uint32_t now=origin+us*160u;
        if((uint32_t)(now-edge)>=256000u){
            edge=now;step=nextStep(step,forward);fdM2HeliEdge(&s,now,step,forward);
        }
        uint16_t previous=duty;duty=fdM2HeliDuty(&s,now,2000u,duty);
        assert(duty>=previous&&duty-previous<=1u);
        if(us==1500000u)assert(duty==1200u);
        if(us==3000000u)assert(duty==2000u);
    }
    assert(!s.holds&&!s.bad_turns&&s.good_turns>100u);
    assert(fdM2HeliDuty(&s,s.at+8000u,300u,duty)==300u); /* decrease immediately */
    assert(fdM2HeliDuty(&s,s.at,2000u,300u)==300u); /* no saved credit */
}
static void overdueAndRecovery(void)
{
    FdM2HeliStart s;uint32_t now=1000000u,step=3u;
    fdM2HeliBegin(&s,1u,now,now,256000u,step,400u);
    assert(fdM2HeliDuty(&s,now+397335u,2000u,400u)==400u);
    assert(s.reason==FD_HELI_OVERDUE&&s.holds==1u&&!s.healthy);
    uint32_t first=s.first_hold_at;
    for(unsigned i=1u;i<=4u;i++)
        assert(fdM2HeliDuty(&s,now+397335u+i*8000u,2000u,400u)==400u);
    assert(s.holds==1u&&s.first_hold_at==first&&!s.credit);
    now+=6u*256000u;step=nextStep(step,1u);fdM2HeliEdge(&s,now,step,1u);
    assert(!s.healthy&&s.bad_turns==1u&&s.period==256000u); /* no IIR poisoning */
    for(unsigned i=0;i<6u;i++){
        now+=256000u;step=nextStep(step,1u);fdM2HeliEdge(&s,now,step,1u);
        if(i<5u)assert(!s.healthy);
    }
    assert(s.healthy&&s.good_turns==1u&&s.period==256000u);
    assert(fdM2HeliDuty(&s,now,2000u,400u)==400u); /* discard missed time */
    assert(fdM2HeliDuty(&s,now+8000u,2000u,400u)==400u);
    assert(s.first_hold_at==first);
}
static void partialTurnCannotRestoreTrust(void)
{
    FdM2HeliStart s;uint32_t now=1000000u,step=1u;
    fdM2HeliBegin(&s,1u,now,now,256000u,step,400u);
    for(unsigned i=0u;i<3u;i++) {
        now+=256000u;step=nextStep(step,1u);fdM2HeliEdge(&s,now,step,1u);
    }
    assert(s.turn_edges==3u);
    assert(fdM2HeliDuty(&s,now+397335u,2000u,400u)==400u);
    assert(!s.turn_edges&&!s.healthy);
    /* Edge after overdue is still inside the generous per-sector bound.
     * Pre-hold evidence must not shorten the fresh full-turn requirement. */
    now+=400000u;step=nextStep(step,1u);fdM2HeliEdge(&s,now,step,1u);
    for(unsigned i=0u;i<5u;i++) {
        assert(!s.healthy);
        now+=256000u;step=nextStep(step,1u);fdM2HeliEdge(&s,now,step,1u);
    }
    assert(s.healthy&&s.good_turns==1u);
}
static void rejectBadFeedback(void)
{
    FdM2HeliStart s;uint32_t now=1000000u;
    fdM2HeliBegin(&s,1u,now,now,261256u,1u,400u);
    fdM2HeliEdge(&s,now+1073040u,2u,1u); /* captured 6706.5 us gap */
    assert(!s.healthy&&s.period==261256u&&s.bad_turns==1u);
    assert(fdM2HeliDuty(&s,now+1073100u,1800u,400u)==400u);
    assert(s.reason==FD_HELI_BAD_EDGE&&s.first_hold_duty==400u);
    fdM2HeliBegin(&s,2u,now,now,256000u,1u,400u);
    fdM2HeliEdge(&s,now+256000u,4u,1u);assert(!s.healthy); /* wrong sector */
    fdM2HeliBegin(&s,3u,now,now,256000u,1u,400u);
    fdM2HeliEdge(&s,now+1000u,2u,1u);assert(!s.healthy); /* implausibly early */
    s.active=0u;assert(fdM2HeliDuty(&s,now+9000u,0u,400u)==0u);
}
int main(void)
{
    steadyRise(100000u,1u);steadyRise(0xffff0000u,1u);steadyRise(0xffff0000u,0u);
    overdueAndRecovery();partialTurnCannotRestoreTrust();rejectBadFeedback();
    puts("PASS 3s output slew, wrap/direction, immediate decrease, overdue hold, captured bad gap and full-turn recovery without catch-up");
}
