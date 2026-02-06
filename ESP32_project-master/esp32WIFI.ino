#include <WiFi.h>
#include <HTTPClient.h>

// ===== WiFi 설정 =====
const char* ssid = "S24";
const char* password = "dial8787@@";

// ===== 서버 주소 =====
const char* serverUrl = "http://10.244.238.191:3000/api/control";

// ===== UART 설정 (STM32와 연결) =====
#define STM_RX 16
#define STM_TX 17

// ===== JSON 파싱을 도와주는 함수 (필수) =====
int getJsonValue(String json, String key) {
  int keyIndex = json.indexOf("\"" + key + "\"");
  if (keyIndex == -1) return 0; // 키가 없으면 0

  int colonIndex = json.indexOf(":", keyIndex);
  if (colonIndex == -1) return 0;

  int commaIndex = json.indexOf(",", colonIndex);
  int braceIndex = json.indexOf("}", colonIndex);
  
  int endIndex;
  // 콤마가 없거나, 닫는 괄호가 더 먼저 나오면(마지막 항목인 경우)
  if (commaIndex == -1 || (braceIndex != -1 && braceIndex < commaIndex)) {
    endIndex = braceIndex;
  } else {
    endIndex = commaIndex;
  }

  if (endIndex == -1) return 0;

  String valueStr = json.substring(colonIndex + 1, endIndex);
  return valueStr.toInt();
}
 
void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, STM_RX, STM_TX);

  Serial.println("\n--- WiFi Connection Start ---");
  WiFi.begin(ssid, password);

  int attempt = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    attempt++;
    Serial.printf("Attempt %d: Status = %d\n", attempt, WiFi.status());

    if (attempt > 30) {
      Serial.println("Connection Timeout!");
      WiFi.disconnect();
      WiFi.begin(ssid, password);
      attempt = 0;
    }
  }

  Serial.println("\nWiFi Connected!");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.setTimeout(3000); 

    int httpCode = http.GET();

    if (httpCode == 200) {
      String payload = http.getString();
      // Serial.println("RX: " + payload); // 필요시 주석 해제하여 확인

      // ★★★ 여기서 서버 데이터를 모두 추출합니다 ★★★
      int speed    = getJsonValue(payload, "speed");
      int steering = getJsonValue(payload, "steering");
      int val_sp1  = getJsonValue(payload, "speed1");
      int val_sp2  = getJsonValue(payload, "speed2");
      int val_off  = getJsonValue(payload, "off");
      int val_auto = getJsonValue(payload, "auto");
      int val_wat  = getJsonValue(payload, "water");

      // STM32로 전송 포맷: <속도,조향,sp1,sp2,off,auto,water>
      char tx[64];
      snprintf(tx, sizeof(tx), "<%d,%d,%d,%d,%d,%d,%d>\n", 
               speed, steering, val_sp1, val_sp2, val_off, val_auto, val_wat);
      
      Serial2.print(tx); 
      Serial.print("TX -> STM32: ");
      Serial.print(tx);

    } else {
      Serial.printf("HTTP Error: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("Reconnecting WiFi...");
    WiFi.disconnect();
    WiFi.begin(ssid, password);
  }

  delay(500); // 0.5초 간격
}