#ifndef FIREBASE_H
#define FIREBASE_H

#include <Firebase_ESP_Client.h>
// Provide the token generation process info.
#include <addons/TokenHelper.h>
// Provide the RTDB payload printing info and other helper functions.
#include <addons/RTDBHelper.h>
#include "Config.h"
#include <WiFi.h>
#include <time.h>

// Forward declarations for motor control functions
void extendHanger();
void retractHanger();
void stopMotor();

// ============================================================================
// NTP TIME CONFIGURATION
// ============================================================================
// NTP Time Configuration:
// - System time is kept in UTC (GMT_OFFSET_SEC = 0)
// - Firebase timestamps are stored in UTC for consistency
// - Local time display functions add timezone offset as needed
//
// LOCAL_TIMEZONE_OFFSET_SEC: Your local timezone offset from UTC
// Examples:
//   UTC+0  (London): 0
//   UTC+1  (Paris):  3600
//   UTC+8  (Singapore/Manila): 28800
//   UTC-5  (New York): -18000
//   UTC-8  (Los Angeles): -28800
#define NTP_SERVER1 "pool.ntp.org"
#define NTP_SERVER2 "time.nist.gov"
#define NTP_SERVER3 "time.google.com"
#define GMT_OFFSET_SEC 0                    // Keep system time in UTC
#define DAYLIGHT_OFFSET_SEC 0               // No daylight saving adjustment
#define LOCAL_TIMEZONE_OFFSET_SEC 28800     // +8 hours for local time display
#define NTP_TIMEOUT_MS 10000                // 10 seconds timeout for NTP sync

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================
String getDeviceBasePath();
bool isFirebaseConnected();
bool initializeFirebase();
void sendSensorDataToFirebase();
void sendSystemStatusToFirebase();
void registerDeviceToFirebase();
void updateFirebaseConfiguration();
void sendAlertToFirebase(const String& level, const String& message);
void listenForFirebaseConfigChanges();
void clearFirebaseRequestedState();

// NTP Time Functions
bool initializeNTP();
unsigned long long getCurrentTimestampMillis();  // UTC timestamp in milliseconds since epoch
bool isTimeSet();
void printUTCTime();
void printLocalTime();

// ============================================================================
// GLOBAL FIREBASE OBJECTS
// ============================================================================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// ============================================================================
// FUNCTION IMPLEMENTATIONS
// ============================================================================

String getDeviceBasePath(){
    String basePath = "/devices/" + String(deviceID);
    Serial.print("Firebase device base path: ");
    Serial.println(basePath);
    return basePath;
}

bool isFirebaseConnected() {
    bool connected = Firebase.ready();
    Serial.print("Firebase connection status: ");
    Serial.println(connected ? "CONNECTED" : "DISCONNECTED");
    return connected;
}

bool initializeFirebase(){
    Serial.println("Initializing Firebase...");

    // Check WiFi connection first
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected, skipping Firebase initialization");
        return false;
    }

    // Initialize Firebase configuration
    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_DATABASE_URL;

    // Assign the user sign in credentials
    auth.user.email = FIREBASE_USER_EMAIL;
    auth.user.password = FIREBASE_USER_PASSWORD;

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    // To refresh the token 5 minutes before expired
    config.signer.preRefreshSeconds = 5 * 60;

  if (Firebase.signUp(&config, &auth, "", "")) {
      Serial.println("Firebase signUp successful");
    } else {
      Serial.println("Firebase signUp failed");
    }

    // Initialize Firebase
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    
    // Wait for Firebase to initialize with timeout
    Serial.println("Waiting for Firebase authentication...");
    unsigned long startTime = millis();
    const unsigned long timeoutMs = 30000; // 30 second timeout
    
    while (!Firebase.ready() && (millis() - startTime) < timeoutMs) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println();
    
    if (Firebase.ready()) {
        Serial.println("Firebase initialized successfully!");
        return true;
    } else {
        Serial.println("Firebase initialization timed out!");
        return false;
    }
}

// ============================================================================
// NTP TIME FUNCTIONS
// ============================================================================

