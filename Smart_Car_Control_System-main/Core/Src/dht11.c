/*
 * dht11.c
 *
 *  Created on: Jan 12, 2026
 *      Author: NAHYUN
 */

#include "dht11.h"

extern TIM_HandleTypeDef htim6;
#define dht_htim (&htim6) // delay_us에서 사용


// 마이크로초 지연
static void delay_us(uint16_t us)
{
   __HAL_TIM_SET_COUNTER (dht_htim, 0); //카운터 초기화
   HAL_TIM_Base_Start(dht_htim); //타이머 시작
   while (__HAL_TIM_GET_COUNTER(dht_htim) < us); // 대기
   HAL_TIM_Base_Stop(dht_htim); //타이머 정지
}

//HAL 라이브러리 표준 방식 VS 레지스터 직접 제어 방식(성능 최적화)
void DHT11_PinOutput(void){
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   GPIO_InitStruct.Pin = DHT11_PIN;
   GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
   GPIO_InitStruct.Pull = GPIO_NOPULL;
   GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH; //LOW or HIGH
   HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

void DHT11_PinInput(void){
   GPIO_InitTypeDef GPIO_InitStruct = {0};
   GPIO_InitStruct.Pin = DHT11_PIN;
   GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
   GPIO_InitStruct.Pull = GPIO_NOPULL;

   HAL_GPIO_Init(DHT11_PORT, &GPIO_InitStruct);
}

void DHT11_Init(){
   DHT11_PinOutput(); // 기본 상태: HIGH(대기)
   HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);

   HAL_Delay(1000); // 센서가 전원을 켜고 안정화될 때까지 대기

   __HAL_TIM_SET_COUNTER(dht_htim, 0); // 타이머 카운터 초기화
}


/*
 * MCU -> DHT11 시작 신호 전송 - low power 모드에서 running 모드로 변경 (시작 신호 대기)
 * 기본 상태: HIGH, 통신 시: LOW
 * 최소 18ms 동안 LOW로 유지한 후, 다시 HIGH로 올려 20~40μs 동안 대기
*/
void DHT11_Start(void){
    DHT11_PinOutput(); // 출력모드로 변경(MCU가 센서에게 start signal 전송)

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET); //LOW
    HAL_Delay(18);              // ≥18ms

    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET); //HIGH
    delay_us(30);               // 20~40us

    DHT11_PinInput(); // 입력 모드로 변경(센서가 MCU에게 응답, 온습도 값 보내는데 이를 MCU가 기다림)
}


/*
 * 시작 신호를 감지하면 80us동안 LOW 전송
 * -> LOW에서 HIGH로 변경. 센서는 MCU로 데이터 전송 준비를 위해 80us동안 유지
 */
static uint8_t DHT11_CheckResponse(void)
{
    uint8_t response = 0;
    delay_us(40); // 시작 신호 후 센서가 LOW로 내릴 때까지 잠시 대기

    // 1. 센서가 응답으로 80us 동안 LOW를 유지하는지 확인
    if (!HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)){
        delay_us(80);

        // 2. 그 후 다시 80us동안 HIGH를 유지하는지 확인
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))
            response = HAL_OK; // 맞다면 OK
        else
            response = HAL_ERROR; // 아니라면 ERROR
    }

    // 센서가 데이터를 보내기 시작하기 전(HIGH 구간 끝)까지 대기
    uint16_t timeout = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) && timeout < 100){
       delay_us(1);
       timeout++;
    } // 무한 루프 방지 타임아웃 추가

    return response;
}


/*
 * 0과 1 판별
 * 시작 단계 (Low 신호): 모든 데이터 비트('0'이든 '1'이든)는 항상 50μs 동안 Low 전압으로 시작
 * 판단 단계 (High 신호): Low 이후에 전압이 High로 올라가는데, 이 High가 유지되는 시간을 보고 값을 판단
 *       0: 50μs의 Low 신호 이후, High 전압이 26~28μs 동안만 짧게 유지 // 0일때 응답 신호 보내는 중
 *       1: 50μs의 Low 신호 이후, High 전압이 70μs 동안 길게 유지
 * 전송 종료: 모든 전송이 끝나면 센서가 50μs 동안 전압을 낮춘 후,
 *          풀업 저항에 의해 다시 High(대기 상태)로 복구
 */
static uint8_t DHT11_ReadBit(void) {
    // 비트 시작인 50us Low 구간이 끝날 때까지 대기
    while (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)));

    // High 유지 시간을 측정하여 0/1 판별 (중간 지점인 40us에서 체크)
    delay_us(40);

    // 40us 뒤에 상태를 체크
    if (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN))) {
       return 0; // 26~28us HIGH이므로 '0'
    }

    else {
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)); // 70us High 끝날 때까지 대기
        return 1; // '1'
    }
}

DHT11_DataTypedef DHT11_SensorData;

uint8_t DHT11_ReadData(void){
   uint8_t data[5] = {0};

   DHT11_Start(); // 1. 시작 신호 전송 및 입력 모드 전환

   if (DHT11_CheckResponse() == HAL_OK){ // 2. 센서 응답 확인
      // 3. 40bit Read
      for (int i = 0; i < 5; i++){
         for (int j = 0; j < 8; j++){

            // 타임아웃
            /*uint16_t timeout = 0;
            while (!(HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN)) && timeout < 100){
               delay_us(1);
               timeout++;
            }*/

            data[i] <<= 1; // 왼쪽으로 1칸 밀기 -> 변화가 없으면 0이고
            if (DHT11_ReadBit()){
               data[i] |= 1; // 1이면 비어있는 맨오른쪽 자리에 읽은 1을 넣음
            }
         }
      }

      // 4. 체크섬 검사
      if(data[0] + data[1] +data[2] + data[3] == data[4]) {
            DHT11_SensorData.Humidity = data[0];    // 습도 정수부
            DHT11_SensorData.Temperature = data[2]; // 온도 정수부
            return 1; // 읽기 성공
        }
   }
   return 0; // 읽기 실패
}

void FAN_Control(void) {
    // 1. 에어컨 가동 조건 (29도 이상)
    if (DHT11_SensorData.Temperature >= 29) {
        HAL_GPIO_WritePin(FAN_AC_PORT, FAN_AC_PIN, GPIO_PIN_RESET);      // 에어컨 ON
        HAL_GPIO_WritePin(FAN_Heater_PORT, FAN_Heater_PIN, GPIO_PIN_RESET); // 히터 OFF

    }


    // 2. 히터 가동 조건 (27도 이하)
    else if (DHT11_SensorData.Temperature <= 27) {
        HAL_GPIO_WritePin(FAN_AC_PORT, FAN_AC_PIN, GPIO_PIN_SET);    // 에어컨 OFF - PB8 (RESET: ON, SET: OFF)
        HAL_GPIO_WritePin(FAN_Heater_PORT, FAN_Heater_PIN, GPIO_PIN_SET);   // 히터 ON - PC1 (SET: ON, RESET: OFF)
    }

    // 3. 적정 온도 (28도 ~ 29도)
    else {
        HAL_GPIO_WritePin(FAN_AC_PORT, FAN_AC_PIN, GPIO_PIN_SET);    // 모두 OFF
        HAL_GPIO_WritePin(FAN_Heater_PORT, FAN_Heater_PIN, GPIO_PIN_RESET);
    }
}
