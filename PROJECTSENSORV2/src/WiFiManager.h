#ifndef WIFIMANAGER_H
#define WIFIMANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "Config.h"

// Function declarations
void initializeWiFi();
void connectToWiFi();
bool isWiFiConnected();

// Function to connect to WiFi network
void connectToWiFi() {
  // Use credentials from Preferences (sysConfig) instead of hardcoded values
  WiFi.begin(sysConfig.ssid, sysConfig.password);
  Serial.print("Connecting to WiFi network: ");
  Serial.println(sysConfig.ssid);
  
  int attempts = 0;
  const int maxAttempts = 20;
  
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal Strength: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  } else {
    Serial.println("WiFi connection failed!");
    Serial.println("Operating in offline mode...");
  }
}

// Function to initialize WiFi connection
void initializeWiFi() {
  Serial.println("Initializing WiFi...");
  
  // Debug: Show what credentials we're using
  Serial.print("Using WiFi SSID from config: ");
  Serial.println(sysConfig.ssid);
  Serial.print("Password length: ");
  Serial.println(strlen(sysConfig.password));
  
  // Check if credentials are empty
  if (strlen(sysConfig.ssid) == 0) {
    Serial.println("ERROR: No WiFi SSID configured!");
    Serial.println("Device should have entered config mode...");
    return;
  }
  
  connectToWiFi();
}

// Function to check WiFi connection status
bool isWiFiConnected() {
  return (WiFi.status() == WL_CONNECTED);
}

#endif // WIFIMANAGER_H