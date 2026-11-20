#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fd6288_m2_sine_adc_capture.h"

volatile FdM2SineAdcCapture fd_m2_sine_adc_capture;
volatile uint8_t fd_m2_sine_adc_enabled;
uint8_t fd_m2_sine_adc_pending_valid;
uint32_t fd_m2_sine_adc_origin;
FdM2SineAdcSample fd_m2_sine_adc_pending;

typedef struct {
    uint32_t interval, duty, timer, bridge;
    uint32_t step, running, qualification;
} ControlState;

static ControlState control = {400u, 3800u, 123u, 1u, 4u, 1u, 2u};

static FdM2SineAdcSample sampleAt(uint32_t cycles, uint16_t period,
    uint16_t elapsed)
{
    FdM2SineAdcSample sample = {
        .cycles = cycles,
        .a = 123u, .b = 234u, .c = 345u,
        .elapsed = elapsed, .period = period,
        .duty = 3800u, .trigger = 1900u, .cnt = 2200u,
        .step = 4u, .decision = 7u, .flags = 3u, .qualification = 2u
    };
    return sample;
}

static void begin(uint32_t origin)
{
    fdM2SineAdcReset(12u, origin);
    fd_m2_sine_adc_enabled = 1u;
}

static void commitSample(FdM2SineAdcSample sample)
{
    fdM2SineAdcStage(sample);
    fdM2SineAdcCommit();
}

static void resetPreservesInvalidSamples(void)
{
    FdM2SineAdcSample old = sampleAt(0x12345678u, 555u, 444u);
    fd_m2_sine_adc_capture.samples[17] = old;
    fd_m2_sine_adc_capture.header.count = 100u;
    fd_m2_sine_adc_capture.header.state = FD_M2_ADC_POST;
    fd_m2_sine_adc_capture.header.max_stage_cycles = 999u;
    fd_m2_sine_adc_capture.header.reserved[2] = 987u;
    fd_m2_sine_adc_enabled = fd_m2_sine_adc_pending_valid = 1u;
    fdM2SineAdcReset(42u, 0xf0000000u);
    FdM2SineAdcHeader h = fd_m2_sine_adc_capture.header;
    assert(h.magic == 0x464d4431u && h.abi == 1u && h.trial == 42u);
    assert(!h.count && h.state == FD_M2_ADC_ACTIVE && !h.post_remaining);
    assert(h.first_at == UINT32_MAX && h.trigger_at == UINT32_MAX);
    assert(!h.max_stage_cycles && !h.max_commit_cycles && !h.max_irq_cycles);
    assert(!h.irq_count && !h.budget_trips && !h.reserved[2]);
    assert(!fd_m2_sine_adc_enabled && !fd_m2_sine_adc_pending_valid);
    assert(fd_m2_sine_adc_origin == 0xf0000000u);
    FdM2SineAdcSample retained = fd_m2_sine_adc_capture.samples[17];
    assert(!memcmp(&old, &retained, sizeof(old)));
}

static void stagePreservesSnapshotAndWrap(void)
{
    begin(0xfffffff0u);
    FdM2SineAdcSample sample = sampleAt(0x14u, 600u, 731u);
    ControlState before = control;
    fdM2SineAdcStage(sample);
    assert(fd_m2_sine_adc_pending_valid);
    sample.elapsed = 0u;
    sample.period = 1u;
    control.interval = 3u;
    control.step = 5u;
    ControlState changed = control;
    fdM2SineAdcCommit();
    assert(!memcmp(&changed, &control, sizeof(control)));
    assert(!fd_m2_sine_adc_pending_valid);
    assert(fd_m2_sine_adc_capture.header.count == 1u);
    FdM2SineAdcSample saved = fd_m2_sine_adc_capture.samples[0];
    assert(saved.cycles == 36u && saved.elapsed == 731u && saved.period == 600u);
    assert(saved.step == 4u && saved.qualification == 2u);
    assert(fd_m2_sine_adc_capture.header.first_at == 36u);
    fdM2SineAdcCommit();
    assert(fd_m2_sine_adc_capture.header.count == 1u);
    control = before;
}

