/* Actual logger, deterministic register stubs; NOT a motor simulation. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_TRACE
#define TARGET_STARTUP_SEQUENCE
#define TARGET_STARTUP_PAIR_TRACE
#define TARGET_STARTUP_WAVE_TRACE
#ifndef FD6288_TRACE_VARIANT
#define FD6288_TRACE_VARIANT 9u
#endif
#if FD6288_TRACE_VARIANT == 15
#define TARGET_STARTUP_ACQ_LOG
#elif FD6288_TRACE_VARIANT >= 16
#define TARGET_STARTUP_EDGE_LOG
#endif
#if FD6288_TRACE_VARIANT >= 18
#define TARGET_STARTUP_LOSS_LOG
#endif
#if FD6288_TRACE_VARIANT == 21
#define TARGET_STARTUP_NOEDGE_LOG
#endif
char step = 1, old_routine = 1, forward = 1;
volatile char rising = 1;
uint8_t running = 1;
volatile uint8_t heli_coast_active;
volatile uint16_t duty_cycle = 400, waitTime = 500;
volatile uint32_t commutation_interval = 2000;
uint16_t adjusted_input = 198;
static struct { uint32_t CTRL, CYCCNT; } timer;
static struct { uint32_t DEMCR; } core;
static struct { uint32_t CCR1, CCR4; } pwm = {1334, 667};
#define DWT (&timer)
#define CoreDebug (&core)
#define TIM1 (&pwm)
#define INTERVAL_TIMER_COUNT 2000u
#define CoreDebug_DEMCR_TRCENA_Msk 1u
#define DWT_CTRL_CYCCNTENA_Msk 1u
#define __DMB() ((void)0)
static uint32_t mask;
static void (*preempt)(void);
static uint32_t __get_PRIMASK(void) { return mask; }
static void __disable_irq(void) {
    if (!mask && preempt) { void (*p)(void) = preempt; preempt = 0; p(); }
    mask = 1u;
}
static void __set_PRIMASK(uint32_t m) { mask = m; }
/* WAVE_IMPLEMENTATION */
static void reset(void)
{
    memset((void *)&fd_startup_wave, 0, sizeof(fd_startup_wave));
    fd_wave_begun = fd_wave_stage = 0u;
    timer.CYCCNT = 0xfffff000u; mask = 0u;
    fdTraceInit();
}
static void begin(uint32_t id) {
    fdTraceTrial(id); fdTraceBegin(); fdTraceEvent(FD_EVENT_COMMUTATE, 0);
}
static void sample(uint8_t reason) {
    timer.CYCCNT += 8000u; // 50 us calls, 100 us recorded waveform
    fdTraceSample(1500,10,780,27,reason,1u,2u);
    assert(!mask);
}
static void finish(uint8_t reason,uint8_t stage) { fdTraceTrialFinish(reason,stage); }
static void interrupt_finish(void) { finish(8u,2u); }
int main(int argc,char **argv) {
    reset(); begin(1);
    for(unsigned i=0;i<1200;i++) {
        sample(i%20==0?FD_SAMPLE_ACCEPT:FD_SAMPLE_PRE);
        fdTraceNoFresh();
        if(i%20==0) fdTraceEvent(FD_EVENT_COMMUTATE,0);
    }
    volatile FdWaveBank *b=&fd_startup_wave.banks[0];
    assert(b->samples_seen==1200 && b->accepted==60);
#ifdef TARGET_STARTUP_ACQ_LOG
    assert(b->samples_stored==50);
    uint32_t first=0;
#elif defined(TARGET_STARTUP_EDGE_LOG) && !defined(TARGET_STARTUP_LOSS_LOG)
    assert(b->samples_stored==121 && b->reserved[0]==8000);
    uint32_t first=0;
#else
    assert(b->samples_stored==600);
    uint32_t first=(b->samples_stored-256u)%256u;
#endif
    assert(b->events==121 && b->no_fresh==1200);
    uint32_t last=(b->samples_stored-1u)%256u;
    assert(b->samples[last].at-b->samples[first].at==(last>=first?last-first:255u)*FD_WAVE_SAMPLE_TICKS);
    assert(b->samples[last].va==1500 && b->samples[last].pwm==1334);
    assert(b->samples[last].state==(2u|8u|32u));
    assert(b->events_ring[(b->events-1u)%96u].kind==FD_EVENT_COMMUTATE);
    finish(8,2); assert(fd_startup_wave.no_lock_bank==0);
    FdWaveBank saved=*b;
    begin(2); sample(FD_SAMPLE_PRE); finish(8,2);
    assert(memcmp(&saved,(const void *)&fd_startup_wave.banks[0],sizeof(saved))==0);
    begin(3); sample(FD_SAMPLE_PRE); finish(7,2);
    assert(fd_startup_wave.banks[1].state==FD_WAVE_ABORTED);
    begin(4); sample(FD_SAMPLE_ACCEPT);
    fdTraceEvent(FD_EVENT_REV_HANDOFF,6); fdTraceEvent(FD_EVENT_COMMUTATE,0);
    for(unsigned i=0;i<600;i++) sample(FD_SAMPLE_UNARMED);
    fdTraceEvent(FD_EVENT_TIMEOUT,0); finish(9,4);
    assert(fd_startup_wave.lost_bank==1);
#if defined(TARGET_STARTUP_ACQ_LOG) || defined(TARGET_STARTUP_LOSS_LOG)
    assert(fd_startup_wave.banks[1].samples_stored==1);
    assert(fd_startup_wave.banks[1].samples[0].state==(2u|8u|32u));
#elif defined(TARGET_STARTUP_EDGE_LOG)
    assert(fd_startup_wave.banks[1].samples_stored==121);
    assert(fd_startup_wave.banks[1].samples[120].state==(4u|8u|32u));
#else
    assert(fd_startup_wave.banks[1].samples_stored==301);
    assert(fd_startup_wave.banks[1].samples[300%256].state==(4u|8u|32u));
#endif
    begin(5); fdTraceEvent(FD_EVENT_REV_HANDOFF,6); finish(9,4);
    assert(fd_startup_wave.lost_bank==1);
    begin(6); sample(FD_SAMPLE_ACCEPT); fdTraceEvent(FD_EVENT_REV_HANDOFF,6);
    timer.CYCCNT+=160000000u;
    fdTraceEvent(FD_EVENT_ZC,0); sample(FD_SAMPLE_ACCEPT); finish(1,4);
    assert(fd_startup_wave.success_bank==2);
    FdWaveBank all[3]; memcpy(all,(const void *)fd_startup_wave.banks,sizeof(all));
    begin(7); sample(FD_SAMPLE_ACCEPT); finish(8,2);
    assert(fd_startup_wave.active_bank==FD_WAVE_NONE && fd_startup_wave.trials_seen==7);
    assert(memcmp(all,(const void *)fd_startup_wave.banks,sizeof(all))==0);
    fd_startup_wave.command=1; fdTraceControlTick();
    assert(fd_startup_wave.rejected_commands==1 && !fd_startup_wave.command);
    assert(argc==2); FILE *f=fopen(argv[1],"wb"); assert(f);
    assert(fwrite((const void *)&fd_startup_wave,1,sizeof(fd_startup_wave),f)==sizeof(fd_startup_wave));
    assert(!fclose(f));
    // No rearm or output mutation. Safety/native fault is not a false class.
    reset(); begin(1); finish(2,2);
    assert(fd_startup_wave.no_lock_bank==FD_WAVE_NONE && fd_startup_wave.banks[0].state==2);
    reset(); fdTraceTrial(1); finish(4,2);
    assert(fd_startup_wave.active_bank==FD_WAVE_NONE && fd_startup_wave.banks[0].ended==0);
    assert(fd_startup_wave.banks[0].state==FD_WAVE_ABORTED);
    // Logger preemption must not append after another context finishes.
    reset(); begin(1); preempt=interrupt_finish; sample(FD_SAMPLE_ACCEPT);
    assert(fd_startup_wave.banks[0].samples_seen==0 && !mask);
    reset(); begin(1); preempt=interrupt_finish; fdTraceEvent(FD_EVENT_TIMEOUT,0);
    assert(fd_startup_wave.banks[0].timeouts==0 && !mask);
    // Every accepted edge gets an event, even a decimated-out waveform sample.
    reset(); begin(1); sample(FD_SAMPLE_PRE); sample(FD_SAMPLE_ACCEPT);
    b=&fd_startup_wave.banks[0];
    assert(b->samples_stored==1 && b->events==2);
    assert(b->events_ring[1].kind==FD_WAVE_ADC_ACCEPT && b->events_ring[1].values[2]==780);
    assert(duty_cycle==400 && pwm.CCR1==1334 && pwm.CCR4==667 && running==1);
#if defined(TARGET_STARTUP_EDGE_LOG) && !defined(TARGET_STARTUP_LOSS_LOG)
    // Late first edge: ring contains prehistory, exact trigger and 12 ms post.
    reset(); begin(1);
    for(unsigned i=0;i<2000;i++) sample(FD_SAMPLE_PRE);
    b=&fd_startup_wave.banks[0];
    assert(b->samples_stored==1000 && b->reserved[0]==FD_WAVE_NONE);
    sample(FD_SAMPLE_ACCEPT);
    uint32_t triggered=b->reserved[0];
    for(unsigned i=0;i<400;i++) sample(FD_SAMPLE_ACCEPT);
    assert(b->reserved[0]==triggered && b->samples_stored==1121);
    assert(b->samples[(1121-256)%256].at < triggered);
    assert(b->samples[1120%256].at==triggered+FD_WAVE_EDGE_POST_TICKS);
    FdWaveSample held=b->samples[1120%256];
    for(unsigned i=0;i<2000;i++) sample(FD_SAMPLE_PRE);
    assert(!memcmp(&held,(const void *)&b->samples[1120%256],sizeof(held)));
    assert((b->reserved[1]&65535u)==1500 && (b->reserved[1]>>16)==10);
    assert((b->reserved[2]&65535u)==780);
    assert((b->reserved[3]&65535u)==1334 && (b->reserved[3]>>16)==667);
    // No first edge: retain rolling tail rather than freeze empty startup.
    reset(); begin(1); for(unsigned i=0;i<6000;i++) sample(FD_SAMPLE_PRE);
    assert(fd_startup_wave.banks[0].reserved[0]==FD_WAVE_NONE);
    assert(fd_startup_wave.banks[0].samples_stored==3000);
#endif
#ifdef TARGET_STARTUP_LOSS_LOG
#ifdef TARGET_STARTUP_NOEDGE_LOG
    reset(); begin(1);
    for(unsigned i=0;i<6000;i++) sample(FD_SAMPLE_PRE);
    b=&fd_startup_wave.banks[0];
    assert(b->samples_stored==250 && b->samples[0].at==8000);
    assert(b->samples[249].at==8000+249u*192000u);
    assert(!b->accepted && b->reserved[0]==FD_WAVE_NONE);
    FdWaveBank prefix=*b;
    for(unsigned i=0;i<100;i++) sample(FD_SAMPLE_PRE);
    assert(b->samples_stored==250);
    assert(!memcmp((const void *)b->samples,prefix.samples,sizeof(prefix.samples)));
    // A new trial restores coarse rate; first edge switches to fast rate.
    finish(8,2);
    char prefix_path[1024];
    assert(snprintf(prefix_path,sizeof(prefix_path),"%s.noedge",argv[1])>0);
    FILE *prefix_file=fopen(prefix_path,"wb"); assert(prefix_file);
    assert(fwrite((const void *)&fd_startup_wave,1,sizeof(fd_startup_wave),prefix_file)==sizeof(fd_startup_wave));
    assert(!fclose(prefix_file));
    begin(2);
    for(unsigned i=0;i<24;i++) sample(FD_SAMPLE_PRE);
    b=&fd_startup_wave.banks[1]; assert(b->samples_stored==1);
    sample(FD_SAMPLE_ACCEPT); assert(b->samples_stored==2);
    sample(FD_SAMPLE_PRE); assert(b->samples_stored==2);
    sample(FD_SAMPLE_PRE); assert(b->samples_stored==3);
    assert(b->samples[2].at-b->samples[1].at==16000u);
#endif
    reset(); begin(1);
    for(unsigned i=0;i<1000;i++) sample(FD_SAMPLE_PRE);
    fdTraceEvent(FD_EVENT_TIMEOUT,0); // Blind retry must not freeze.
    assert(!fd_wave_sample_frozen);
    sample(FD_SAMPLE_ACCEPT);
    for(unsigned i=0;i<246;i++) sample(FD_SAMPLE_PRE);
    sample(FD_SAMPLE_ACCEPT); // Second edge after ~12.3 ms.
    uint32_t second=timer.CYCCNT-fd_startup_wave.banks[0].started;
    for(unsigned i=0;i<450;i++) sample(FD_SAMPLE_UNARMED);
    fdTraceEvent(FD_EVENT_TIMEOUT,0);
    b=&fd_startup_wave.banks[0];
    assert(fd_wave_sample_frozen && (b->reserved[4]>>24)==1);
    uint32_t stored=b->samples_stored;
    assert(b->samples[(stored-256)%256].at < second);
    assert(b->samples[(stored-1)%256].at >= second+22400u*160u);
    FdWaveBank saved_loss=*b;
    for(unsigned i=0;i<1000;i++) sample(FD_SAMPLE_ACCEPT);
    fdTraceEvent(FD_EVENT_TIMEOUT,0);
    assert(b->samples_stored==stored && b->accepted>saved_loss.accepted);
    assert(!memcmp((const void *)b->samples,saved_loss.samples,sizeof(saved_loss.samples)));
    // New trial resets trigger; handoff freezes independently of early loss.
    finish(8,2); begin(2); assert(!fd_wave_sample_frozen);
    sample(FD_SAMPLE_ACCEPT); sample(FD_SAMPLE_PRE);
    fdTraceEvent(FD_EVENT_REV_HANDOFF,6);
    assert(fd_wave_sample_frozen);
    assert((fd_startup_wave.banks[1].reserved[4]>>24)==2);
#endif
#ifdef TARGET_STARTUP_ACQ_LOG
    // Full 300 ms across DWT wrap, then late service: no prefix overwrite.
    reset(); begin(1);
    for(unsigned i=0;i<6000;i++) sample(FD_SAMPLE_PRE);
    b=&fd_startup_wave.banks[0];
    assert(b->samples_stored==250 && b->samples[0].at==8000);
    FdWaveSample first_sample=b->samples[0], last_sample=b->samples[249];
    for(unsigned i=0;i<2000;i++) sample(FD_SAMPLE_PRE);
    assert(b->samples_stored==250 && b->samples_seen==8000);
    assert(!memcmp(&first_sample,(const void *)&b->samples[0],sizeof(first_sample)));
    assert(!memcmp(&last_sample,(const void *)&b->samples[249],sizeof(last_sample)));
    // Accepted edge between coarse samples remains exact in event channel.
    reset(); begin(1); sample(FD_SAMPLE_PRE); sample(FD_SAMPLE_ACCEPT);
    assert(fd_startup_wave.banks[0].samples_stored==1);
    assert(fd_startup_wave.banks[0].events_ring[1].at==16000);
#endif
    puts("PASS S3-D actual C: independent classes, pinned immutability, reuse, rings, wrap, decimation, accepted ADC events, aborts, preemption, no output writes");
}
