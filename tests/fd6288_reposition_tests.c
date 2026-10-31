#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_SEQUENCE_V2
#define TARGET_STARTUP_FEEDBACK
#define TARGET_STARTUP_S4
#define TARGET_STARTUP_S4E
#define TARGET_STARTUP_HANDOFF_REAL
#define TARGET_STARTUP_RESCUE_RETAIN
#define TARGET_STARTUP_RESCUE_BRIDGE
#define TARGET_STARTUP_RESCUE_RAMP
#define TARGET_STARTUP_FIRST_WAIT
#define TARGET_FD_NATIVE_EARLY
#define TARGET_FD_DOUBLE_ALIGN
#define TARGET_FD_ALIGN_HOLD
#define TARGET_FD_ALIGN_BUDGET
#define TARGET_FD_ALIGN_SETTLE
#define TARGET_FD_LAUNCH_ADVANCE
#define TARGET_FD_REPOSITION
#define TARGET_FD_NO_DIAGNOSTICS
#include "../Inc/fd6288_startup_sequence.h"
static void align(FdRevPolicy *p) {
    assert(fdRevAdvancePolicy(p,p->sector_at+FD_REV_ALIGN_TICKS)==FD_REV_STEP);
    assert(fdRevAdvancePolicy(p,p->sector_at+FD_REV_SECOND_ALIGN_TICKS)==FD_REV_LAUNCH);
}
int main(void) {
 for(unsigned wrap=0;wrap<2;wrap++) {
    uint32_t start=wrap?UINT32_MAX-1000u:0u;
    FdRevPolicy p; fdRevBeginPolicy(&p,start); align(&p);
    uint32_t launch=p.sector_at;
    /* Healthy feedback remains immediate on edges1..5, then measured delay. */
    for(unsigned i=0;i<6;i++) {
        uint32_t edge=p.sector_at+1000u*FD_REV_US;
        assert(fdRevEdgePolicy(&p,edge,8)==(i==5));
        assert(p.observed_once && !p.reposition_used);
        if(i<5)assert(fdRevAdvancePolicy(&p,edge)==FD_REV_STEP);
        else assert(fdRevAdvancePolicy(&p,edge+p.delay)==FD_REV_HANDOFF);
    }
    assert(fdRevAdvancePolicy(&p,launch+60000u*FD_REV_US)==FD_REV_NONE);
    fdRevBeginPolicy(&p,start); align(&p); launch=p.sector_at;
    assert(fdRevAdvancePolicy(&p,launch+30000u*FD_REV_US)==FD_REV_RETRY);
    assert(fdRevAdvancePolicy(&p,p.sector_at+22500u*FD_REV_US)==FD_REV_RETRY);
    assert(!p.reposition_used && p.rescue_used);
    /* Regression: S4R previously interrupted the first16ms rescue at150ms. */
    assert(fdRevAdvancePolicy(&p,launch+60000u*FD_REV_US)==FD_REV_NONE);
    const unsigned rescue_us[]={16000,14000,12000,10000,9000,8000};
    for(unsigned i=0;i<6;i++) {
        assert(p.rescue_left==6u-i && p.period==rescue_us[i]*FD_REV_US);
        uint32_t end=p.sector_at+p.period;
        assert(fdRevAdvancePolicy(&p,end-1)==FD_REV_NONE);
        assert(fdRevAdvancePolicy(&p,end)==FD_REV_RETRY);
        assert(!p.reposition_used);
    }
    assert(p.sector_at-start==211500u*FD_REV_US && !p.rescue_left);
    while((uint32_t)(p.sector_at+p.period-start)<FD_REV_ACQUIRE_TICKS) {
        assert(fdRevAdvancePolicy(&p,p.sector_at+p.period)==FD_REV_RETRY);
        assert(!p.reposition_used);
    }
    /* Synthetic exhausted-rescue state ONLY to exercise dormant fallback:
     * real390ms timeline above never has enough budget to enter it. */
    uint32_t release=start+150000u*FD_REV_US;
    p=(FdRevPolicy){.stage=FD_REV_OPEN,.trial_started=start,
        .sector_at=release-FD_REV_RETRY_TICKS,.period=FD_REV_RETRY_TICKS,.rescue_used=1};
    assert(fdRevCanReposition(&p,release)); /* exact240ms remaining */
    assert(!fdRevCanReposition(&p,release+1));
    assert(!fdRevCanReposition(&p,release-1)); /* sector not expired */
    p.rescue_left=1;assert(!fdRevCanReposition(&p,release));p.rescue_left=0;
    p.observed_once=1;assert(!fdRevCanReposition(&p,release));p.observed_once=0;
    p.reposition_used=1;assert(!fdRevCanReposition(&p,release));p.reposition_used=0;
    assert(fdRevAdvancePolicy(&p,release)==FD_REV_RELEASE);
    assert(p.stage==FD_REV_RELAX && p.reposition_used);
    assert(!fdRevEdgePolicy(&p,release+100,8));
    assert(!p.observed_once);
    assert(fdRevAdvancePolicy(&p,release+20000u*FD_REV_US-1)==FD_REV_NONE);
    assert(fdRevAdvancePolicy(&p,release+20000u*FD_REV_US)==FD_REV_REALIGN);
    assert(p.trial_started==start);
    assert(p.reposition_used && p.rescue_used && !p.rescue_left);
    assert(fdDoubleAlignDuty(&p,p.sector_at)==210u);
    align(&p);
    for(unsigned i=0;i<8;i++)assert(fdRevAdvancePolicy(&p,p.sector_at+p.period)==FD_REV_RETRY);
    assert(p.reposition_used); /* never returns RELEASE again */
    assert(!fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS-1,start));
    assert(fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start));
    for(unsigned stage=FD_REV_ALIGN;stage<=FD_REV_RELAX;stage++) {
        p.stage=stage;
        assert(fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start)==(stage!=FD_REV_CLOSED));
    }
    /* Even one genuine edge disables this narrow cold-start fallback. */
    fdRevBeginPolicy(&p,start); align(&p); launch=p.sector_at;
    fdRevEdgePolicy(&p,launch+1000u*FD_REV_US,8);
    assert(fdRevAdvancePolicy(&p,p.edge_at)==FD_REV_STEP);
    for(unsigned i=0;i<15;i++)assert(fdRevAdvancePolicy(&p,p.sector_at+p.period)==FD_REV_RETRY);
    assert(p.observed_once && !p.reposition_used);
    /* A late rejected edge cannot suppress the no-edge fallback. */
    fdRevBeginPolicy(&p,start); align(&p);
    assert(!fdRevEdgePolicy(&p,p.sector_at+p.period,8));
    assert(!p.observed_once);
 }
 puts("PASS S4R2 full rescue preserved, insufficient budget skips retry, healthy feedback, synthetic fallback boundaries, immutable trial deadline, wrap");
 return 0;
}
