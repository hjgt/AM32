#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_SEQUENCE_V2
#define TARGET_STARTUP_FEEDBACK
#define TARGET_STARTUP_S4
#define TARGET_STARTUP_S4E
#define TARGET_STARTUP_HANDOFF_REAL
#define TARGET_STARTUP_RESCUE_RETAIN
#define TARGET_STARTUP_RESCUE_BRIDGE
#define TARGET_STARTUP_PHASE_DELAY
#define TARGET_STARTUP_SEED_DELAY
#define TARGET_STARTUP_RESCUE_RAMP
#define TARGET_STARTUP_FIRST_WAIT
#define TARGET_FD_DOUBLE_ALIGN
#include "../Inc/fd6288_startup_sequence.h"
static unsigned next(unsigned step, unsigned forward) {
    return forward ? (step == 6 ? 1 : step + 1) : (step == 1 ? 6 : step - 1);
}
int main(void) {
    for(unsigned wrap=0;wrap<2;wrap++) for(unsigned forward=0;forward<2;forward++) {
        uint32_t start=wrap ? UINT32_MAX-1000u : 1000u;
        FdRevPolicy p;
        fdRevBeginPolicy(&p,start);
        unsigned step=next(fdDoubleAlignSeed(forward),forward);
        assert(step==(forward?5u:3u));
        assert(p.stage==FD_REV_ALIGN && p.align_left==2 && !p.confidence);
        for(unsigned phase=0;phase<2;phase++) {
            uint32_t at=p.sector_at;
            uint32_t dwell=phase?FD_REV_SECOND_ALIGN_TICKS:FD_REV_ALIGN_TICKS;
            for(unsigned us=0;us<dwell/FD_REV_US;us+=100) {
                uint32_t t=at+us*FD_REV_US;
                assert(!fdRevEdgePolicy(&p,t,13));
                assert(!p.confidence && !p.seen);
                assert(fdRevAdvancePolicy(&p,t)==FD_REV_NONE);
                unsigned duty=fdDoubleAlignDuty(&p,t);
                assert(duty>=210 && duty<=360);
#ifdef TARGET_FD_ALIGN_HOLD
                if(phase==1) assert(duty==360);
                if(!us && !phase) assert(duty==210);
#else
                if(!us) assert(duty==210);
#endif
                if(us>=10000) assert(duty==360);
            }
#ifdef TARGET_FD_LAUNCH_ADVANCE
            assert(fdRevAdvancePolicy(&p,at+dwell)==(phase?FD_REV_LAUNCH:FD_REV_STEP));
            if(phase)step=fdDoubleAlignLaunchSeed();
#else
            assert(fdRevAdvancePolicy(&p,at+dwell)==FD_REV_STEP);
#endif
            step=next(step,forward);
            assert(fdRevAdvancePolicy(&p,at+dwell)==FD_REV_NONE);
            assert(!p.confidence && !p.empty_retries && !p.rescue_used);
        }
#ifdef TARGET_FD_LAUNCH_ADVANCE
        assert(step==(forward?2u:6u));
#else
        assert(step==1);
#endif
        assert(p.stage==FD_REV_OPEN && !p.align_left);
        assert(p.sector_at==start+FD_REV_ALIGN_TICKS+FD_REV_SECOND_ALIGN_TICKS);
        assert(!fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS-1,start));
        assert(fdRevDeadlinePolicy(&p,start+FD_REV_ACQUIRE_TICKS,start));
#ifdef TARGET_FD_ALIGN_BUDGET
        assert(FD_REV_ACQUIRE_TICKS==300000u*FD_REV_US+FD_REV_ALIGN_TICKS+FD_REV_SECOND_ALIGN_TICKS);
        assert(!fdRevDeadlinePolicy(&p,start+300000u*FD_REV_US,start));
        /* No-edge retries cannot refresh the absolute deadline. */
        FdRevPolicy stalled=p;
        for(unsigned us=(FD_REV_ALIGN_TICKS+FD_REV_SECOND_ALIGN_TICKS)/FD_REV_US;us<=FD_REV_ACQUIRE_TICKS/FD_REV_US;us+=1000)
            (void)fdRevAdvancePolicy(&stalled,start+us*FD_REV_US);
        assert(fdRevDeadlinePolicy(&stalled,start+FD_REV_ACQUIRE_TICKS,start));
        stalled.stage=FD_REV_PENDING; stalled.confidence=5;
        assert(fdRevDeadlinePolicy(&stalled,start+FD_REV_ACQUIRE_TICKS,start));
#else
        assert(FD_REV_ACQUIRE_TICKS==300000u*FD_REV_US);
#endif
        /* Real edges, not positioning steps, alone earn handoff. */
        for(unsigned n=1;n<=6;n++) {
            uint32_t t=p.sector_at+8000u*FD_REV_US;
            fdRevEdgePolicy(&p,t,13);
            assert(p.confidence==n && p.stage==FD_REV_PENDING);
#ifdef TARGET_FD_NATIVE_EARLY
#ifdef TARGET_FD_FIRST_EDGE_500US
            if(n==1) assert(p.delay==500u*FD_REV_US);
            else if(n<6) assert(p.delay==0);
#else
            if(n<6) assert(p.delay==0);
#endif
            else assert(p.delay>0);
#endif
            if(p.delay) assert(fdRevAdvancePolicy(&p,t+p.delay-1)==FD_REV_NONE);
            assert(fdRevAdvancePolicy(&p,t+p.delay)==(n==6?FD_REV_HANDOFF:FD_REV_STEP));
        }
        assert(p.stage==FD_REV_CLOSED);
        assert(!fdRevDeadlinePolicy(&p,start+300000u*FD_REV_US,start));
        /* Every new trial restarts both positioning phases, no stale lock. */
        fdRevBeginPolicy(&p,start);
        assert(p.align_left==2 && !p.confidence && !p.rescue_used);
    }
    puts("PASS double alignment: both directions, exact dwell, ramp, ignored edges, wrap, deadline, real-edge handoff, restart");
}
