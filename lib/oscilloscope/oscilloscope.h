#pragma once

#include <stdint.h>

#define OSCILLOSCOPE_TIMER0_UPDATE 1
#define OSCILLOSCOPE_TIMER0_CH0_UPDATE 2
#define OSCILLOSCOPE_TIMER0_CH1_UPDATE 3
#define OSCILLOSCOPE_TIMER0_CH2_UPDATE 4

#define OSCILLOSCOPE_PHASE_CURRENTs_ADC_START 4
#define OSCILLOSCOPE_PHASE_CURRENTs_ADC_READY 5

#define OSCILLOSCOPE_DC_LINK_ADC_START 6
#define OSCILLOSCOPE_DC_LINK_ADC_READY 7

#define OSCILLOSCOPE_MAIN_BLDC_LOOP 8

#define OSCILLOSCOPE_GENERAL_ADC_START 9
#define OSCILLOSCOPE_GENERAL_TEMPERATURE_ADC_READY 10

typedef enum { ACTIVE = 0, FINISHED = !ACTIVE } OscilloscopeStatus;

typedef struct {
    uint16_t* data;
    uint16_t size;
    uint16_t index;
    OscilloscopeStatus status;
} Oscilloscope;

void oscilloscope_init(Oscilloscope* oscilloscope, uint16_t* data, uint16_t size);
void oscilloscope_update(Oscilloscope* oscilloscope, uint16_t type, uint16_t time, uint16_t data);

extern volatile Oscilloscope oscilloscope;
