#ifndef TARGET_FD_FULL_RETRY
#define TARGET_FD_FULL_RETRY
#endif
#define main unusedLegacyPolicyTests
#include "fd6288_reposition_tests.c"
#undef main
static int fullRetryPolicyTests(void) {
 for(unsigned forward=0;forward<2;forward++) {
    unsigned seed=fdRetryLaunchSeed(forward);
    unsigned physical=forward?(seed==6?1:seed+1):(seed==1?6:seed-1);
#ifdef TARGET_FD_RETRY_ADJACENT
    assert(physical==3u);
    assert(seed==(forward?2u:4u));
#else
    assert(physical==(forward?4u:2u));
#endif
    assert(fdDoubleAlignLaunchSeed()==1u); /* first attempt unchanged */
 }
 for(unsigned wrap=0;wrap<2;wrap++) {
    uint32_t start=wrap?UINT32_MAX-1000u:0u;
#ifdef TARGET_FD_STALE_RETRY
    /* Trial9-like trace: one edge near175ms, then none. No motor simulation. */
    FdRevPolicy replay;fdRevBeginPolicy(&replay,start);
    unsigned releases=0;
    for(unsigned us=0;us<=820000u;us+=10u) {
        uint32_t now=start+us*FD_REV_US;
        if(fdRevDeadlinePolicy(&replay,now,start)) {assert(us==820000u);break;}
        if(us==174980u) {
            fdRevEdgePolicy(&replay,now,8);
            assert(replay.seen && !replay.missing_sectors);
        }
        unsigned action=fdRevAdvancePolicy(&replay,now);
        if(action==FD_REV_RELEASE) {releases++;assert(us>=390000u && us<410000u);}
    }
    assert(releases==1u);
    uint32_t at=start+390000u*FD_REV_US;
    FdRevPolicy eligibility={.stage=FD_REV_OPEN,.observed_once=1,.rescue_used=1,
        .missing_sectors=3,.edge_at=at-3u*FD_REV_RETRY_TICKS};
    assert(fdRevRetryEligible(&eligibility,at));
    assert(!fdRevRetryEligible(&eligibility,at-1));
    eligibility.missing_sectors=2;assert(!fdRevRetryEligible(&eligibility,at));
    eligibility.missing_sectors=3;eligibility.confidence=1;assert(!fdRevRetryEligible(&eligibility,at));
    eligibility.confidence=0;eligibility.seen=1;assert(!fdRevRetryEligible(&eligibility,at));
    eligibility.seen=0;eligibility.stage=FD_REV_PENDING;assert(!fdRevRetryEligible(&eligibility,at));
    eligibility.stage=FD_REV_CLOSED;assert(!fdRevRetryEligible(&eligibility,at));
    eligibility.stage=FD_REV_OPEN;eligibility.rescue_left=1;assert(!fdRevRetryEligible(&eligibility,at));
    eligibility.rescue_left=0;eligibility.reposition_used=1;assert(!fdRevRetryEligible(&eligibility,at));
    /* A fresh accepted edge immediately revokes stale-feedback eligibility. */
    eligibility.reposition_used=0;eligibility.sector_at=at;eligibility.period=FD_REV_RETRY_TICKS;
    fdRevEdgePolicy(&eligibility,at+FD_REV_US,8);
    assert(!eligibility.missing_sectors && !fdRevRetryEligible(&eligibility,at+FD_REV_US));
#endif
    FdRevPolicy p;fdRevBeginPolicy(&p,start);align(&p);
    while ((uint32_t)(p.sector_at+p.period-start)<FD_REV_ACQUIRE_TICKS) {
        uint32_t end=p.sector_at+p.period;
        assert(!fdRevDeadlinePolicy(&p,end,start));
        assert(fdRevAdvancePolicy(&p,end)==FD_REV_RETRY);
        assert(!p.reposition_used);
    }
    assert(!fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start));
    uint32_t release=p.sector_at+p.period;
    assert(release-start==391500u*FD_REV_US);
    assert(fdRevAdvancePolicy(&p,release)==FD_REV_RELEASE);
    assert(p.stage==FD_REV_RELAX && p.reposition_used);
    assert(!fdRevEdgePolicy(&p,release+100,8));
    assert(fdRevAdvancePolicy(&p,release+FD_REPOSITION_RELEASE_TICKS-1)==FD_REV_NONE);
    assert(fdRevAdvancePolicy(&p,release+FD_REPOSITION_RELEASE_TICKS)==FD_REV_REALIGN);
    assert(p.trial_started==start && p.reposition_used && !p.rescue_used);
    align(&p);
    assert(p.sector_at-start==501500u*FD_REV_US);
    FdRevPolicy recovered=p;
    for(unsigned i=0;i<6;i++) {
        uint32_t edge=recovered.sector_at+1000u*FD_REV_US;
        fdRevEdgePolicy(&recovered,edge,8);
        assert(fdRevAdvancePolicy(&recovered,edge+recovered.delay)==(i==5?FD_REV_HANDOFF:FD_REV_STEP));
    }
    assert(recovered.reposition_used && !fdRevDeadlinePolicy(&recovered,start+FD_FULL_RETRY_END,start));
    /* Second attempt gets one finite six-sector rescue, not infinite renewal. */
    for(unsigned i=0;i<2;i++)assert(fdRevAdvancePolicy(&p,p.sector_at+p.period)==FD_REV_RETRY);
    assert(p.rescue_used && p.rescue_left==6);
    for(unsigned i=0;i<6;i++)assert(fdRevAdvancePolicy(&p,p.sector_at+p.period)==FD_REV_RETRY);
    assert(!p.rescue_left);
    while ((uint32_t)(p.sector_at+p.period-start)<FD_FULL_RETRY_END)
        assert(fdRevAdvancePolicy(&p,p.sector_at+p.period)==FD_REV_RETRY);
    assert(!fdRevCanReposition(&p,p.sector_at+p.period));
    assert(!fdRevDeadlinePolicy(&p,start+FD_FULL_RETRY_END-1,start));
    assert(fdRevDeadlinePolicy(&p,start+FD_FULL_RETRY_END,start));
    for(unsigned stage=FD_REV_ALIGN;stage<=FD_REV_RELAX;stage++) {
        p.stage=stage;
        assert(fdRevDeadlinePolicy(&p,start+FD_FULL_RETRY_END,start)==(stage!=FD_REV_CLOSED));
    }
    /* Do not turn a partial feedback failure into an unqualified restart. */
    fdRevBeginPolicy(&p,start);align(&p);
    fdRevEdgePolicy(&p,p.sector_at+1000u*FD_REV_US,8);
    fdRevAdvancePolicy(&p,p.edge_at);
    assert(p.observed_once);
    assert(fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start));
    assert(!fdRevCanReposition(&p,start+FD_REV_ACQUIRE_TICKS));
    /* Missed entry window never grants a late full-power restart. */
    p.observed_once=0;p.rescue_used=1;p.rescue_left=0;
    assert(!fdRevDeadlinePolicy(&p,start+FD_FULL_RETRY_ENTRY_END-1,start));
    assert(fdRevDeadlinePolicy(&p,start+FD_FULL_RETRY_ENTRY_END,start));
    assert(!fdRevCanReposition(&p,start+FD_FULL_RETRY_ENTRY_END));
    /* Healthy first attempt still uses native early edges and six-edge handoff. */
    fdRevBeginPolicy(&p,start);align(&p);
    for(unsigned i=0;i<6;i++) {
        uint32_t edge=p.sector_at+1000u*FD_REV_US;
        fdRevEdgePolicy(&p,edge,8);
        assert(fdRevAdvancePolicy(&p,edge+p.delay)==(i==5?FD_REV_HANDOFF:FD_REV_STEP));
    }
    assert(!p.reposition_used && !fdRevDeadlinePolicy(&p,start+FD_FULL_RETRY_END,start));
 }
 puts("PASS S4T complete first attempt, finite second rescue, absolute cutoff, wrap, healthy handoff, partial-edge exclusion");
 return 0;
}
