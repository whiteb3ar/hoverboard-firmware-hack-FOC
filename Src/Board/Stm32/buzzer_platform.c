#include "setup.h"
#include "buzzer.h"

void toggle_buzzer()
{
  HAL_GPIO_TogglePin(BUZZER_PORT, BUZZER_PIN);
}

void switch_buzzer_off()
{
  HAL_GPIO_WritePin(BUZZER_PORT, BUZZER_PIN, GPIO_PIN_RESET);
}
