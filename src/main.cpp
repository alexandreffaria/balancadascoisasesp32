#include <Arduino.h>
#include <HX711_ADC.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(-1);

#if defined(ESP32)
#include "EEPROM.h" // For ESP32, include EEPROM library
#endif

// Pins:
const int HX711_dout = 13; // MCU > HX711 dout pin
const int HX711_sck = 15; // MCU > HX711 sck pin
const int buttonPin = 12; // Push button connected to GPIO 2 on ESP32
const int SDA_PIN = 14;  // You can choose any available GPIO pin
const int SCL_PIN = 2;  // You can choose any available GPIO pin

// HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int calVal_eepromAddress = 0;
unsigned long t = 0;
bool buttonState = false;
bool lastButtonState = false;

void setup() {
  // Initialize with I2C address 0x3C
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  Serial.begin(57600);
  delay(10);
  Serial.println();
  Serial.println("Starting...");

  LoadCell.begin();
  float calibrationValue;
  calibrationValue = 465.0;

#if defined(ESP32)
  // EEPROM.begin(512); // Uncomment this if you use ESP32 and want to fetch the calibration value from EEPROM
#endif

  EEPROM.get(calVal_eepromAddress, calibrationValue);

  unsigned long stabilizingTime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingTime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU > HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Startup is complete");
  }

  pinMode(buttonPin, INPUT_PULLUP); // Set the button pin as an input with internal pull-up resistor

  // Clear the display
  display.clearDisplay();
  display.display();
}

void loop() {
  static boolean newDataReady = false;
  const int serialPrintInterval = 0;

  // Check for new data/start next conversion:
  if (LoadCell.update()) {
    newDataReady = true;
  }

  // Get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      float i = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(i);
      newDataReady = false;
      t = millis();

      // Clear the display
      display.clearDisplay();

      // Set text size
      display.setTextSize(1);

      // Set text color
      display.setTextColor(SSD1306_WHITE);

      // Display scale reading on OLED
      display.setCursor(0, 0);
      display.println("Gramas (g):");
      display.setTextSize(3);
      display.print(int(i));
      display.println("g");

      // Update the display
      display.display();
    }
  }

  // Receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) {
    char inByte = Serial.read();
    if (inByte == 't') {
      LoadCell.tareNoDelay();
      Serial.println("Tare initiated");
    }
  }

  // Check if the button is pressed to initiate tare operation:
  buttonState = digitalRead(buttonPin);
  if (buttonState != lastButtonState) {
    if (buttonState == LOW) { // Button is pressed
      LoadCell.tareNoDelay();
      Serial.println("Button Tare initiated");
    }
    lastButtonState = buttonState;
  }

  // Check if the last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    Serial.println("Tare complete");
  }
}
