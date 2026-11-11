/* Diagnostic only: reuse 21568 bytes for latest-trial, full-rate prefix.
 * Read-only peripheral access; no sample interpolation or detector changes. */
extern char step, old_routine;
extern volatile char rising;
extern volatile uint16_t duty_cycle, waitTime;
extern volatile uint32_t commutation_interval;
extern volatile uint8_t heli_coast_active;
_Static_assert(sizeof(FdDenseSample) == 16, "dense sample ABI");
_Static_assert(offsetof(FdStartupWave, samples) == 128, "dense prefix ABI");
_Static_assert(offsetof(FdStartupWave, events_ring) == 20608, "dense events ABI");
_Static_assert(sizeof(FdStartupWave) == 21568, "same RAM budget");
volatile FdStartupWave fd_startup_wave;
static uint8_t dense_begun, dense_stage;
static inline uint8_t denseActive(void)
{
    return fd_startup_wave.active_bank == 0u && dense_begun;
}
static inline __attribute__((always_inline)) void denseEvent(uint8_t kind, uint16_t aux)
{
    uint32_t n = fd_startup_wave.events++;
    if (n >= 48u) return;
    volatile FdWaveEvent *e = &fd_startup_wave.events_ring[n];
    e->at = DWT->CYCCNT - fd_startup_wave.started;
    e->elapsed = INTERVAL_TIMER_COUNT;
    e->interval = commutation_interval;
    e->values[0] = aux; e->values[1] = waitTime; e->values[2] = adjusted_input;
    e->duty = duty_cycle; e->step = step;
    e->flags = (forward ? 1u : 0u) | (rising ? 2u : 0u) |
        (running ? 4u : 0u) | (heli_coast_active ? 8u : 0u) | (old_routine ? 16u : 0u);
    e->kind = kind; e->stage = dense_stage;
}
void fdTraceInit(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    fd_startup_wave.magic = FD_WAVE_MAGIC; fd_startup_wave.abi = 3u;
    fd_startup_wave.variant = FD6288_TRACE_VARIANT;
    fd_startup_wave.clock_hz = 160000000u;
    fd_startup_wave.size = sizeof(FdStartupWave);
    fd_startup_wave.active_bank = FD_WAVE_NONE;
    fd_startup_wave.no_lock_bank = fd_startup_wave.lost_bank = fd_startup_wave.success_bank = FD_WAVE_NONE;
    fd_startup_wave.reserved[0] = 1280u;
    fd_startup_wave.reserved[1] = 16u;
    fd_startup_wave.reserved[2] = 48u;
}
void fdTraceTrial(uint32_t id)
{
    /* Called by bench in a critical section. Never clear large arrays. */
    fd_startup_wave.active_bank = FD_WAVE_NONE;
    dense_begun = 0u; dense_stage = 2u;
    fd_startup_wave.trials_seen = fd_startup_wave.trial_id = id;
    fd_startup_wave.no_lock_bank = fd_startup_wave.lost_bank = fd_startup_wave.success_bank = FD_WAVE_NONE;
    fd_startup_wave.state = FD_WAVE_ACTIVE;
    fd_startup_wave.started = fd_startup_wave.ended = fd_startup_wave.reason = fd_startup_wave.final_stage = 0u;
    fd_startup_wave.handoff_at = FD_WAVE_NONE;
    fd_startup_wave.seen = fd_startup_wave.stored = fd_startup_wave.events = 0u;
    fd_startup_wave.no_fresh = fd_startup_wave.accepted = fd_startup_wave.timeouts = 0u;
    fd_startup_wave.max_sample_cycles = fd_startup_wave.max_event_cycles = fd_startup_wave.last_at = 0u;
    fd_startup_wave.active_bank = 0u;
}
void fdTraceBegin(void)
{
    if (fd_startup_wave.active_bank == 0u && !dense_begun) {
        fd_startup_wave.started = DWT->CYCCNT; dense_begun = 1u;
    }
}
void fdTracePinFailure(void) { }
void fdTraceEvent(uint8_t kind, uint16_t aux)
{
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    if (denseActive()) {
        uint32_t began = DWT->CYCCNT;
        if (kind == FD_EVENT_TIMEOUT) fd_startup_wave.timeouts++;
        if (kind == FD_EVENT_REV_STEP) dense_stage = 2u;
        if (kind == FD_EVENT_REV_EDGE) dense_stage = aux >> 8;
        if (kind == FD_EVENT_REV_HANDOFF) {
            dense_stage = 4u; fd_startup_wave.handoff_at = began - fd_startup_wave.started;
        }
        denseEvent(kind, aux);
        uint32_t cost = DWT->CYCCNT - began;
        if (cost > fd_startup_wave.max_event_cycles) fd_startup_wave.max_event_cycles = cost;
    }
    __set_PRIMASK(mask);
}
void fdTraceSample(uint16_t va, uint16_t vb, uint16_t vc, int16_t margin,
    uint8_t reason, uint8_t pre_seen, uint8_t confirmations)
{
    (void)margin;
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    if (denseActive()) {
        uint32_t began = DWT->CYCCNT;
        fd_startup_wave.seen++;
        if (reason == FD_SAMPLE_ACCEPT) fd_startup_wave.accepted++;
        uint32_t n = fd_startup_wave.stored;
        if (n < 1280u) {
            volatile FdDenseSample *s = &fd_startup_wave.samples[n];
            s->at = began - fd_startup_wave.started;
            s->va = va; s->vb = vb; s->vc = vc;
            s->pwm = TIM1->CCR1; s->trigger = TIM1->CCR4;
            s->meta = ((uint8_t)step & 7u) | ((reason & 15u) << 3) | (pre_seen ? 128u : 0u);
            s->state = (confirmations > 7u ? 7u : confirmations) |
                ((dense_stage & 7u) << 3) | (rising ? 64u : 0u) | (forward ? 128u : 0u);
            fd_startup_wave.last_at = s->at;
            __DMB(); fd_startup_wave.stored = n + 1u;
        }
        uint32_t cost = DWT->CYCCNT - began;
        if (cost > fd_startup_wave.max_sample_cycles) fd_startup_wave.max_sample_cycles = cost;
    }
    __set_PRIMASK(mask);
}
void fdTraceNoFresh(void)
{
    uint32_t mask = __get_PRIMASK(); __disable_irq();
    if (denseActive()) fd_startup_wave.no_fresh++;
    __set_PRIMASK(mask);
}
void fdTraceTrialFinish(uint8_t reason, uint8_t stage)
{
    if (fd_startup_wave.active_bank != 0u) return;
    uint32_t began = DWT->CYCCNT;
    fd_startup_wave.ended = dense_begun ? began - fd_startup_wave.started : 0u;
    fd_startup_wave.reason = reason; fd_startup_wave.final_stage = stage;
    uint32_t state = FD_WAVE_ABORTED;
    if (dense_begun && reason == 8u && stage != 4u) state = FD_WAVE_NO_LOCK;
    else if (reason == 9u && stage == 4u) state = FD_WAVE_LOST;
    else if (reason == 1u && stage == 4u) state = FD_WAVE_SUCCESS;
    __DMB(); fd_startup_wave.state = state;
    fd_startup_wave.active_bank = FD_WAVE_NONE; dense_begun = 0u;
    uint32_t cost = DWT->CYCCNT - began;
    if (cost > fd_startup_wave.max_finish_cycles) fd_startup_wave.max_finish_cycles = cost;
}
void fdTraceFreeze(uint32_t reason) { (void)reason; }
void fdTraceControlTick(void)
{
    if (fd_startup_wave.command) {
        fd_startup_wave.command = 0u; fd_startup_wave.rejected_commands++;
    }
}
