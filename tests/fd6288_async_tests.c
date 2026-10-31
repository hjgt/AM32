#include <assert.h>
#include <stdint.h>
#include <string.h>
#include "fd6288_async.h"
#ifdef TARGET_FD_NO_DIAGNOSTICS
#define EXPECT_COUNT 0
#else
#define EXPECT_COUNT 1
#endif
static struct { uint32_t SR, ARR; } timer;
static struct { uint32_t CYCCNT; } counter;
#define COM_TIMER (&timer)
#define DWT (&counter)
#define COM_TIMER_IRQ 0
static unsigned enabled, mask_state, stepped, stopped;
static uint32_t __get_PRIMASK(void) { return mask_state; }
static void __disable_irq(void) { mask_state = 1; }
static void __set_PRIMASK(uint32_t v) { mask_state = v; }
static void NVIC_ClearPendingIRQ(int v) { (void)v; }
#define DISABLE_COM_TIMER_INT() (enabled=0)
#define SET_AND_ENABLE_COM_INT(t) (timer.ARR=(t), enabled=1)
static unsigned armed, running, stepper_sine, prop_brake_active;
static unsigned stuck_rotor_inhibit, drive_safety_limit_inhibit;
static unsigned fd_rev_acquiring, heli_coast_active, input, adjusted_input, forward;
#define TARGET_STARTUP_SEQUENCE
#if 1
enum { FD_REV_ALIGN=1, FD_REV_OPEN=2, FD_REV_PENDING=3, FD_REV_RELAX=5 };
static struct { unsigned stage; } fd_rev;
#endif
static unsigned bemfcounter, bad_count, zero_crosses, old_routine;
static struct { unsigned inhibit; } fd_power_bench;
#define FD_BENCH_LOST_FEEDBACK 7
#define FD_BENCH_COMM_LATE 10
#define FD_BENCH_COMM_INVALID 11
static void fdBenchService(void) { }
static void fdBenchStop(unsigned reason) { (void)reason; stopped++; fd_power_bench.inhibit=1; fdAsyncCancel(); }
static void commutate(void) { stepped++; fdAsyncCancel(); }
static unsigned zcfound, rising, min_bemf_counts_up, min_bemf_counts_down;
static unsigned consumed, discarded, crossings, coast_records;
#define ADC1 1
#define TARGET_FD_ADC_COHERENT
static unsigned fresh, queued, rearms, sample_version;
static unsigned LL_ADC_IsActiveFlag_JEOS(int adc) { (void)adc; return fresh; }
static void trigger(void) { if(queued) { queued=0; fresh=1; sample_version++; } }
static void adcZcdRearmInjected(void) { assert(!fresh && !queued); queued=1; rearms++; }
static void LL_ADC_ClearFlag_JEOS(int adc) { (void)adc; fresh=0; discarded++; }
static void getBemfState(void) { assert(fresh); fresh=0; consumed++; bemfcounter++; }
static void heliCoastRecordZeroCross(void) { coast_records++; }
static void zcfoundroutine(void) { crossings++; }
#ifdef TARGET_FD_ALIGN_SETTLE
static unsigned align_observed;
static void fdObserveAlignmentAdc(void) { align_observed++; LL_ADC_ClearFlag_JEOS(ADC1); }
#endif
static void fdRevTick(void) { assert(fd_rev_acquiring && fd_rev.stage==FD_REV_PENDING); fd_rev_acquiring=0; zero_crosses=6; commutate(); }
/* IMPLEMENTATION */
static void reset(void)
{
    fresh=1; queued=rearms=sample_version=0;
    memset((void *)&fd_async_diag,0,sizeof(fd_async_diag));
    armed=running=1; input=adjusted_input=400; forward=1;
    stepper_sine=prop_brake_active=stuck_rotor_inhibit=drive_safety_limit_inhibit=0;
    fd_rev_acquiring=heli_coast_active=fd_power_bench.inhibit=0;
    enabled=mask_state=stepped=stopped=zero_crosses=0;
    DWT->CYCCNT=1000;
    old_routine=1; zcfound=bemfcounter=consumed=discarded=crossings=coast_records=0;
    rising=1; min_bemf_counts_up=min_bemf_counts_down=0;
}
int main(void)
{
    reset(); fd_rev_acquiring=1; fd_rev.stage=FD_REV_PENDING;
    fdAsyncScheduleDeadline(1000,9081); assert(enabled && fd_async_diag.pending==2);
    DWT->CYCCNT=9080; fdAsyncTimer(); assert(!stepped && enabled);
    DWT->CYCCNT=9081; fdAsyncTimer(); assert(stepped==1 && zero_crosses==6 && !fd_rev_acquiring);
    fdAsyncTimer(); assert(stepped==1);
    reset(); fd_rev_acquiring=1; fd_rev.stage=FD_REV_PENDING;
    fdAsyncScheduleDeadline(1000,9000); fd_rev_acquiring=0;
    DWT->CYCCNT=9000; fdAsyncTimer(); assert(!stepped);
    reset(); fd_rev_acquiring=1; fd_rev.stage=FD_REV_PENDING;
    fdAsyncScheduleDeadline(1000,9000); DWT->CYCCNT=17001;
    fdAsyncTimer(); assert(!stepped && stopped);

    assert(fdAsyncRemaining(100,180)==1);
    assert(fdAsyncRemaining(100,181)==2);
    assert(fdAsyncRemaining(0xfffffff0u,64)==1);
    assert(!fdAsyncTooLate(9000,1000));
    assert(fdAsyncTooLate(9001,1000));
    reset(); fdAsyncSchedule(1000,100); assert(enabled && timer.ARR==99);
    DWT->CYCCNT=8999; fdAsyncTimer(); assert(!stepped && enabled);
    DWT->CYCCNT=9000; fdAsyncTimer(); assert(stepped==1 && zero_crosses==1 && !enabled);
    fdAsyncTimer(); assert(stepped==1); /* no duplicate callback */
    reset(); fdAsyncSchedule(1000,100); input=0; DWT->CYCCNT=9000;
    fdAsyncTimer(); assert(!stepped && !enabled && fd_async_diag.stale==EXPECT_COUNT);
    reset(); fdAsyncSchedule(1000,100); fdAsyncCancel(); DWT->CYCCNT=9000;
    fdAsyncTimer(); assert(!stepped && fd_async_diag.cancelled==EXPECT_COUNT);
    reset(); fdAsyncSchedule(1000,100); fd_async_diag.epoch++; DWT->CYCCNT=9000;
    fdAsyncTimer(); assert(!stepped && fd_async_diag.stale==EXPECT_COUNT);
    reset(); fdAsyncSchedule(1000,100); forward=0; DWT->CYCCNT=9000;
    fdAsyncTimer(); assert(!stepped);
    reset(); fdAsyncSchedule(1000,100); heli_coast_active=1; DWT->CYCCNT=9000;
    fdAsyncTimer(); assert(!stepped);
    reset(); fdAsyncSchedule(1000,100); stuck_rotor_inhibit=1; DWT->CYCCNT=9000;
    fdAsyncTimer(); assert(!stepped);
    reset(); fdAsyncSchedule(1000,100); drive_safety_limit_inhibit=1; DWT->CYCCNT=9000;
    fdAsyncTimer(); assert(!stepped);
    reset(); fdAsyncSchedule(1000,100); DWT->CYCCNT=17001;
    fdAsyncTimer(); assert(!stepped && stopped==1 && fd_async_diag.late_stop==EXPECT_COUNT);
    reset(); DWT->CYCCNT=0xfffffff0u; fdAsyncSchedule(DWT->CYCCNT,2);
    DWT->CYCCNT=144; fdAsyncTimer(); assert(stepped==1);
    reset(); fdAsyncSchedule(1000,65535); assert(timer.ARR==65534);
    reset(); fdAsyncSchedule(1000,65536); assert(stopped && !enabled);
    reset(); fdAsyncSchedule(1000,100); fdAsyncSchedule(1000,100);
    assert(stopped && !enabled && !stepped);
    reset(); mask_state=1; fdAsyncSchedule(1000,100); assert(mask_state==1);
    DWT->CYCCNT=9000; fdAsyncTimer(); assert(mask_state==1);
    reset(); fdAdcJeosService(); assert(consumed==1 && crossings==1 && coast_records==1);
    trigger(); fdAdcJeosService(); assert(consumed==2 && crossings==1); /* one event per sector */
    reset(); running=0; fdAdcJeosService(); assert(!consumed && discarded==1);
    reset(); armed=0; fdAdcJeosService(); assert(!consumed && discarded==1);
    reset(); fd_power_bench.inhibit=1; fdAdcJeosService(); assert(!consumed && discarded==1);
    reset(); heli_coast_active=1; fdAdcJeosService(); assert(consumed==1 && coast_records==1);
    reset(); fd_rev_acquiring=1; fdAdcJeosService(); assert(consumed==1 && crossings==1);
#ifdef TARGET_FD_DOUBLE_ALIGN
    reset(); fd_rev_acquiring=1; fd_rev.stage=FD_REV_ALIGN;
    fdAdcJeosService(); assert(!consumed && !crossings && discarded==1);
#ifdef TARGET_FD_ALIGN_SETTLE
    assert(align_observed==1 && !stepped);
#endif
    fd_rev.stage=FD_REV_OPEN; trigger();
    fdAdcJeosService(); assert(consumed==1 && crossings==1);
#ifdef TARGET_FD_REPOSITION
    reset(); fd_rev_acquiring=1; fd_rev.stage=FD_REV_RELAX;
    fdAdcJeosService(); assert(!consumed && !crossings && discarded==1);
#endif
#endif
    reset();
    /* Completed context is consumed: later PWM triggers cannot overwrite it. */
    for(unsigned i=0;i<100;i++) trigger();
    assert(fresh && sample_version==0);
    fdAdcJeosService(); assert(rearms==1 && queued && !fresh);
    fdAdcJeosService(); assert(rearms==1); /* spurious IRQ cannot enqueue twice */
    trigger(); assert(fresh && sample_version==1 && !queued);
    for(unsigned i=0;i<100;i++) trigger();
    assert(sample_version==1);
    fdAdcJeosService(); assert(rearms==2 && queued);
    return 0;
}
