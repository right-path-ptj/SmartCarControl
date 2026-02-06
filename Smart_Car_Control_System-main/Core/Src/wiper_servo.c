/*
 * wiper_servo.c
 *
 *  Created on: Jan 14, 2026
 *      Author: kkt
 */

#include "wiper_servo.h"

// ------------------------------------------------------------
// 내부 헬퍼
// ------------------------------------------------------------
static uint16_t ServoUs_FromDeg(const WiperServo_t *w, uint16_t deg);

static void ServoPwm_StartIfNeeded(WiperServo_t *w)
{
  if (!w || !w->htim) return;
  if (!w->pwm_on) {
    HAL_TIM_PWM_Start(w->htim, w->channel);
    w->pwm_on = 1U;
  }
}

static void ServoPwm_StopIfNeeded(WiperServo_t *w)
{
  if (!w || !w->htim) return;
  if (w->pwm_on) {
    HAL_TIM_PWM_Stop(w->htim, w->channel);
    w->pwm_on = 0U;
  }
}

// ------------------------------------------------------------
// 버튼 디바운스/엣지
// ------------------------------------------------------------
static void BtnDebounce_Init(BtnDebounce_t *b, GPIO_PinState init, uint32_t now, uint32_t debounce_ms)
{
  (void)debounce_ms;
  b->stable = init;
  b->last_raw = init;
  b->last_change_ms = now;
}

static GPIO_PinState BtnDebounce_Update(BtnDebounce_t *b, GPIO_PinState raw, uint32_t now, uint32_t debounce_ms)
{
  if (raw != b->last_raw) {
    b->last_raw = raw;
    b->last_change_ms = now;
  }
  if ((now - b->last_change_ms) >= debounce_ms) {
    b->stable = b->last_raw;
  }
  return b->stable;
}

static uint8_t BtnPressedEdge(BtnDebounce_t *b,
                              GPIO_PinState raw,
                              uint32_t now,
                              uint32_t debounce_ms,
                              GPIO_PinState pressed_level)
{
  GPIO_PinState prev = b->stable;
  GPIO_PinState cur  = BtnDebounce_Update(b, raw, now, debounce_ms);
  if (prev != pressed_level && cur == pressed_level) return 1;
  return 0;
}

// ------------------------------------------------------------
// 각도 -> us 변환
// ------------------------------------------------------------
static uint16_t ServoUs_FromDeg(const WiperServo_t *w, uint16_t deg)
{
  if (deg > 180U) deg = 180U;
  uint32_t range = (uint32_t)(w->servo_max_us - w->servo_min_us);
  uint32_t us = (uint32_t)w->servo_min_us + (range * (uint32_t)deg) / 180U;
  return (uint16_t)us;
}

// ------------------------------------------------------------
// 공개 API
// ------------------------------------------------------------
void WiperServo_SetPulseUs(WiperServo_t *w, uint16_t us)
{
  if (!w || !w->htim) return;

  if (us < 500)  us = 500;
  if (us > 2500) us = 2500;

  uint32_t pclk1  = HAL_RCC_GetPCLK1Freq();
  uint32_t timclk = pclk1;

#ifdef RCC_CFGR_PPRE1
  if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1) {
    timclk = pclk1 * 2U;
  }
#endif

  uint32_t tick_hz = timclk / (w->htim->Init.Prescaler + 1U);
  uint32_t ccr     = (tick_hz * (uint32_t)us) / 1000000U;

  uint32_t arr = __HAL_TIM_GET_AUTORELOAD(w->htim);
  if (ccr > arr) ccr = arr;

  __HAL_TIM_SET_COMPARE(w->htim, w->channel, (uint16_t)ccr);
}

