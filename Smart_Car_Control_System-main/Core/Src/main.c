/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sensor_interface.h"
#include "wiper_servo.h"
#include "rain_sensor.h"
#include <stdint.h>
#include "led_control.h"
#include "dht11.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BTN_DEBOUNCE_MS 60   // 인터럽트 바운스 억제(단순 필터)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
WiperServo_t g_wiper;
WiperServo_t *pWiper;
uint8_t rx_data;


// 버튼별 마지막 인터럽트 시간(디바운스)
static volatile uint32_t g_irq_last_30 = 0;
static volatile uint32_t g_irq_last_70 = 0;
static volatile uint32_t g_irq_last_0  = 0;
static volatile uint32_t g_irq_last_auto = 0;
static volatile uint32_t g_irq_last_washer = 0;


//통신코드 시작
#define RX_BUF_SIZE 64
volatile uint8_t new_control_received = 0;

char rx_buf[RX_BUF_SIZE];
uint8_t rx_idx = 0;
uint8_t rx_byte;

// ESP32로부터 받을 제어 값
int32_t target_speed = 0;
int32_t target_steering = 0;
int32_t target_speed1 = 0; // 추가
int32_t target_speed2 = 0; // 추가
int32_t target_off = 0;    // 추가
int32_t target_auto = 0;   // 추가
int32_t target_water = 0;  // 추가

volatile uint8_t data_ready_flag = 0; // 데이터 수신 완료 플래그 추가
//통신코드 끝

// esp 웹서버용 전역변수
int32_t btn_speed_1 = 0;
int32_t btn_speed_2 = 0;
int32_t btn_off = 0;
int32_t btn_auto = 0;
int32_t btn_water =0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief EXTI 콜백 (버튼 누르는 순간만 들어옴)
 *        - STOP > FAST > SLOW 우선순위는 "동시에 눌렀을 때"만 의미가 있는데,
 *          인터럽트는 핀별로 들어오니 여기서는 눌린 버튼에 해당하는 모드로 즉시 전환.
 *        - 바운스는 now-last < BTN_DEBOUNCE_MS 이면 무시하는 방식.
 */

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
      LED_Control_Update(LED_MODE_OFF);
   }
   else if (GPIO_Pin == duty70_Pin) {          // FAST (PC4)
      if ((now - g_irq_last_70) < BTN_DEBOUNCE_MS) return;
       g_irq_last_70 = now;
       WiperServo_SetMode(&g_wiper, WIPER_MODE_FAST, now);
       LED_Control_Update(LED_MODE_FAST);
   }
   else if (GPIO_Pin == duty30_Pin) {          // SLOW (PA7)
      if ((now - g_irq_last_30) < BTN_DEBOUNCE_MS) return;
       g_irq_last_30 = now;
       WiperServo_SetMode(&g_wiper, WIPER_MODE_SLOW, now);
       LED_Control_Update(LED_MODE_SLOW);
   }
   else if (GPIO_Pin == auto_Pin) {            // AUTO 버튼
      //LED_Control_Update(LED_MODE_AUTO);      // AUTO LED 켜기
      if((now - g_irq_last_auto) < BTN_DEBOUNCE_MS) return;
         g_irq_last_auto = now;
         // active-low 구성(Pull-up) 기준: 눌리면 RESET
         if (HAL_GPIO_ReadPin(auto_GPIO_Port, auto_Pin) == GPIO_PIN_RESET) {
            AutoButton_Toggle();      // 여기서 AUTO ON/OFF만 바꿈
         }
      LED_Control_Update(LED_MODE_AUTO);      // AUTO LED 켜기
   }
   else if (GPIO_Pin == pump_button_Pin ){ 		// pump 버튼
	   if ((now - g_irq_last_washer) < BTN_DEBOUNCE_MS) return;
	   g_irq_last_washer = now;

	   HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // PC0 HIGH → 릴레이 ON

	   LED_Control_Update(LED_MODE_PUMP); // 펌프 버튼 누르면 불들어옴--확인용

   }

}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART1) {
		// 현재 모드가 AUTO일 때만 라즈베리파이의 카메라 인식 결과를 반영
		if (LED_Control_GetCurrentMode() == LED_MODE_AUTO) {
			printf("rasp_start");
			// 데이터가 아스키(ASCII) 문자로 올 경우를 대비해 '0', '1', '2'로 체크
			if (rx_data == '0' || rx_data == 0) {
				printf("rasp0");
				WiperServo_SetMode(&g_wiper, WIPER_MODE_STOP, HAL_GetTick());
			}
			else if (rx_data == '1' || rx_data == 1) {
				printf("rasp1");
				WiperServo_SetMode(&g_wiper, WIPER_MODE_SLOW, HAL_GetTick());
			}
			else if (rx_data == '2' || rx_data == 2) {
				printf("rasp2");
				WiperServo_SetMode(&g_wiper, WIPER_MODE_FAST, HAL_GetTick());
			}
		}
		// 다음 데이터를 받기 위해 인터럽트 재활성화 (중요!)
		HAL_UART_Receive_IT(&huart1, &rx_data, 1);
	}
    else if (huart->Instance == USART4) { // ESP32가 연결된 UART3일 경우
		// 종료 문자를 만나면 (데이터 끝)
		if (rx_byte == '>' || rx_byte == '\n' || rx_byte == '\r') {
			rx_buf[rx_idx] = '\0'; // 문자열 끝 표시
			data_ready_flag = 1;   // 메인 루프에 알림
		}
		// 시작 문자를 만나면 (데이터 시작)
		else if (rx_byte == '<') {
			rx_idx = 0;
		}
		// 일반 데이터인 경우 버퍼에 저장
		else if (rx_idx < RX_BUF_SIZE - 1) {
			rx_buf[rx_idx++] = rx_byte;
		}

		// 다음 1바이트를 받기 위해 다시 인터럽트 활성화 (필수!)
		HAL_UART_Receive_IT(&huart4, &rx_byte, 1);
	}

}




