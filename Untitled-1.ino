#include <U8g2lib.h>
#include <U8x8lib.h>
#include <ESP8266WiFi.h>
#include <DHT.h>
#include <PubSubClient.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Wire.h>

#define PIRPIN    D4
#define DHTPIN    D2
#define DHTTYPE   DHT22
#define wifi_ssid "***"
#define wifi_password "***"
#define SENSORNAME "sensor_1"
#define OTApassword "***"
float diffTEMP = 0.2;
float tempValue;
float diffHUM = 1;
float humValue;
int OTAport = 8266;

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ D5, /* data=*/ D6, /* reset=*/ U8X8_PIN_NONE);


void setup() {
  
  Serial.begin(115200);
  pinMode(DHTPIN, INPUT);  //define DHT pin
  delay(10);

  ArduinoOTA.setPort(OTAport);
  ArduinoOTA.setHostname(SENSORNAME);
  ArduinoOTA.setPassword((const char *)OTApassword);

  setup_wifi();

  ArduinoOTA.onStart([]() {
    Serial.println("Starting");
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
  Serial.println("Ready");
  Serial.print("IPess: ");
  Serial.println(WiFi.localIP());

  u8g2.begin();
}
void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(wifi_ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

float calculateHeatIndex(float humidity, float temp) {
  float heatIndex = 0;
  if (temp >= 80) {
    heatIndex = -42.379 + 2.04901523 * temp + 10.14333127 * humidity;
    heatIndex = heatIndex - .22475541 * temp * humidity - .00683783 * temp * temp;
    heatIndex = heatIndex - .05481717 * humidity * humidity + .00122874 * temp * temp * humidity;
    heatIndex = heatIndex + .00085282 * temp * humidity * humidity - .00000199 * temp * temp * humidity * humidity;
  } else {
    heatIndex = 0.5 * (temp + 61.0 + ((temp - 68.0) * 1.2) + (humidity * 0.094));
  }

  if (humidity < 13 && 80 <= temp <= 112) {
    float adjustment = ((13 - humidity) / 4) * sqrt((17 - abs(temp - 95.)) / 17);
    heatIndex = heatIndex - adjustment;
  }

  return heatIndex;
}

bool checkBoundSensor(float newValue, float prevValue, float maxDiff) {
  return newValue < prevValue - maxDiff || newValue > prevValue + maxDiff;
}

void loop() {
  // put your main code here, to run repeatedly:
  ArduinoOTA.handle();
  
  float newTempValue = dht.readTemperature();
  float newHumValue = dht.readHumidity();

  if (checkBoundSensor(newTempValue, tempValue, diffTEMP)) {
    tempValue = newTempValue;
  }

  if (checkBoundSensor(newHumValue, humValue, diffHUM)) {
    humValue = newHumValue;
  }

  String hs = "Humidity: " + String((float)humValue) + " % ";
  String ts = "TEMP: " + String((float)tempValue) + " " + char(223) + "C ";
  String heati = "Real Feel: " + String((float)calculateHeatIndex(humValue, tempValue)) + " " + char(223) + "C ";
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.setCursor(0, 15);
  u8g2.print(ts);
  u8g2.setCursor(0, 30);
  u8g2.print(hs);
  u8g2.setCursor(0, 45);
  u8g2.print(heati);
  u8g2.sendBuffer();
  delay(3000);
}