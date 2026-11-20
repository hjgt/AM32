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
#include "../Inc/fd6288_startup_sequence.h"
static void expire(FdRevPolicy *s) {
    uint32_t at = s->sector_at + s->period;
    assert(fdRevAdvancePolicy(s, at - 1) == FD_REV_NONE);
    assert(fdRevAdvancePolicy(s, at) == FD_REV_RETRY);
    assert(fdRevAdvancePolicy(s, at) == FD_REV_NONE);
}
int main(void) {
#ifdef TARGET_STARTUP_FIRST_WAIT
    for(unsigned wrap=0;wrap<2;wrap++) {
        uint32_t start=wrap?UINT32_MAX-1000u:0u;
        FdRevPolicy p;
        fdRevBeginPolicy(&p,start);
        assert(p.period==30000u*FD_REV_US);
        assert(fdRevAdvancePolicy(&p,start+22500u*FD_REV_US)==FD_REV_NONE);
        expire(&p);
        assert(p.period==FD_REV_RETRY_TICKS && p.empty_retries==1 && !p.rescue_used);
        expire(&p);assert(p.rescue_left==6);
        for(unsigned i=0;i<6;i++)expire(&p);
        assert(p.period==FD_REV_RETRY_TICKS && p.rescue_used);
        for(unsigned us=1000;us<=29999;us+=1000) {
            fdRevBeginPolicy(&p,start);
            uint32_t at=start+us*FD_REV_US;
            fdRevEdgePolicy(&p,at,24);
            assert(p.confidence==1 && p.stage==FD_REV_PENDING);
            assert(p.delay==500u*FD_REV_US && p.period==FD_REV_RETRY_TICKS);
            assert(fdRevAdvancePolicy(&p,at+p.delay)==FD_REV_STEP);
            expire(&p);assert(p.period==FD_REV_RETRY_TICKS);
        }
        fdRevBeginPolicy(&p,start);
        fdRevEdgePolicy(&p,start+30000u*FD_REV_US,24);
        assert(p.confidence==0 && p.stage==FD_REV_OPEN);
        assert(!fdRevDeadlinePolicy(&p,start+300000u*FD_REV_US-1u,start));
        assert(fdRevDeadlinePolicy(&p,start+300000u*FD_REV_US,start));
    }
#endif
    const unsigned us[] = {16000,14000,12000,10000,9000,8000};
    for (unsigned wrap=0; wrap<2; wrap++) {
        FdRevPolicy s;
        fdRevBeginPolicy(&s, wrap ? UINT32_MAX-1000 : 0);
        expire(&s); assert(!s.rescue_used);
        expire(&s); assert(s.rescue_left==6);
        for(unsigned i=0;i<6;i++) {
            assert(s.period==us[i]*FD_REV_US);
            expire(&s);
        }
        assert(!s.rescue_left && s.rescue_used && s.period==FD_REV_RETRY_TICKS);
        for(unsigned i=0;i<20;i++) {expire(&s);assert(!s.rescue_left);}
        fdRevBeginPolicy(&s,0); expire(&s);expire(&s);
        uint32_t at=s.sector_at+1000*FD_REV_US;
        fdRevEdgePolicy(&s,at,24);
        assert(s.rescue_left==5 && s.delay==500*FD_REV_US);
        assert(fdRevAdvancePolicy(&s,at+s.delay)==FD_REV_STEP);
        assert(s.period==FD_REV_RETRY_TICKS);
        expire(&s);assert(s.rescue_left==4 && s.period==12000*FD_REV_US);
        at=s.sector_at+1000*FD_REV_US;
        fdRevEdgePolicy(&s,at,24);fdRevAdvancePolicy(&s,at+s.delay);
        at=s.sector_at+1000*FD_REV_US;
        fdRevEdgePolicy(&s,at,24);
        assert(!s.rescue_left && s.confidence==2);
        fdRevBeginPolicy(&s,0);assert(!s.rescue_used && !s.rescue_left);
        for(unsigned i=0;i<6;i++) {
            at=s.sector_at+1000*FD_REV_US;
            fdRevEdgePolicy(&s,at,24);
            assert(fdRevAdvancePolicy(&s,at+s.delay)==(i==5?FD_REV_HANDOFF:FD_REV_STEP));
        }
        assert(s.stage==FD_REV_CLOSED && !s.rescue_used);
    }
    puts("S4H rescue ramp: cadence, wrap, one-shot budget, edge interruption and normal handoff PASS");
}
