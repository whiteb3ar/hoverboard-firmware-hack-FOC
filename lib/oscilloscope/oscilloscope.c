#include "oscilloscope.h";

void oscilloscope_init(Oscilloscope* oscilloscope, uint16_t* data, uint16_t size)
{
    oscilloscope->data = data;
    oscilloscope->size = size;

    oscilloscope->index = 0;
    oscilloscope->status = ACTIVE;
}

void oscilloscope_update(Oscilloscope* oscilloscope, uint16_t type, uint16_t time, uint16_t data)
{
    if (oscilloscope->index >= oscilloscope->size - 3 || !oscilloscope->data)
    {
        oscilloscope->status = FINISHED;
    }
    else if (oscilloscope->data)
    {
        oscilloscope->data[oscilloscope->index++] = type;
        oscilloscope->data[oscilloscope->index++] = time;
        oscilloscope->data[oscilloscope->index++] = data;
    }
}
