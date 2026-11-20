/* Included only in the instrumented November 11 Release, after its startup
 * adapter is defined. These hooks write RAM and do not feed the controller. */
enum { FD_RELEASE_SECTORS = 12u, FD_RELEASE_SAMPLES = 32u };

typedef struct {
    uint32_t cycles;
    uint16_t a, b, c, duty, trigger, cnt;
} FdReleaseSample;

typedef struct {
    uint32_t at, edge;
    uint16_t period, edge_period;
    uint8_t step, rising, count, reserved;
    FdReleaseSample samples[FD_RELEASE_SAMPLES];
} FdReleaseSector;

volatile struct {
    uint32_t magic, abi, trial, handoff_at, sectors;
    FdReleaseSector entries[FD_RELEASE_SECTORS];
} fd_release_closed_capture = { .magic = 0x46524331u, .abi = 1u };
_Static_assert(sizeof(FdReleaseSector) == 528u, "Release sector capture ABI");
_Static_assert(sizeof(fd_release_closed_capture) == 6356u, "Release capture ABI");
static uint8_t fd_release_capture_active;
static uint16_t fd_release_adc_seen;

static void fdReleaseCaptureBegin(uint32_t now)
{
    fd_release_closed_capture.trial = fd_power_bench.active_id;
    fd_release_closed_capture.handoff_at = now;
    fd_release_closed_capture.sectors = 0u;
    fd_release_capture_active = 1u;
}

static void fdReleaseCaptureSwitch(void)
{
    if (!fd_release_capture_active || fd_rev_acquiring || !fd_power_bench.active_id ||
        !running || heli_coast_active) return;
    unsigned i = fd_release_closed_capture.sectors;
    if (i == FD_RELEASE_SECTORS) {
        fd_release_capture_active = 0u;
        return;
    }
    volatile FdReleaseSector *row = &fd_release_closed_capture.entries[i];
    row->at = DWT->CYCCNT - fd_release_closed_capture.handoff_at;
    row->edge = UINT32_MAX;
    row->period = (uint16_t)commutation_interval;
    row->edge_period = 0u;
    row->step = step;
    row->rising = rising != 0;
    row->count = 0u;
    fd_release_adc_seen = 0u;
    fd_release_closed_capture.sectors = i + 1u;
}

static uint8_t fdReleaseCaptureSampling(void)
{
    return fd_release_capture_active && fd_release_closed_capture.sectors &&
        fd_release_closed_capture.trial == fd_power_bench.active_id &&
        !fd_rev_acquiring && !heli_coast_active && !fd_power_bench.inhibit;
}

static void fdReleaseCaptureSample(void)
{
    if (!fdReleaseCaptureSampling()) return;
    volatile FdReleaseSector *row =
        &fd_release_closed_capture.entries[fd_release_closed_capture.sectors - 1u];
    uint16_t seen = fd_release_adc_seen++;
    unsigned n = row->count;
    if (n >= FD_RELEASE_SAMPLES || (seen >= 8u && (seen & 1u))) return;
    row->samples[n] = (FdReleaseSample){
        .cycles = DWT->CYCCNT - fd_release_closed_capture.handoff_at - row->at,
        .a = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_1),
        .b = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_2),
        .c = LL_ADC_INJ_ReadConversionData12(ADC1, LL_ADC_INJ_RANK_3),
        .duty = TIM1->CCR1, .trigger = TIM1->CCR4, .cnt = TIM1->CNT
    };
    row->count = n + 1u;
}

static void fdReleaseCaptureEdge(uint32_t accepted_at)
{
    if (!fdReleaseCaptureSampling()) return;
    volatile FdReleaseSector *row =
        &fd_release_closed_capture.entries[fd_release_closed_capture.sectors - 1u];
    row->edge = accepted_at - fd_release_closed_capture.handoff_at;
    row->edge_period = INTERVAL_TIMER_COUNT;
}
