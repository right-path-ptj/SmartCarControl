/*
 * dht11.h
 *
 *  Created on: Jan 12, 2026
 *      Author: NAHYUN
 */

#ifndef INC_DHT11_H_
#define INC_DHT11_H_


#include "stm32l0xx_hal.h"

#define DHT11_PORT   GPIOB
#define DHT11_PIN   GPIO_PIN_3 // DHT_PinOutput에서 사용

#define FAN_AC_PORT     GPIOB
#define FAN_AC_PIN      GPIO_PIN_8 // AC

#define FAN_Heater_PORT GPIOC
#define FAN_Heater_PIN  GPIO_PIN_1 // Heater

typedef struct{
   uint8_t Temperature;
   uint8_t Humidity;
} DHT11_DataTypedef;

// 전역 변수 선언
extern DHT11_DataTypedef DHT11_SensorData;

// 외부에서 호풀할 함수 (static 함수들은 X)
void DHT11_Init(void);
uint8_t DHT11_ReadData(void);
void FAN_Control(void);

#endif /* INC_DHT11_H_ */
