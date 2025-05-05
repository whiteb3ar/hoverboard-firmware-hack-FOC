#define ARM_MATH_CM3



#include "defines_Gd32F130C8T6.h"
#include "setup.h"
#include "it.h"
#include "commsMasterSlave.h"

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <math.h>
#include <app.h>

#define STATE_Disable 64
#define STATE_Shutoff 128

//----------------------------------------------------------------------------
// MAIN function
//----------------------------------------------------------------------------
int main (void)
{
	#ifdef CHECK_BUTTON
		// Wait until button is released
		while (BUTTON_PUSHED == digitalRead(BUTTON)){fwdgt_counter_reload();} // Reload watchdog while button is pressed
		//while (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN)){fwdgt_counter_reload();} // Reload watchdog while button is pressed
		Delay(10); //debounce to prevent immediate ShutOff (100 is to much with a switch instead of a push button)
	#endif

	while(1)
	{
		
		#ifdef SLAVE	
		#else	//MASTER_OR_SINGLE
			#ifdef CHECK_BUTTON
				// Shut device off when button is pressed
				if (BUTTON_PUSHED == digitalRead(BUTTON))
				//if (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN))
				{
					while (BUTTON_PUSHED == digitalRead(BUTTON)) {fwdgt_counter_reload();}
					//while (gpio_input_bit_get(BUTTON_PORT, BUTTON_PIN)) {fwdgt_counter_reload();}
					ShutOff();
				}
			#endif
		#endif	

		
  }
}


void start_adc(void);
void light_led(void);
int is_button_pressed(void);
int is_uart3_available(void);

void uart2_init(void);
void uart3_init(void);

void uart3_transmit(uint8_t *data, int size);

void unit_uart2_dma(uint8_t *buffer, int size);
void unit_uart3_dma(uint8_t *buffer, int size);

void init_eeprom(void);
void read_configuration(uint16_t *buffer);
void read_configuration_value(uint16_t address, uint16_t *value);
void write_configuration(uint16_t *buffer);
void write_configuration_value(uint16_t address, uint16_t *value);

Hardware hardware = {
    .activate_latch = activate_latch,
    .hardware_init = hardware_init,
    .is_button_pressed = is_button_pressed,
    .light_led = light_led,
    .start_adc = start_adc,
    .is_uart3_available = is_uart3_available,
    .uart3_transmit = uart3_transmit,
    .unit_uart2_dma = unit_uart2_dma,
    .unit_uart3_dma = unit_uart3_dma,

    .init_eeprom = init_eeprom,
    .read_configuration = read_configuration,
    .read_configuration_value = read_configuration_value,

	.reset_watchog = reset_watchog,
    .reset = reset
};
