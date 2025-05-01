#define ARM_MATH_CM3



#include "defines_Gd32F130C8T6.h"
#include "setup.h"
#include "it.h"
#include "commsMasterSlave.h"

#include "commsSteering.h"

#include "commsBluetooth.h"
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

void set_left_motor_disabled(uint8_t disabled)
{
	if (disabled)
	{
		timer_automatic_output_disable(TIMER_BLDC);
	}
	else
	{
		timer_automatic_output_enable(TIMER_BLDC);
	}
}

void read_left_motor_hall(uint8_t *values)
{
	values[0] = digitalRead(HALL_A);
	values[1] = digitalRead(HALL_B);
	values[2] = digitalRead(HALL_C);
}

void set_left_motor_pwm(uint16_t u, uint16_t v, uint16_t w)
{
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, u);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, v);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, w);
}

void set_right_motor_disabled(uint8_t disabled)
{
	//do nothing
}

void read_right_motor_hall(uint8_t *values)
{
  //do nothing
}

void set_right_motor_pwm(uint16_t u, uint16_t v, uint16_t w)
{
  //do nothing
}

Motor motor_left = {
    .read_hall = read_left_motor_hall,
    .set_disabled = set_left_motor_disabled,
    .set_pwm = set_left_motor_pwm
};

Motor motor_right = {
    .read_hall = read_right_motor_hall,
    .set_disabled = set_right_motor_disabled,
    .set_pwm = set_right_motor_pwm
};

Logger logger = {
    .uart2_putchar = uart2_putchar,
    .uart3_putchar = uart3_putchar
};

void hardware_init(void) {
	//SystemClock_Config();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / 1000);	//  Configure SysTick to generate an interrupt every millisecond

	// Init watchdog
	if (Watchdog_init() == ERROR)
	{
		while(1)
		{
			//error
		}
	}

	Interrupt_init();
	TimeoutTimer_init();
	GPIO_init();

	#ifdef USART0_BAUD
			USART0_Init(USART0_BAUD);
	#endif
	#ifdef USART1_BAUD
			USART1_Init(USART1_BAUD);
	#endif

	ADC_init();
	PWM_init();
	
	// Device has 1,6 seconds to do all the initialization
	// afterwards watchdog will be fired
	fwdgt_counter_reload();
}

void activate_latch(void) {
	#ifdef SELF_HOLD
		// Activate self hold direct after GPIO-init
		digitalWrite(SELF_HOLD,SET);
	#endif
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

FlagStatus buzzerToggle = RESET;

void toggle_buzzer(void) {
	buzzerToggle = buzzerToggle == RESET ? SET : RESET;

	digitalWrite(BUZZER,buzzerToggle);
}

void switch_buzzer_off(void) {
	digitalWrite(BUZZER, RESET);
}

void reset_watchog(void)
{
	fwdgt_counter_reload();
}

void reset(void)
{
	#ifdef USART_MASTERSLAVE
		#ifdef MASTER
			wStateSlave = STATE_Shutoff;
			SendSlave(0);
		#endif
	
		usart_deinit(USART_MASTERSLAVE);
	#endif
	
	#ifdef SELF_HOLD
		digitalWrite(SELF_HOLD, RESET);
	#endif

	while(1)
	{
		// Reload watchdog until device is off
		fwdgt_counter_reload();
	}
}

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
