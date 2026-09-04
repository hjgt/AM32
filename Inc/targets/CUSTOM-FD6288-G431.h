// CUSTOM-FD6288-G431.h
// Custom target for STM32G431KBU6 + FD6288 ESC board
//
// Pin mapping (measured from schematic):
// HIN1=PA8(TIM1_CH1), HIN2=PA9(TIM1_CH2), HIN3=PA10(TIM1_CH3)
// LIN1=PA11(TIM1_CH1N), LIN2=PA12(TIM1_CH2N), LIN3=PF0(TIM1_CH3N)
// DSHOT input = PB5
// Serial telemetry TX = PB6 (USART1)
// Phase A BEMF ADC = PA0 (5.1K to A, 7.1K to B/C, 1K to GND)
// Phase B BEMF ADC = PA1 (5.1K to B, 7.1K to A/C, 1K to GND)
// Phase C BEMF ADC = PA2 (5.1K to C, 7.1K to A/B, 1K to GND)
// Virtual neutral ADC = PA5 (7.1K to A/B/C, 1K to GND)
//
// NOTE: COMP2 hardware ZCD is NOT usable on this board.
//   PA2 (C-phase BEMF) and PA5 (virtual neutral) are BOTH COMP2 negative inputs.
//   ADC-based ZCD (SOFTWARE polling) is used instead via PHASE_x_ADC + VIRTUAL_NEUTRAL_ADC macros.

#define MCU_G431
#define SERIAL_PIN_PB6
#define INPUT_PIN_PB5

// Voltage monitoring (shared with Phase A ADC on PA0)
#define VOLTAGE_ADC_PA0

// ADC-based BEMF zero crossing detection (USE_ADC mode)
// This bypasses the hardware COMP and polls ADC values instead
#define PHASE_A_ADC_PA0
#define PHASE_B_ADC_PA1
#define PHASE_C_ADC_PA2
#define VIRTUAL_NEUTRAL_ADC_PA5

// High-side PWM outputs (TIM1 CH1/2/3)
#define PHASE_A_PWM_PA8
#define PHASE_B_PWM_PA9
#define PHASE_C_PWM_PA10

// Low-side PWM outputs (TIM1 CH1N/2N/3N)
#define PHASE_A_LPWM_PA11
#define PHASE_B_LPWM_PA12
#define PHASE_C_LPWM_PF0

// Dead time: 70 counts @ 160MHz = ~437ns, suitable for FD6288
#define DEAD_TIME 70

#define VARIABLE_PWM
#define USE_DSHOT_TELEMETRY
#define FIRMWARE_NAME "CUSTOM-FD6288-G431"
