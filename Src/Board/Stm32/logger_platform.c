#include "setup.h"
#include "logger.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;

void uart_putchar(char *data)
{
  #if defined(DEBUG_SERIAL_USART2)
    HAL_UART_Transmit(&huart2, (uint8_t *)data, 1, 1000);
  #elif defined(DEBUG_SERIAL_USART3)
    HAL_UART_Transmit(&huart3, (uint8_t *)data, 1, 1000);
  #endif
}

Logger logger = {
    .putchar = uart_putchar
};
