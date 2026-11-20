/* Policy boundary tests, not a mechanical motor model. */
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
#include "../Inc/fd6288_startup_sequence.h"
#ifdef TARGET_STARTUP_SEED_DELAY
enum { SEED_DELAY = 500u * FD_REV_US };
#else
enum { SEED_DELAY = 0u };
#endif
int main(void)
{
    for (unsigned wrap=0; wrap<2; wrap++) {
        for (unsigned adv=0; adv<256; adv++) {
            FdRevPolicy s;
            uint32_t start=wrap ? UINT32_MAX-10000u : 0u;
            fdRevBeginPolicy(&s,start);
            assert(fdRevAdvancePolicy(&s,start+FD_REV_RETRY_TICKS-1)==FD_REV_NONE);
            assert(fdRevAdvancePolicy(&s,start+FD_REV_RETRY_TICKS)==FD_REV_RETRY);
            assert(fdRevAdvancePolicy(&s,s.sector_at+FD_REV_RETRY_TICKS)==FD_REV_RETRY);
            assert(s.rescue_left==6 && s.period==FD_S4_RESCUE_TICKS);
            uint32_t edge=s.sector_at+1000u*FD_REV_US;
            fdRevEdgePolicy(&s,edge,adv);
            assert(s.confidence==1 && s.delay==SEED_DELAY && s.rescue_left==5);
            assert(!s.measured && !s.last_real_period);
            if(SEED_DELAY) {
                assert(fdRevAdvancePolicy(&s,edge+SEED_DELAY-1)==FD_REV_NONE);
                assert(!fdRevEdgePolicy(&s,edge+1,adv));
                assert(s.confidence==1 && s.delay==SEED_DELAY);
            }
            assert(fdRevAdvancePolicy(&s,edge+SEED_DELAY)==FD_REV_STEP);
            for(unsigned n=2;n<=6;n++) {
                edge+=12000u*FD_REV_US;
                fdRevEdgePolicy(&s,edge,adv);
                uint32_t expected=(12000u*FD_REV_US)/2u-
                    (12000u*FD_REV_US)*(adv>24?24:adv)/64u;
                if(n<6 && expected>3000u*FD_REV_US) expected=3000u*FD_REV_US;
                assert(s.delay==expected && s.confidence==n && !s.rescue_left);
                assert(!fdRevEdgePolicy(&s,edge+1,adv)); /* duplicate ignored */
                assert(s.confidence==n);
                assert(fdRevAdvancePolicy(&s,edge+s.delay-1)==FD_REV_NONE);
                assert(fdRevAdvancePolicy(&s,edge+s.delay)==
                    (n==6?FD_REV_HANDOFF:FD_REV_STEP));
            }
        }
    }
    /* A timeout invalidates the old pair: next seed uses only its seed rule. */
    FdRevPolicy s;
    fdRevBeginPolicy(&s,0);
    fdRevEdgePolicy(&s,160000,16);fdRevAdvancePolicy(&s,160000+SEED_DELAY);
    fdRevEdgePolicy(&s,480000,16);
    assert(s.delay==80000);
    fdRevAdvancePolicy(&s,560000);
    assert(fdRevAdvancePolicy(&s,560000+FD_REV_RETRY_TICKS)==FD_REV_RETRY);
    assert(!s.confidence && !s.last_real_period);
    fdRevEdgePolicy(&s,s.sector_at+160000,16);
    assert(s.confidence==1 && s.delay==SEED_DELAY);
    assert(!s.measured && !s.last_real_period);
    puts("PASS phase delay: all advance values, cap, wrap, duplicate, handoff and timeout reset");
}
