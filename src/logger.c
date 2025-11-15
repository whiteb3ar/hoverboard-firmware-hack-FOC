#include <stdio.h>
#include <stdarg.h>
#include "logger.h"

uint8_t g_log_switch_state = 1;

void log(const char *format, ...) {
    if (!g_log_switch_state) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
