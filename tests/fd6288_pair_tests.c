/* Actual paired logger with register stubs. Not a motor simulation. */
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_TRACE
#define TARGET_STARTUP_SEQUENCE
#define TARGET_STARTUP_PAIR_TRACE
char step = 1, old_routine = 1, forward = 1;
volatile char rising = 1;
uint8_t running = 1;
volatile uint8_t heli_coast_active;
volatile uint16_t duty_cycle = 400;
uint16_t adjusted_input = 198, battery_voltage = 800;
int16_t actual_current = 100;
static struct { uint32_t CTRL, CYCCNT; } timer;
static struct { uint32_t DEMCR; } core;
static struct { uint32_t CCR1, CCR4; } pwm = {1334, 667};
#define DWT (&timer)
#define CoreDebug (&core)
#define TIM1 (&pwm)
#define CoreDebug_DEMCR_TRCENA_Msk 1u
#define DWT_CTRL_CYCCNTENA_Msk 1u
#define __DMB() ((void)0)
static uint32_t mask;
static void (*preempt)(void);
static uint32_t __get_PRIMASK(void) { return mask; }
static void __disable_irq(void) {
    if (!mask && preempt) { void (*p)(void) = preempt; preempt = 0; p(); }
    mask = 1;
}
static void __set_PRIMASK(uint32_t m) { mask = m; }
/* PAIR_IMPLEMENTATION */
static void reset(void)
{
    memset((void *)&fd_startup_pair, 0, sizeof(fd_startup_pair));
    timer.CYCCNT = 0xfffff000u;
    fdTraceInit();
}
static void begin(uint32_t id)
{
    fdTraceTrial(id); fdTraceBegin();
    fdTraceEvent(FD_EVENT_COMMUTATE, 0);
}
static void sample(int16_t m, uint8_t why)
{
    timer.CYCCNT += 8000u;
    fdTraceSample(1500, 10, (uint16_t)(750 + m), m, why, 1, 2);
}
static void finish(unsigned success)
{
    if (success) { fdTraceEvent(FD_EVENT_REV_HANDOFF, 6); timer.CYCCNT += 160000000u; }
    fdTraceTrialFinish(success ? 1 : 8, success ? 4 : 2);
}
static void interrupt_finish(void) { fdTraceTrialFinish(8, 2); }
int main(int argc, char **argv)
{
    reset(); begin(1);
    sample(30, FD_SAMPLE_UNARMED); sample(0, FD_SAMPLE_HYST);
    sample(-30, FD_SAMPLE_PRE); sample(40, FD_SAMPLE_ACCEPT);
    sample(-40, FD_SAMPLE_REPORTED); /* raw reverse after detector reported */
    volatile FdPairSector *s = &fd_startup_pair.banks[0].records[0];
    assert(s->reverse_crossings == 2 && s->forward_crossings == 1);
    assert(s->first_post == 8000 && s->reverse_at == 24000 && s->accepted_at == 32000);
    assert(s->samples == 5 && s->valid_samples == 5 && s->directed_min == -40);
    assert(s->first_adc[2] == 780 && s->last_adc[2] == 710);
    assert(s->last_sample == 40000); /* CYCCNT wrap */
    fdTraceEvent(FD_EVENT_REV_STEP, 12000); step = 2; rising = 0;
    fdTraceEvent(FD_EVENT_COMMUTATE, 0); s = &fd_startup_pair.banks[0].records[1];
    sample(30, FD_SAMPLE_PRE); sample(-30, FD_SAMPLE_CONFIRM);
    assert(s->forward_crossings == 1 && s->directed_first == -30);
    fdTraceEvent(FD_EVENT_REV_EDGE, (2 << 8) | 2);
    fdTraceEvent(FD_EVENT_REV_DECISION, (80 << 8) | 5);
    assert(s->confidence == 2 && s->sector_percent == 80 && s->decision == 5);
    finish(0); assert(fd_startup_pair.failure_bank == 0);
    FdPairBank saved = fd_startup_pair.banks[0];
    for (unsigned id = 2; id <= 5; id++) { begin(id); sample(0, FD_SAMPLE_HYST); finish(0); }
    assert(memcmp(&saved, (const void *)&fd_startup_pair.banks[0], sizeof(saved)) == 0);
    begin(6); sample(30, FD_SAMPLE_PRE); sample(-30, FD_SAMPLE_ACCEPT);
    fdTraceEvent(FD_EVENT_REV_HANDOFF, 6);
    unsigned sectors = fd_startup_pair.banks[1].sectors;
    fdTraceEvent(FD_EVENT_COMMUTATE, 0); fdTraceEvent(FD_EVENT_ZC, 0);
    sample(-20, FD_SAMPLE_ACCEPT);
    assert(fd_startup_pair.banks[1].sectors == sectors);
    timer.CYCCNT += 160000000u; fdTraceTrialFinish(1, 4);
    assert(fd_startup_pair.success_bank == 1 && fd_startup_pair.banks[1].closed_zc == 1);
    FdPairBank good = fd_startup_pair.banks[1];
    begin(7); sample(50, FD_SAMPLE_ACCEPT); finish(0);
    assert(fd_startup_pair.active_bank == FD_PAIR_NONE && fd_startup_pair.trials_seen == 7);
    assert(memcmp(&good, (const void *)&fd_startup_pair.banks[1], sizeof(good)) == 0);
    assert(memcmp(&saved, (const void *)&fd_startup_pair.banks[0], sizeof(saved)) == 0);
    fd_startup_pair.command = FD_TRACE_ARM_COMMAND; fdTraceControlTick();
    assert(fd_startup_pair.rejected_commands == 1 && fd_startup_pair.success_bank == 1);
    assert(duty_cycle == 400 && pwm.CCR1 == 1334 && mask == 0);
    if (argc > 1) {
        FILE *f = fopen(argv[1], "wb"); assert(f);
        assert(fwrite((const void *)&fd_startup_pair, sizeof(fd_startup_pair), 1, f) == 1);
        assert(fclose(f) == 0);
    }
    reset(); begin(1); finish(1); assert(fd_startup_pair.success_bank == 0);
    begin(2); fdTraceTrialFinish(7, 2); assert(fd_startup_pair.failure_bank == FD_PAIR_NONE);
    begin(3); finish(0); assert(fd_startup_pair.failure_bank == 1);
    reset(); begin(1);
    for (unsigned i = 1; i < 80; i++) fdTraceEvent(FD_EVENT_COMMUTATE, 0);
    assert(fd_startup_pair.banks[0].sectors == 72 && fd_startup_pair.banks[0].overflow == 8);
    sample(30, FD_SAMPLE_ACCEPT); finish(0); /* safe after capacity exhaustion */
    reset(); begin(1); s = &fd_startup_pair.banks[0].records[0];
    s->samples = UINT16_MAX; sample(0, FD_SAMPLE_HYST);
    assert(s->samples == UINT16_MAX && (s->issues & FD_PAIR_SATURATED));
    step = 3; sample(0, FD_SAMPLE_HYST); assert(s->issues & FD_PAIR_PHASE_MISMATCH);
    reset(); begin(1); preempt = interrupt_finish; sample(30, FD_SAMPLE_ACCEPT);
    assert(fd_startup_pair.failure_bank == 0 && fd_startup_pair.banks[0].records[0].samples == 0);
    assert(mask == 0);
    puts("PASS paired logger: both retention orders, duplicates, early-zero, overflow, wrap, crossings, saturation, no output writes");
    return 0;
}