void WiperServo_SetSweepDeg(WiperServo_t *w, uint16_t min_deg, uint16_t max_deg)
{
  if (!w) return;

  // swap if needed
  if (min_deg > max_deg) {
    uint16_t tmp = min_deg;
    min_deg = max_deg;
    max_deg = tmp;
  }

  if (min_deg > 180U) min_deg = 180U;
  if (max_deg > 180U) max_deg = 180U;

  w->sweep_min_us = ServoUs_FromDeg(w, min_deg);
  w->sweep_max_us = ServoUs_FromDeg(w, max_deg);
  w->park_us      = w->sweep_min_us;

  // 현재 us가 범위를 벗어나면 안쪽으로 넣어줌
  if (w->us < w->sweep_min_us) w->us = w->sweep_min_us;
  if (w->us > w->sweep_max_us) w->us = w->sweep_max_us;

  // STOP 상태면 주차 위치로 확실히 맞춤(원하면 유지)
  if (w->mode == WIPER_MODE_STOP) {
    w->us = w->park_us;
  }
}

void WiperServo_Init(WiperServo_t *w, TIM_HandleTypeDef *htim, uint32_t channel)
{
  if (!w) return;

  w->htim = htim;
  w->channel = channel;

  // 서보 캘리브레이션(필요시 조정)
  w->servo_min_us = 975;
  w->servo_max_us = 2025;
  w->center_us    = 1500;

  // 주기/버튼
  w->dt_ms       = 20;
  w->debounce_ms = 60;
  w->pressed_level = GPIO_PIN_SET;

  // ★ 왕복 안정화용(끝단 홀드)
  w->end_hold_ms    = 80;      // 끝단 도달 후 80ms 멈춰서 안정화
  w->hold_until_ms  = 0;

  // ★ 속도(너무 크면 서보가 못 따라가서 치우침이 생기기 쉬움)
  // 기존 10/35가 빠르다면 아래처럼 줄이는 게 보통 안정적임
  w->step_slow = 6;
  w->step_fast = 18;

  // PWM 완전정지 옵션
  w->pwm_on = 0;
  w->stop_pwm_off_on_stop = 1; // STOP 후 PWM OFF
  w->stop_pending = 0;
  w->stop_req_ms = 0;
  w->stop_settle_ms = 400;     // park 이동 후 PWM 끄기까지 대기

  // 상태 초기화
  w->mode = WIPER_MODE_STOP;
  w->dir  = +1;
  w->last_servo_ms = 0;
  w->db_inited = 0;
  w->db_slow.stable = GPIO_PIN_RESET;
  w->db_fast.stable = GPIO_PIN_RESET;
  w->db_stop.stable = GPIO_PIN_RESET;

  // ★ 기본 sweep 범위(여기 숫자만 바꾸면 됨)
  w->us = w->center_us; // SetSweepDeg에서 park로 맞춰줌
  WiperServo_SetSweepDeg(w, 45, 135); // 예: 총 90도 스윕(원하면 수정)

  // 초기 STOP은 park로
  w->us = w->park_us;
}

void WiperServo_Start(WiperServo_t *w)
{
  if (!w || !w->htim) return;

  ServoPwm_StartIfNeeded(w);
  w->stop_pending = 0;
  w->hold_until_ms = 0;

  w->us  = w->park_us;
  w->dir = +1;
  WiperServo_SetPulseUs(w, w->us);
}

void WiperServo_SetMode(WiperServo_t *w, WiperMode m, uint32_t now_ms)
{
  if (!w) return;

  w->mode = m;
  w->last_servo_ms = now_ms;
  w->hold_until_ms = 0;

  if (m == WIPER_MODE_STOP) {
    // park로 보내고(필요시) PWM OFF
    ServoPwm_StartIfNeeded(w);

    w->us  = w->park_us;
    w->dir = +1;
    WiperServo_SetPulseUs(w, w->us);

    if (w->stop_pwm_off_on_stop) {
      w->stop_pending = 1U;
      w->stop_req_ms  = now_ms;
    } else {
      w->stop_pending = 0U;
    }
    return;
  }

  // SLOW/FAST: STOP에서 PWM을 껐을 수 있으니 다시 켬
  w->stop_pending = 0U;
  ServoPwm_StartIfNeeded(w);

  // 시작 시 범위를 벗어나면 sweep_min으로 복귀
  if (w->us < w->sweep_min_us || w->us > w->sweep_max_us) {
    w->us = w->sweep_min_us;
  }
  w->dir = +1;
  WiperServo_SetPulseUs(w, w->us);
}

