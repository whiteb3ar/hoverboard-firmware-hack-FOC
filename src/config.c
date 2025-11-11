#include <config.h>

uint32_t PWM_FREQ;
uint32_t PWM_RES;

uint32_t DEAD_TIME;

/* Runtime definitions for motor enable flags. Defaults match previous behavior
 * (motors enabled unless explicitly disabled).
 */
uint8_t MOTOR_LEFT_ENA = 1;
uint8_t MOTOR_RIGHT_ENA = 1;

void initialize_config() {
    PWM_FREQ = 16000;
    PWM_RES  = 64000000 / 2 / PWM_FREQ; // = 2000

    DEAD_TIME = 48;
}