static void circularStorageAndSlowSpeed(void)
{
    begin(1000u);
    for (unsigned i = 0u; i < 145u; i++) {
        FdM2SineAdcSample sample = sampleAt(1000u + i * 100u, 3266u, 4899u);
        sample.a = (uint16_t)i;
        commitSample(sample);
    }
    assert(fd_m2_sine_adc_capture.header.count == 145u);
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_ACTIVE);
    assert(fd_m2_sine_adc_capture.header.trigger_at == UINT32_MAX);
    assert(fd_m2_sine_adc_capture.header.first_at == 0u);
    for (unsigned i = 17u; i < 145u; i++) {
        FdM2SineAdcSample saved = fd_m2_sine_adc_capture.samples[i & 127u];
        assert(saved.cycles == i * 100u && saved.a == i);
    }
    commitSample(sampleAt(16000u, 0u, 65000u));
    commitSample(sampleAt(16100u, 2000u, 3000u));
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_ACTIVE);
    fd_m2_sine_adc_enabled = 0u;
    fdM2SineAdcStage(sampleAt(16200u, 100u, 1000u));
    fdM2SineAdcCommit();
    assert(fd_m2_sine_adc_capture.header.count == 147u);
}

static void lowSpeedHandoffLongGapFreezesBeforePeriodGrowth(void)
{
    /* Latest failed run handed off at 1632.85 us/sector, ~510 RPM.
     * The old <=2000 tick gate ignored this entire slow-speed failure;
     * feedback later stretched to 6706.5 us and the tail overwrote onset. */
    begin(1000u);
    for (unsigned i = 0u; i < 132u; i++)
        commitSample(sampleAt(1000u + i * 6672u, 3266u, 4899u));
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_ACTIVE);
    uint32_t trigger_cycle = 1000u + 132u * 6672u;
    FdM2SineAdcSample trigger = sampleAt(trigger_cycle, 3266u, 4900u);
    trigger.step = 2u;
    commitSample(trigger);
    FdM2SineAdcHeader h = fd_m2_sine_adc_capture.header;
    assert(h.state == FD_M2_ADC_POST && h.trigger_step == 2u);
    assert(h.trigger_at == trigger_cycle - 1000u);
    assert(h.trigger_elapsed == 4900u && h.trigger_period == 3266u);
    assert(h.post_remaining == 16u);
    for (unsigned i = 0u; i < 16u; i++) {
        /* A late acceptance can enlarge the period without moving the
         * already latched onset or extending its post-trigger window. */
        commitSample(sampleAt(trigger_cycle + (i + 1u) * 6672u,
            13413u, 100u));
        assert(fd_m2_sine_adc_capture.header.trigger_at == h.trigger_at);
        assert(fd_m2_sine_adc_capture.header.trigger_period == 3266u);
    }
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_FROZEN);
    assert(fd_m2_sine_adc_capture.header.count == 149u);
    assert(!fd_m2_sine_adc_enabled);
    commitSample(sampleAt(trigger_cycle + 2000000u, 25207u, 45009u));
    assert(fd_m2_sine_adc_capture.header.count == 149u);
    assert(fd_m2_sine_adc_capture.header.trigger_elapsed == 4900u);
}

static void triggerAndPostWindow(void)
{
    begin(1000u);
    for (unsigned i = 0u; i < 132u; i++)
        commitSample(sampleAt(1000u + i * 10u, 1500u, 1000u));
    uint32_t trigger_cycle = 3000u;
    FdM2SineAdcSample trigger = sampleAt(trigger_cycle, 2000u, 3001u);
    trigger.step = 6u;
    commitSample(trigger);
    FdM2SineAdcHeader h = fd_m2_sine_adc_capture.header;
    assert(h.state == FD_M2_ADC_POST && h.count == 133u);
    assert(h.trigger_at == 2000u && h.trigger_step == 6u);
    assert(h.trigger_elapsed == 3001u && h.trigger_period == 2000u);
    assert(h.post_remaining == 16u);
    assert(fd_m2_sine_adc_capture.samples[132u & 127u].cycles == 2000u);
    for (unsigned i = 0u; i < 16u; i++) {
        commitSample(sampleAt(trigger_cycle + (i + 1u) * 10u, 1000u, 1900u));
        assert(fd_m2_sine_adc_capture.header.trigger_at == 2000u);
        if (i < 15u) {
            assert(fd_m2_sine_adc_enabled);
            assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_POST);
            assert(fd_m2_sine_adc_capture.header.post_remaining == 15u - i);
        }
    }
    assert(!fd_m2_sine_adc_enabled);
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_FROZEN);
    assert(fd_m2_sine_adc_capture.header.count == 149u);
    assert(!fd_m2_sine_adc_capture.header.post_remaining);
    commitSample(sampleAt(9999u, 100u, 900u));
    assert(fd_m2_sine_adc_capture.header.count == 149u);
    fd_m2_sine_adc_pending = sampleAt(9999u, 100u, 900u);
    fd_m2_sine_adc_pending_valid = 1u;
    fdM2SineAdcCommit();
    assert(!fd_m2_sine_adc_pending_valid);
    assert(fd_m2_sine_adc_capture.header.count == 149u);
    fdM2SineAdcStop();
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_FROZEN);
    assert(fd_m2_sine_adc_capture.header.trigger_at == 2000u);
}

