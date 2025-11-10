#include "hal/hal.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void hal_delay(uint32_t ms) {
#ifdef _WIN32
    Sleep(ms);  // Windows Sleep takes milliseconds
#else
    usleep(ms * 1000);  // usleep takes microseconds, so multiply by 1000
#endif
}
