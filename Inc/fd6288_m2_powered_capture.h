#pragma once

/* Diagnostic only. FdEdgeSample is defined by fd6288_edge_observe.h. */
enum { FD_M2_POWERED_MAX_SAMPLES = 256u, FD_M2_POWERED_MAX_SECTORS = 6u,
       FD_M2_POWERED_CAPTURE_TICKS = 40000u * 160u };
typedef struct {
    uint32_t at;
    uint16_t step, compare;
    int16_t current, peak_current;
    uint16_t voltage, min_voltage;
} FdM2PoweredSector;
typedef struct {
    uint32_t trial, started, ended, count, seen, sectors, active, max_cycles;
    FdM2PoweredSector sector[FD_M2_POWERED_MAX_SECTORS];
    FdEdgeSample samples[FD_M2_POWERED_MAX_SAMPLES];
} FdM2PoweredCapture;
_Static_assert(sizeof(FdM2PoweredSector) == 16u, "M2 powered sector ABI");
_Static_assert(sizeof(FdM2PoweredCapture) == 6272u, "M2 powered capture ABI");
extern volatile FdM2PoweredCapture fd_m2_powered_capture;

static inline void fdM2PoweredReset(uint32_t trial)
{
    volatile FdM2PoweredCapture *s = &fd_m2_powered_capture;
    s->trial = trial;
    s->started = s->ended = s->count = s->seen = 0u;
    s->sectors = s->active = s->max_cycles = 0u;
}

static inline void fdM2PoweredFinish(uint32_t now)
{
    volatile FdM2PoweredCapture *s = &fd_m2_powered_capture;
    if (!s->active) return;
    s->active = 0u;
    s->ended = now - s->started;
}

static inline void fdM2PoweredMark(uint32_t now, uint16_t step, uint16_t compare,
                                   int16_t current, uint16_t voltage)
{
    volatile FdM2PoweredCapture *s = &fd_m2_powered_capture;
    if (!s->active || now - s->started >= FD_M2_POWERED_CAPTURE_TICKS) return;
    unsigned slot = s->sectors;
    if (slot >= FD_M2_POWERED_MAX_SECTORS) return;
    s->sector[slot] = (FdM2PoweredSector){
        .at = now - s->started, .step = step, .compare = compare,
        .current = current, .peak_current = current,
        .voltage = voltage, .min_voltage = voltage
    };
    s->sectors = slot + 1u;
}

static inline void fdM2PoweredSupply(uint32_t now, int16_t current, uint16_t voltage)
{
    volatile FdM2PoweredCapture *s = &fd_m2_powered_capture;
    if (!s->active || !s->sectors) return;
    if (now - s->started >= FD_M2_POWERED_CAPTURE_TICKS) {
        fdM2PoweredFinish(now);
        return;
    }
    volatile FdM2PoweredSector *slot = &s->sector[s->sectors - 1u];
    if (current > slot->peak_current) slot->peak_current = current;
    if (voltage < slot->min_voltage) slot->min_voltage = voltage;
}

static inline void fdM2PoweredSample(FdEdgeSample v)
{
    volatile FdM2PoweredCapture *s = &fd_m2_powered_capture;
    uint32_t now = v.at;
    v.at -= s->started;
    if (v.at >= FD_M2_POWERED_CAPTURE_TICKS) {
        fdM2PoweredFinish(now);
        return;
    }
    unsigned sector = s->sectors;
    if (!sector) return;
    uint32_t age = v.at - s->sector[sector - 1u].at;
    uint32_t dense = (sector == 1u ? 2000u : 500u) * 160u;
    uint32_t index = s->seen++;
    /* Keep every ACCEPT and the samples immediately after each boundary. */
    if (age >= dense && (index & 7u) && v.reason != 7u) return;

    v.reserved = sector;
    v.present = 1u;
    unsigned count = s->count;
    s->samples[count] = v;
    s->count = count + 1u;
    if (s->count == FD_M2_POWERED_MAX_SAMPLES) fdM2PoweredFinish(now);
}
