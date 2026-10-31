#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_SEQUENCE_V2
#define TARGET_STARTUP_FEEDBACK
#define TARGET_STARTUP_S4
#define TARGET_STARTUP_S4E
#define TARGET_STARTUP_HANDOFF_REAL
#define TARGET_STARTUP_FIRST_WAIT
#define TARGET_FD_DOUBLE_ALIGN
#define TARGET_FD_ALIGN_HOLD
#define TARGET_FD_ALIGN_BUDGET
#define TARGET_FD_ALIGN_SETTLE
#define TARGET_FD_LAUNCH_ADVANCE
#define TARGET_FD_PLANNED_ACQ
#define TARGET_FD_NO_DIAGNOSTICS
#include "../Inc/fd6288_startup_sequence.h"
static FdRevPolicy launch(uint32_t t) {
    FdRevPolicy p; fdRevBeginPolicy(&p,t);
    assert(fdRevAdvancePolicy(&p,t+FD_REV_ALIGN_TICKS-1)==FD_REV_NONE);
    assert(fdRevAdvancePolicy(&p,t+FD_REV_ALIGN_TICKS)==FD_REV_STEP);
    assert(fdRevAdvancePolicy(&p,t+FD_REV_ALIGN_TICKS+FD_REV_SECOND_ALIGN_TICKS)==FD_REV_LAUNCH);
    assert(p.period==16000u*FD_REV_US); return p;
}
int main(void) {
 for(unsigned wrap=0;wrap<2;wrap++) {
    uint32_t start=wrap?UINT32_MAX-1000u:1000u;
    FdRevPolicy p=launch(start);
    assert(!fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS-1,start));
    assert(fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start));
    for(unsigned i=0;i<30;i++) {
        uint32_t end=p.sector_at+p.period;
        assert(fdRevAdvancePolicy(&p,end-1)==FD_REV_NONE);
        assert(fdRevAdvancePolicy(&p,end)==FD_REV_PLANNED);
        assert(fdRevAdvancePolicy(&p,end)==FD_REV_NONE);
        assert(!p.confidence && !p.measured);
        assert(p.period>=8000u*FD_REV_US);
    }
    p=launch(start);
    for(unsigned i=0;i<6;i++) {
        uint32_t edge=p.sector_at+p.period*5u/8u;
        assert(fdRevEdgePolicy(&p,edge,8)==(i==5));
        assert(p.confidence==i+1);
        assert(!fdRevEdgePolicy(&p,edge+1,8));
        if(i<5) {
            assert(p.stage==FD_REV_OPEN);
            assert(fdRevAdvancePolicy(&p,p.sector_at+p.period)==FD_REV_PLANNED);
        } else {
            assert(fdRevAdvancePolicy(&p,edge+p.delay-1)==FD_REV_NONE);
            assert(fdRevAdvancePolicy(&p,edge+p.delay)==FD_REV_HANDOFF);
            assert(p.stage==FD_REV_CLOSED && p.last_real_period);
        }
    }
    p=launch(start);
    fdRevEdgePolicy(&p,p.sector_at+p.period/2,8);
    fdRevAdvancePolicy(&p,p.sector_at+p.period);
    fdRevAdvancePolicy(&p,p.sector_at+p.period);
    assert(!p.confidence); /* missing sector breaks adjacency */
    fdRevEdgePolicy(&p,p.sector_at+1,8);
    assert(!p.confidence); /* switching boundary cannot seed */
    p=launch(start);
    for(unsigned i=0;i<8;i++) {
        /* Coherent edges but unsuitable handoff phase: keep planned cadence. */
        fdRevEdgePolicy(&p,p.sector_at+p.period/4,8);
        assert(p.stage==FD_REV_OPEN);
        fdRevAdvancePolicy(&p,p.sector_at+p.period);
    }
    assert(p.confidence==6);
    assert(fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start));
    p.stage=FD_REV_PENDING;
    assert(fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start));
 }
 puts("PASS planned cadence, six-edge handoff, phase rejection, missing edge, wrap");
}