static void stopKeepsInflightSample(void)
{
    begin(1000u);
    fdM2SineAdcStage(sampleAt(1200u, 1000u, 2000u));
    fdM2SineAdcStop();
    assert(!fd_m2_sine_adc_enabled && fd_m2_sine_adc_pending_valid);
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_STOPPED);
    fdM2SineAdcCommit();
    assert(fd_m2_sine_adc_capture.header.count == 1u);
    assert(fd_m2_sine_adc_capture.samples[0].cycles == 200u);
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_STOPPED);
    assert(fd_m2_sine_adc_capture.header.trigger_at == UINT32_MAX);
    fdM2SineAdcCommit();
    assert(fd_m2_sine_adc_capture.header.count == 1u);
    fdM2SineAdcReset(13u, 10u);
    assert(!fd_m2_sine_adc_pending_valid);
    fdM2SineAdcCommit();
    assert(!fd_m2_sine_adc_capture.header.count);

    begin(0u);
    commitSample(sampleAt(200u, 1000u, 1600u));
    fdM2SineAdcStage(sampleAt(300u, 1000u, 1800u));
    fdM2SineAdcStop();
    fdM2SineAdcCommit();
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_STOPPED);
    assert(fd_m2_sine_adc_capture.header.count == 2u);
    assert(fd_m2_sine_adc_capture.header.trigger_at == 200u);
}

static void recordingBudgetDoesNotChangeControl(void)
{
    begin(0u);
    ControlState before = control;
    fdM2SineAdcStage(sampleAt(100u, 1000u, 500u));
    fd_m2_sine_adc_capture.header.max_irq_cycles = 1234u;
    fd_m2_sine_adc_capture.header.irq_count = 55u;
    fdM2SineAdcCost(320u, 1u);
    fdM2SineAdcCost(319u, 0u);
    fdM2SineAdcCost(100u, 1u);
    assert(fd_m2_sine_adc_enabled && fd_m2_sine_adc_pending_valid);
    assert(fd_m2_sine_adc_capture.header.max_stage_cycles == 320u);
    assert(fd_m2_sine_adc_capture.header.max_commit_cycles == 319u);
    assert(!fd_m2_sine_adc_capture.header.budget_trips);
    fdM2SineAdcCost(321u, 1u);
    assert(!fd_m2_sine_adc_enabled && !fd_m2_sine_adc_pending_valid);
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_BUDGET);
    assert(fd_m2_sine_adc_capture.header.max_stage_cycles == 321u);
    assert(fd_m2_sine_adc_capture.header.budget_trips == 1u);
    fdM2SineAdcCommit();
    assert(!fd_m2_sine_adc_capture.header.count);
    fdM2SineAdcCost(333u, 0u);
    assert(fd_m2_sine_adc_capture.header.max_commit_cycles == 333u);
    assert(fd_m2_sine_adc_capture.header.budget_trips == 2u);
    assert(fd_m2_sine_adc_capture.header.max_irq_cycles == 1234u);
    assert(fd_m2_sine_adc_capture.header.irq_count == 55u);
    assert(!memcmp(&before, &control, sizeof(control)));
    fdM2SineAdcStop();
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_BUDGET);

    begin(0u);
    fdM2SineAdcStage(sampleAt(10u, 1000u, 500u));
    fdM2SineAdcCost(321u, 0u);
    assert(fd_m2_sine_adc_capture.header.state == FD_M2_ADC_BUDGET);
    assert(!fd_m2_sine_adc_enabled && !fd_m2_sine_adc_pending_valid);
    assert(!memcmp(&before, &control, sizeof(control)));
}

int main(void)
{
    resetPreservesInvalidSamples();
    stagePreservesSnapshotAndWrap();
    circularStorageAndSlowSpeed();
    lowSpeedHandoffLongGapFreezesBeforePeriodGrowth();
    triggerAndPostWindow();
    stopKeepsInflightSample();
    recordingBudgetDoesNotChangeControl();
    puts("PASS ADC recorder snapshot, wrap, ring, all-speed first-gap freeze, stop and recording-only budget");
    return 0;
}
