/*
 * This file implements FOC motor control.
 * This control method offers superior performanace
 * compared to previous cummutation method. The new method features:
 * ► reduced noise and vibrations
 * ► smooth torque output
 * ► improved motor efficiency -> lower energy consumption
 *
 * Copyright (C) 2019-2020 Emanuel FERU <aerdronix@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "defines.h"
//#include "setup.h"
#include "config.h"
#include "app.h"
#include "buzzer.h"
#include "motor.h"

// Matlab includes and defines - from auto-code generation
// ###############################################################################
#include "BLDC_controller.h" /* Model's header file */
#include "rtwtypes.h"

extern RT_MODEL *const rtM_Left;
extern RT_MODEL *const rtM_Right;

extern DW rtDW_Left;  /* Observable states */
extern ExtU rtU_Left; /* External inputs */
extern ExtY rtY_Left; /* External outputs */
extern P rtP_Left;

extern DW rtDW_Right;  /* Observable states */
extern ExtU rtU_Right; /* External inputs */
extern ExtY rtY_Right; /* External outputs */
// ###############################################################################

extern uint8_t ctrlModReq;

extern volatile int pwml;
extern volatile int pwmr;
extern volatile adc_buf_t adc_buffer;

extern Hardware hardware;
extern Buzzer buzzer;

extern Motor motor_left;
extern Motor motor_right;


static int16_t pwm_margin; /* This margin allows to have a window in the PWM signal for proper FOC Phase currents measurement */
static int16_t curDC_max = (I_DC_MAX * A2BIT_CONV);
int16_t curL_phaA = 0, curL_phaB = 0, curL_DC = 0;
int16_t curR_phaB = 0, curR_phaC = 0, curR_DC = 0;

volatile uint32_t bldc_timer = 0;

uint8_t enable = 0; // initially motors are disabled for SAFETY
static uint8_t enableFin = 0;

static const uint16_t pwm_res = 64000000 / 2 / PWM_FREQ; // = 2000

static uint16_t offsetcount = 0;
static int16_t offsetrlA = 2000;
static int16_t offsetrlB = 2000;
static int16_t offsetrrB = 2000;
static int16_t offsetrrC = 2000;
static int16_t offsetdcl = 2000;
static int16_t offsetdcr = 2000;

int16_t batVoltage = (400 * BAT_CELLS * BAT_CALIB_ADC) / BAT_CALIB_REAL_VOLTAGE;
static int32_t batVoltageFixdt = (400 * BAT_CELLS * BAT_CALIB_ADC) / BAT_CALIB_REAL_VOLTAGE << 16; // Fixed-point filter output initialized at 400 V*100/cell = 4 V/cell converted to fixed-point

