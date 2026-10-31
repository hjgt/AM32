/* Appended to existing register stubs plus the ACTUAL logger implementation. */
static void save(const char *file) {
    FILE *f=fopen(file,"wb"); assert(f);
    assert(fwrite((const void *)&fd_startup_wave,1,sizeof(fd_startup_wave),f)==sizeof(fd_startup_wave));
    assert(!fclose(f));
}
static unsigned prefix_count(volatile FdWaveBank *b) { return b->samples_stored&65535; }
static unsigned ring_count(volatile FdWaveBank *b) { return b->samples_stored>>16; }
int main(int argc,char **argv) {
    assert(argc==2);
    reset();begin(1); /* Starts near DWT wrap. */
    for(unsigned i=0;i<1000;i++) sample(FD_SAMPLE_PRE);
    volatile FdWaveBank *b=&fd_startup_wave.banks[0];
    assert(prefix_count(b)==125 && ring_count(b)==500);
    FdWaveSample prefix[128]; memcpy(prefix,(const void*)b->samples,sizeof(prefix));
    for(unsigned i=0;i<2400;i++) sample(FD_SAMPLE_PRE); /* 170ms */
    sample(FD_SAMPLE_ACCEPT);
    uint32_t first=b->reserved[0];
    for(unsigned i=0;i<130;i++) sample(FD_SAMPLE_PRE);
    uint32_t frozen=b->samples_stored;
    assert(b->last_sample_at<=first+FD_WAVE_SPLIT_POST);
    assert(first+FD_WAVE_SPLIT_POST-b->last_sample_at<FD_WAVE_SAMPLE_TICKS);
    for(unsigned i=0;i<1000;i++) sample(FD_SAMPLE_PRE);
    assert(frozen==b->samples_stored);
    assert(!memcmp(prefix,(const void*)b->samples,sizeof(prefix)));
    for(unsigned i=0;i<120;i++) fdTraceEvent(FD_EVENT_COMMUTATE,0);
    assert(b->events==122 && b->events_ring[0].at<10000);
    assert(b->events_ring[1].kind==FD_WAVE_ADC_ACCEPT);
    timer.CYCCNT=b->started+48000000u;finish(8,2);
    FdWaveBank pinned;memcpy(&pinned,(const void*)b,sizeof(pinned));
    begin(2);sample(FD_SAMPLE_PRE);finish(7,2);
    begin(3);finish(7,2);
    begin(4);sample(FD_SAMPLE_ACCEPT);
    fdTraceEvent(FD_EVENT_REV_HANDOFF,0);
    volatile FdWaveBank *loss=&fd_startup_wave.banks[1];
    unsigned ring=ring_count(loss);
    for(unsigned i=0;i<1200;i++) sample(FD_SAMPLE_PRE);
    assert(ring_count(loss)==ring && prefix_count(loss)==125);
    finish(9,4);
    begin(5);sample(FD_SAMPLE_PRE);finish(7,2);
    begin(6);sample(FD_SAMPLE_ACCEPT);fdTraceEvent(FD_EVENT_REV_HANDOFF,0);
    timer.CYCCNT+=160000000u;finish(1,4);
    begin(7);assert(fd_startup_wave.active_bank==FD_WAVE_NONE);
    assert(!memcmp(&pinned,(const void*)b,sizeof(pinned)));
    save(argv[1]);

    /* Full no-edge bound, no prefix overwrite, and re-use clears packed count. */
    reset();begin(1);
    for(unsigned i=0;i<6000;i++) sample(FD_SAMPLE_PRE);
    b=&fd_startup_wave.banks[0];
    assert(prefix_count(b)==125 && ring_count(b)==3000);
    frozen=b->samples_stored;
    for(unsigned i=0;i<500;i++) sample(FD_SAMPLE_PRE);
    assert(frozen==b->samples_stored);
    finish(7,2);begin(2);assert(b->samples_stored==0);
    sample(FD_SAMPLE_ACCEPT);
    fdTraceEvent(FD_EVENT_TIMEOUT,0);
    ring=ring_count(b);
    for(unsigned i=0;i<1000;i++) sample(FD_SAMPLE_PRE);
    assert(prefix_count(b)==125 && ring_count(b)==ring);
    /* Pending interrupt finish must not append after publication. */
    frozen=b->samples_stored;preempt=interrupt_finish;sample(FD_SAMPLE_PRE);
    assert(b->samples_stored==frozen && fd_startup_wave.active_bank==FD_WAVE_NONE);
    assert(pwm.CCR1==1334 && pwm.CCR4==667 && duty_cycle==400);
    puts("PASS split logger: immutable prefix, late/early/no edge, timed and event freeze, wrap, reuse, pinning, preemption, event overflow; drive registers unchanged");
}
