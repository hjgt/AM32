#include <assert.h>
#include <stdio.h>
#include "../Inc/fd6288_recovery_policy.h"
static uint32_t stable(FdRecoveryPolicy *s,uint32_t now,uint32_t period) {
    fdRecoveryReset(s);
    for(unsigned i=0;i<120;i++) {
        assert(fdRecoveryGate(s,1,0)==FD_REC_NONE);
        assert(fdRecoveryRealEdge(s,now)==FD_REC_NONE);
        assert(fdRecoveryTick(s,now,1000,210)==FD_REC_NONE);
        now+=period;
    }
    assert(s->eligible);return now-period;
}
int main(void) {
 for(unsigned wrap=0;wrap<2;wrap++) for(unsigned p=200;p<=2000;p+=200) {
    FdRecoveryPolicy s;uint32_t last=stable(&s,wrap?UINT32_MAX-10000:10000,p);
    uint32_t at=last+p+p/2;
    assert(fdRecoveryTick(&s,at-1,1000,210)==FD_REC_NONE);
    assert(fdRecoveryTick(&s,at,1000,210)==FD_REC_ENTER);
    assert(s.used && s.probes==0 && s.streak==0);
    assert(fdRecoveryDutyCap(&s,at,1000)==400);
    assert(fdRecoveryDutyCap(&s,at,200)==200); /* never increase operator demand */
    FdRecoveryPolicy timeout=s;
    assert(fdRecoveryTick(&timeout,timeout.started+timeout.budget,1000,210)==FD_REC_STOP);
    assert(fdRecoveryRealEdge(&timeout,timeout.started+timeout.budget)==FD_REC_STOP);
    assert(fdRecoveryDutyCap(&timeout,at,1000)==0);
    FdRecoveryPolicy probes=s;
    for(unsigned i=0;i<2;i++) {
        assert(fdRecoveryTick(&probes,probes.probe_at,1000,210)==FD_REC_PROBE);
        assert(probes.probes==i+1 && !probes.streak);
    }
    assert(fdRecoveryTick(&probes,probes.probe_at,1000,210)==FD_REC_STOP);
    FdRecoveryPolicy late=s;
    assert(fdRecoveryTick(&late,late.probe_at+p/4+1,1000,210)==FD_REC_STOP);
    FdRecoveryPolicy interrupted=s;
    assert(fdRecoveryGate(&interrupted,1,1)==FD_REC_STOP);
    assert(fdRecoveryGate(&interrupted,1,0)==FD_REC_STOP);
    assert(fdRecoveryGate(&interrupted,0,0)==FD_REC_CANCEL);
    assert(!interrupted.used && !interrupted.eligible);
    /* Six real edges, not probes or duplicates, earn a ramp. Use a period
     * inside the allowed range; even the slowest completes before 10ms. */
    uint32_t t=at+p/4;
    for(unsigned i=0;i<6;i++) {
        assert(fdRecoveryRealEdge(&s,t)==(i==5?FD_REC_REGAINED:FD_REC_NONE));
        assert(fdRecoveryRealEdge(&s,t)==FD_REC_NONE);
        assert(s.streak==i+1);
        if(i<5)t+=p*3/4;
    }
    assert(s.state==FD_REC_RAMP && s.used);
    assert(fdRecoveryDutyCap(&s,t,1000)==400);
    uint16_t prev=400;uint32_t ramp=t;
    while((uint32_t)(t-ramp)<FD_REC_RELEASE_US) {
        t+=p;
        assert(fdRecoveryRealEdge(&s,t)==FD_REC_NONE);
        uint16_t cap=fdRecoveryDutyCap(&s,t,1000);
        assert(cap>=prev && cap<=1000);prev=cap;
        uint8_t a=fdRecoveryTick(&s,t,1000,210);
        assert(a==((uint32_t)(t-ramp)>=FD_REC_RELEASE_US?FD_REC_FINISHED:FD_REC_NONE));
    }
    assert(s.used && fdRecoveryDutyCap(&s,t,1000)==1000);
    assert(fdRecoveryTick(&s,t+p+p/2,1000,210)==FD_REC_STOP); /* no renewed budget */
    last=stable(&s,1000,p);
    assert(fdRecoveryTick(&s,last+2*p,1000,210)==FD_REC_STOP); /* stale estimate */
    last=stable(&s,1000,p);
    assert(fdRecoveryTick(&s,last+p+p/2,400,210)==FD_REC_STOP); /* ADC window floor */
 }
 FdRecoveryPolicy s;fdRecoveryReset(&s);
 for(unsigned i=0;i<6;i++) fdRecoveryRealEdge(&s,1000+i*1000);
 assert(!s.eligible && fdRecoveryTick(&s,50000,1000,210)==FD_REC_NONE);
 puts("PASS recovery policy: eligibility, bounded probes/time, true-edge qualification, cap/ramp, wrap, stale/late rejection, zero/fault, no budget refill");
}
