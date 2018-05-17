#include <U8g2lib.h>
#include <U8x8lib.h>
#include <WiFiManager.h>         
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* host = "ws-feed.gdax.com";
const int httpsPort = 443;

U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0);
WebSocketsClient webSocket;

float currentValue = 0;
float currentPL = 0;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WSc] Connected to url: %s\n",  payload);
        // send message to server when Connected
        webSocket.sendTXT("{\"type\":\"subscribe\",\"channels\":[{\"name\":\"ticker\",\"product_ids\":[\"ETH-USD\"]}]}");
      }
      break;
    case WStype_TEXT:
      Serial.printf("[WSc] get text: %s\n", payload);
      parseData((char *)payload);
      // send message to server
      // webSocket.sendTXT("message here");
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);
      // send data to server
      // webSocket.sendBIN(payload, length);
      break;
    }
}

void parseData(char* json) {
  const size_t bufferSize = JSON_OBJECT_SIZE(10) + 210;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.parseObject(json);

  const float price = root["price"]; // "4388.01000000"
  const float open_24h = root["open_24h"];

  if (price) {
    currentValue = price;
    currentPL = (price - open_24h) / open_24h * 100;
  }
}

void setup() {
  // Open Serial for debug output
  Serial.begin(115200);
  delay(10);
  Serial.println();
  Serial.println();
  
  // Initialize OLED display
  u8g2.begin();
  delay(1000);
  
  // Connect WiFi
  connectWiFi();

  // Connect to WebSocket
  webSocket.beginSSL(host, httpsPort);
  webSocket.onEvent(webSocketEvent);
}

void connectWiFi() {
  drawStr2Lines("Connecting", "WiFi...");
  WiFiManager wifiManager;
  wifiManager.setAPCallback(configModeCallback);
  wifiManager.autoConnect("Wemos AP", "");
  drawStr2Lines("WiFi", "Connected!");
}

void configModeCallback (WiFiManager *myWiFiManager) {
  drawStr2Lines("Config needed", myWiFiManager->getConfigPortalSSID().c_str());
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void drawStr2Lines(const char *s1, const char *s2) {
  int width;
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_crox3h_tf);
    width = u8g2.getStrWidth(s1);
    u8g2.drawStr((128 - width) / 2, 12, s1);
    width = u8g2.getStrWidth(s2);
    u8g2.drawStr((128 - width) / 2, 31, s2);
  } while ( u8g2.nextPage() );
}

void drawStr1Line(const char *s) {
  int width;
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_inr21_mf);
    width = u8g2.getStrWidth(s);
    u8g2.drawStr((128 - width) / 2, 31, s);
  } while ( u8g2.nextPage() );  
}

void printData() {
  char buffer1[16];
  char buffer2[16];
  int ret;
  
  ret = snprintf(buffer1, sizeof buffer1, "$%.2f", currentValue);
  if (currentPL > 0) {
    ret = snprintf(buffer2, sizeof buffer2, "+%.2f%%", currentPL);
  } else {
    ret = snprintf(buffer2, sizeof buffer2, "%.2f%%", currentPL);
  }
  unsigned long currentMillis = millis();
  if (currentMillis % 10000 < 5000) {
    drawStr1Line(buffer1);
  } else {
    drawStr1Line(buffer2);
  }
}

void loop() {
  webSocket.loop();
  printData();
}


