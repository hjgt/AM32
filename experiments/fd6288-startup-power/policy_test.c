#include "policy.h"
#include <assert.h>
#include <stdio.h>

static unsigned checks;
#define CHECK(x) do { ++checks; assert(x); } while (0)
static uint32_t ticks(unsigned us) { return us * FD_POWER_TICKS_PER_US; }
static uint16_t floor_at(FdPowerPolicy *s, uint32_t t, unsigned zc)
{
    return fdPowerFloor(s, t, 360u, 610u, 2000u, zc, 1u);
}
static void begin(FdPowerPolicy *s, uint32_t t, unsigned extra)
{
    fdPowerInit(s);
    CHECK(!fdPowerBegin(s, t, 1u, extra)); // no observed zero yet
    fdPowerOperatorZero(s);
    CHECK(fdPowerBegin(s, t, 1u, extra));
}
int main(void)
{
    FdPowerPolicy s;
    begin(&s, 0u, 40u);
    CHECK(floor_at(&s, 0u, 0u) == 400u);
    CHECK(floor_at(&s, ticks(99999), 0u) == 400u);
    CHECK(floor_at(&s, ticks(100000), 0u) == 400u);
    CHECK(floor_at(&s, ticks(110000), 0u) == 380u);
    CHECK(floor_at(&s, ticks(120000), 0u) == 360u);
    CHECK(!fdPowerBegin(&s, ticks(125000), 1u, 40u));
    CHECK(floor_at(&s, ticks(299999), 0u) == 360u);
    CHECK(floor_at(&s, ticks(300000), 0u) == 0u);
    CHECK(s.stop_reason == FD_POWER_WINDOW_END);
    CHECK(floor_at(&s, 0u, 0u) == 0u); // later counter wrap cannot undo stop

    begin(&s, 0u, 40u);
    CHECK(floor_at(&s, ticks(60000), 4u) == 400u);
    CHECK(floor_at(&s, ticks(61000), 5u) == 400u); // start early release
    CHECK(floor_at(&s, ticks(71000), 0u) == 380u); // lost ZC never renews boost
    CHECK(floor_at(&s, ticks(81000), 0u) == 360u);
    CHECK(s.phase == FD_POWER_DONE);

    begin(&s, 0u, 40u);
    CHECK(floor_at(&s, ticks(110000), 0u) == 380u); // missed hold/fade boundary
    begin(&s, 0u, 40u);
    CHECK(floor_at(&s, ticks(130000), 0u) == 360u); // do not start a late fade

    const uint32_t start = UINT32_MAX - ticks(10000);
    begin(&s, start, 40u);
    CHECK(floor_at(&s, start + ticks(99999), 0u) == 400u);
    CHECK(floor_at(&s, start + ticks(110000), 0u) == 380u);
    CHECK(floor_at(&s, start + ticks(120000), 0u) == 360u);
    CHECK(floor_at(&s, start + ticks(300000), 5u) == 0u);
    CHECK(s.stop_reason == FD_POWER_WINDOW_END); // normal acquisition also stops

    begin(&s, 0u, 40u);
    CHECK(fdPowerFloor(&s, 0u, 360, 610, 350, 0, 1) == 350u);
    CHECK(fdPowerFloor(&s, 0u, 360, 380, 2000, 0, 1) == 380u);
    CHECK(fdPowerFloor(&s, 0u, 360, 610, 0, 0, 1) == 0u);
    CHECK(fdPowerFloor(&s, 0u, 360, 610, 2000, 0, 0) == 0u); // fault/Coast
    CHECK(!fdPowerBegin(&s, ticks(100), 1u, 40u));
    CHECK(floor_at(&s, ticks(200), 0u) == 0u); // canceled, no renewed drive
    fdPowerOperatorZero(&s);
    CHECK(fdPowerBegin(&s, ticks(300), 1u, 40u));
    CHECK(floor_at(&s, ticks(300), 0u) == 400u);
    fdPowerOperatorZero(&s);
    CHECK(fdPowerFloor(&s, ticks(301), 360, 610, 2000, 0, 0) == 0u);
    CHECK(fdPowerBegin(&s, ticks(400), 1u, 40u));

    fdPowerOperatorZero(&s);
    CHECK(!fdPowerBegin(&s, 0u, 0u, 40u)); // not a permitted cold start
    CHECK(!fdPowerBegin(&s, 1u, 1u, 40u));
    fdPowerOperatorZero(&s);
    CHECK(!fdPowerBegin(&s, 0u, 1u, 81u)); // invalid policy configuration
    begin(&s, 0u, 0u);
    CHECK(floor_at(&s, 0u, 0u) == 360u); // proposed P0, no boost
    CHECK(floor_at(&s, ticks(110000), 0u) == 360u);
    CHECK(floor_at(&s, ticks(300000), 5u) == 0u); // same bench window for P0
    begin(&s, 0u, 80u);
    CHECK(floor_at(&s, 0u, 0u) == 440u); // later, NOT approved/tested on motor
    CHECK(fdPowerFloor(&s, 0u, 65535, 610, 2000, 0, 1) == 0u);
    CHECK(s.phase == FD_POWER_DONE);
    CHECK(s.stop_reason == FD_POWER_BAD_PARAMETERS);

    /* Bounded/non-increasing release for representative floor/cap extremes. */
    const uint16_t bases[] = {0u, 210u, 360u, 1999u, 2000u};
    const uint16_t caps[] = {0u, 200u, 380u, 610u, 2000u};
    for (unsigned b = 0; b < 5; b++) for (unsigned c = 0; c < 5; c++) {
        begin(&s, 0u, 80u);
        uint16_t last = 2000u;
        for (unsigned us = 0; us <= 150000; us += 1000) {
            uint16_t value = fdPowerFloor(&s, ticks(us), bases[b], 2000,
                caps[c], 0u, 1u);
            CHECK(value <= last && value <= caps[c] && value <= 2000u);
            last = value;
        }
    }
    printf("PASS: %u offline startup-power policy assertions.\n", checks);
    puts("Pure shared policy only: not a motor, ADC, ISR or hardware safety validation.");
}
