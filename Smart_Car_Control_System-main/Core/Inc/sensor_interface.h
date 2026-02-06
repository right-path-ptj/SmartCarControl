/*
 * sensor_interface.h
 *
 *  Created on: Jan 14, 2026
 *      Author: NAHYUN
 */

#ifndef INC_SENSOR_INTERFACE_H_
#define INC_SENSOR_INTERFACE_H_

#include "wiper_servo.h"
#include "main.h"
#include "rain_sensor.h"

/**
 * @brief 시스템 전체 초기화 (ADC 및 서보 타이머)
 */
void SensorInterface_Init(ADC_HandleTypeDef *hadc, WiperServo_t *pWiper, TIM_HandleTypeDef *htim);
/**
 * @brief 빗물 양을 읽어 서보 모터의 속도 모드를 결정하고 실행
 * 이 함수를 main의 while 루프에서 호출하세요.
 */
void SensorInterface_SyncRainToServo(WiperServo_t *pWiper);

#endif /* INC_SENSOR_INTERFACE_H_ */