void WiperServo_Task(WiperServo_t *w, uint32_t now_ms)
{
  if (!w) return;

  // STOP 상태에서도 “park 후 PWM OFF” 처리를 해야 하므로 Task가 계속 불려야 함
  if (w->mode == WIPER_MODE_STOP) {
    if (w->stop_pending && w->pwm_on) {
      if ((now_ms - w->stop_req_ms) >= (uint32_t)w->stop_settle_ms) {
        ServoPwm_StopIfNeeded(w);
        w->stop_pending = 0U;
      }
    }
    return;
  }

  // 동작 상태면 PWM 켜져 있어야 함
  ServoPwm_StartIfNeeded(w);

  // 끝단 홀드 시간엔 업데이트 금지(안정화)
  if (now_ms < w->hold_until_ms) return;

  // dt 주기 체크
  if ((now_ms - w->last_servo_ms) < w->dt_ms) return;
  w->last_servo_ms = now_ms;

  uint16_t step = (w->mode == WIPER_MODE_FAST) ? w->step_fast : w->step_slow;

  int32_t next = (int32_t)w->us + (int32_t)w->dir * (int32_t)step;

  // 범위 클램프 + 방향 반전 + 끝단 홀드
  if (next >= (int32_t)w->sweep_max_us) {
    next = (int32_t)w->sweep_max_us;
    w->dir = -1;
    w->hold_until_ms = now_ms + (uint32_t)w->end_hold_ms; // ★ 끝단 홀드
  } else if (next <= (int32_t)w->sweep_min_us) {
    next = (int32_t)w->sweep_min_us;
    w->dir = +1;
    w->hold_until_ms = now_ms + (uint32_t)w->end_hold_ms; // ★ 끝단 홀드
  }

  w->us = (uint16_t)next;
  WiperServo_SetPulseUs(w, w->us);
}

void WiperServo_ButtonsTask(WiperServo_t *w,
                            uint32_t now_ms,
                            GPIO_PinState raw_slow,
                            GPIO_PinState raw_fast,
                            GPIO_PinState raw_stop)
{
  if (!w) return;

  if (!w->db_inited) {
    BtnDebounce_Init(&w->db_slow, raw_slow, now_ms, w->debounce_ms);
    BtnDebounce_Init(&w->db_fast, raw_fast, now_ms, w->debounce_ms);
    BtnDebounce_Init(&w->db_stop, raw_stop, now_ms, w->debounce_ms);
    w->db_inited = 1;
  }

  if (BtnPressedEdge(&w->db_stop, raw_stop, now_ms, w->debounce_ms, w->pressed_level)) {
    WiperServo_SetMode(w, WIPER_MODE_STOP, now_ms);
  } else if (BtnPressedEdge(&w->db_fast, raw_fast, now_ms, w->debounce_ms, w->pressed_level)) {
    WiperServo_SetMode(w, WIPER_MODE_FAST, now_ms);
  } else if (BtnPressedEdge(&w->db_slow, raw_slow, now_ms, w->debounce_ms, w->pressed_level)) {
    WiperServo_SetMode(w, WIPER_MODE_SLOW, now_ms);
  } else {
    BtnDebounce_Update(&w->db_stop, raw_stop, now_ms, w->debounce_ms);
    BtnDebounce_Update(&w->db_fast, raw_fast, now_ms, w->debounce_ms);
    BtnDebounce_Update(&w->db_slow, raw_slow, now_ms, w->debounce_ms);
  }
}
