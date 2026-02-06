/*
 * led_control.h
 *
 *  Created on: Jan 15, 2026
 *      Author: NAHYUN
 */

#ifndef INC_LED_CONTROL_H_
#define INC_LED_CONTROL_H_

#include "main.h"

// 시스템 모드 정의 (버튼과 매칭)
typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_SLOW,
    LED_MODE_FAST,
    LED_MODE_AUTO,
	LED_MODE_PUMP
} LED_SystemMode;

// 직관적인 함수 이름 정의
void LED_Control_All_Off(void);
void LED_Control_Update(LED_SystemMode mode);
LED_SystemMode LED_Control_GetCurrentMode(void);
#endif /* INC_LED_CONTROL_H_ */
