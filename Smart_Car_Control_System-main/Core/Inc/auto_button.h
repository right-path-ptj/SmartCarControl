/*
 * auto_button.h
 *
 *  Created on: Jan 15, 2026
 *      Author: NAHYUN
 */

#ifndef INC_AUTO_BUTTON_H_
#define INC_AUTO_BUTTON_H_



#include <stdint.h>
#include "main.h"   // auto_Pin, auto_GPIO_Port, auto_LED_Pin 등이 여기 정의되어 있다고 가정

#ifdef __cplusplus
extern "C" {
#endif

void    AutoButton_Init(void);
void    AutoButton_Toggle(void);
uint8_t AutoButton_IsEnabled(void);
void    AutoButton_SetEnabled(uint8_t enabled);       // 수동버튼 누를 때 AUTO 해제 등에 사용

#ifdef __cplusplus
}
#endif

#endif /* INC_AUTO_BUTTON_H_ */