/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_ADC_Init();
  MX_TIM2_Init();
  MX_USART1_UART_Init();
  MX_TIM3_Init();
  MX_USART4_UART_Init();
  MX_TIM6_Init();
  /* USER CODE BEGIN 2 */
  RainSensor_Init(&hadc);
  WiperServo_Init(&g_wiper, &htim2, TIM_CHANNEL_1);
  WiperServo_Start(&g_wiper);
  AutoButton_Init();
  DHT11_Init();

  printf("System Started. Waiting for ESP32...\r\n"); // 디버깅용 코드

  // UART4(ESP32 연결용) 인터럽트 수신 시작
  HAL_UART_Receive_IT(&huart4, &rx_byte, 1);

  // UART2
  // 1. PWM 출력 시작 (중요!)
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);

  // UART1
  HAL_UART_Receive_IT(&huart1, &rx_data, 1);

  // 시작은 STOP 상태로 두고 싶으면:
  WiperServo_SetMode(&g_wiper, WIPER_MODE_STOP, HAL_GetTick());


  //SensorInterface_Init(&hadc, &htim2);
  //DHT11_Init();

  //UART 신호 받는 부분
  //HAL_UART_Receive_IT(&huart1, &rx_data, 1); // Raspberry 신호 수신
  //HAL_UART_Receive_IT(&huart4, &rx_data, 1); // ESP32 신호 수신 ====================> 태정이랑 같이 작업 할 부분 !!!
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  //printf("humidity: %d\r\n",DHT11_SensorData.Humidity);
	  //printf("temperature: %d\r\n",DHT11_SensorData.Temperature);

	uint32_t now = HAL_GetTick();

   // 1. 현재 모드가 AUTO인지 확인
   if (LED_Control_GetCurrentMode() == LED_MODE_AUTO) {
    SensorInterface_SyncRainToServo(&g_wiper); // 빗물값에 따라 속도 제어
   }
	  //printf("humidity: 112%d\r\n",DHT11_SensorData.Humidity);
	  //printf("temperature: 123%d\r\n",DHT11_SensorData.Temperature);
	//printf("DHT value: %d\n",DHT11_ReadData());

   // 자동 냉난방 시스템
   //uint8_t ok = DHT11_ReadData();
   //printf("DHT ok=%d | H=%d | T=%d\r\n", ok, DHT11_SensorData.Humidity, DHT11_SensorData.Temperature);

   if (DHT11_ReadData()){
    FAN_Control();
    HAL_Delay(500);
   }

   // 디버깅용
   //printf("humidity: 113332%d\r\n",DHT11_SensorData.Humidity);
   //printf("temperature: 133323%d\r\n",DHT11_SensorData.Temperature);

   //HAL_Delay(3000);

   RainSensor_ReadPercentage();

   // 4. 와이퍼 서보 실제 구동 (주기적 호출 필수)
   WiperServo_Task(&g_wiper, now);

   // 펌프 제어 코드. 핀맵만 설정해주면 자동으로 펌프작동됨. 스위치를 눌러서 작동하도록 수정해야함.
   //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // PC0 HIGH → 릴레이 ON
   //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // PC0 HIGH → 릴레이 ON
   //HAL_Delay(5000);
   //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET); // PC0 HIGH → 릴레이 ON
   //HAL_Delay(5000);

   /*if (data_ready_flag) {
	  // 데이터 포맷: "속도,조향,sp1,sp2,off,auto,water"
	  // 예: "100,-20,1,0,0,1,50"

	  // sscanf로 7개의 값을 순서대로 파싱하여 변수에 저장
	  if (sscanf(rx_buf, "%ld,%ld,%ld,%ld,%ld,%ld,%ld",
				 &target_speed, &target_steering,
				 &target_speed1, &target_speed2,
				 &target_off, &target_auto, &target_water) == 7) {

		  // 1. 모터 제어는 기존대로 speed와 steering만 사용해서 즉시 반영
		  Update_Motor_Control(target_speed, target_steering);

		  // 2. 디버그 출력 (7개 값 모두 확인)
		  printf("RX: Spd:%ld, Str:%ld, S1:%ld, S2:%ld, Off:%ld, Auto:%ld, Wat:%ld\r\n",
				 target_speed, target_steering,
				 target_speed1, target_speed2,
				 target_off, target_auto, target_water);

		  // 3. (옵션) 추가된 변수에 따른 동작이 필요하면 여기에 작성
		  // 예: if (target_auto == 1) { ... }
	  }

	  data_ready_flag = 0;
	  rx_idx = 0;
  }*/
   /* USER CODE BEGIN 3 */
      // ... (기존 코드 생략) ...

      // UART 데이터 수신 완료 시 처리
   if (data_ready_flag) {
           // [중요] 이전 상태를 기억하기 위한 정적 변수 선언
           // static 변수는 함수가 종료되어도 값이 유지됩니다.
           static int32_t prev_speed = 0;
           static int32_t prev_steering = 0;
           static int32_t prev_speed1 = 0;
           static int32_t prev_speed2 = 0;
           static int32_t prev_off = 0;
           static int32_t prev_auto = 0;
           static int32_t prev_water = 0;

           // 데이터 파싱
           if (sscanf(rx_buf, "%ld,%ld,%ld,%ld,%ld,%ld,%ld",
                      &target_speed, &target_steering,
                      &target_speed1, &target_speed2,
                      &target_off, &target_auto, &target_water) == 7) {

               // 1. 모터 제어 (값이 바뀌었을 때만 업데이트)
               if (target_speed != prev_speed || target_steering != prev_steering) {
                   Update_Motor_Control(target_speed, target_steering);
                   prev_speed = target_speed;
                   prev_steering = target_steering;
               }

               uint32_t current_time = HAL_GetTick();

               // 2. 와이퍼 모드 제어 (0 -> 1 로 변하는 순간만 동작)

               // (1) 정지 명령
               if (target_off == 1 && prev_off == 0) {
                   WiperServo_SetMode(&g_wiper, WIPER_MODE_STOP, current_time);
                   LED_Control_Update(LED_MODE_OFF);
                   printf("CMD: Wiper STOP (Executed)\r\n");
               }
               prev_off = target_off; // 현재 상태 저장

               // (2) 고속 명령
               if (target_speed2 == 1 && prev_speed2 == 0) {
                   WiperServo_SetMode(&g_wiper, WIPER_MODE_FAST, current_time);
                   LED_Control_Update(LED_MODE_FAST);
                   printf("CMD: Wiper FAST (Executed)\r\n");
               }
               prev_speed2 = target_speed2;

               // (3) 저속 명령
               if (target_speed1 == 1 && prev_speed1 == 0) {
                   WiperServo_SetMode(&g_wiper, WIPER_MODE_SLOW, current_time);
                   LED_Control_Update(LED_MODE_SLOW);
                   printf("CMD: Wiper SLOW (Executed)\r\n");
               }
               prev_speed1 = target_speed1;

               // 3. AUTO 모드 (가장 중요: 0->1 변하는 순간 한 번만 토글)
               if (target_auto == 1 && prev_auto == 0) {
                   AutoButton_Toggle();
                   LED_Control_Update(LED_MODE_AUTO);
                   printf("CMD: Auto Mode Toggle (Executed)\r\n");
               }
               prev_auto = target_auto;

               // 4. 워셔액 펌프 (상태가 변했을 때만 GPIO 쓰기)
               if (target_water != prev_water) {
                   if (target_water == 1) {
                       HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_SET); // ON
                   } else {
                       // 펌프 끌 때, 물리 버튼이 안 눌려있는지 체크(안전장치)
                       if(HAL_GPIO_ReadPin(pump_button_GPIO_Port, pump_button_Pin) != GPIO_PIN_RESET) {
                           HAL_GPIO_WritePin(GPIOC, GPIO_PIN_0, GPIO_PIN_RESET); // OFF
                       }
                   }
                   prev_water = target_water;
               }
           }

           // 수신 플래그 초기화
           data_ready_flag = 0;
           rx_idx = 0;
       }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1|RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
    return len;
}

