#include <U8g2lib.h>
#include <U8x8lib.h>
#include <WiFiManager.h>         
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include "font.h"

const char* host = "ws-feed.gdax.com";
const int httpsPort = 443;
const int displayWidth = 128;
const int displayHeight = 32;

U8G2_SSD1306_128X64_NONAME_2_HW_I2C u8g2(U8G2_R0);
WebSocketsClient webSocket;

float currentValueUSD = 0;
float currentValueEUR = 0;
float currentPLUSD = 0;
float currentPLEUR = 0;
unsigned long startMillis;
unsigned long currentMillis;
const unsigned long period = 5000;

int activePage = 0;
int numPages = 2;
bool scrolling = false;
int scrollOffset = 0;

extern "C" {
#include "user_interface.h"
}
uint32_t freemem = system_get_free_heap_size();

extern "C" {
#include <cont.h>
  extern cont_t g_cont;
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
      //Serial.printf("[WSc] Disconnected!\n");
      break;
    case WStype_CONNECTED:
      {
        Serial.printf("[WSc] Connected to url: %s\n",  payload);
        webSocket.sendTXT("{\"type\":\"subscribe\",\"channels\":[{\"name\":\"ticker\",\"product_ids\":[\"ETH-USD\",\"ETH-EUR\"]}]}");
      }
      break;
    case WStype_TEXT:
      //Serial.printf("[WSc] get text: %s\n", payload);
      parseData((char *)payload);
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);
      break;
    }
}

void parseData(char* json) {
  const size_t bufferSize = JSON_OBJECT_SIZE(10) + 210;
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.parseObject(json);

  const float price = root["price"];
  const char* product = root["product_id"];
  const float open_24h = root["open_24h"];

  if (price) {
    if (strcmp(product, "ETH-EUR") == 0) {
      currentValueEUR = price;
      currentPLEUR = (currentValueEUR - open_24h) / open_24h * 100;
    } else {
      currentValueUSD = price;
      currentPLUSD = (currentValueUSD - open_24h) / open_24h * 100;
    }
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
  u8g2.enableUTF8Print();
  delay(1000);
  
  // Connect WiFi
  connectWiFi();

  // Connect to WebSocket
  webSocket.beginSSL(host, httpsPort);
  webSocket.onEvent(webSocketEvent);

  startMillis = millis();
}

void drawStr2Lines(const char *s1, const char *s2, int offset = 0) {
  int width;
  u8g2.setFont(u8g2_font_inconsolata_tx);
  width = u8g2.getUTF8Width(s1);
  u8g2.setCursor((displayWidth - width) / 2, 31 + offset);
  u8g2.print(s1);
  width = u8g2.getUTF8Width(s2);
  u8g2.setCursor((displayWidth - width) / 2, 62 + offset);
  u8g2.print(s2);
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

void drawPage(int page, int offset = 0) {
  char buffer1[50];
  char buffer2[50];
  switch (page) {
    case 0: snprintf(buffer1, sizeof(buffer1), "$%.2f", currentValueUSD);
            snprintf(buffer2, sizeof(buffer2), "%+.2f%%", currentPLUSD);
            drawStr2Lines(buffer1, buffer2, offset);
            break;
    case 1: snprintf(buffer1, sizeof(buffer1), "€%.2f", currentValueEUR);
            snprintf(buffer2, sizeof(buffer2), "%+.2f%%", currentPLEUR);
            drawStr2Lines(buffer1, buffer2, offset);
            break;
  }
}

long RAMfree(void)
{
  long s, h;
  Serial.printf("\n Heap free = \'%d\', Stack free = \'%d\', Stack guard bytes were ", (h=system_get_free_heap_size()), (s=cont_get_free_stack(&g_cont)));
  if (!cont_check(&g_cont)) {
    Serial.printf("NOT ");
  }
  Serial.println("overwritten");
  return (s+h);
}

void printData() {
  currentMillis = millis();
  if (currentMillis - startMillis >= period) {
    scrolling = true;
    startMillis = currentMillis;
  } else {
    u8g2.firstPage();
    do {
      if (!scrolling) {
        drawPage(activePage);
      } else {
        scrollOffset--;
        drawPage(activePage, scrollOffset);
        drawPage((activePage + 1) % numPages, scrollOffset + 64);
        if (scrollOffset <= -64) {
          scrollOffset = 0;
          scrolling = false;
          activePage = (activePage + 1) % numPages;
        }
      }
    } while ( u8g2.nextPage() );  
  }
}

void loop() {
  webSocket.loop();
  printData();
}


