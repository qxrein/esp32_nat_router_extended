#include <ESP8266WiFi.h>
#include <espnow.h>
#include "DHT.h"

#define DHTPIN 2
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

typedef struct struct_message {
  float temperature;
  float humidity;
} struct_message;

struct_message sensorData;

uint8_t receiverAddress[] = {0x3C, 0x8A, 0x1F, 0xAD, 0xAD, 0xF1};
const char* ssid = "ESP32_NAT_Router";
const char* password = "";

void onDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Send status: ");
  Serial.println(sendStatus == 0 ? "Success" : "Fail");
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nESP8266 DHT11 ESP-NOW Sender");
  
  dht.begin();
  WiFi.mode(WIFI_STA);
  
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to ESP32 AP");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Channel: ");
    Serial.println(WiFi.channel());
  } else {
    Serial.println("\nFailed to connect to ESP32 AP");
    ESP.restart();
  }
  
  Serial.print("ESP8266 MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  if (esp_now_init() != 0) {
    Serial.println("ESP-NOW init failed");
    ESP.restart();
  }
  Serial.println("ESP-NOW initialized");
  
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
  esp_now_register_send_cb(onDataSent);
  
  if (esp_now_add_peer(receiverAddress, ESP_NOW_ROLE_SLAVE, WiFi.channel(), NULL, 0) != 0) {
    Serial.println("Failed to add peer");
    return;
  }
  Serial.println("Peer added successfully");
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, restarting...");
    ESP.restart();
  }
  
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  
  if (isnan(t) || isnan(h)) {
    Serial.println("Sensor read failed");
    delay(2000);
    return;
  }
  
  sensorData.temperature = t;
  sensorData.humidity = h;
  
  uint8_t result = esp_now_send(receiverAddress, (uint8_t *)&sensorData, sizeof(sensorData));
  
  if (result == 0) {
    Serial.printf("Sent -> Temp: %.2f°C, Humidity: %.2f%%\n", t, h);
  } else {
    Serial.printf("Send Error (%u)\n", result);
  }
  
  delay(2000);
}
