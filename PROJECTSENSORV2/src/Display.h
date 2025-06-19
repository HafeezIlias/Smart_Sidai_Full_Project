#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Config.h"

// Function declarations
void initializeDisplay();
void showStartupScreen();
void updateDisplay();
void showErrorScreen(String error);

// Function to show startup screen
void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Smart Hanger");
  display.setCursor(0, 12);
  display.println("Starting...");
  display.display();
  delay(1000);
}

// Function to initialize OLED display
void initializeDisplay() {
  Serial.println("Initializing OLED display...");
  
  // Initialize the display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    return;
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  showStartupScreen();
  Serial.println("OLED display initialized successfully");
}

// Function to update main display with sensor data
void updateDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  
  display.println("Smart Hanger");
  display.print("State: ");
  if (currentState == EXTENDED) display.println("EXT");
  else if (currentState == RETRACTED) display.println("RET");
  else display.println("---");
  
  display.print("T:");
  display.print(temperature, 1);
  display.print("C H:");
  display.print(humidity, 1);
  display.println("%");
  
  display.print("Rain:");
  display.println(rainDetected ? "YES" : "NO");
  
  display.print("Dist:");
  display.print(hangerDistance, 0);
  display.println("cm");
  
  display.display();
}

// Function to show error messages
void showErrorScreen(String error) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ERROR!");
  display.println(error);
  display.display();
}

#endif // DISPLAY_H