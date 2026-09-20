/* OFFLINE DESIGN PROTOTYPE ONLY. Not included by firmware or its Makefile.
 * No GPIO, PWM, commutation, protection-counter, EEPROM or boot operations.
 * Single-owner state; interrupt integration has NOT been implemented/tested.
 */
#ifndef FD_STARTUP_POWER_POLICY_H
#define FD_STARTUP_POWER_POLICY_H
#include <stdint.h>

enum { FD_POWER_DONE, FD_POWER_HOLD, FD_POWER_FADE };
enum { FD_POWER_NO_STOP, FD_POWER_WINDOW_END, FD_POWER_DRIVE_DISABLED,
    FD_POWER_BAD_PARAMETERS };
enum {
    FD_POWER_TICKS_PER_US = 160u, /* raw 160 MHz cycle counter, not CNT/160 */
    FD_POWER_HOLD_TICKS = 100000u * FD_POWER_TICKS_PER_US,
    FD_POWER_FADE_TICKS = 20000u * FD_POWER_TICKS_PER_US,
    FD_POWER_TRIAL_TICKS = 300000u * FD_POWER_TICKS_PER_US,
    FD_POWER_MAX_EXTRA = 80u, /* arithmetic bound, NOT a proven safe motor value */
    FD_POWER_RELEASE_CROSSINGS = 5u
};
typedef struct {
    uint32_t started, fade_elapsed;
    uint16_t extra;
    uint8_t ready, phase, trial_active, stop_reason;
} FdPowerPolicy;

static inline void fdPowerInit(FdPowerPolicy *s)
{
    *s = (FdPowerPolicy){0}; /* observing a genuine operator zero is required */
}

/* Call ONLY for explicit decoded operator zero, not internally forced input=0.
 * This does not authorize a start: native arm/Coast/fault checks still apply.
 */
static inline void fdPowerOperatorZero(FdPowerPolicy *s)
{
    s->ready = 1u;
    s->phase = FD_POWER_DONE;
    s->extra = 0u;
    s->trial_active = 0u;
    s->stop_reason = FD_POWER_NO_STOP;
}

/* Invoked only for a native, permitted cold-start attempt. It never starts a
 * motor itself. Extra=40 is proposed P1; extra=80 is a reserved later trial.
 * No second pulse in a positive-throttle episode, even after desync/fault.
 */
static inline uint8_t fdPowerBegin(FdPowerPolicy *s, uint32_t now,
    uint8_t cold_start_allowed, uint16_t extra)
{
    if (!s->ready) return 0u;
    s->ready = 0u;
    s->phase = FD_POWER_DONE;
    if (!cold_start_allowed || extra > FD_POWER_MAX_EXTRA) return 0u;
    s->started = now;
    s->extra = extra;
    s->fade_elapsed = 0u;
    s->phase = extra ? FD_POWER_HOLD : FD_POWER_DONE;
    s->trial_active = 1u;
    s->stop_reason = FD_POWER_NO_STOP;
    return 1u;
}

static inline void fdPowerRequestStop(FdPowerPolicy *s, uint8_t reason)
{
    s->ready = 0u;
    s->phase = FD_POWER_DONE;
    s->trial_active = 0u;
    s->stop_reason = reason;
}

/* Return a floor REQUEST in AM32's 0..2000 units, not a PWM command. Apply
 * only within the existing startup-floor branch, BEFORE the ordinary safety
 * caps and ramp. Existing final caps and C06 clean-window stop remain needed.
 * true_crossings is the private consecutive DRIVE count, NEVER zero_crosses
 * from variant A (which includes forced steps). A value of five ends BOOST,
 * it does not authorize switching commutation mode or certify physical lock.
 * hard_cap is the computed effective ceiling: 0 here means no permitted drive,
 * NOT the configurator's "current limit disabled" value.
 * Both proposed P0/P1 have a 300 ms BENCH trial window, even on success.
 * stop_reason is a request for a latched bridge shutdown, NOT an implemented
 * shutdown. Setting a floor to zero alone cannot stop an AM32 running motor.
 * The future adapter MUST service the latch independently of startup-floor
 * eligibility, and protect pending commutations/final PWM writes. This is a
 * deployment blocker, not a feature already present in the flashed firmware.
 * Services must be frequent; this is not a hardware energy/current limiter.
 */
static inline uint16_t fdPowerFloor(FdPowerPolicy *s, uint32_t now,
    uint16_t base, uint16_t startup_cap, uint16_t hard_cap,
    uint8_t true_crossings, uint8_t drive_allowed)
{
    if (s->stop_reason != FD_POWER_NO_STOP) return 0u;
    if (base > 2000u || startup_cap > 2000u || hard_cap > 2000u) {
        fdPowerRequestStop(s, FD_POWER_BAD_PARAMETERS);
        return 0u;
    }
    if (!drive_allowed || !hard_cap) {
        if (s->trial_active) fdPowerRequestStop(s, FD_POWER_DRIVE_DISABLED);
        return 0u;
    }
    if (s->trial_active && (uint32_t)(now - s->started) >= FD_POWER_TRIAL_TICKS) {
        fdPowerRequestStop(s, FD_POWER_WINDOW_END);
        return 0u;
    }
    uint32_t extra = 0u;
    if (s->phase != FD_POWER_DONE) {
        uint32_t elapsed = now - s->started; /* wrap-safe raw cycle difference */
        if (elapsed >= FD_POWER_HOLD_TICKS + FD_POWER_FADE_TICKS) {
            s->phase = FD_POWER_DONE;
        } else {
            if (s->phase == FD_POWER_HOLD &&
                (true_crossings >= FD_POWER_RELEASE_CROSSINGS ||
                 elapsed >= FD_POWER_HOLD_TICKS)) {
                s->phase = FD_POWER_FADE;
                /* A delayed service must not extend the nominal deadline. */
                s->fade_elapsed = elapsed < FD_POWER_HOLD_TICKS ?
                    elapsed : FD_POWER_HOLD_TICKS;
            }
            if (s->phase == FD_POWER_HOLD) {
                extra = s->extra;
            } else {
                uint32_t faded = elapsed - s->fade_elapsed;
                if (faded >= FD_POWER_FADE_TICKS) {
                    s->phase = FD_POWER_DONE;
                } else {
                    extra = (uint32_t)s->extra *
                        (FD_POWER_FADE_TICKS - faded) / FD_POWER_FADE_TICKS;
                }
            }
        }
    }
    uint32_t floor = (uint32_t)base + extra;
    if (floor > startup_cap) floor = startup_cap;
    if (floor > hard_cap) floor = hard_cap;
    return (uint16_t)floor;
}
#endif
