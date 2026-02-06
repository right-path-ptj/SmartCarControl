<img width="2759" height="133" alt="image" src="https://github.com/user-attachments/assets/28fcf1d5-3cf2-4230-9318-5191a748ad46" />#  SMART_CAR_SYSTEM: 차량 원격 스마트 제어 플랫폼
<img width="3070" height="1428" alt="image" src="https://github.com/user-attachments/assets/2f382f94-4cc4-466e-801e-0429404a58e5" />

SMART_CAR_SYSTEM은 차량의 AI 기반 자동 및 수동 와이퍼 제어 및 운전 및 주차 제어 기능, 온도에 따른 냉공조 시스템
웹을 통해 차량의 다양한 기능을 제어할 수 있어 사용자에게 편리한 환경을 제공합니다.

## 📌 주요 기능

### 🚗 차량 제어 기능
웹을 통해 차량을 원격으로 제어할 수 있습니다.
- **차량 시동**: On/Off
- **워셔액**: on/off
- **차량 와이퍼**: AI 카레라 및 온습도 센서를 통한 자동 제어 및 수동제어
- **차량 냉공조 시스템**: On/Off(온도 및 습도에 따라 자동 제어 기능 제공)
- **차량 주행**: 정밀 주행 기능

### 📊 차량 조작 기능
차량의 상태를 실시간으로 모니터링할 수 있습니다.
- **차량 속도**: 차량 속도 제어
- **차량 와이퍼**: AI모드 및 단계 조절
- **워셔액**: on/off 제어

## 🎥 시연 영상

https://www.youtube.com/channel/UCnHLdUptgpBMaZFYZ_yiMAA?si=f5Up8a_bFTqANl29

## 🛠 하드웨어 구성

### Sensor Side

#### Arduino Water Pump
- 워셔액 분사 펌프
#### DHT11
- 온도 및 습도:
#### SW-420
- 진동 감지 센서 모듈
#### SZH-SSBH-022]
- 빗물 감지 센서 모

### P298P
- 차량 움직임 제어 모터 쉴드
#### sh02-DC Motor
- 차량 움직임 제어
#### 360° Servo Motor
- 차량 와이퍼 제어
#### Nucelo-L073RZ
- 센서 데이터 수신 및 차량 상태 관리
- ESP32와 통신하여 데이터를 웹 서버로 전송
- 웹에서 수신한 제어 명령에 따라서 출력 액추에이터 제어

### ESP32-WROOM-32D
- Master로부터 받은 차량 상태 데이터를 웹 서버로 전송
- 웹에서 받은 제어 명령을 Master로 전달

## 💻 소프트웨어 구성

### 🔧 임베디드 시스템
- **개발 언어**: C
- **IDE**: stm32CubeIde
- **통신 프로토콜**: UART, Wi-Fi
  
### 🌐 웹 서비스
- **개발 언어**: JavaScript
- **프레임워크**: Node.js, HTML, css



### 센서 통신 데이터 포맷
- uint8_t 21Byte
000(차량 속도)
000(조향 각도)
000(와이퍼 수동 1단)
000(와이퍼 수동 2단)
000(와이퍼 자동)
000(와이퍼 스탑)
000(워터펌프 분사)



## 🏗️ Diagram & Flow Chart
<img width="1430" height="801" alt="image" src="https://github.com/user-attachments/assets/48d9aebc-7e41-4cad-bae6-07df772a5789" />
<img width="1430" height="815" alt="image" src="https://github.com/user-attachments/assets/89f8df85-3123-4d97-ac03-fd3d811aabbb" />
<img width="1400" height="735" alt="image" src="https://github.com/user-attachments/assets/ce4b88e7-4611-4586-8db4-a6d6804c344a" />




## 📌 프로젝트 구조
<img width="2043" height="1204" alt="image" src="https://github.com/user-attachments/assets/ee23ff77-221a-4077-88b9-e15305626ee4" />

## 🛠 보고서
* [3조_프로젝트_계획서](3조_프로젝트_계획서.docx)
* [3조_프로젝트_최종보고서](3조_프로젝트_최종보고서.docx)

## 팀 구성원

| 이름 | GitHub |
|------|--------|
| 김다현 | [https://github.com/ym97001-lgtm) |
| 박태정 | [https://github.com/right-path-ptj) |
| 김경태 | [https://github.com/kkt1008) |
| 정나현 | [https://github.com/NAHYUN28) |
| 안재형 | [https://github.com/JHYUNG-A) |


