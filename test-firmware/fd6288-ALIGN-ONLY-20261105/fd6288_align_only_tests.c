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
#define TARGET_FD_ALIGN_ONLY
#include "../Inc/fd6288_startup_sequence.h"
int main(void)
{
    for (unsigned wrap = 0; wrap < 2; ++wrap) {
        uint32_t start = wrap ? UINT32_MAX - 1000u : 1000u;
        FdRevPolicy p;
        fdRevBeginPolicy(&p, start);
        assert(fdRevAdvancePolicy(&p, start + FD_REV_ALIGN_TICKS - 1u) == FD_REV_NONE);
        assert(fdRevAdvancePolicy(&p, start + FD_REV_ALIGN_TICKS) == FD_REV_STEP);
        uint32_t second = p.sector_at;
        assert(p.align_left == 1u && p.stage == FD_REV_ALIGN);
        for (unsigned us = 0; us < 90000u; us += 100u) {
            uint32_t now = second + us * FD_REV_US;
            assert(!fdRevEdgePolicy(&p, now, 13u));
            assert(!p.confidence && !p.seen);
            assert(fdDoubleAlignDuty(&p, now) == 360u);
            assert(fdRevAdvancePolicy(&p, now) ==
                (us == 60000u ? FD_REV_ALIGN_MARK : FD_REV_NONE));
            assert(p.stage == FD_REV_ALIGN && p.sector_at == second);
            assert(!p.rescue_used && !p.reposition_used);
        }
        assert(!fdRevDeadlinePolicy(&p, start + FD_ALIGN_ONLY_TOTAL_TICKS - 1u, start));
        assert(fdRevDeadlinePolicy(&p, start + FD_ALIGN_ONLY_TOTAL_TICKS, start));
        assert(fdRevAdvancePolicy(&p, start + FD_ALIGN_ONLY_TOTAL_TICKS) == FD_REV_ALIGN_STOP);
        assert(p.stage == FD_REV_OFF);
        assert(fdRevAdvancePolicy(&p, start + 500000u * FD_REV_US) == FD_REV_NONE);
        /* Delayed mark does not extend the second vector's budget. */
        fdRevBeginPolicy(&p, start);
        assert(fdRevAdvancePolicy(&p, start + FD_REV_ALIGN_TICKS) == FD_REV_STEP);
        assert(fdRevAdvancePolicy(&p, start + 115000u * FD_REV_US) == FD_REV_ALIGN_MARK);
        assert(fdRevAdvancePolicy(&p, start + FD_ALIGN_ONLY_TOTAL_TICKS) == FD_REV_ALIGN_STOP);
        /* A missed entire hold window stops directly, never launches. */
        fdRevBeginPolicy(&p, start);
        assert(fdRevAdvancePolicy(&p, start + FD_REV_ALIGN_TICKS) == FD_REV_STEP);
        assert(fdRevAdvancePolicy(&p, start + 125000u * FD_REV_US) == FD_REV_ALIGN_STOP);
        /* Absolute deadline also bounds a delayed first transition. */
        fdRevBeginPolicy(&p, start);
        assert(fdRevAdvancePolicy(&p, start + 50000u * FD_REV_US) == FD_REV_STEP);
        assert(fdRevDeadlinePolicy(&p, start + FD_ALIGN_ONLY_TOTAL_TICKS, start));
        /* Unexpected acquisition state fails closed, not a retry. */
        p.stage = FD_REV_OPEN;
        assert(fdRevAdvancePolicy(&p, start) == FD_REV_ALIGN_STOP);
        fdRevBeginPolicy(&p, start);
        assert(p.stage == FD_REV_ALIGN && p.align_left == 2u);
    }
    puts("PASS align-only: original dwell/power, mark without launch, bounded hold, ignored edges, no retries, delayed service, wrap, restart");
}
