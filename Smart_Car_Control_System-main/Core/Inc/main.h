/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define pump_Pin GPIO_PIN_0
#define pump_GPIO_Port GPIOC
#define FAN_Heater_Pin GPIO_PIN_1
#define FAN_Heater_GPIO_Port GPIOC
#define wheel_forward_Pin GPIO_PIN_2
#define wheel_forward_GPIO_Port GPIOC
#define wheel_backward_Pin GPIO_PIN_3
#define wheel_backward_GPIO_Port GPIOC
#define servo_pwm_Pin GPIO_PIN_0
#define servo_pwm_GPIO_Port GPIOA
#define rain_digital_Pin GPIO_PIN_1
#define rain_digital_GPIO_Port GPIOA
#define rain_digital_EXTI_IRQn EXTI0_1_IRQn
#define USART_TX_Pin GPIO_PIN_2
#define USART_TX_GPIO_Port GPIOA
#define USART_RX_Pin GPIO_PIN_3
#define USART_RX_GPIO_Port GPIOA
#define rain_analog_Pin GPIO_PIN_4
#define rain_analog_GPIO_Port GPIOA
#define wheel_left_Pin GPIO_PIN_6
#define wheel_left_GPIO_Port GPIOA
#define duty30_Pin GPIO_PIN_7
#define duty30_GPIO_Port GPIOA
#define duty30_EXTI_IRQn EXTI4_15_IRQn
#define duty0_Pin GPIO_PIN_5
#define duty0_GPIO_Port GPIOC
#define duty0_EXTI_IRQn EXTI4_15_IRQn
#define auto_LED_Pin GPIO_PIN_2
#define auto_LED_GPIO_Port GPIOB
#define pump_button_Pin GPIO_PIN_14
#define pump_button_GPIO_Port GPIOB
#define pump_button_EXTI_IRQn EXTI4_15_IRQn
#define duty70_LED_Pin GPIO_PIN_15
#define duty70_LED_GPIO_Port GPIOB
#define wheel_right_Pin GPIO_PIN_7
#define wheel_right_GPIO_Port GPIOC
#define duty30_LED_Pin GPIO_PIN_8
#define duty30_LED_GPIO_Port GPIOA
#define Rasp_TX_Pin GPIO_PIN_9
#define Rasp_TX_GPIO_Port GPIOA
#define Rasp_RX_Pin GPIO_PIN_10
#define Rasp_RX_GPIO_Port GPIOA
#define auto_Pin GPIO_PIN_12
#define auto_GPIO_Port GPIOA
#define auto_EXTI_IRQn EXTI4_15_IRQn
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define ESP_TX_Pin GPIO_PIN_10
#define ESP_TX_GPIO_Port GPIOC
#define ESP_RX_Pin GPIO_PIN_11
#define ESP_RX_GPIO_Port GPIOC
#define pump_LED_Pin GPIO_PIN_12
#define pump_LED_GPIO_Port GPIOC
#define dht11_Pin GPIO_PIN_3
#define dht11_GPIO_Port GPIOB
#define duty70_Pin GPIO_PIN_6
#define duty70_GPIO_Port GPIOB
#define duty70_EXTI_IRQn EXTI4_15_IRQn
#define FAN_AC_Pin GPIO_PIN_8
#define FAN_AC_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
