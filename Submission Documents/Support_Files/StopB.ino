#include "time.h"
#include <esp_now.h>
#include <WiFi.h>
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>


#define STOP_A  0
#define STOP_B  1
#define BUS  2

// Define hardware type, size, and output pins for 8*8 LED Display
#define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW
#define MAX_DEVICES 2
#define CS_PIN 5
#define DATA_PIN 19
#define CLK_PIN 18

// Motion Macros
#define MOVING 0
#define STOP_AT_A 1
#define STOP_AT_B 2
#define STOP_BETWEEN 3
#define MOTION_ACC 0
#define REST_ACC 1
#define ERR_VAL 4
#define TIME_OUT_FOR_MOTION_IN_MILLI 5000

// Out of Service Macros
#define INSERVICE 0
#define OUTOFSERIVCE 1

// Time of travel
#define INITIAL_TIME 60
#define TIME_FROM_A_TO_B 180 
#define TIME_FROM_B_TO_A 60
#define STOPTIME 60

// Delay Status
#define DELAYED 0
#define ONTIME 1

#define BUTTON_PIN 0 // GPIO21 pin connected to button

uint8_t motionStatus = MOVING;

// NTP Server and External Wifi Related Configurations:
const char* externalSSID =  "ELP305 P2";
const char* externalPassowrd = "Qwerty@123";  

const char* ntpServer = "time.google.com";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 19800;//GMT+5:30

// Starting time of all the devices.
const uint8_t Starthours = 16;
const uint8_t Startminutes = 10;

const int ownIdentity = STOP_B; // Identity = 0 for StopA, 1 for StopB, 2 for Bus Unit

uint8_t broadcastAddressStopA[] = {0xEC, 0x64, 0xC9, 0x86, 0x4C, 0x38};
uint8_t broadcastAddressStopB[] = {0x30, 0xC9, 0x22, 0x28, 0x4F, 0x60};
uint8_t broadcastAddressBusUnit[] = {0xEC, 0x64, 0xC9, 0x82, 0x7E, 0x34};

const char* ssidStopA  = "ESP32-Access-StopA";
const char* ssidStopB  = "ESP32-Access-StopB";
const char* ssidBusUnit = "ESP32-Access-BusUnit";

uint8_t running_timer = 60;
uint8_t delay_timer = 0;
uint8_t delayStatus = ONTIME;
uint8_t outofserviceStatus = OUTOFSERIVCE;

uint8_t passenger = 0;

float rssiA_B = 0.0;
// float rssiB = 0.0;

uint8_t inReadings[5];
bool valueRecieved = false;
uint8_t incomingAddr[6]; 

uint8_t outReadings[5];

esp_now_peer_info_t peerInfo1;
esp_now_peer_info_t peerInfo2;

esp_now_send_status_t lastPacketSendStatus = ESP_NOW_SEND_FAIL;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastPacketSendStatus = status;
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&inReadings, incomingData, sizeof(inReadings));
  memcpy(&incomingAddr, mac, sizeof(incomingAddr));
  valueRecieved = true;
}

hw_timer_t * timer = NULL;
volatile uint8_t count = 0; 
bool timerWork = false;

void IRAM_ATTR onTimer(){ // 8 instances. 1 to 8.
  running_timer = running_timer - 2;
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

MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// Used to print out of service message in case of failure
void displayOutOfService(){
  
  myDisplay.setIntensity(5);
  myDisplay.displayClear();
  myDisplay.displayText("Out of Service", PA_CENTER, 100, 0, PA_SCROLL_LEFT, PA_SCROLL_LEFT); //(Text, Letak, speed)
  Serial.println("OutOfSerive");

  if (myDisplay.displayAnimate()) {
    myDisplay.displayReset();
  }
}

// // Display Time on the 8*8 LED Matrix in minutes
void displayNumber(int num){
  
  if(num > 99) num = num % 100;
  myDisplay.displayClear();
  char str[4];
  itoa(num, str, 10);
  if(delayStatus == DELAYED) {
    // str[]
    str[2] = str[1];
    str[1] = str[0];
    str[0] = 'D';
    }
  myDisplay.displayText(str, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  if (myDisplay.displayAnimate()) {
    myDisplay.displayReset();
  }
}

//Check if the bus is delayed
void delayTest(){
  if(running_timer < 30 && rssiA_B == 0){
    delayStatus == DELAYED;
    return;
  }
  else{
    delayStatus = ONTIME;
    return;
  }
}

// void outofserviceTest(){
//   // Flag
//   // Delay Timer
// }

void displayOnLED(){
  if(outofserviceStatus == OUTOFSERIVCE){
    displayOutOfService();
  }
  else{
    if((ownIdentity == STOP_B) && (motionStatus == STOP_AT_B)){
      displayNumber(0);
    }
    else{
      uint8_t num = (running_timer)/60;
      num = num + 1;
      displayNumber(num);
    }
  }
}

void IRAM_ATTR PassengerHandler(){
  passenger++;
}

void setup() {
  // Starting Serial in case of serial connection
  Serial.begin(115200);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(BUTTON_PIN, PassengerHandler, FALLING);

  // Paralo setup
  myDisplay.begin();
  // Set the intensity (brightness) of the display (0-15):
  myDisplay.setIntensity(5);
  // Clear the display:
  myDisplay.displayClear();

  Serial.println("LED Matrix Display Initialized");

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

  outofserviceStatus = INSERVICE;
  delay_timer = 0;
  running_timer = INITIAL_TIME;

  WiFi.softAP(ssidStopB, NULL,2);
  WiFi.mode(WIFI_AP_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);
  // esp_wifi_set_channel(0);
  memcpy(peerInfo1.peer_addr, broadcastAddressStopB, 6);
  peerInfo1.channel = 2;  
  peerInfo1.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo1) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(peerInfo2.peer_addr, broadcastAddressBusUnit, 6);
  peerInfo2.channel = 2;  
  peerInfo2.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo2) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Setup Regarding the sending values
  outReadings[0] = ownIdentity; //Identity
  outReadings[1] = MOVING; // Motion
  outReadings[2] = INSERVICE; // Out Of Service 
  outReadings[3] = passenger; // Passenger
  outReadings[4] = 1; // 

  Serial.println("start timer ");

  timer = timerBegin(0, 80, true);  // timer 0, MWDT clock period = 12.5 ns * TIMGn_Tx_WDT_CLK_PRESCALE -> 12.5 ns * 80 -> 1000 ns = 1 us, countUp
  timerAttachInterrupt(timer, &onTimer, true); // edge (not level) triggered 
  timerAlarmWrite(timer, 2000000, true); // 2000000 * 2 us = 2 s, autoreload true
  timerAlarmEnable(timer); // enable

}