bool initializeNTP() {
    Serial.println("Initializing NTP time synchronization...");
    
    // Check WiFi connection first
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected - cannot initialize NTP");
        return false;
    }
    
    // Configure NTP with multiple servers for redundancy
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    
    Serial.print("Waiting for NTP time sync");
    unsigned long startTime = millis();
    
    // Wait for time to be set with timeout
    while (!isTimeSet() && (millis() - startTime) < NTP_TIMEOUT_MS) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    
    if (isTimeSet()) {
        Serial.println("✓ NTP time synchronized successfully!");
        printUTCTime();
        printLocalTime();
        return true;
    } else {
        Serial.println("❌ NTP time synchronization failed!");
        return false;
    }
}

unsigned long long getCurrentTimestampMillis() {
    if (!isTimeSet()) {
        Serial.println("Warning: Time not set, using device uptime");
        return (unsigned long long)millis();
    }
    
    time_t now;
    time(&now);
    
    // Convert to milliseconds since Unix epoch (January 1, 1970)
    unsigned long long timestampMillis = (unsigned long long)now * 1000;
    
    return timestampMillis;
}

bool isTimeSet() {
    time_t now;
    time(&now);
    return now > 1000000000; // Check if time is after year 2001 (Unix timestamp > 1 billion)
}

void printUTCTime() {
    if (!isTimeSet()) {
        Serial.println("Time not set");
        return;
    }
    
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%A, %B %d %Y %H:%M:%S UTC", &timeinfo);
    Serial.print("Current UTC time: ");
    Serial.println(buffer);
}

void printLocalTime() {
    if (!isTimeSet()) {
        Serial.println("Time not set");
        return;
    }
    
    // Get UTC time and add local offset
    time_t utcTime;
    time(&utcTime);
    time_t localTime = utcTime + LOCAL_TIMEZONE_OFFSET_SEC;
    
    struct tm* localTimeInfo = gmtime(&localTime);
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%A, %B %d %Y %H:%M:%S +08", localTimeInfo);
    Serial.print("Current local time: ");
    Serial.println(buffer);
}

