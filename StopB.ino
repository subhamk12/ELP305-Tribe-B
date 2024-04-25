#include <esp_now.h>
#include <WiFi.h>

/*

There will be 8 divisions of a clock signal.

1. Bu -> A
2. Bu -> B
3. Bu RSSI Check
4. A -> Bu
5. B -> Bu
6. Bu RSSI Check
7. Buffer
8. Buffer

*/

#define STOP_A  0
#define STOP_B  1
#define BUS  2

const int ownIdentity = STOP_B; // Identity = 0 for StopA, 1 for StopB, 2 for Bus Unit

uint8_t broadcastAddressStopA[] = {0xEC, 0x64, 0xC9, 0x86, 0x4C, 0x38};
uint8_t broadcastAddressStopB[] = {0x30, 0xC9, 0x22, 0x28, 0x4F, 0x60};
uint8_t broadcastAddressBusUnit[] = {0xEC, 0x64, 0xC9, 0x82, 0x7E, 0x34};

const char* ssidStopA  = "ESP32-Access-StopA";
const char* ssidStopB  = "ESP32-Access-StopB";
const char* ssidBusUnit = "ESP32-Access-BusUnit";


typedef struct struct_message{
  int identity;
  int motion;
  bool delay;
  bool passenger;
  bool outofserviceMsg;
} struct_message;

float rssi[2]={0.0,0.0};

struct_message inReadings;
bool valueRecieved = false;
uint8_t incomingAddr[6]; 

struct_message outReadings;

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

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssidStopB, NULL);
  WiFi.mode(WIFI_AP);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_send_cb(OnDataSent);

  memcpy(peerInfo1.peer_addr, broadcastAddressStopA, 6);
  peerInfo1.channel = 0;  
  peerInfo1.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo1) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  memcpy(peerInfo2.peer_addr, broadcastAddressBusUnit, 6);
  peerInfo2.channel = 0;  
  peerInfo2.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo2) != ESP_OK) {
    Serial.println("Failed to add peer");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Some Setup
  outReadings.identity = ownIdentity;
  outReadings.motion = 0;
  outReadings.delay = true;
  outReadings.passenger = false;
  outReadings.outofserviceMsg = true;

  Serial.println("start timer ");

  timer = timerBegin(0, 80, true);  // timer 0, MWDT clock period = 12.5 ns * TIMGn_Tx_WDT_CLK_PRESCALE -> 12.5 ns * 80 -> 1000 ns = 1 us, countUp
  timerAttachInterrupt(timer, &onTimer, true); // edge (not level) triggered 
  timerAlarmWrite(timer, 1000000, true); // 2000000 * 2 us = 2 s, autoreload true
  timerAlarmEnable(timer); // enable

}

void loop() {
  
  if(!timerwork) goto label;
  switch (count) {

    case 1:
    // Active in BU
      // esp_now_send(broadcastAddressStopA, (uint8_t *) &outReadings, sizeof(outReadings));
      // Serial.print("\r\nLast Packet Send Status to Stop A:\t");
      // Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;
    case 2:
    // Active in BU
      // esp_now_send(broadcastAddressStopB, (uint8_t *) &outReadings, sizeof(outReadings));
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
      // esp_now_send(broadcastAddressBusUnit, (uint8_t *) &outReadings, sizeof(outReadings));
      // Serial.print("\r\nLast Packet Send Status to Bus Unit:\t");
      // Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;
    case 5:
    // Active in Stop B
      esp_now_send(broadcastAddressBusUnit, (uint8_t *) &outReadings, sizeof(outReadings));
      Serial.print("\r\nLast Packet Send Status to Bus Unit:\t");
      Serial.println(lastPacketSendStatus == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
      timerWork = false;
      break;
    case 6:
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
      timerWork = false;
      break;
    default:
      break;
  
  }
  
label:
  if(valueRecieved){
    valueRecieved = false;
    Serial.printf("Incoming Readings: 1. Identity: %d\n",inReadings.identity);
    Serial.printf("MAC Address: %x %x %x %x %x %x\n",incomingAddr[0],incomingAddr[1],incomingAddr[2],incomingAddr[3],incomingAddr[4],incomingAddr[5]);
  }
}

