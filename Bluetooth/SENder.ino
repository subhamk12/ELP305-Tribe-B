#include "BluetoothSerial.h"
#define Right_VRX_PIN 12
#define Right_VRY_PIN 13
#define Left_VRX_PIN 25
#define Left_VRY_PIN 26

int left_x_val;
int left_y_val;
int right_x_val;
int right_y_val;  

BluetoothSerial SerialBT;

String MACadd = "EC:64:C9:86:4C:3A";//Write Drone side MAC address
uint8_t address[6]  = {0xEC, 0x64, 0xC9, 0x86, 0x4C, 0x3A};//Write Drone side MAC address in HEX
bool connected;

void setup() {
  Serial.begin(115200);
  SerialBT.begin("ESP32test", true); 
  Serial.println("The device started in master mode, make sure remote BT device is on!");
  
  // connect(address) is fast (upto 10 secs max), connect(name) is slow (upto 30 secs max) as it needs
  // to resolve name to address first, but it allows to connect to different devices with the same name.
  // Set CoreDebugLevel to Info to view devices bluetooth address and device names
  connected = SerialBT.connect(address);
  
  if(connected) {
    Serial.println("Connected Succesfully!");
  } else {
    while(!SerialBT.connected(10000)) {
      Serial.println("Failed to connect. Make sure remote device is available and in range, then restart app."); 
    }
  }
  // disconnect() may take upto 10 secs max
  if (SerialBT.disconnect()) {
    Serial.println("Disconnected Succesfully!");
  }
  // this would reconnect to the name(will use address, if resolved) or address used with connect(name/address).
  SerialBT.connect();

  pinMode(Left_VRX_PIN, INPUT);
  pinMode(Left_VRY_PIN, INPUT);
  pinMode(Right_VRX_PIN, INPUT);
  pinMode(Right_VRX_PIN, INPUT);
  
}

uint8_t calculate_checksum(uint8_t *data) {
  uint8_t checksum = 0;
  checksum |= 0b11000000 & data[1];
  checksum |= 0b00110000 & data[2];
  checksum |= 0b00001100 & data[3];
  checksum |= 0b00000011 & data[4];
  return checksum;
}

void loop() {

  uint8_t send_data[6];
  
  left_x_val = analogRead(Left_VRX_PIN) >> 4;//value 0-255 (">> 4" convert maximum value from 4095 to 255 )
  left_y_val = analogRead(Left_VRY_PIN) >> 4;//value 0-255  
  right_x_val = analogRead(Right_VRX_PIN) >> 4;//value 0-255
  right_y_val = analogRead(Right_VRY_PIN) >> 4;//value 0-255

  send_data[0] = 'T';
  send_data[1] = left_x_val;
  send_data[2] = left_y_val;
  send_data[3] = right_x_val;
  send_data[4] = right_y_val;
  send_data[5] = calculate_checksum(send_data);
  SerialBT.write(send_data, 6);

  delay(20);
}