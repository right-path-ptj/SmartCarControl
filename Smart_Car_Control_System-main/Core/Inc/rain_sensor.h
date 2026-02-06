/*
 * rain_sensor.h
 *
 *  Created on: Jan 13, 2026
 *      Author: NAHYUN
 */

#ifndef INC_RAIN_SENSOR_H_
#define INC_RAIN_SENSOR_H_

#include "stm32l0xx_hal.h"
#include <stdbool.h>


// 아날로그 신호 A4 - A4
#define RAIN_A0_PORT	GPIOA
#define RAIN_A0_PIN		GPIO_PIN_4
//#define RAIN_A0_ADC_CH 	ADC_CHANNEL_0

// 디지털 신호 D0 - A1
#define RAIN_D0_PORT	GPIOA
#define RAIN_D0_PIN 	GPIO_PIN_1

typedef struct {
	uint32_t Rain_RawValue;
	uint8_t Rain_Percent;
} Rain_DataTypedef;

extern Rain_DataTypedef Rain_SensorData;
extern volatile bool is_raining;

void RainSensor_Init(ADC_HandleTypeDef *hadc);
uint32_t RainSensor_ReadRaw(void);
uint8_t RainSensor_ReadPercentage(void);
//bool RainSensor_CheckFalling(void);

#endif /* INC_RAIN_SENSOR_H_ */