void sendSensorDataToFirebase() {
  if (!isFirebaseConnected()) {
    Serial.println("Firebase not connected - skipping sensor data upload");
    return;
  }
  
  String path = getDeviceBasePath() + "/sensors/logs";
  FirebaseJson json;
  
  unsigned long long timestamp = getCurrentTimestampMillis();
  
  json.set("temperature", temperature);
  json.set("humidity", humidity);
  json.set("distance", hangerDistance);
  json.set("rainDetected", rainDetected);
  json.set("userPresent", userPresent);
  json.set("timestamp", (long long)timestamp);      // UTC timestamp in milliseconds
  json.set("deviceUptime", String(millis()));       // Device uptime for debugging
  
  Serial.print("Sending sensor data to Firebase path: ");
  Serial.println(path);
  Serial.print("UTC Timestamp (ms): ");
  Serial.println((long long)timestamp);
  
  if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("✓ Sensor data sent successfully to Firebase");
  } else {
    Serial.print("❌ Failed to send sensor data to Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

void sendSystemStatusToFirebase() {
  if (!isFirebaseConnected()) {
    Serial.println("Firebase not connected - skipping system status upload");
    return;
  }
  
  // Don't send status updates when motor is running to avoid interfering with user commands
  if (motorRunning) {
    Serial.println("   Motor is running - skipping status update to avoid interference");
    return;
  }
  
  String path = getDeviceBasePath() + "/status";
  FirebaseJson json;
  
  unsigned long long timestamp = getCurrentTimestampMillis();
  
  json.set("state", getStateString());
  json.set("autoMode", sysConfig.autoMode);
  json.set("motorRunning", motorRunning);
  json.set("timestamp", (long long)timestamp);
  json.set("deviceUptime", String(millis()));
  
  Serial.print("Sending system status to Firebase path: ");
  Serial.println(path);
  Serial.print("   Status data - State: "); Serial.print(getStateString());
  Serial.print(", AutoMode: "); Serial.print(sysConfig.autoMode ? "true" : "false");
  Serial.print(", MotorRunning: "); Serial.println(motorRunning ? "true" : "false");
  Serial.print("   UTC Timestamp (ms): "); Serial.println((long long)timestamp);
  
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("✓ System status sent successfully to Firebase");
  } else {
    Serial.print("❌ Failed to send system status to Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

void registerDeviceToFirebase() {
  if (!isFirebaseConnected()) {
    Serial.println("Firebase not connected - skipping device registration");
    return;
  }
  
  String path = getDeviceBasePath() + "/info";
  FirebaseJson json;
  
  unsigned long long timestamp = getCurrentTimestampMillis();
  
  json.set("deviceName", deviceName);
  json.set("deviceLocation", deviceLocation);
  json.set("deviceID", deviceID);
  json.set("lastRegistered", (long long)timestamp);
  json.set("version", "1.0");
  json.set("deviceUptime", String(millis()));
  
  Serial.print("Registering device to Firebase path: ");
  Serial.println(path);
  Serial.print("Registration timestamp (ms): ");
  Serial.println((long long)timestamp);
  
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("✓ Device registered successfully to Firebase");
  } else {
    Serial.print("❌ Failed to register device to Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

void updateFirebaseConfiguration() {
  if (!isFirebaseConnected()) {
    Serial.println("Firebase not connected - skipping configuration update");
    return;
  }
  
  String path = getDeviceBasePath() + "/config";
  FirebaseJson json;
  
  json.set("autoMode", sysConfig.autoMode);
  json.set("motorTimeout", sysConfig.motorTimeout);
  json.set("sensorReadInterval", sysConfig.sensorReadInterval);
  json.set("distanceRetracted", sysConfig.distanceRetracted);
  json.set("distanceExtended", sysConfig.distanceExtended);
  json.set("deviceName", sysConfig.deviceName);
  json.set("deviceLocation", sysConfig.deviceLocation);
  
  // Add custom drying thresholds
  json.set("useCustomThresholds", sysConfig.useCustomThresholds);
  json.set("tempMinThreshold", sysConfig.tempMinThreshold);
  json.set("tempMaxThreshold", sysConfig.tempMaxThreshold);
  json.set("humidityMinThreshold", sysConfig.humidityMinThreshold);
  json.set("humidityMaxThreshold", sysConfig.humidityMaxThreshold);
  
  // Add PIR control settings
  json.set("enablePirControl", sysConfig.enablePirControl);
  json.set("retractOnUserPresent", sysConfig.retractOnUserPresent);
  json.set("pirRetractDelay", sysConfig.pirRetractDelay);
  
  // Add motor command requests
  json.set("requestedState", sysConfig.requestedState);
  
  unsigned long long timestamp = getCurrentTimestampMillis();
  
  json.set("lastUpdated", (long long)timestamp);
  json.set("deviceUptime", String(millis()));
  
  Serial.print("Updating Firebase configuration at path: ");
  Serial.println(path);
  Serial.print("Configuration update timestamp (ms): ");
  Serial.println((long long)timestamp);
  
  if (Firebase.RTDB.setJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("✓ Firebase configuration updated successfully");
  } else {
    Serial.print("❌ Failed to update Firebase configuration: ");
    Serial.println(fbdo.errorReason());
  }
}

void sendAlertToFirebase(const String& level, const String& message) {
  if (!isFirebaseConnected()) {
    Serial.println("Firebase not connected - skipping alert");
    return;
  }
  
  String path = getDeviceBasePath() + "/alerts";
  FirebaseJson json;
  
  unsigned long long timestamp = getCurrentTimestampMillis();
  
  json.set("level", level);
  json.set("message", message);
  json.set("timestamp", (long long)timestamp);
  json.set("deviceName", deviceName);
  json.set("deviceUptime", String(millis()));
  
  Serial.print("Sending alert to Firebase path: ");
  Serial.println(path);
  Serial.print("Alert level: ");
  Serial.print(level);
  Serial.print(", Message: ");
  Serial.println(message);
  Serial.print("Alert timestamp (ms): ");
  Serial.println((long long)timestamp);
  
  if (Firebase.RTDB.pushJSON(&fbdo, path.c_str(), &json)) {
    Serial.println("✓ Alert sent successfully to Firebase");
  } else {
    Serial.print("❌ Failed to send alert to Firebase: ");
    Serial.println(fbdo.errorReason());
  }
}

void listenForFirebaseCommands() {
  if (!isFirebaseConnected()) return;
  
  String path = getDeviceBasePath() + "/commands/action";
  
  if (Firebase.RTDB.getString(&fbdo, path.c_str())) {
    String command = fbdo.stringData();
    
    if (command.length() > 0) {
      Serial.print("Received Firebase command: ");
      Serial.println(command);
      
      if (command == "extend") {
        Serial.println("✓ Manual EXTEND command received");
        manualMode = true;
        sysConfig.autoMode = false; // Sync the config
        extendHanger();
        if (Firebase.RTDB.setString(&fbdo, path.c_str(), "")) {
          Serial.println("✓ Command cleared from Firebase");
        }
        Serial.println("✓ Device switched to MANUAL mode - will stay extended until next command");
      } else if (command == "retract") {
        Serial.println("✓ Manual RETRACT command received");
        manualMode = true;
        sysConfig.autoMode = false; // Sync the config
        retractHanger();
        if (Firebase.RTDB.setString(&fbdo, path.c_str(), "")) {
          Serial.println("✓ Command cleared from Firebase");
        }
        Serial.println("✓ Device switched to MANUAL mode - will stay retracted until next command");
      } else if (command == "auto") {
        Serial.println("✓ AUTO mode command received");
        manualMode = false;
        sysConfig.autoMode = true; // Sync the config
        if (Firebase.RTDB.setString(&fbdo, path.c_str(), "")) {
          Serial.println("✓ Command cleared from Firebase");
        }
        Serial.println("✓ Device switched to AUTO mode - will respond to weather/PIR conditions");
      } else if (command == "stop") {
        Serial.println("✓ Emergency STOP command received");
        stopMotor();
        if (Firebase.RTDB.setString(&fbdo, path.c_str(), "")) {
          Serial.println("✓ Command cleared from Firebase");
        }
      }
    }
  } else {
    // Only print error if it's not a "path not exist" error (which is normal)
    if (fbdo.httpCode() != 404) {
      Serial.print("Failed to read Firebase commands: ");
      Serial.println(fbdo.errorReason());
    }
  }
}

// New function to listen for configuration changes from Firebase
void listenForFirebaseConfigChanges() {
  if (!isFirebaseConnected()) return;
  
  String path = getDeviceBasePath() + "/config";
  
  Serial.println("Checking for Firebase configuration changes...");
  
  if (Firebase.RTDB.getJSON(&fbdo, path.c_str())) {
    FirebaseJson json = fbdo.jsonObject();
    FirebaseJsonData jsonData;
    bool configChanged = false;
    
    // Check and update autoMode
    if (json.get(jsonData, "autoMode") && jsonData.typeNum == FirebaseJson::JSON_BOOL) {
      bool newAutoMode = jsonData.boolValue;
      if (sysConfig.autoMode != newAutoMode) {
        sysConfig.autoMode = newAutoMode;
        manualMode = !newAutoMode;  // Update the global manualMode variable
        configChanged = true;
        Serial.print("✓ AutoMode updated to: "); Serial.println(newAutoMode ? "ENABLED" : "DISABLED");
      }
    }
    
    // Check and update motorTimeout
    if (json.get(jsonData, "motorTimeout") && (jsonData.typeNum == FirebaseJson::JSON_INT || jsonData.typeNum == FirebaseJson::JSON_DOUBLE)) {
      unsigned long newTimeout = (unsigned long)jsonData.intValue;
      if (sysConfig.motorTimeout != newTimeout && newTimeout > 0 && newTimeout <= 300000) { // Max 5 minutes
        sysConfig.motorTimeout = newTimeout;
        configChanged = true;
        Serial.print("✓ Motor timeout updated to: "); Serial.print(newTimeout); Serial.println(" ms");
      }
    }
    
    // Check and update sensorReadInterval
    if (json.get(jsonData, "sensorReadInterval") && (jsonData.typeNum == FirebaseJson::JSON_INT || jsonData.typeNum == FirebaseJson::JSON_DOUBLE)) {
      unsigned long newInterval = (unsigned long)jsonData.intValue;
      if (sysConfig.sensorReadInterval != newInterval && newInterval >= 1000 && newInterval <= 60000) { // 1s to 60s
        sysConfig.sensorReadInterval = newInterval;
        configChanged = true;
        Serial.print("✓ Sensor read interval updated to: "); Serial.print(newInterval); Serial.println(" ms");
      }
    }
    
    // Check and update distance settings
    if (json.get(jsonData, "distanceExtended") && (jsonData.typeNum == FirebaseJson::JSON_DOUBLE || jsonData.typeNum == FirebaseJson::JSON_INT)) {
      float newDistanceExtended = jsonData.floatValue;
      if (abs(sysConfig.distanceExtended - newDistanceExtended) > 0.5 && newDistanceExtended >= 10 && newDistanceExtended <= 200) { // 10cm to 200cm
        sysConfig.distanceExtended = newDistanceExtended;
        configChanged = true;
        Serial.print("✓ Extended distance updated to: "); Serial.print(newDistanceExtended); Serial.println(" cm");
      }
    }
    
    if (json.get(jsonData, "distanceRetracted") && (jsonData.typeNum == FirebaseJson::JSON_DOUBLE || jsonData.typeNum == FirebaseJson::JSON_INT)) {
      float newDistanceRetracted = jsonData.floatValue;
      if (abs(sysConfig.distanceRetracted - newDistanceRetracted) > 0.5 && newDistanceRetracted >= 5 && newDistanceRetracted <= 100) { // 5cm to 100cm
        sysConfig.distanceRetracted = newDistanceRetracted;
        configChanged = true;
        Serial.print("✓ Retracted distance updated to: "); Serial.print(newDistanceRetracted); Serial.println(" cm");
      }
    }
    
    if (json.get(jsonData, "distanceTolerance") && (jsonData.typeNum == FirebaseJson::JSON_DOUBLE || jsonData.typeNum == FirebaseJson::JSON_INT)) {
      float newDistanceTolerance = jsonData.floatValue;
      if (abs(sysConfig.distanceTolerance - newDistanceTolerance) > 0.5 && newDistanceTolerance >= 1 && newDistanceTolerance <= 20) { // 1cm to 20cm
        sysConfig.distanceTolerance = newDistanceTolerance;
        configChanged = true;
        Serial.print("✓ Distance tolerance updated to: "); Serial.print(newDistanceTolerance); Serial.println(" cm");
      }
    }
    
    // Check and update deviceName
    if (json.get(jsonData, "deviceName") && jsonData.typeNum == FirebaseJson::JSON_STRING) {
      String newDeviceName = jsonData.stringValue;
      if (strcmp(sysConfig.deviceName, newDeviceName.c_str()) != 0 && newDeviceName.length() < 32) {
        strcpy(sysConfig.deviceName, newDeviceName.c_str());
        deviceName = newDeviceName.c_str(); // Update global variable
        configChanged = true;
        Serial.print("✓ Device name updated to: "); Serial.println(newDeviceName);
      }
    }
    
    // Check and update deviceLocation
    if (json.get(jsonData, "deviceLocation") && jsonData.typeNum == FirebaseJson::JSON_STRING) {
      String newLocation = jsonData.stringValue;
      if (strcmp(sysConfig.deviceLocation, newLocation.c_str()) != 0 && newLocation.length() < 32) {
        strcpy(sysConfig.deviceLocation, newLocation.c_str());
        deviceLocation = newLocation.c_str(); // Update global variable
        configChanged = true;
        Serial.print("✓ Device location updated to: "); Serial.println(newLocation);
      }
    }
    
    // Check and update custom drying thresholds
    if (json.get(jsonData, "useCustomThresholds") && jsonData.typeNum == FirebaseJson::JSON_BOOL) {
      bool newUseCustom = jsonData.boolValue;
      if (sysConfig.useCustomThresholds != newUseCustom) {
        sysConfig.useCustomThresholds = newUseCustom;
        configChanged = true;
        Serial.print("✓ Use custom thresholds updated to: "); Serial.println(newUseCustom ? "ENABLED" : "DISABLED");
      }
    }
    
    if (json.get(jsonData, "tempMinThreshold") && (jsonData.typeNum == FirebaseJson::JSON_DOUBLE || jsonData.typeNum == FirebaseJson::JSON_INT)) {
      float newTempMin = jsonData.floatValue;
      if (abs(sysConfig.tempMinThreshold - newTempMin) > 0.1 && newTempMin >= 0 && newTempMin <= 50) {
        sysConfig.tempMinThreshold = newTempMin;
        configChanged = true;
        Serial.print("✓ Minimum temperature threshold updated to: "); Serial.print(newTempMin); Serial.println("°C");
      }
    }
    
    if (json.get(jsonData, "tempMaxThreshold") && (jsonData.typeNum == FirebaseJson::JSON_DOUBLE || jsonData.typeNum == FirebaseJson::JSON_INT)) {
      float newTempMax = jsonData.floatValue;
      if (abs(sysConfig.tempMaxThreshold - newTempMax) > 0.1 && newTempMax >= 0 && newTempMax <= 60) {
        sysConfig.tempMaxThreshold = newTempMax;
        configChanged = true;
        Serial.print("✓ Maximum temperature threshold updated to: "); Serial.print(newTempMax); Serial.println("°C");
      }
    }
    
    if (json.get(jsonData, "humidityMinThreshold") && (jsonData.typeNum == FirebaseJson::JSON_DOUBLE || jsonData.typeNum == FirebaseJson::JSON_INT)) {
      float newHumidityMin = jsonData.floatValue;
      if (abs(sysConfig.humidityMinThreshold - newHumidityMin) > 0.1 && newHumidityMin >= 0 && newHumidityMin <= 100) {
        sysConfig.humidityMinThreshold = newHumidityMin;
        configChanged = true;
        Serial.print("✓ Minimum humidity threshold updated to: "); Serial.print(newHumidityMin); Serial.println("%");
      }
    }
    
    if (json.get(jsonData, "humidityMaxThreshold") && (jsonData.typeNum == FirebaseJson::JSON_DOUBLE || jsonData.typeNum == FirebaseJson::JSON_INT)) {
      float newHumidityMax = jsonData.floatValue;
      if (abs(sysConfig.humidityMaxThreshold - newHumidityMax) > 0.1 && newHumidityMax >= 0 && newHumidityMax <= 100) {
        sysConfig.humidityMaxThreshold = newHumidityMax;
        configChanged = true;
        Serial.print("✓ Maximum humidity threshold updated to: "); Serial.print(newHumidityMax); Serial.println("%");
      }
    }
    
    // Check and update PIR control settings
    if (json.get(jsonData, "enablePirControl") && jsonData.typeNum == FirebaseJson::JSON_BOOL) {
      bool newEnablePir = jsonData.boolValue;
      if (sysConfig.enablePirControl != newEnablePir) {
        sysConfig.enablePirControl = newEnablePir;
        configChanged = true;
        Serial.print("✓ PIR control updated to: "); Serial.println(newEnablePir ? "ENABLED" : "DISABLED");
      }
    }
    
    if (json.get(jsonData, "retractOnUserPresent") && jsonData.typeNum == FirebaseJson::JSON_BOOL) {
      bool newRetractMode = jsonData.boolValue;
      if (sysConfig.retractOnUserPresent != newRetractMode) {
        sysConfig.retractOnUserPresent = newRetractMode;
        configChanged = true;
        Serial.print("✓ PIR mode updated to: "); 
        Serial.println(newRetractMode ? "Retract when user present" : "Extend when user present");
      }
    }
    
    if (json.get(jsonData, "pirRetractDelay") && (jsonData.typeNum == FirebaseJson::JSON_INT || jsonData.typeNum == FirebaseJson::JSON_DOUBLE)) {
      unsigned long newPirDelay = (unsigned long)jsonData.intValue;
      if (sysConfig.pirRetractDelay != newPirDelay && newPirDelay >= 0 && newPirDelay <= 60000) { // Max 60 seconds
        sysConfig.pirRetractDelay = newPirDelay;
        configChanged = true;
        Serial.print("✓ PIR action delay updated to: "); Serial.print(newPirDelay); Serial.println(" ms");
      }
    }
    
    // Check for state commands (extend/retract requests from app)
    if (json.get(jsonData, "requestedState") && jsonData.typeNum == FirebaseJson::JSON_STRING) {
      String requestedState = jsonData.stringValue;
      Serial.print("   🔍 DEBUG: requestedState read from Firebase: '"); Serial.print(requestedState); Serial.println("'");
      Serial.print("   🔍 DEBUG: current local requestedState: '"); Serial.print(sysConfig.requestedState); Serial.println("'");
      Serial.print("   🔍 DEBUG: manualCommandInProgress: "); Serial.println(manualCommandInProgress ? "true" : "false");
      
      // Only process new requests when no manual command is in progress
      if (!manualCommandInProgress && requestedState.length() > 0 && requestedState != "none") {
        Serial.print("✅ NEW State change requested from app: "); Serial.println(requestedState);
        
        if (requestedState == "extend" || requestedState == "EXTENDED" || requestedState == "extended") {
          Serial.println("   🎯 >>> App requested EXTEND - starting manual command");
          Serial.print("   🔧 Current state before command: "); Serial.println(getStateString());
          Serial.println("   🔧 Setting manualMode = true, autoMode = false");
          Serial.println("   🛑 PAUSING all automatic operations until EXTENDED state achieved");
          
          // Set the local requestedState to match Firebase (DON'T clear it yet)
          strcpy(sysConfig.requestedState, "extend");
          manualMode = true;
          sysConfig.autoMode = false;
          manualCommandInProgress = true;
          targetState = EXTENDED;
          
          Serial.println("   🚀 Calling extendHanger() function...");
          extendHanger();
          configChanged = true;
          Serial.println("   ✅ extendHanger() function initiated");
          Serial.println("   📝 requestedState will stay 'extend' until command completes");
          
        } else if (requestedState == "retract" || requestedState == "RETRACTED" || requestedState == "retracted") {
          Serial.println("   🎯 >>> App requested RETRACT - starting manual command");
          Serial.print("   🔧 Current state before command: "); Serial.println(getStateString());
          Serial.println("   🔧 Setting manualMode = true, autoMode = false");
          Serial.println("   🛑 PAUSING all automatic operations until RETRACTED state achieved");
          
          // Set the local requestedState to match Firebase (DON'T clear it yet)
          strcpy(sysConfig.requestedState, "retract");
          manualMode = true;
          sysConfig.autoMode = false;
          manualCommandInProgress = true;
          targetState = RETRACTED;
          
          Serial.println("   🚀 Calling retractHanger() function...");
          retractHanger();
          configChanged = true;
          Serial.println("   ✅ retractHanger() function initiated");
          Serial.println("   📝 requestedState will stay 'retract' until command completes");
          
        } else {
          Serial.print("   ❌ Unknown requestedState value: '"); Serial.print(requestedState); Serial.println("'");
          Serial.println("   📝 Valid values are: extend, extended, EXTENDED, retract, retracted, RETRACTED");
        }
        
      } else if (manualCommandInProgress) {
        Serial.println("   ℹ️ Manual command already in progress - ignoring new requestedState");
        Serial.print("   Current target: "); 
        switch(targetState) {
            case EXTENDED: Serial.print("EXTENDED"); break;
            case RETRACTED: Serial.print("RETRACTED"); break;
            default: Serial.print("UNKNOWN"); break;
        }
        Serial.print(", requestedState: "); Serial.println(sysConfig.requestedState);
        
      } else {
        Serial.print("   ℹ️ requestedState is empty or 'none': '"); Serial.print(requestedState); Serial.println("'");
      }
    }
    
    // Save configuration if any changes were made (excluding requestedState during manual commands)
    if (configChanged) {
      saveConfig();
      Serial.println("✓ Configuration changes saved to device");
      
      // Send confirmation back to Firebase
      if (!manualCommandInProgress) {
        sendAlertToFirebase("INFO", "Configuration successfully updated on device: " + String(deviceName));
      } else {
        Serial.println("   📝 Manual command in progress - requestedState will be preserved until completion");
      }
    }
    
  } else {
    // Only print error if it's not a "path not exist" error (which is normal for first time)
    if (fbdo.httpCode() != 404) {
      Serial.print("Failed to read Firebase configuration: ");
      Serial.println(fbdo.errorReason());
    }
  }
}

void clearFirebaseRequestedState() {
    if (!isFirebaseConnected()) {
        Serial.println("Firebase not connected - cannot clear requestedState");
        return;
    }
    
    String configPath = getDeviceBasePath() + "/config/requestedState";
    Serial.print("Clearing Firebase requestedState at path: "); Serial.println(configPath);
    
    if (Firebase.RTDB.setString(&fbdo, configPath.c_str(), "none")) {
        Serial.println("   ✅ Firebase requestedState cleared successfully");
    } else {
        Serial.print("   ❌ Failed to clear Firebase requestedState: ");
        Serial.println(fbdo.errorReason());
    }
}

#endif // FIREBASE_H