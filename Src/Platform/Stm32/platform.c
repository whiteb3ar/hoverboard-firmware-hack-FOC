#include "platform.h"
#include "stm32f1xx_hal.h"

void delay(uint16_t ms) {
    HAL_Delay(ms);
}
