#include "sensor_interface.h"

void SensorInterface_Init(ADC_HandleTypeDef *hadc, WiperServo_t *pWiper, TIM_HandleTypeDef *htim) {
    // 빗물 센서 초기화 (ADC 연결)
    RainSensor_Init(hadc);

    // 서보 모터 구조체 초기화 (TIM2, 채널 1 가정)
    WiperServo_Init(pWiper, htim, TIM_CHANNEL_1);

    // PWM 출력 시작
    WiperServo_Start(pWiper);
}

void SensorInterface_SyncRainToServo(WiperServo_t *pWiper) {
	// 1. 센서로부터 0~100% 값 획득
	uint8_t rain_p = RainSensor_ReadPercentage();
	uint32_t now_ms = HAL_GetTick();

	// 2. 빗물 양에 따른 서보 모드 결정 (구조체 기반 함수 호출)
	if (rain_p < 10) {
		// 비가 거의 안 오면 정지
		if (pWiper->mode != WIPER_MODE_STOP) {
			WiperServo_SetMode(pWiper, WIPER_MODE_STOP, now_ms);
		}
	}
	else if (rain_p < 40) {
		// 비가 적당히 오면 저속 (Slow)
		if (pWiper->mode != WIPER_MODE_SLOW) {
			WiperServo_SetMode(pWiper, WIPER_MODE_SLOW, now_ms);
		}
	}
	else {
		// 비가 많이 오면 고속 (Fast)
		if (pWiper->mode != WIPER_MODE_FAST) {
			WiperServo_SetMode(pWiper, WIPER_MODE_FAST, now_ms);
		}
	}

	// 3. 결정된 모드로 서보 실제 구동 (주기적 업데이트)
	//WiperServo_Task(pWiper, now_ms);
}