void main_bldc_irq_loop()
{
  bldc_timer++;

  set_buzzer_next_state(&buzzer, bldc_timer);

  if (buzzer.state == BUZZER_TOGGLE)
  {
    hardware.toggle_buzzer();
  }
  else if (buzzer.state == BUZZER_OFF)
  {
    hardware.switch_buzzer_off();
  }
  return;
  
  if (offsetcount < 2000)
  { // calibrate ADC offsets
    offsetcount++;

    offsetrlA = (adc_buffer.rlA + offsetrlA) / 2;
    offsetrlB = (adc_buffer.rlB + offsetrlB) / 2;
    offsetrrB = (adc_buffer.rrB + offsetrrB) / 2;
    offsetrrC = (adc_buffer.rrC + offsetrrC) / 2;
    offsetdcl = (adc_buffer.dcl + offsetdcl) / 2;
    offsetdcr = (adc_buffer.dcr + offsetdcr) / 2;

    return;
  }

  if (bldc_timer % 1000 == 0)
  { // Filter battery voltage at a slower sampling rate
    filtLowPass32(adc_buffer.batt1, BAT_FILT_COEF, &batVoltageFixdt);
    batVoltage = (int16_t)(batVoltageFixdt >> 16); // convert fixed-point to integer
  }

  // Get Left motor currents
  curL_phaA = (int16_t)(offsetrlA - adc_buffer.rlA);
  curL_phaB = (int16_t)(offsetrlB - adc_buffer.rlB);
  curL_DC = (int16_t)(offsetdcl - adc_buffer.dcl);

  // Get Right motor currents
  curR_phaB = (int16_t)(offsetrrB - adc_buffer.rrB);
  curR_phaC = (int16_t)(offsetrrC - adc_buffer.rrC);
  curR_DC = (int16_t)(offsetdcr - adc_buffer.dcr);

  // Disable PWM when current limit is reached (current chopping)
  // This is the Level 2 of current protection. The Level 1 should kick in first given by I_MOT_MAX
  motor_left.set_disabled(ABS(curL_DC) > curDC_max || enable == 0);
  motor_right.set_disabled(ABS(curR_DC) > curDC_max || enable == 0);

  // Create square wave for buzzer
  bldc_timer++;

  set_buzzer_next_state(&buzzer, bldc_timer);

  if (buzzer.state == BUZZER_TOGGLE)
  {
    hardware.toggle_buzzer();
  }
  else if (buzzer.state == BUZZER_OFF)
  {
    hardware.switch_buzzer_off();
  }

  // Adjust pwm_margin depending on the selected Control Type
  if (rtP_Left.z_ctrlTypSel == FOC_CTRL)
  {
    pwm_margin = 110;
  }
  else
  {
    pwm_margin = 0;
  }

  // ############################### MOTOR CONTROL ###############################
  static boolean_T OverrunFlag = false;

  /* Check for overrun */
  if (OverrunFlag)
  {
    return;
  }

  OverrunFlag = true;

  /* Make sure to stop BOTH motors in case of an error */
  enableFin = enable && !rtY_Left.z_errCode && !rtY_Right.z_errCode;

  // ========================= LEFT MOTOR ============================
  // Get hall sensors values

  hall_state_t left_hall;
  motor_left.read_hall(&left_hall);

  /* Set motor inputs here */
  rtU_Left.b_motEna = enableFin;
  rtU_Left.z_ctrlModReq = ctrlModReq;
  rtU_Left.r_inpTgt = pwml;
  rtU_Left.b_hallA = left_hall.u;
  rtU_Left.b_hallB = left_hall.v;
  rtU_Left.b_hallC = left_hall.w;
  rtU_Left.i_phaAB = curL_phaA;
  rtU_Left.i_phaBC = curL_phaB;
  rtU_Left.i_DCLink = curL_DC;
// rtU_Left.a_mechAngle   = ...; // Angle input in DEGREES [0,360] in fixdt(1,16,4) data type. If `angle` is float use `= (int16_t)floor(angle * 16.0F)` If `angle` is integer use `= (int16_t)(angle << 4)`

/* Step the controller */
#ifdef MOTOR_LEFT_ENA
  BLDC_controller_step(rtM_Left);
#endif

  /* Get motor outputs here */
  int ul = rtY_Left.DC_phaA;
  int vl = rtY_Left.DC_phaB;
  int wl = rtY_Left.DC_phaC;
  // errCodeLeft  = rtY_Left.z_errCode;
  // motSpeedLeft = rtY_Left.n_mot;
  // motAngleLeft = rtY_Left.a_elecAngle;

  /* Apply commands */
  pwm_output_t left_pwm;

  left_pwm.u = (uint16_t)CLAMP(ul + pwm_res / 2, pwm_margin, pwm_res - pwm_margin);
  left_pwm.v = (uint16_t)CLAMP(vl + pwm_res / 2, pwm_margin, pwm_res - pwm_margin);
  left_pwm.w = (uint16_t)CLAMP(wl + pwm_res / 2, pwm_margin, pwm_res - pwm_margin);

  motor_left.set_pwm(&left_pwm);
  // =================================================================

  // ========================= RIGHT MOTOR ===========================
  // Get hall sensors values
  hall_state_t right_hall;
  motor_right.read_hall(&right_hall);

  /* Set motor inputs here */
  rtU_Right.b_motEna = enableFin;
  rtU_Right.z_ctrlModReq = ctrlModReq;
  rtU_Right.r_inpTgt = pwmr;
  rtU_Right.b_hallA = right_hall.u;
  rtU_Right.b_hallB = right_hall.v;
  rtU_Right.b_hallC = right_hall.w;
  rtU_Right.i_phaAB = curR_phaB;
  rtU_Right.i_phaBC = curR_phaC;
  rtU_Right.i_DCLink = curR_DC;
// rtU_Right.a_mechAngle   = ...; // Angle input in DEGREES [0,360] in fixdt(1,16,4) data type. If `angle` is float use `= (int16_t)floor(angle * 16.0F)` If `angle` is integer use `= (int16_t)(angle << 4)`

/* Step the controller */
#ifdef MOTOR_RIGHT_ENA
  BLDC_controller_step(rtM_Right);
#endif

  /* Get motor outputs here */
  int ur = rtY_Right.DC_phaA;
  int vr = rtY_Right.DC_phaB;
  int wr = rtY_Right.DC_phaC;
  // errCodeRight  = rtY_Right.z_errCode;
  // motSpeedRight = rtY_Right.n_mot;
  // motAngleRight = rtY_Right.a_elecAngle;

  /* Apply commands */
  pwm_output_t right_pwm;

  right_pwm.u = (uint16_t)CLAMP(ur + pwm_res / 2, pwm_margin, pwm_res - pwm_margin);
  right_pwm.v = (uint16_t)CLAMP(vr + pwm_res / 2, pwm_margin, pwm_res - pwm_margin);
  right_pwm.w = (uint16_t)CLAMP(wr + pwm_res / 2, pwm_margin, pwm_res - pwm_margin);

  motor_right.set_pwm(&right_pwm);
  // =================================================================

  /* Indicate task complete */
  OverrunFlag = false;

  // ###############################################################################
}
