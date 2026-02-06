/*
 * led_control.c
 *
 *  Created on: Jan 15, 2026
 *      Author: NAHYUN
 */


#include "led_control.h"

/**
 * @brief 모든 와이퍼 관련 LED를 끈다.
 */

static LED_SystemMode current_system_mode = LED_MODE_OFF;


void LED_Control_All_Off(void) {
    HAL_GPIO_WritePin(duty30_LED_GPIO_Port, duty30_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(duty70_LED_GPIO_Port, duty70_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(auto_LED_GPIO_Port, auto_LED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(pump_LED_GPIO_Port, pump_LED_Pin, GPIO_PIN_RESET);
}

/**
 * @brief 현재 모드에 맞춰 LED 하나만 점등시킨다.
 * @param mode 점등할 모드 (OFF 시 모두 소등)
 */
void LED_Control_Update(LED_SystemMode mode) {
	current_system_mode = mode; // 현재 모드 저장

    // 1. 우선 모든 LED를 꺼서 중복 점등 방지
    LED_Control_All_Off();

    // 2. 모드에 해당하는 LED만 점등
    switch (mode) {
        case LED_MODE_SLOW:
            HAL_GPIO_WritePin(duty30_LED_GPIO_Port, duty30_LED_Pin, GPIO_PIN_SET);
            break;
        case LED_MODE_FAST:
            HAL_GPIO_WritePin(duty70_LED_GPIO_Port, duty70_LED_Pin, GPIO_PIN_SET);
            break;
        case LED_MODE_AUTO:
            HAL_GPIO_WritePin(auto_LED_GPIO_Port, auto_LED_Pin, GPIO_PIN_SET);
            break;
        case LED_MODE_PUMP:
             HAL_GPIO_WritePin(pump_LED_GPIO_Port, pump_LED_Pin,GPIO_PIN_SET);
             break;
        case LED_MODE_OFF:
        default:
            // 이미 위에서 모두 껐으므로 아무것도 하지 않음
            break;
    }
}


// 현재 모드를 반환하는 함수 추가
LED_SystemMode LED_Control_GetCurrentMode(void) {

    return current_system_mode;
}
