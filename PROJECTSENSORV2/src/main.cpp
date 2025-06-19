#define SMART_HANGER_CONFIG_IMPLEMENTATION
#include <Arduino.h>
#include "Config.h"
#include "ConfigPortal.h"
#include "Firebase.h"
#include "Sensor.h"
#include "WiFiManager.h"
#include "Display.h"
#include "ControlLogic.h"
#include "SystemError.h"

// ====================================================================================
// FUNCTION DECLARATIONS
// ====================================================================================
void printSystemHeader();
void initializeBasicModules();
void printConfigSummary();
bool initializeHardware();
void showWiFiFailureScreen();
bool initializeConnectivity();
bool initializeCloudServices();
void printStartupSummary(bool wifiConnected, bool firebaseConnected);
void handlePeriodicTasks();
void handleSystemTasks();
void handleFirebaseOperations();
void handleSafetyChecks();
void handleControlLogic();
void handleDisplayUpdate();

// ====================================================================================
// HELPER FUNCTIONS - INITIALIZATION
// ====================================================================================

void printSystemHeader() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("========================================");
  Serial.println("   Smart Clothes Hanger System v1.0");
  Serial.println("========================================");
  Serial.println("Starting initialization...");
}

void printConfigSummary() {
  Serial.println("Configuration Summary:");
  Serial.printf("  Device: %s at %s\n", sysConfig.deviceName, sysConfig.deviceLocation);
  Serial.printf("  Auto Mode: %s\n", sysConfig.autoMode ? "ENABLED" : "DISABLED");
  Serial.printf("  PIR Control: %s\n", sysConfig.enablePirControl ? "ENABLED" : "DISABLED");
  
  if (sysConfig.useCustomThresholds) {
    Serial.printf("  Custom Thresholds: Temp %.1f-%.1f°C, Humidity %.1f-%.1f%%\n",
                  sysConfig.tempMinThreshold, sysConfig.tempMaxThreshold,
                  sysConfig.humidityMinThreshold, sysConfig.humidityMaxThreshold);
  }
}

void initializeBasicModules() {
  Serial.println("→ Initializing basic modules...");
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  
  initializeConfig();
  manualMode = !sysConfig.autoMode;
  
  printConfigSummary();
  Serial.println("✓ Basic modules ready");
}

bool initializeHardware() {
  Serial.println("→ Initializing hardware...");
  
  // Check for config mode first
  if (shouldEnterConfigMode()) {
    Serial.println("Entering configuration mode...");
    enterConfigMode();
    return false; // This will loop forever until restart
  }
  
  // Initialize hardware components
  initializeDisplay();
  initializeSensors();
  initializeMotorControl();
  updateDisplay();
  
  Serial.println("✓ Hardware initialized");
  return true;
}

void showWiFiFailureScreen() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("WiFi Failed!");
  display.setCursor(0, 12);
  display.println("Hold BOOT + Reset");
  display.setCursor(0, 24);
  display.println("for Config Mode");
  display.display();
}

bool initializeConnectivity() {
  Serial.println("→ Initializing connectivity...");
  
  // Initialize WiFi
  initializeWiFi();
  bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  
  if (!wifiConnected) {
    Serial.println("❌ WiFi failed - continuing in offline mode");
    showWiFiFailureScreen();
    return false;
  }
  
  Serial.println("✓ WiFi connected");
  
  // Initialize NTP
  bool ntpReady = initializeNTP();
  if (ntpReady) {
    Serial.println("✓ NTP synchronized");
  } else {
    Serial.println("⚠ NTP failed - using device uptime");
  }
  
  checkConfigButton();
  return true;
}

bool initializeCloudServices() {
  Serial.println("→ Initializing cloud services...");
  
  bool firebaseReady = initializeFirebase();
  
  if (firebaseReady) {
    registerDeviceToFirebase();
    updateFirebaseConfiguration();
    sendAlertToFirebase("INFO", "System started successfully");
    Serial.println("✓ Firebase connected");
    return true;
  } else {
    Serial.println("❌ Firebase failed - offline mode");
    return false;
  }
}

void printStartupSummary(bool wifiConnected, bool firebaseConnected) {
  Serial.println("========================================");
  Serial.println("🚀 SYSTEM READY!");
  
  if (firebaseConnected) {
    Serial.println("📡 Status: Online Mode (Full connectivity)");
  } else if (wifiConnected) {
    Serial.println("📶 Status: WiFi Only (Limited connectivity)");
  } else {
    Serial.println("🔌 Status: Offline Mode (Local operation only)");
  }
  
  Serial.println("========================================");
}

// ====================================================================================
// HELPER FUNCTIONS - MAIN LOOP
// ====================================================================================

void handlePeriodicTasks() {
  static unsigned long lastNtpSync = 0;
  const unsigned long NTP_SYNC_INTERVAL = 6 * 60 * 60 * 1000; // 6 hours
  
  // Periodic NTP resync
  if (WiFi.status() == WL_CONNECTED && (millis() - lastNtpSync) > NTP_SYNC_INTERVAL) {
    if (initializeNTP()) {
      lastNtpSync = millis();
    }
  }
}

void handleSystemTasks() {
  checkManualCommandCompletion();
  checkConfigButton();
  
  // Read sensors and show basic status
  readSensors();
  
  if (manualCommandInProgress) {
    Serial.printf("🛑 Manual command in progress: %s → %s\n", 
                  getStateString().c_str(), 
                  (targetState == EXTENDED) ? "EXTENDED" : "RETRACTED");
  }
}

void handleFirebaseOperations() {
  if (!isFirebaseConnected()) {
    return; // Skip if Firebase not available
  }
  
  listenForFirebaseConfigChanges();
  delay(2000); // Allow commands to process
  
  if (!manualCommandInProgress) {
    sendSensorDataToFirebase();
  }
  
  sendSystemStatusToFirebase();
  
  if (needToClearFirebaseState) {
    clearFirebaseRequestedState();
    needToClearFirebaseState = false;
  }
}

void handleSafetyChecks() {
  // Critical safety checks - always run
  checkLimitDistances();
  
  if (currentState == HANGER_ERROR) {
    Serial.println("⚠️ ERROR STATE DETECTED!");
    handleSystemError();
  }
}

void handleControlLogic() {
  if (manualCommandInProgress) {
    return; // Skip automatic control during manual operations
  }
  
  executeControlLogic();
}

void handleDisplayUpdate() {
  updateDisplay();
}

// ====================================================================================
// MAIN FUNCTIONS
// ====================================================================================

void setup() {
  printSystemHeader();
  
  // Initialize in logical order
  initializeBasicModules();
  
  if (!initializeHardware()) {
    return; // Config mode entered, will restart
  }
  
  bool wifiConnected = initializeConnectivity();
  bool firebaseConnected = wifiConnected ? initializeCloudServices() : false;
  
  printStartupSummary(wifiConnected, firebaseConnected);
}

void loop() {
  // Handle different aspects of the system
  handlePeriodicTasks();
  handleSystemTasks();
  handleFirebaseOperations();
  handleSafetyChecks();
  handleControlLogic();
  handleDisplayUpdate();
  
  // Wait before next cycle
  delay(sysConfig.sensorReadInterval);
}
