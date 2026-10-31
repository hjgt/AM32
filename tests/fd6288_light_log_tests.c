#include <assert.h>
#define TARGET_FD_NATIVE_EARLY
#include "../Inc/fd6288_light_log.h"
int main(void)
{
    assert(sizeof(FdLightRecord) == 48u);
    for (unsigned i = 1; i <= 40; ++i) {
        fdLightBegin(UINT32_MAX - 100u);
        fdLightEdge(59u, 1u); /* DWT wrap: 160 cycles */
        fdLightRetry();
        fdLightEdge(219u, 6u);
        fdLightHandoff(379u);
        fdLightFinish(7u, 4u, 0u, 123u, 42);
        fdLightFinish(9u, 0u, 0u, 0u, 0); /* no double completion */
        const volatile FdLightRecord *r = &fd_light_log.records[(i - 1u) % 32u];
        assert(r->id == i && r->edges == 2 && r->retries == 1);
        assert(r->first_edge == 160 && r->handoff == 480);
        assert(r->max_streak == 6 && r->reason == 7);
    }
    assert(fd_light_log.completed == 40);
    fdLightBegin(0);
    fdLightFinish(8, 2, 400, 45001, 250);
    assert(fd_light_log.records[8].handoff == UINT32_MAX);
    assert(fd_light_log.records[8].edges == 0);
    return 0;
}
