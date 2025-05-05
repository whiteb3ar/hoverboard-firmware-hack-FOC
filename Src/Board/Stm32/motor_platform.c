#include "setup.h"
#include "motor.h"

void set_left_motor_disabled(uint8_t disabled)
{
  if (disabled)
  {
    LEFT_TIM->BDTR &= ~TIM_BDTR_MOE;
  }
  else
  {
    LEFT_TIM->BDTR |= TIM_BDTR_MOE;
  }
}

void read_left_motor_hall(hall_state_t* hallState)
{
  hallState->u = !(LEFT_HALL_U_PORT->IDR & LEFT_HALL_U_PIN);
  hallState->v = !(LEFT_HALL_V_PORT->IDR & LEFT_HALL_V_PIN);
  hallState->w = !(LEFT_HALL_W_PORT->IDR & LEFT_HALL_W_PIN);
}

void set_left_motor_pwm(pwm_output_t *pwmOutput)
{
  LEFT_TIM->LEFT_TIM_U = pwmOutput->u;
  LEFT_TIM->LEFT_TIM_V = pwmOutput->v;
  LEFT_TIM->LEFT_TIM_W = pwmOutput->w;
}

void set_right_motor_disabled(uint8_t disabled)
{
  if (disabled)
  {
    RIGHT_TIM->BDTR &= ~TIM_BDTR_MOE;
  }
  else
  {
    RIGHT_TIM->BDTR |= TIM_BDTR_MOE;
  }
}

void read_right_motor_hall(hall_state_t* hallState)
{
  hallState->u = !(RIGHT_HALL_U_PORT->IDR & RIGHT_HALL_U_PIN);
  hallState->v = !(RIGHT_HALL_V_PORT->IDR & RIGHT_HALL_V_PIN);
  hallState->w = !(RIGHT_HALL_W_PORT->IDR & RIGHT_HALL_W_PIN);
}

void set_right_motor_pwm(pwm_output_t *pwmOutput)
{
  RIGHT_TIM->RIGHT_TIM_U = pwmOutput->u;
  RIGHT_TIM->RIGHT_TIM_V = pwmOutput->v;
  RIGHT_TIM->RIGHT_TIM_W = pwmOutput->w;
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
