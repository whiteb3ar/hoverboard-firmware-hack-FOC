#include "defines_Gd32F130C8T6.h"
//#include "setup.h"
#include "motor.h"

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

void read_left_motor_hall(hall_state_t* hallState)
{
	hallState->u = digitalRead(HALL_A);
	hallState->v = digitalRead(HALL_B);
	hallState->w = digitalRead(HALL_C);
}

void set_left_motor_pwm(pwm_output_t *pwmOutput)
{
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_G, pwmOutput->u);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_B, pwmOutput->v);
	timer_channel_output_pulse_value_config(TIMER_BLDC, TIMER_BLDC_CHANNEL_Y, pwmOutput->w);
}

void set_right_motor_disabled(uint8_t disabled)
{
	//do nothing
}

void read_right_motor_hall(hall_state_t* hallState)
{
  //do nothing
}

void set_right_motor_pwm(pwm_output_t *pwmOutput)
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
