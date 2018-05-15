#include <U8g2lib.h>
#include <U8x8lib.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>         
#include <WiFiClientSecure.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

const char* host = "api.gdax.com";
const int httpsPort = 443;
const char* url = "/products/ETH-USD/ticker";
const char* fingerprint = "69 39 26 2d de 76 cd 44 01 77 43 9b 0f 59 63 49 a7 cb b6 02";

U8G2_SSD1306_128X32_UNIVISION_1_HW_I2C u8g2(U8G2_R0);

bool busyDownloading = false;
unsigned long startMillis;
unsigned long currentMillis;
const unsigned long pollPeriod = 1000;
String currentValue = "";

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

  startMillis = millis();

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
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
  int width;
  u8g2.firstPage();    
  do {
    u8g2.setFont(u8g2_font_inr24_mn);
    width = u8g2.getStrWidth(currentValue.c_str());
    u8g2.drawStr((128 - width) / 2, 31, currentValue.c_str());
  } while ( u8g2.nextPage() );  
}

void fetchData() {
  WiFiClientSecure client;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }

  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line;
  while(client.available()) {
    char character = client.read();
    line += character;
  }
  Serial.println(line);

  String oldValue = String(currentValue);
  currentValue = line.substring(30, 36);
  printData();
}

void loop() {
  ArduinoOTA.handle();
  currentMillis = millis();
  if ((currentMillis - startMillis >= pollPeriod) && (busyDownloading == false)) {
    busyDownloading = true;
    fetchData();
    startMillis = currentMillis;
    busyDownloading = false;
  }
}


