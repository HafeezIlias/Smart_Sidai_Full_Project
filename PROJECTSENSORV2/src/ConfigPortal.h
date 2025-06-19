#ifndef CONFIG_PORTAL_H
#define CONFIG_PORTAL_H

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>
#include "Config.h"

// Config portal objects
AsyncWebServer server(80);
DNSServer dns;

// Function declarations
void startConfigPortal();
void enterConfigMode();
void checkConfigButton();
bool shouldEnterConfigMode();

void startConfigPortal() {
  WiFi.softAP("SmartHanger-Setup");
  dns.start(53, "*", WiFi.softAPIP());
  
  // Load current configuration
  String currentSSID = String(sysConfig.ssid);
  String currentPassword = String(sysConfig.password);
  String currentDeviceName = String(sysConfig.deviceName);
  String currentDeviceLocation = String(sysConfig.deviceLocation);
  
  // Serve configuration web page
  server.on("/", HTTP_GET, [currentSSID, currentPassword, currentDeviceName, currentDeviceLocation](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Smart Hanger Configuration</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 500px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h1 { text-align: center; color: #333; }
        .form-group { margin-bottom: 15px; }
        label { display: block; margin-bottom: 5px; font-weight: bold; }
        input[type="text"], input[type="password"] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; box-sizing: border-box; }
        .password-container { position: relative; }
        .toggle-password { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; color: #666; font-size: 14px; }
        .toggle-password:hover { color: #333; }
        button { background: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; width: 100%; font-size: 16px; }
        button:hover { background: #45a049; }
        .info { background: #e7f3ff; padding: 10px; border-radius: 5px; margin-bottom: 20px; }
    </style>
</head>
<body>
    <div class="container">
        <h1> Smart Hanger Setup</h1>
        <div class="info">
            <strong>Device:</strong> Smart Hanger<br>
            <strong>Status:</strong> Configuration Mode
        </div>
        <form action="/save" method="post">
            <div class="form-group">
                <label for="ssid">WiFi Network Name</label>
                <input type="text" id="ssid" name="ssid" value=")rawliteral";
    html += currentSSID;
    html += R"rawliteral(" required>
            </div>
            <div class="form-group">
                <label for="password">WiFi Password</label>
                <div class="password-container">
                    <input type="password" id="password" name="password" value=")rawliteral";
    html += currentPassword;
    html += R"rawliteral(">
                    <span class="toggle-password" onclick="togglePassword()">👁️</span>
                </div>
            </div>
            <div class="form-group">
                <label for="deviceName">Device Name</label>
                <input type="text" id="deviceName" name="deviceName" value=")rawliteral";
    html += currentDeviceName;
    html += R"rawliteral(">
            </div>
            <div class="form-group">
                <label for="deviceLocation">Device Location</label>
                <input type="text" id="deviceLocation" name="deviceLocation" value=")rawliteral";
    html += currentDeviceLocation;
    html += R"rawliteral(">
            </div>
            <button type="submit">Save & Restart</button>
        </form>
    </div>
    
    <script>
        function togglePassword() {
            const passwordInput = document.getElementById('password');
            const toggleButton = document.querySelector('.toggle-password');
            
            if (passwordInput.type === 'password') {
                passwordInput.type = 'text';
                toggleButton.innerHTML = '🙈';
            } else {
                passwordInput.type = 'password';
                toggleButton.innerHTML = '👁️';
            }
        }
    </script>
</body>
</html>
)rawliteral";
    request->send(200, "text/html", html);
  });
  
  // Handle configuration saving
  server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request) {
    // Save WiFi credentials
    if (request->hasParam("ssid", true)) {
        String ssid = request->getParam("ssid", true)->value();
        strncpy(sysConfig.ssid, ssid.c_str(), sizeof(sysConfig.ssid) - 1);
        sysConfig.ssid[sizeof(sysConfig.ssid) - 1] = '\0';
    }
    
    if (request->hasParam("password", true)) {
        String password = request->getParam("password", true)->value();
        strncpy(sysConfig.password, password.c_str(), sizeof(sysConfig.password) - 1);
        sysConfig.password[sizeof(sysConfig.password) - 1] = '\0';
    }
    
    // Save device info
    if (request->hasParam("deviceName", true)) {
        String deviceName = request->getParam("deviceName", true)->value();
        strncpy(sysConfig.deviceName, deviceName.c_str(), sizeof(sysConfig.deviceName) - 1);
        sysConfig.deviceName[sizeof(sysConfig.deviceName) - 1] = '\0';
    }
    
    if (request->hasParam("deviceLocation", true)) {
        String deviceLocation = request->getParam("deviceLocation", true)->value();
        strncpy(sysConfig.deviceLocation, deviceLocation.c_str(), sizeof(sysConfig.deviceLocation) - 1);
        sysConfig.deviceLocation[sizeof(sysConfig.deviceLocation) - 1] = '\0';
    }
    
    // Save configuration to EEPROM
    saveConfig();
    
    String responseHtml = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Configuration Saved</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f0f0f0; }
        .container { max-width: 400px; margin: 50px auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); text-align: center; }
        h1 { color: #4CAF50; }
        p { color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>✅ Configuration Saved!</h1>
        <p>Your Smart Hanger will now restart and connect to the WiFi network.</p>
        <p>This window will close automatically.</p>
    </div>
    <script>
        setTimeout(function(){ window.close(); }, 3000);
    </script>
</body>
</html>
)rawliteral";
    
    request->send(200, "text/html", responseHtml);
    Serial.println("Configuration saved successfully!");
    delay(3000);
    ESP.restart();
  });
  
  server.begin();
}

// Utility functions for compatibility
void checkConfigButton() {
  // Check if config button is pressed (GPIO 0 - BOOT button)
  if (digitalRead(CONFIG_PIN) == LOW) {
    Serial.println("Config button pressed - entering config mode...");
    enterConfigMode();
  }
}

void enterConfigMode() {
  Serial.println("=== ENTERING CONFIG MODE ===");
  
  // Show config mode on display
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  display.setCursor(0, 0);
  display.println("CONFIG MODE");
  
  display.setCursor(0, 12);
  display.println("WiFi:SmartHangerSetup");
  
  display.setCursor(0, 24);
  display.println("Go to: 192.168.4.1");
  display.display();
  
  // Start config portal
  startConfigPortal();
  
  // Stay in config mode until restart
  Serial.println("Config portal started. Device will restart after configuration.");
  while (true) {
    dns.processNextRequest();
    delay(100);
  }
}

bool shouldEnterConfigMode() {
  // Enter config mode if:
  // 1. Config button is pressed during startup
  // 2. WiFi credentials are empty
  // 3. WiFi connection fails multiple times
  
  if (digitalRead(CONFIG_PIN) == LOW) {
    Serial.println("Config button pressed during startup");
    return true;
  }
  
  if (strlen(sysConfig.ssid) == 0) {
    Serial.println("No WiFi credentials stored");
    return true;
  }
  
  return false;
}

void startConfigurationMode() {
    Serial.println("Starting configuration mode...");
    startConfigPortal();
    
    // Keep portal running until configured
    while (true) {
        dns.processNextRequest();
        delay(100);
    }
}

#endif