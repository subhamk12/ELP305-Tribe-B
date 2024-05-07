#include <esp_now.h>
#include <WiFi.h>

const char* ssid     = "ESP32-Access-PointB";
float arr[100];

float rssi=0;
int con=0;
float avg=0;
float sum=0;
int flag=0;
// REPLACE WITH THE MAC Address of your receiver 
uint8_t broadcastAddress[] = {0x30,0xC9,0x22,0x28,0x4F,0x60};

// Define variables to store BME280 readings to be sent
float value1;
float value2;


// Define variables to store incoming readings
float incomingvalue1;
float incomingvalue2;


// Variable to store if sending data was successful
String success;

//Structure example to send data
//Must match the receiver structure
typedef struct struct_message {
    float val1;
    float val2;
} struct_message;

// Create a struct_message called BME280Readings to hold sensor readings
struct_message OutgoingReadings;

// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;

esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if (status ==0){
    success = "Delivery Success :)";
  }
  else{
    success = "Delivery Fail :(";
  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
  incomingvalue1 = incomingReadings.val1;
  incomingvalue2 = incomingReadings.val2;
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(115200);
  WiFi.softAP(ssid,NULL);
 

  // Init OLED display
 
 
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_AP);
  for (int i=0;i<100;i++){
    arr[i]=0;
  }

  
  


  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);
}
 
void loop() {
  getReadings();
 
  // Set values to send


  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &OutgoingReadings, sizeof(OutgoingReadings));
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; ++i) {
    if (WiFi.SSID(i) == "ESP32-Access-PointA") {
      rssi = WiFi.RSSI(i);
      Serial.print("Strength of target ESP32: ");
      Serial.println(rssi);
      if (flag==0){
        arr[con]=rssi;
        con++;
        sum=sum+rssi;
        avg=sum/con;
      }
      if (flag==1){
        int store=arr[con];
        arr[con]=rssi;
        sum=sum-store+rssi;
        avg=sum/100;
        con++;
      }
    
      if (con==100){
        flag=1;
        con=0;
      }
      break;
    }
  }
  
  Serial.print("average strength: ");
  Serial.println(avg);
  OutgoingReadings.val1 = value1;
  OutgoingReadings.val2 = value2;
  Serial.println("OUTGOING READINGS");
  Serial.print("value1: ");
  Serial.print(value1);
  Serial.print(" value2: ");
  Serial.print(value2);
  Serial.println();
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
  updateDisplay();
  delay(10000);
}
void getReadings(){
  value1 = random(1,1000)/37.00;
  value2 = random(1,1000)/23.00;
}

void updateDisplay(){
  
  // Display Readings in Serial Monitor
  Serial.println("INCOMING READINGS");
  Serial.print("value1: ");
  Serial.print(incomingReadings.val1);
  Serial.print(" value2: ");
  Serial.print(incomingReadings.val2);
  Serial.println();
  
}