void loop() {
  
  if(!timerWork) goto label;
  switch (count) {

    case 1:
    // Flags reset
    
      rssiA_B = 0;

    // Active in BU
      // esp_now_send(broadcastAddressStopA, (uint8_t *) &outReadings, sizeof(outReadings));
      // if(lastPacketSendStatus != ESP_NOW_SEND_SUCCESS){break;}
      // Serial.print("\r\nLast Packet Send Status to Stop A:\t");
      // Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;
    case 2:
    // Active in BU
      // esp_now_send(broadcastAddressStopB, (uint8_t *) &outReadings, sizeof(outReadings));
      // if(lastPacketSendStatus != ESP_NOW_SEND_SUCCESS){break;}
      // Serial.print("\r\nLast Packet Send Status to Stop A:\t");
      // Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;
    case 3:
    // Active in BU
      // int n = WiFi.scanNetworks();
      // for (int i = 0; i < n; ++i) {
      //   if (WiFi.SSID(i) == ssidStopA) {
      //     rssi[0] = WiFi.RSSI(i);
      //     Serial.print("\nStrength of target ESP32 Stop A: ");
      //     Serial.println(rssi[0]);
      //   }
      //   else if (WiFi.SSID(i) == ssidStopB) {
      //     rssi[1] = WiFi.RSSI(i);
      //     Serial.print("Strength of target ESP32 Stop B: ");
      //     Serial.println(rssi[1]);
      //   }
      // }
      timerWork = false;
      break;
    case 4:
    // Active in Stop A
      timerWork = false;
      break;
    case 5:
    // Active in Stop B
      // esp_now_send(broadcastAddressBusUnit, (uint8_t *) &outReadings, sizeof(outReadings));
      // if(lastPacketSendStatus != ESP_NOW_SEND_SUCCESS){break;}
      // Serial.print("\r\nLast Packet Send Status to Bus Unit:\t");
      // Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      
      timerWork = false;
      break;
    case 6:
    outReadings[0] = ownIdentity;
    outReadings[3] = passenger;
    { esp_now_send(broadcastAddressBusUnit, (uint8_t *) &outReadings, sizeof(outReadings));
      // Serial.println(result);
      if(lastPacketSendStatus != ESP_NOW_SEND_SUCCESS){break;}
      Serial.print("\r\nLast Packet Send Status to Bus Unit:\t");
      Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;}
    //Active in BU
      // int n = WiFi.scanNetworks();
      // for (int i = 0; i < n; ++i) {
      //   if (WiFi.SSID(i) == ssidStopA) {
      //     rssi[0] = WiFi.RSSI(i);
      //     Serial.print("\nStrength of target ESP32 Stop A: ");
      //     Serial.println(rssi[0]);
      //   }
      //   else if (WiFi.SSID(i) == ssidStopB) {
      //     rssi[1] = WiFi.RSSI(i);
      //     Serial.print("Strength of target ESP32 Stop B: ");
      //     Serial.println(rssi[1]);
      //   }
      // }
      timerWork = false;
      break;
    case 7:
      // Available for use
      timerWork = false;
      break;
    case 8:
      // Available for use

      // Reset all the variables
      Serial.print("Reset");
      timerWork = false;
      break;
    default:
      break;
  
  }
  
displayOnLED();
delayTest();

label:
  if(valueRecieved){
    valueRecieved = false;
    Serial.printf("Incoming Readings:   Identity: %d\n",inReadings[0]);
    Serial.printf("MAC Address: %x %x %x %x %x %x\n",incomingAddr[0],incomingAddr[1],incomingAddr[2],incomingAddr[3],incomingAddr[4],incomingAddr[5]);

    if(inReadings[0] == BUS){
      motionStatus = inReadings[1];
      outofserviceStatus = inReadings[2];
      rssiA_B = inReadings[4];
    
      if((ownIdentity == STOP_B) && (motionStatus == STOP_AT_A)){
        running_timer = 180;
      }
      else if((ownIdentity == STOP_B) && (motionStatus == STOP_AT_B)){
        running_timer = 0;
      }
      else if((motionStatus == STOP_BETWEEN)){
        delayStatus = DELAYED;
      }
      else if((motionStatus == MOVING) && (running_timer == 0)){
        running_timer = 15;
      }
    }
  }
}