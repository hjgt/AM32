/*
 * Documentation stub only -- this file is not included by the build.
 *
 * The canonical and complete CUSTOM_FD6288_G431 target is the matching block
 * in ../targets.h. Keeping a second set of active macros here previously left
 * two conflicting pin maps (notably PA0/PA3 voltage sense and PA5 neutral).
 * Do not add target defines to this file.
 *
 * Current canonical map summary:
 *   BEMF ADC A/B/C : PA0 / PA1 / PA2
 *   voltage ADC    : PA5 (ADC2_IN13)
 *   current ADC    : PA6 (ADC2_IN3)
 *   input / serial : PB5 / PB6
 *   high-side PWM  : PA8 / PA9 / PA10
 *   low-side PWM   : PA11 / PA12 / PF0
 *   software neutral is (A+B+C)/3; PA3 is not the voltage-sense pin.
 */
