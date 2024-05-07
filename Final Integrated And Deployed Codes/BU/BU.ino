#include <esp_now.h>
#include <WiFi.h>
#include "time.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Arduino.h>
#include <ThingSpeak.h>

#define CHANNEL_ID 2522697
#define CHANNEL_API_KEY "452QHPC69IP1KGY7"
/*

There will be 8 divisions of a clock signal.

1. Flag reset
2. Bu RSSI Check
3. 
4. bu -> A
5. Bu -> B
6. a -> Bu
7. b -> Bu
8. buffer

*/

#define STOP_A  0
#define STOP_B  1
#define BUS  2

// Motion Macros
#define MOVING 0
#define STOP_AT_A 1
#define STOP_AT_B 2
#define STOP_BETWEEN 3
#define MOTION_ACC 0
#define REST_ACC 1
#define ERR_VAL 4
#define TIME_OUT_FOR_MOTION_IN_MILLI 5000

// Out of serviceStatus Macros
#define INSERVICE 0
#define OUTOFSERIVCE 1

// 
#define BUTTON_PIN 0 // GIOP21 pin connected to button

// NTP Server and External Wifi Related Configurations:
const char* externalSSID =  "ELP305 P2";
const char* externalPassowrd = "Qwerty@123";  

const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 19800;//GMT+5:30

// Starting time of all the devices.
const uint8_t Starthours = 16;
const uint8_t Startminutes = 10;

const int ownIdentity = BUS; // Identity = 0 for StopA, 1 for StopB, 2 for Bus Unit

const int rssiThreshold = -80; // RSSI Threshold  

volatile uint8_t motionBU = MOVING;
volatile uint8_t motionAccelerometer = MOTION_ACC; // 0 for moving, 1 for stop

uint8_t broadcastAddressStopA[] = {0xEC, 0x64, 0xC9, 0x86, 0x4C, 0x38};
uint8_t broadcastAddressStopB[] = {0x30, 0xC9, 0x22, 0x28, 0x4F, 0x60};
uint8_t broadcastAddressBusUnit[] = {0xEC, 0x64, 0xC9, 0x82, 0x7E, 0x34};

const char* ssidStopA  = "ESP32-Access-StopA";
const char* ssidStopB  = "ESP32-Access-StopB";
const char* ssidBusUnit = "ESP32-Access-BusUnit";

float rssiA = 0.0;
float rssiB = 0.0;

uint8_t inReadings[5];
bool valueRecieved = false;
uint8_t incomingAddr[6]; 

uint8_t outReadings[5];

esp_now_peer_info_t peerInfo1;
esp_now_peer_info_t peerInfo2;

esp_now_send_status_t lastPacketSendStatus = ESP_NOW_SEND_FAIL;

hw_timer_t * timer = NULL;
volatile uint8_t count = 0; 
bool timerWork = false;

uint8_t serviceStatus = OUTOFSERIVCE;
uint8_t serviceFirst = 0;

int red_led = 14; // Pin GPIO for Red Leg
int green_led = 27; // Pin GPIO for Green Leg
int blue_led = 26; // Pin GPIO for Blue Leg
int flickerDelay = 10;  // delay between intensity changes (in milliseconds)

// Values related to MPU
Adafruit_MPU6050 mpu;

const float dt = 0.01; // time interval in seconds
float acceleration = 0;
float velocity_x = 0;
float velocity_y = 0;
float distance = 0;
float acceleration_x=0;
float acceleration_y=0;
float acceleration_z=0;
float velocity=0;
// float motion=0;
unsigned long startTime=0;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastPacketSendStatus = status;
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&inReadings, incomingData, sizeof(inReadings));
  memcpy(&incomingAddr, mac, sizeof(incomingAddr));
  valueRecieved = true;
}

void IRAM_ATTR onTimer(){ // 8 instances. 1 to 8.
  if(count < 8){
    count++;
    timerWork = true;
    return;
  }
  else{
    timerWork = true;
    count = 1;
  }
  return;
}

/*
Detects the motion of the Bus Unit, and performs stop detection.
Outputs -> 0: Motion
*/
uint8_t motion(){ 
  if(!motionAccelerometer) return MOVING;
  if(rssiA >= rssiThreshold){
    return STOP_AT_A;
  }
  if(rssiB >= rssiThreshold){
    return STOP_AT_B;
  }
  return STOP_BETWEEN;
}

