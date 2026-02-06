/*
 * wiper_servo.h
 *
 *  Created on: Jan 14, 2026
 *      Author: kkt
 */

#ifndef INC_WIPER_SERVO_H_
#define INC_WIPER_SERVO_H_

#ifndef WIPER_SERVO_H
#define WIPER_SERVO_H

#include "stm32l0xx_hal.h"
#include <stdint.h>

// ============================================================
// 1) 와이퍼 동작 모드 정의
// ============================================================
typedef enum {
  WIPER_MODE_STOP = 0,
  WIPER_MODE_SLOW,
  WIPER_MODE_FAST
} WiperMode;

// ============================================================
// 2) 버튼 디바운스 구조체
// ============================================================
typedef struct {
  GPIO_PinState stable;
  GPIO_PinState last_raw;
  uint32_t last_change_ms;
} BtnDebounce_t;

// ============================================================
// 3) 와이퍼 서보 컨트롤러 컨텍스트
// ============================================================
typedef struct {
  // HW
  TIM_HandleTypeDef *htim;
  uint32_t channel;

  // Runtime state
  volatile WiperMode mode;
  uint16_t us;
  int8_t   dir;
  uint32_t last_servo_ms;

  // 끝단 안정화 홀드(끝단 도달 후 잠깐 멈춤)
  uint16_t end_hold_ms;        // 예: 80ms
  uint32_t hold_until_ms;      // now_ms < hold_until_ms 이면 위치 업데이트 안 함

  // PWM 출력 상태/STOP 완전정지 옵션
  uint8_t  pwm_on;             // 1=PWM Start 상태, 0=PWM Stop 상태
  uint8_t  stop_pwm_off_on_stop; // 1이면 STOP 후 PWM OFF (완전정지)
  uint8_t  stop_pending;       // STOP 후 PWM OFF 대기중
  uint32_t stop_req_ms;        // STOP 요청 시각
  uint16_t stop_settle_ms;     // park로 이동 후 PWM 끄기까지 대기(ms)

  // Debounce
  BtnDebounce_t db_slow;
  BtnDebounce_t db_fast;
  BtnDebounce_t db_stop;
  uint8_t       db_inited;

  // Config
  uint16_t servo_min_us;       // e.g. 975
  uint16_t servo_max_us;       // e.g. 2025
  uint16_t center_us;          // e.g. 1500

  uint16_t sweep_min_us;       // sweep min
  uint16_t sweep_max_us;       // sweep max
  uint16_t park_us;            // 정지(주차) 위치 = sweep_min_us

  uint32_t dt_ms;              // 업데이트 주기
  uint32_t debounce_ms;

  uint16_t step_slow;          // 속도(펄스 변화량)
  uint16_t step_fast;

  GPIO_PinState pressed_level; // 버튼 눌림 레벨
} WiperServo_t;

// ============================================================
// 4) API
// ============================================================
void WiperServo_Init(WiperServo_t *w, TIM_HandleTypeDef *htim, uint32_t channel);
void WiperServo_Start(WiperServo_t *w);
void WiperServo_SetMode(WiperServo_t *w, WiperMode m, uint32_t now_ms);
void WiperServo_Task(WiperServo_t *w, uint32_t now_ms);

void WiperServo_SetPulseUs(WiperServo_t *w, uint16_t us);

// 각도 기반으로 sweep 범위를 바꾸는 함수(편의)
void WiperServo_SetSweepDeg(WiperServo_t *w, uint16_t min_deg, uint16_t max_deg);

// 폴링 방식 버튼 처리(선택)
void WiperServo_ButtonsTask(WiperServo_t *w,
                            uint32_t now_ms,
                            GPIO_PinState raw_slow,
                            GPIO_PinState raw_fast,
                            GPIO_PinState raw_stop);

#endif // WIPER_SERVO_H
#endif /* INC_WIPER_SERVO_H_ */
