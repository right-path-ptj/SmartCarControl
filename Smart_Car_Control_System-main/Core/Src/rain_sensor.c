/*
 * rain_sensor.c
 *
 *  Created on: Jan 13, 2026
 *      Author: NAHYUN
 */

#include "rain_sensor.h"

Rain_DataTypedef Rain_SensorData;

volatile bool is_raining = false;

/*
 * ADC 구조체의 주소를 저장할 포인터 변수
 * rain_sensor.c 파일 내에서만 사용하도록 static 설정 (캡슐화)
 */
static ADC_HandleTypeDef *pRainAdc;

// 아날로그 신호 A4 - A4
#define RAIN_A0_PORT	GPIOA
#define RAIN_A0_PIN		GPIO_PIN_4
//#define RAIN_A0_ADC_CH 	ADC_CHANNEL_0

// 디지털 신호 D0 - A1
#define RAIN_D0_PORT	GPIOA
#define RAIN_D0_PIN 	GPIO_PIN_1

// 메인에서 생성된 ADC 핸들러(hadc1 등)의 주소를 받아와 포인터에 저장
void RainSensor_Init(ADC_HandleTypeDef *hadc){
	pRainAdc = hadc; // 전달받은 주소를 전역 포인터 변수에 대입
}

/*
 * ADC 값이 0(0V)에서 4095(3.3V)까지 12-bit로 출력
 * 가공되지 않은 ADC 원본 데이터 읽기 (0~4095)
 */
uint32_t RainSensor_ReadRaw(void){
	if (pRainAdc == NULL) return 0; // ADC 설정이 안 되어 있으면 0 반환

	HAL_ADC_Start(pRainAdc); // ADC 변환 시작 명령

	// 변환이 완료될 때까지 최대 10ms 동안 대기 (Polling 방식)
	if (HAL_ADC_PollForConversion(pRainAdc, 10) == HAL_OK){
		Rain_SensorData.Rain_RawValue = HAL_ADC_GetValue(pRainAdc); // 변환된 값을 읽어서 구조체 변수에 저장
	}

	HAL_ADC_Stop(pRainAdc); // 전력 소모를 줄이기 위해 ADC 중지

	return Rain_SensorData.Rain_RawValue; // 값 반환
}

/*
 * 0~100%로 환산된 비의 양 읽기
 * MH-RD 센서는 물이 많을수록 저항이 낮아져 전압(ADC값)이 낮아짐
 * 4095(건조)일 때 0%, 0(비)일 때 100%로 계산
 */
uint8_t RainSensor_ReadPercentage(void){
	uint32_t raw = RainSensor_ReadRaw();

	Rain_SensorData.Rain_Percent = (uint8_t)((4095 - raw) * 100 / 4095);

	return Rain_SensorData.Rain_Percent;
}

/*
 * 비가 오는지 여부 확인 (Digital Input)
 * 0: Falling, 1: Dry
 */
/*
bool RainSensor_CheckFalling(void){
	if (HAL_GPIO_ReadPin(RAIN_D0_PORT, RAIN_D0_PIN) == GPIO_PIN_SET){
		return true; // 건조한 상태
	}

	return false; // 비 오는 상태
}
*/

/*
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if (GPIO_Pin == RAIN_D0_PIN){ // D0에서 인터럽트가 발생했다면
		if (HAL_GPIO_ReadPin(RAIN_D0_PORT, RAIN_D0_PIN) == GPIO_PIN_RESET){
			is_raining = true; // 비 오는 상태
			}
		else{
			is_raining = false; // 건조 상태
		}
	}
	uint32_t now = HAL_GetTick();
	if (GPIO_Pin == duty0_Pin) {                // STOP (PC5)
		if ((now - g_irq_last_0) < BTN_DEBOUNCE_MS) return;
	    g_irq_last_0 = now;
	    WiperServo_SetMode(&g_wiper, WIPER_MODE_STOP, now);
	}
	else if (GPIO_Pin == duty70_Pin) {          // FAST (PC4)
		if ((now - g_irq_last_70) < BTN_DEBOUNCE_MS) return;
	    g_irq_last_70 = now;
	    WiperServo_SetMode(&g_wiper, WIPER_MODE_FAST, now);
	}
	else if (GPIO_Pin == duty30_Pin) {          // SLOW (PA7)
		if ((now - g_irq_last_30) < BTN_DEBOUNCE_MS) return;
	    g_irq_last_30 = now;
	    WiperServo_SetMode(&g_wiper, WIPER_MODE_SLOW, now);
	  }
}*/
