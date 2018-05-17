#include <U8g2lib.h>
#include <U8x8lib.h>
#include <WiFiManager.h>         
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

const char* host = "ws-feed.gdax.com";
const int httpsPort = 443;
const int displayWidth = 128;
const int displayHeight = 32;

U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0);
WebSocketsClient webSocket;

float currentValue = 0;
float currentPL = 0;
unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 5000;

int activePage = 0;
int numPages = 2;

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

  startMillis = millis();
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
    u8g2.drawStr((displayWidth - width) / 2, 12, s1);
    width = u8g2.getStrWidth(s2);
    u8g2.drawStr((displayWidth - width) / 2, 31, s2);
  } while ( u8g2.nextPage() );
}

void drawStr1Line(const char *s, int ypos = 31) {
  int width;
  u8g2.firstPage();
  do {
    u8g2.setFont(u8g2_font_inr21_mf);
    width = u8g2.getStrWidth(s);
    u8g2.drawStr((displayWidth - width) / 2, ypos, s);
  } while ( u8g2.nextPage() );  
}

void drawAndScroll(const char *s, const char *next) {
  int gap = 5;
  int ypos = displayHeight - 1;
  int width;
  while (ypos > -gap) {
    u8g2.firstPage();
    do {
      u8g2.setFont(u8g2_font_inr21_mf);
      width = u8g2.getStrWidth(s);
      u8g2.drawStr((displayWidth - width) / 2, ypos, s);
      width = u8g2.getStrWidth(next);
      u8g2.drawStr((displayWidth - width) / 2, ypos + displayHeight - 1 + gap, next);
    } while ( u8g2.nextPage() ); 
    ypos--;
    delay(10);
  }
}

char *getContentForPage(int page) {
  char *buffer;
  int ret;
  size_t needed;
  
  switch (page) {
    case 0: needed = snprintf(NULL, 0, "$%.2f", currentValue) + 1;
            buffer = (char *)malloc(needed);
            ret = snprintf(buffer, needed, "$%.2f", currentValue);
            break;
    case 1: needed = snprintf(NULL, 0, "%+.2f%%", currentPL) + 1;
            buffer = (char *)malloc(needed);
            ret = snprintf(buffer, needed, "%+.2f%%", currentPL);
            break;
  }
  return buffer;
}

void printData() {
  char *buffer = getContentForPage(activePage);
  Serial.println("current:");
  Serial.println(buffer);
  currentMillis = millis();
  if (currentMillis - startMillis >= period) {
    char *nextBuffer = getContentForPage((activePage + 1) % numPages);
    Serial.println("next:");
    Serial.println(nextBuffer);
    drawAndScroll(buffer, nextBuffer);
    activePage++;
    if (activePage >= numPages) {
      activePage = 0;
    }
    startMillis = currentMillis;
    free(nextBuffer);
  } else {
    drawStr1Line(buffer);
  }

  free(buffer);
}

void loop() {
  webSocket.loop();
  printData();
}


