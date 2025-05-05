#pragma once

#include <stdint.h>
#include <stdbool.h>


typedef struct {
    void (*putchar)(char *);
} Logger;
