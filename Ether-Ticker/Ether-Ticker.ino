#include <U8g2lib.h>
#include <U8x8lib.h>
#include <WiFiManager.h>         
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* host = "api.gdax.com";
const int httpsPort = 443;
const char* url = "/products/ETH-USD/ticker";
const char* fingerprint = "69 39 26 2d de 76 cd 44 01 77 43 9b 0f 59 63 49 a7 cb b6 02";

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
      printData();
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

//  const char* type = root["type"]; // "ticker"
//  long trade_id = root["trade_id"]; // 20153558
//  long sequence = root["sequence"]; // 3262786978
//  const char* time = root["time"]; // "2017-09-02T17:05:49.250000Z"
//  const char* product_id = root["product_id"]; // "BTC-USD"
  const float price = root["price"]; // "4388.01000000"
  const float open_24h = root["open_24h"];
//  const char* side = root["side"]; // "buy"
//  const char* last_size = root["last_size"]; // "0.03000000"
//  const char* best_bid = root["best_bid"]; // "4388"
//  const char* best_ask = root["best_ask"]; // "4388.01"
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

  webSocket.beginSSL("ws-feed.gdax.com", 443);
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

void printData() {
  char buffer1[16];
  char buffer2[16];
  int ret;
  
  ret = snprintf(buffer1, sizeof buffer1, "$%.2f", currentValue);
  ret = snprintf(buffer2, sizeof buffer2, "%.2f%%", currentPL);

  drawStr2Lines(buffer1, buffer2);
  
/*  
  int width;
  u8g2.firstPage();    
  do {
    u8g2.setFont(u8g2_font_inr24_mn);
    width = u8g2.getStrWidth(currentValue.c_str());
    u8g2.drawStr((128 - width) / 2, 31, currentValue.c_str());
  } while ( u8g2.nextPage() );  
*/
}

void loop() {
  webSocket.loop();
}


