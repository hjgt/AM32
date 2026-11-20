#include <assert.h>
#include <stdio.h>
#include "fd6288_m2_run.h"
int main(void)
{
    FdM2CrossingTime t={0};uint32_t start=0xfffff000u;
    fdM2CrossingSample(&t,start,-3);fdM2CrossingSample(&t,start+6576u,283);
    assert(t.crossing&&t.crossing_at==start+68u);
    assert(fdM2CrossingDelay(&t,start+6576u,1000u)==6508u);
    assert(fdM2CrossingDelay(&t,start+6576u,21u)==80u);
    assert(!fdM2CrossingDelay(&t,start+6576u,20u));
    assert(!fdM2CrossingDelay(&t,t.crossing_at+16001u,1000u));
    fdM2CrossingSample(&t,start+8000u,-30);assert(!t.crossing);
    t=(FdM2CrossingTime){0};fdM2CrossingSample(&t,0,-4095);
    fdM2CrossingSample(&t,9000u,4095);assert(t.crossing_at==4500u);
    t=(FdM2CrossingTime){0};fdM2CrossingSample(&t,0,-30);
    fdM2CrossingSample(&t,9001u,50);assert(!t.crossing);
    assert(fdM2StablePreTime(4555u,6501u,3663u,2998u,5996u,460u,244u));
    assert(!fdM2StablePreTime(490u,6501u,4072u,2938u,5876u,460u,244u));
    assert(!fdM2StablePreTime(3199u,6501u,3663u,2998u,5996u,460u,244u));
    assert(!fdM2StablePreTime(9001u,6501u,3663u,2998u,5996u,460u,244u));
    assert(!fdM2StablePreTime(4555u,9001u,3663u,2998u,5996u,460u,244u));
    assert(!fdM2StablePreTime(4555u,6501u,3200u,2998u,5996u,460u,244u));
    assert(!fdM2StablePreTime(4555u,6501u,6000u,2998u,5996u,460u,244u));
    assert(!fdM2StablePreTime(4555u,6501u,3663u,400u,5996u,460u,244u));
    assert(!fdM2StablePreTime(4555u,6501u,6933u,2998u,7500u,460u,244u));
    puts("PASS captured near-zero interpolation, age and scheduling lead, wraps, stale brackets and settled PRE timing guards");
}
