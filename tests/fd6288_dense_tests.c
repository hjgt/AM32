int main(int argc,char **argv) {
    assert(argc==2);
    timer.CYCCNT=0xfffff000u; fdTraceInit();
    assert(fd_startup_wave.abi==3 && sizeof(fd_startup_wave)==21568);
    fdTraceTrial(1);fdTraceBegin();
    for(unsigned i=0;i<1400;i++) {
        timer.CYCCNT+=6667;
        fdTraceSample(1500,10,780,0,i%10,i%2,15);
        if(i%10==0)fdTraceEvent(FD_EVENT_COMMUTATE,0);
    }
    assert(fd_startup_wave.stored==1280 && fd_startup_wave.seen==1400);
    assert(fd_startup_wave.samples[1279].at==1280u*6667u);
    assert(fd_startup_wave.samples[0].at==6667);
    assert((fd_startup_wave.samples[0].state&7)==7); /* saturated confirmations */
    assert(fd_startup_wave.events==140);
    assert(fd_startup_wave.events_ring[47].at==471u*6667u);
    fdTraceTrialFinish(8,2);
    fdTraceSample(1,2,3,0,0,0,0);assert(fd_startup_wave.seen==1400);
    /* latest trial replaces previous; unused array suffix is ignored */
    fdTraceTrial(7);fdTraceBegin();
    for(unsigned i=0;i<1300;i++) {
        step=i<720?1:2; rising=step%2;
        timer.CYCCNT+=6667;
        fdTraceSample(1500,10,780,0,i%10,i%2,2);
        if(i==0||i==720)fdTraceEvent(FD_EVENT_COMMUTATE,0);
    }
    fdTraceNoFresh();fdTraceTrialFinish(8,2);
    assert(fd_startup_wave.trial_id==7 && fd_startup_wave.events==2 && fd_startup_wave.no_fresh==1);
    assert(fd_startup_wave.samples[719].meta%8==1 && fd_startup_wave.samples[720].meta%8==2);
    assert(fd_startup_wave.active_bank==FD_WAVE_NONE && fd_startup_wave.state==FD_WAVE_NO_LOCK);
    assert(pwm.CCR1==1334 && pwm.CCR4==667 && duty_cycle==400 && waitTime==500 && !mask);
    FILE *f=fopen(argv[1],"wb");assert(f);assert(fwrite((const void*)&fd_startup_wave,1,sizeof(fd_startup_wave),f)==sizeof(fd_startup_wave));fclose(f);
    return 0;
}
