
#include "./defines_Gd32F130C8T6.h"
#include <stdio.h>

// Internal constants
const int16_t pwm_res = 72000000 / 2 / PWM_FREQ; // = 2250

// Global variables for voltage and current
//float batteryVoltage = BAT_CELLS * 3.6;
//float currentDC = 0.42;		// to see in serial log that pin is not defined

// Calculation-Routine for BLDC => calculates with 16kHz
void CalculateBLDC(void)
{
	// Calculate battery voltage every 100 cycles
	#ifdef VBATT
		//batteryVoltage = batteryVoltage * 0.999 + ((float)adc_buffer.v_batt * ADC_BATTERY_VOLT) * 0.001;
	#endif
	
	// Calculate current DC
	#ifdef CURRENT_DC
		//currentDC = ABS((adc_buffer.current_dc - offsetdc) * MOTOR_AMP_CONV_DC_AMP);
	#endif
}
