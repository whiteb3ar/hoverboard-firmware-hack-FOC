#include "config.h"
#include "defines.h"

uint32_t DCLINK_PIN;
GpioPort DCLINK_PORT;
uint32_t BUZZER_PIN;
GpioPort BUZZER_PORT;

GpioPort OFF_PORT;
uint32_t OFF_PIN;
uint32_t BUTTON_PIN;
GpioPort BUTTON_PORT;
uint32_t CHARGER_PIN;
GpioPort CHARGER_PORT;
uint32_t PPM_PIN;
GpioPort PPM_PORT;
uint32_t PWM_PIN_CH1;
GpioPort PWM_PORT_CH1;
uint32_t PWM_PIN_CH2;
GpioPort PWM_PORT_CH2;
uint32_t BUTTON1_PIN;
GpioPort BUTTON1_PORT;
uint32_t BUTTON2_PIN;
GpioPort BUTTON2_PORT;

void init_board()
{
    if (BOARD_VARIANT == 0)
    {
        DCLINK_PIN = GPIO_PIN_2;
        DCLINK_PORT = GPIOC;
    }
    else if (BOARD_VARIANT == 1)
    {
        DCLINK_PIN = GPIO_PIN_1;
        DCLINK_PORT = GPIOA;
    }

    if (BOARD_VARIANT == 0)
    {
        BUZZER_PIN = GPIO_PIN_4;
        BUZZER_PORT = GPIOA;
    }
    else if (BOARD_VARIANT == 1)
    {
        BUZZER_PIN = GPIO_PIN_13;
        BUZZER_PORT = GPIOC;
    }

    if (BOARD_VARIANT == 0)
    {
        OFF_PIN = GPIO_PIN_5;
        OFF_PORT = GPIOA;
    }
    else if (BOARD_VARIANT == 1)
    {
        OFF_PIN = GPIO_PIN_15;
        OFF_PORT = GPIOC;
    }

    if (BOARD_VARIANT == 0)
    {
        BUTTON_PIN = GPIO_PIN_1;
        BUTTON_PORT = GPIOA;
    }
    else if (BOARD_VARIANT == 1)
    {
        BUTTON_PIN = GPIO_PIN_9;
        BUTTON_PORT = GPIOB;
    }

    if (BOARD_VARIANT == 0)
    {
        CHARGER_PIN = GPIO_PIN_12;
        CHARGER_PORT = GPIOA;
    }
    else if (BOARD_VARIANT == 1)
    {
        CHARGER_PIN = GPIO_PIN_11;
        CHARGER_PORT = GPIOA;
    }

    if (CONTROL_PPM_LEFT_ENABLED)
    {
        PPM_PIN = GPIO_PIN_3;
        PPM_PORT = GPIOA;
    }
    else if (CONTROL_PPM_RIGHT_ENABLED)
    {
        PPM_PIN = GPIO_PIN_11;
        PPM_PORT = GPIOB;
    }

    if (CONTROL_PWM_LEFT_ENABLED)
    {
        PWM_PIN_CH1 = GPIO_PIN_2;
        PWM_PORT_CH1 = GPIOA;
        PWM_PIN_CH2 = GPIO_PIN_3;
        PWM_PORT_CH2 = GPIOA;
    }
    else if (CONTROL_PWM_RIGHT_ENABLED)
    {
        PWM_PIN_CH1 = GPIO_PIN_10;
        PWM_PORT_CH1 = GPIOB;
        PWM_PIN_CH2 = GPIO_PIN_11;
        PWM_PORT_CH2 = GPIOB;
    }

    if (SUPPORT_BUTTONS_LEFT)
    {
        BUTTON1_PIN = GPIO_PIN_2;
        BUTTON1_PORT = GPIOA;
        BUTTON2_PIN = GPIO_PIN_3;
        BUTTON2_PORT = GPIOA;
    }
    else if (SUPPORT_BUTTONS_RIGHT)
    {
        BUTTON1_PIN = GPIO_PIN_10;
        BUTTON1_PORT = GPIOB;
        BUTTON2_PIN = GPIO_PIN_11;
        BUTTON2_PORT = GPIOB;
    }
}