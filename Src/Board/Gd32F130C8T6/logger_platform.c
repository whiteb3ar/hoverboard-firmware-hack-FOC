#include "Board/Gd32F130C8T6/comms.h"
#include "board.h"
#include "logger.h"
#include "config.h"

void uart_putchar(char *data)
{
  #if defined(DEBUG_SERIAL_USART2)
    SendBuffer(USART0, (uint8_t *)data, 1);
  #elif defined(DEBUG_SERIAL_USART3)
    SendBuffer(USART1, (uint8_t *)data, 1);
  #endif
}

Logger logger = {
    .putchar = uart_putchar
};
