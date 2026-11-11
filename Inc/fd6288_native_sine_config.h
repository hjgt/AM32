#pragma once
/* Trial parameters applied in RAM on load. The fixed 12kHz comparison retains
 * the installed EEPROM page. Timing and auto/manual timing are inherited. */
#define FD_NATIVE_SINE_STARTUP_POWER 150u
#define FD_NATIVE_SINE_MOTOR_KV_CODE 22u /* 900 KV */
#define FD_NATIVE_SINE_MOTOR_POLES 14u
#define FD_NATIVE_SINE_RANGE 15u
#define FD_NATIVE_SINE_POWER 6u
#ifdef TARGET_FD_SINE_FIXED_12KHZ
#define FD_NATIVE_SINE_PWM_KHZ 12u
#else
#define FD_NATIVE_SINE_PWM_KHZ 24u
#endif
