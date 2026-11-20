#include <assert.h>
#include <stdio.h>
#define TARGET_STARTUP_SEQUENCE_V2
#define TARGET_STARTUP_FEEDBACK
#define TARGET_STARTUP_S4
#define TARGET_STARTUP_S4E
#define TARGET_STARTUP_HANDOFF_REAL
#define TARGET_STARTUP_RESCUE_RETAIN
#define TARGET_STARTUP_RESCUE_BRIDGE
#define TARGET_STARTUP_PHASE_DELAY
#define TARGET_STARTUP_SEED_DELAY
#define TARGET_STARTUP_RESCUE_RAMP
#define TARGET_STARTUP_FIRST_WAIT
#define TARGET_FD_DOUBLE_ALIGN
#define TARGET_FD_ALIGN_HOLD
#define TARGET_FD_ALIGN_SETTLE
#define TARGET_FD_ALIGN_BUDGET
#define TARGET_FD_LAUNCH_ADVANCE
#define TARGET_FD_REPOSITION
#define TARGET_FD_FULL_RETRY
#define TARGET_FD_RETRY_ADJACENT
#define TARGET_FD_STALE_RETRY
#include "../Inc/fd6288_startup_sequence.h"
int main(void)
{
    assert(FD_REV_SECOND_ALIGN_TICKS == 300000u * FD_REV_US);
    assert(FD_REV_ACQUIRE_TICKS == 630000u * FD_REV_US);
    assert(FD_FULL_RETRY_ENTRY_END == 650000u * FD_REV_US);
    assert(FD_FULL_RETRY_END == 1300000u * FD_REV_US);
    for (unsigned wrap = 0; wrap < 2; ++wrap) {
        uint32_t start = wrap ? UINT32_MAX - 1000u : 1000u;
        FdRevPolicy p;
        fdRevBeginPolicy(&p, start);
        unsigned launches = 0, releases = 0, realigns = 0;
        uint32_t realigned = 0, released = 0;
        for (unsigned us = 0; us <= 1300000u; us += 10u) {
            uint32_t now = start + us * FD_REV_US;
            if (fdRevDeadlinePolicy(&p, now, start)) {
                assert(us == 1300000u);
                break;
            }
            if (p.stage == FD_REV_ALIGN) {
                assert(!fdRevEdgePolicy(&p, now, 13u));
                if (p.align_left == 1u) assert(fdDoubleAlignDuty(&p, now) == 360u);
            }
            unsigned action = fdRevAdvancePolicy(&p, now);
            if (action == FD_REV_LAUNCH) {
                ++launches;
                if (launches == 1u) assert(us == 330000u);
                else assert(launches == 2u && us - realigned == 330000u);
                assert(p.stage == FD_REV_OPEN);
            } else if (action == FD_REV_RELEASE) {
                ++releases; released = us;
                assert(us >= 630000u && us < 650000u);
            } else if (action == FD_REV_REALIGN) {
                ++realigns; realigned = us;
                assert(us - released == 20000u);
            }
            assert(!p.confidence && !p.seen);
        }
        assert(launches == 2u && releases == 1u && realigns == 1u);
        assert(p.reposition_used);
    }
    puts("PASS long alignment: 330ms launch, same power, 300ms acquisition per attempt, one retry, 1300ms cutoff, wrap");
}
