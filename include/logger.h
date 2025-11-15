#pragma once
#include <stdint.h>

// External switch state
extern uint8_t g_log_switch_state;

void log(const char *format, ...);

#define LOG(...) log(__VA_ARGS__)
