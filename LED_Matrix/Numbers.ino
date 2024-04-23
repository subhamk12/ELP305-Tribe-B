// Include the required Arduino libraries:
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>

// Define hardware type, size, and output pins:
#define HARDWARE_TYPE MD_MAX72XX::ICSTATION_HW
#define MAX_DEVICES 2
#define CS_PIN 5
#define DATA_PIN 19
#define CLK_PIN 18

// Create a new instance of the MD_Parola class with hardware SPI connection:
//MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Setup for software SPI:
MD_Parola myDisplay = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);
int num = 10;

void setup() {
  // Intialize the object:
  myDisplay.begin();
  // Set the intensity (brightness) of the display (0-15):
  myDisplay.setIntensity(0);
  // Clear the display:
  myDisplay.displayClear();
}

void loop() {
  if (myDisplay.displayAnimate()) {
    myDisplay.displayReset();
  }

  num--;
  if (num<=0)num=99;
  char str[3];
  itoa(num, str, 10);
  myDisplay.displayText(str, PA_RIGHT, 0, 0, PA_PRINT, PA_NO_EFFECT);
  delay(500); // Wait for 2 seconds

}
