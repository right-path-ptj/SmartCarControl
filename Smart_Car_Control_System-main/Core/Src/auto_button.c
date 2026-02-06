/*
 * auto_button.c
 *
 *  Created on: Jan 15, 2026
 *      Author: NAHYUN
 */


/*
 * auto_button.c
 *
 *  Created on: Jan 15, 2026
 *      Author: kkt
 */

#include "auto_button.h"
#include "stm32l0xx_hal.h"

#define AUTO_BTN_DEBOUNCE_MS  60U

static volatile uint8_t  s_auto_enabled = 0;

static void AutoButton_UpdateLed(uint8_t en)
{// LED
    // CubeMX에서 PB2 등을 auto_LED로 잡아둔 경우를 지원
#ifdef auto_LED_GPIO_Port
#ifdef auto_LED_Pin
    HAL_GPIO_WritePin(auto_LED_GPIO_Port, auto_LED_Pin, en ? GPIO_PIN_SET : GPIO_PIN_RESET);
#endif
#endif
}

void AutoButton_Init(void)
{
    s_auto_enabled = 0;
    AutoButton_UpdateLed(0);
}

uint8_t AutoButton_IsEnabled(void)
{
    return s_auto_enabled;
}

void AutoButton_SetEnabled(uint8_t enabled)
{
    s_auto_enabled = (enabled != 0U) ? 1U : 0U;
    AutoButton_UpdateLed(s_auto_enabled);
}

void AutoButton_Toggle(void)
{
    s_auto_enabled ^= 1U;
    AutoButton_UpdateLed(s_auto_enabled);
}



