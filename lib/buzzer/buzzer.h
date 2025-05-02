#ifndef BUZZER_H
// #define BUZZER_H

// #include "defines.h"
#include <stdint.h>

// #ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    BUZZER_OFF,
    BUZZER_ON,

    BUZZER_IDLE,
    BUZZER_TOGGLE
} BuzzerState;

#define LOW_PITCH 24
#define MEDIUM_PITCH 10
#define HIGH_PITCH 5

typedef struct {
    /**
     * How many timer counts beep period takes.
     */
    uint16_t period;

    /**
     * 0 - beeps each period, 1 - beeps each 2nd period etc.
     */
    uint8_t pattern;

    /**
     * Frequency.
     */
    uint8_t pitch;
    uint8_t count;
    
    BuzzerState state;
    uint8_t beepIndex;
} Buzzer;

void buzzer_init(Buzzer* buzzer);

void poweronMelody(Buzzer *buzzer);
void beepCount(Buzzer *buzzer, uint8_t count, uint8_t pitch, uint8_t pattern);
void beepLong(Buzzer *buzzer, uint8_t pitch);
void beepShort(Buzzer *buzzer, uint8_t pitch);
void beepShortMany(Buzzer *buzzer, uint8_t count, int8_t direction);

BuzzerState get_buzzer_next_state(Buzzer* buzzer, uint32_t timer);

#endif // BUZZER_H
