#include "platform.h"
#include "buzzer.h"

void buzzer_init(Buzzer *buzzer)
{
    buzzer->period = 5000;
    buzzer->pattern = 0;
    buzzer->pitch = 0;
    buzzer->count = 0;

    buzzer->state = BUZZER_OFF;
    buzzer->beepIndex = 0;
}

void set_buzzer_next_state(Buzzer *buzzer, uint32_t timer)
{
    if (buzzer->pitch != 0 && (timer / buzzer->period) % (buzzer->pattern + 1) == 0)
    {
        if (buzzer->state == BUZZER_OFF)
        {
            buzzer->state = BUZZER_IDLE;

            if (++buzzer->beepIndex > (buzzer->count + 2))
            { // pause 2 periods
                buzzer->beepIndex = 1;
            }
        }

        if (timer % buzzer->pitch == 0 && (buzzer->beepIndex <= buzzer->count || buzzer->count == 0))
        {
            buzzer->state = BUZZER_TOGGLE;
        }
        else
        {
            buzzer->state = BUZZER_IDLE;
        }
    }
    else if (buzzer->state != BUZZER_OFF)
    {
        buzzer->state = BUZZER_OFF;
    }    
}

void poweronMelody(Buzzer *buzzer)
{
    buzzer->count = 0; // prevent interraction with beep counter

    for (int i = 8; i >= 0; i--)
    {
        buzzer->pitch = (uint8_t)i;

        delay(100);
    }

    buzzer->pitch = 0;
}

void beepCount(Buzzer *buzzer, uint8_t count, uint8_t pitch, uint8_t pattern)
{
    buzzer->count = count;
    buzzer->pitch = pitch;
    buzzer->pattern = pattern;
}

void beepLong(Buzzer *buzzer, uint8_t pitch)
{
    buzzer->count = 0; // prevent interraction with beep counter
    buzzer->period = pitch;

    delay(500);

    buzzer->pitch = 0;
}

void beepShort(Buzzer *buzzer, uint8_t pitch)
{
    buzzer->count = 0; // prevent interraction with beep counter
    buzzer->pitch = pitch;

    delay(100);

    buzzer->pitch = 0;
}

void beepShortMany(Buzzer *buzzer, uint8_t count, int8_t direction)
{
    if (direction >= 0)
    { // increasing tone
        for (uint8_t i = 2 * count; i >= 2; i = i - 2)
        {
            beepShort(buzzer,  + 3);
        }
    }
    else
    { // decreasing tone
        for (uint8_t i = 2; i <= 2 * count; i = i + 2)
        {
            beepShort(buzzer, i + 3);
        }
    }
}
