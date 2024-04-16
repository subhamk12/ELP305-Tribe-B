#include <Arduino.h>
#include <WiFi.h>
// #include <ESP8266WiFi.h>

  // WiFi.begin(ssid, password);

  
// #define WIFI_SSID "Pratibha's Galaxy F14 5G"
// #define WIFI_PASSWORD "6rx4mvxpj6fp94x"

// #define WIFI_SSID "OnePlus 9R"
// #define WIFI_PASSWORD "svqx227j"

#define WIFI_SSID "Siddhika's OPPO"
#define WIFI_PASSWORD "siddhikaTailor"


// svqx227j


void setup() {
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  // Serial.print("IP address:\t");
  // IPAddress myIP = WiFi.localIP();
  // Serial.println(myIP);


  Serial.println("STARTING");
}

bool isConnected = false;

void loop() {
  Serial.println(WiFi.RSSI());
  if(WiFi.status() == WL_CONNECTED && !isConnected){
    Serial.println("Connected");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN, HIGH);
    isConnected = true;
    Serial.println(WiFi.macAddress());
    Serial.println(WiFi.localIP());
    // Serial.println(WiFi.RSSI());

  }
  if(WiFi.status() != WL_CONNECTED){
    Serial.println(".");
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    delay(1000);
    isConnected = false;
  }
}