// Detects the motion of BU based on the MPU6050 accelerometer
void detect_motion(){
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  
  acceleration_x = a.acceleration.x -0.4; // Assuming motion is along x-axis, adjust accordingly for other axes
  acceleration_y = a.acceleration.y +0.3;
  acceleration_z = a.acceleration.z -0.3;
  acceleration = sqrt(pow(acceleration_x,2)+pow(acceleration_y,2)+pow(acceleration_z,2));
  velocity_x += acceleration_x * dt;
  velocity_y += acceleration_y * dt;
  velocity = sqrt(pow(velocity_x,2)+pow(velocity_y,2));
  distance += velocity * dt;

  if (acceleration<10.2 && millis()-startTime>TIME_OUT_FOR_MOTION_IN_MILLI){
    motionAccelerometer=REST_ACC;
    return;
  }
  else if(acceleration<10.2 && millis()-startTime<TIME_OUT_FOR_MOTION_IN_MILLI){
    motionAccelerometer=REST_ACC;
    return;
  }
  else if(acceleration_x > 60){
    motionAccelerometer = ERR_VAL;
    return;
  }
  else {
    motionAccelerometer=MOTION_ACC;
    startTime=millis();
    return;
  }
    return;
}

// Red Colour LED of BU
void red_color()
{
  digitalWrite(red_led, LOW);
  digitalWrite(green_led, HIGH);
  digitalWrite(blue_led, HIGH);
  // delay(1000);
}

// Blinking Red Colour LED of BU
void blinking_red_color()
{
  digitalWrite(red_led, LOW);
  digitalWrite(green_led, HIGH);
  digitalWrite(blue_led, HIGH);
  delay(flickerDelay);
  digitalWrite(red_led,HIGH);
  digitalWrite(blue_led,HIGH);
  digitalWrite(green_led,HIGH);
  delay(flickerDelay);
}

// Green Colour LED of BU
void green_color()
{
  digitalWrite(red_led, HIGH);
  digitalWrite(green_led, LOW);
  digitalWrite(blue_led, HIGH);
  // delay(1000);
}

// Blinking Green Colour LED of BU
void blinking_green_color()
{
  digitalWrite(red_led, HIGH);
  digitalWrite(green_led, LOW);
  digitalWrite(blue_led, HIGH);
  delay(flickerDelay);
  digitalWrite(red_led,HIGH);
  digitalWrite(blue_led,HIGH);
  digitalWrite(green_led,HIGH);
  delay(flickerDelay);
}

void IRAM_ATTR serviceHandler(){
  if(serviceStatus == OUTOFSERIVCE){
    serviceStatus = INSERVICE;
  }
  else{
    serviceStatus = OUTOFSERIVCE;
    serviceFirst = 1;
  }
}