void Update_Motor_Control(int32_t raw_speed, int32_t raw_steering) {
    // 1. 전진/후진 방향 결정 및 절대값 변환
    int32_t abs_speed = raw_speed;

    if (raw_speed >= 0) {
        // 전진 모드 (Speed: 0 ~ 100)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);   // DIR A
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_RESET); // DIR B
    } else {
        // 후진 모드 (Speed: -100 ~ 0)
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_3, GPIO_PIN_SET);
        abs_speed = -raw_speed; // 계산을 위해 양수로 변환
    }

    // 2. 기본 속도 설정
    int32_t left_pwr = abs_speed;
    int32_t right_pwr = abs_speed;

    // 3. 조향 로직 적용 (안쪽 바퀴 감속 방식)
    // raw_steering: -100(좌) ~ 0(직진) ~ +100(우)
    if (raw_steering > 0) {
        // 우회전: 오른쪽 바퀴를 감속 (steering 값만큼 뺌)
        right_pwr = abs_speed - raw_steering;
    }
    else if (raw_steering < 0) {
        // 좌회전: 왼쪽 바퀴를 감속 (steering이 음수이므로 더하면 뺼셈 효과)
        left_pwr = abs_speed + raw_steering;
    }

    // 4. 값 범위 안전장치 (0 미만으로 떨어지는 것 방지)
    if (left_pwr < 0) left_pwr = 0;
    if (right_pwr < 0) right_pwr = 0;

    // 입력이 100을 넘을 경우를 대비한 컷오프
    if (left_pwr > 100) left_pwr = 100;
    if (right_pwr > 100) right_pwr = 100;

    // 5. PWM 스케일링 (입력 0~100 --> 타이머 0~1000)
    left_pwr *= 10;
    right_pwr *= 10;

    // 6. 실제 PWM 레지스터에 적용
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, left_pwr);  // 왼쪽 모터
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, right_pwr); // 오른쪽 모터

    // 디버그 출력 (확인용)
     printf("Mode:%s | In(%ld, %ld) -> Out(%ld, %ld)\r\n",
            (raw_speed >= 0 ? "FWD" : "BWD"), abs_speed, raw_steering, left_pwr, right_pwr);
}
// 에러 발생 시(통신 노이즈 등) 다시 수신 상태로 복구
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART4) {
        HAL_UART_Receive_IT(&huart4, &rx_byte, 1);
    }
}


/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
