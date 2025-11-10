#include "hal/hal.h"
#include "stm32f1xx_hal.h"

void hal_delay(uint32_t ms) {
    HAL_Delay(ms);
}