#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
// #include <ESPAsyncWebSocket.h>
#include "esp_camera.h"
#include "page.h"  // Завантажена HTML-сторінка з інтерфейсом
// #include <HardwareSerial.h>
#include <ArduinoJson.h>

// Дані для Wi-Fi AP
const char *ssid = "ESP32-CAM";
const char *password = "12345678";

bool blinker = false;

bool prevSiren;
bool siren = false;
long sirenTimer;
bool firstSirenTone; // true if first, false if second

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");  // WebSocket на шляху /ws
// HardwareSerial Serial2(2);  // Використовуємо другий апаратний UART на ESP32

// Піни камери (для ESP32-CAM AI-THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define FLASH_LED_PIN 4
#define SIREN_PIN 15
#define SIREN_TIMER 700

// Увімкнення/вимкнення ліхтарика
void setFlashlight(bool on) {
  digitalWrite(FLASH_LED_PIN, on ? HIGH : LOW);
}

void onWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;  // Закінчення рядка
    // Serial.printf("Received: %s\n", (char *)data);

    // Розбираємо JSON
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
      // Serial.println("ERROR: Помилка парсингу JSON!");
      return;
    }

    if (doc["id"] == "!light") {
      bool state = doc["state"];
      setFlashlight(state);
      return;
      // Serial.printf("Ліхтарик %s\n", state ? "УВІМКНЕНО" : "ВИМКНЕНО");
    }

    if (doc["id"] == "!blinker") {
      blinker = doc["state"];
      String message = String("!blinker") +  "," + String(blinker) + "\n";
      Serial.print(message);
      return;
    }

    if (doc["id"] == "!siren") {
      prevSiren = siren;
      siren = doc["state"];
      // String message = String("!siren") + "," + String(siren) + "\n";
      // Serial.print(message);
      return;
    }

    // Отримуємо значення координат джойстика
    const char *id = doc["id"];
    int joyX = doc["x"];
    int joyY = doc["y"];

    String message = String(id) + "," + String(joyX) + "," + String(joyY) + "\n";

    // Надсилаємо данні до Arduino UNO через UART
    Serial.print(message);
  }
}

void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
                      AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    onWebSocketMessage(arg, data, len);
  }
}

void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 17;  // можна ще зменшити до 30-40
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 20;  // можна ще зменшити до 30-40
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Помилка ініціалізації камери: 0x%x", err);
    Serial.printf("Free heap: %u\n", ESP.getFreeHeap());
    Serial.printf("Free PSRAM: %u\n", ESP.getFreePsram());
    return;
  }
}

// Отримання зображення з камери
void handleJPG(AsyncWebServerRequest *request) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    request->send(500, "text/plain", "Помилка отримання кадру");
    return;
  }
  AsyncWebServerResponse *response = request->beginResponse_P(200, "image/jpeg", fb->buf, fb->len);
  response->addHeader("Cache-Control", "no-store");
  request->send(response);
  esp_camera_fb_return(fb);
}

void setup() {
  Serial.begin(115200);
  // Serial2.begin(9600, SERIAL_8N1, 16, 17);  // UART2: RX=16, TX=17

  WiFi.softAP(ssid, password);
  // Serial.print("AP IP: ");
  // Serial.println(WiFi.softAPIP());

  // Serial.print("ESP32 IP: ");
  // Serial.println(WiFi.localIP());

  startCamera();

  // Головна сторінка
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", page);
  });

  // Маршрут для отримання зображення
  server.on("/stream", HTTP_GET, handleJPG);

  ws.onEvent(onWebSocketEvent);
  server.addHandler(&ws);

  server.begin();
  pinMode(FLASH_LED_PIN, OUTPUT);
}

void loop() {
  ws.cleanupClients();  // Очищення неактивних клієнтів WebSocket
  if (siren) {
    tone(SIREN_PIN, 998);
    delay(SIREN_TIMER);
    tone(SIREN_PIN, 523);
    delay(SIREN_TIMER);
  } else {
    noTone(SIREN_PIN);
  }
}