// Uploads data to server
void upload(){
  // Connect to Wi-Fi
  // Serial.print("Connecting to ");
  // Serial.println(externalSSID);
  // WiFi.begin(externalSSID, externalPassowrd);
  // while (WiFi.status() != WL_CONNECTED) {
  //   // delay(500);
  //   Serial.print(".");
  // }
  // Serial.println("");
  // if (WiFi.status() == WL_CONNECTED){
  //     Serial.println("WiFi Connected");
  // }

}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, serviceHandler, FALLING);

  // Initializing the LEDs
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(blue_led, OUTPUT);

  // Some Setup
  outReadings[0] = ownIdentity;
  outReadings[1] = motionBU; // Motion
  outReadings[2] = serviceStatus; // Out of service
  outReadings[3] = 0; // Passenger
  outReadings[4] = 1; // Delay

  // MPU Setup
  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  Serial.println("MPU6050 Found!");
  
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);

  // Code to start the system at appropriate time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  // Connect to Wi-Fi
  Serial.print("Connecting to ");
  Serial.println(externalSSID);
  WiFi.begin(externalSSID, externalPassowrd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  if (WiFi.status() == WL_CONNECTED){
      Serial.println("WiFi Connected");
  }

  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  }
  // In case unable to retrive time
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  uint8_t hours = timeinfo.tm_hour;
  uint8_t minutes = timeinfo.tm_min;

  // Wait till the Start time is reached
  while((hours < Starthours )|| (minutes < Startminutes)){
    delay(1000);
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    hours = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
  }

  WiFi.disconnect(true);
  Serial.println("Wifi disconnected");
    
  WiFi.softAP(ssidBusUnit, NULL,2);
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  // esp_wifi_set_channel(0);
  memcpy(peerInfo1.peer_addr, broadcastAddressStopA, 6);
  peerInfo1.channel = 2;  
  peerInfo1.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo1) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(peerInfo2.peer_addr, broadcastAddressStopB, 6);
  peerInfo2.channel = 2;  
  peerInfo2.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo2) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);
  
  // Timer Setup
  Serial.println("start timer ");
  
  timer = timerBegin(0, 80, true);  // timer 0, MWDT clock period = 12.5 ns * TIMGn_Tx_WDT_CLK_PRESCALE -> 12.5 ns * 80 -> 1000 ns = 1 us, countUp
  timerAttachInterrupt(timer, &onTimer, true); // edge (not level) triggered 
  timerAlarmWrite(timer, 2000000, true); // 2000000 * 2 us = 2 s, autoreload true
  timerAlarmEnable(timer); // enable
}
int n;
void loop() {
  
  if(!timerWork) goto label;
  switch (count) {

    case 1:
    {// Reset Flags
      motionBU = MOVING;
      motionAccelerometer = REST_ACC;

      detect_motion();
      // Active in BU

      timerWork = false;
      break;}
    case 2:
    // RSSI
    { n = WiFi.scanNetworks();
      for (int i = 0; i < n; ++i) {
        if (WiFi.SSID(i) == ssidStopA) {
          rssiA = WiFi.RSSI(i);
          Serial.print("\nStrength of target ESP32 Stop A: ");
          Serial.println(rssiA);
        }
        else if (WiFi.SSID(i) == ssidStopB) {
          rssiB = WiFi.RSSI(i);
          Serial.print("Strength of target ESP32 Stop B: ");
          Serial.println(rssiB);
        }
      }
      timerWork = false;
      break;
    }

    case 3:
    // Update Values
    {motionBU = motion();

    outReadings[0] = ownIdentity;
    outReadings[1] = motionBU;
    outReadings[2] = serviceStatus;
    outReadings[3] = 0; // Passenger
    outReadings[4] = 0; // Delay
    timerWork = false;
    break;
    }
    case 4:
    // Bu -> A

     {esp_now_send(broadcastAddressStopA, (uint8_t *) &outReadings, sizeof(outReadings));
      // Serial.println(result2);
      if(lastPacketSendStatus != ESP_NOW_SEND_SUCCESS){
      // Serial.print("\r\nLast Packet Send Status to Stop B: fail\t");
        break;}
      Serial.print("\r\nLast Packet Send Status to Stop B:\t");
      Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;
      }
    case 5:
    // Bu -> B

    {esp_now_send(broadcastAddressStopB, (uint8_t *) &outReadings, sizeof(outReadings));
      // Serial.println(result2);
      if(lastPacketSendStatus != ESP_NOW_SEND_SUCCESS){
      // Serial.print("\r\nLast Packet Send Status to Stop B: fail\t");
        break;}
      Serial.print("\r\nLast Packet Send Status to Stop B:\t");
      Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;
      }

    case 6:

    // A -> Bu
    timerWork = false;
    break;

    case 7:
    // B -> Bu

      // Available for use
      // outReadings[1] = 0;
      // esp_now_send(broadcastAddressStopA, (uint8_t *) &outReadings, sizeof(outReadings));
      // Serial.println("Switched Back");
      timerWork = false;
      break;
    case 8:
    // A <-> B (Based on Stop Status)
    // Uplaod in case of error

      if((serviceStatus == OUTOFSERIVCE) && (serviceFirst == 1)){
        // upload();
        serviceFirst = 0;
      }

      // Available for use
      timerWork = false;
      break;
    default:
      break;
  
  }
  
label:
  if(valueRecieved){
    valueRecieved = false;
    Serial.printf("Incoming Readings: Identity: %d\n",inReadings[0]);
    Serial.printf("MAC Address: %x %x %x %x %x %x\n",incomingAddr[0],incomingAddr[1],incomingAddr[2],incomingAddr[3],incomingAddr[4],incomingAddr[5]);
    // if(inReadings[3] == 1){
    //   blinking_green_color();
    // }
    // else{
    //   if(serviceStatus == OUTOFSERIVCE){
    //     red_color();
    //   }
    //   else{
    //   green_color();}
    // }
  }
  // continue;
}