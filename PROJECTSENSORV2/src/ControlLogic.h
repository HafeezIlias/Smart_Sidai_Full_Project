#ifndef CONTROL_LOGIC_H
#define CONTROL_LOGIC_H

#include "Config.h"
#include "Sensor.h"
#include "Firebase.h"

// Main control functions
void executeControlLogic();
void extendHanger();
void retractHanger();
void handleEmergencyRetraction();
void handlePirControl();
void executePirAction();
void initializeMotorControl();
void stopMotor();
void checkLimitDistances();

// Extend hanger function
void extendHanger() {
  if (currentState == EXTENDED) {
    Serial.println("Hanger already extended");
    return;
  }
  
  Serial.println("Starting hanger extension...");
  digitalWrite(RELAY_PIN, HIGH); // Turn relay on
  
  unsigned long startTime = millis();
  
  // Keep relay high until extension distance is reached
  while (true) {
    // Update distance reading
    hangerDistance = readUltrasonicDistance();
    
    // Check if target reached (use configurable values)
    if (hangerDistance >= sysConfig.distanceExtended - sysConfig.distanceTolerance && 
        hangerDistance <= sysConfig.distanceExtended + sysConfig.distanceTolerance) {
      Serial.printf("Target reached! Distance: %.1f cm (Target: %.1f ± %.1f cm)\n", 
                    hangerDistance, sysConfig.distanceExtended, sysConfig.distanceTolerance);
      break; // Target reached
    }
    
    // Safety timeout check
    if (millis() - startTime > sysConfig.motorTimeout) {
      Serial.println("ERROR: Motor timeout during extension");
      break;
    }
    
    delay(100); // Small delay for sensor reading
  }
  
  digitalWrite(RELAY_PIN, LOW); // Turn relay off
  currentState = EXTENDED;
  Serial.print("Hanger extended successfully. Final distance: ");
  Serial.print(hangerDistance);
  Serial.println(" cm");
}

// Retract hanger function
void retractHanger() {
  if (currentState == RETRACTED) {
    Serial.println("Hanger already retracted");
    return;
  }
  
  Serial.println("Starting hanger retraction...");
  digitalWrite(RELAY_PIN, HIGH); // Turn relay on
  
  unsigned long startTime = millis();
  
  // Keep relay high until retraction distance is reached
  while (true) {
    // Update distance reading
    hangerDistance = readUltrasonicDistance();
    
    // Check if target reached (use configurable values)
    if (hangerDistance >= sysConfig.distanceRetracted - sysConfig.distanceTolerance && 
        hangerDistance <= sysConfig.distanceRetracted + sysConfig.distanceTolerance) {
      Serial.printf("Target reached! Distance: %.1f cm (Target: %.1f ± %.1f cm)\n", 
                    hangerDistance, sysConfig.distanceRetracted, sysConfig.distanceTolerance);
      break; // Target reached
    }
    
    // Safety timeout check
    if (millis() - startTime > sysConfig.motorTimeout) {
      Serial.println("ERROR: Motor timeout during retraction");
      break;
    }
    
    delay(100); // Small delay for sensor reading
  }
  
  digitalWrite(RELAY_PIN, LOW); // Turn relay off
  currentState = RETRACTED;
  Serial.print("Hanger retracted successfully. Final distance: ");
  Serial.print(hangerDistance);
  Serial.println(" cm");
}

// PIR-based control (configurable retract/extend behavior)
void handlePirControl() {
  static bool lastUserPresence = false;
  static unsigned long lastPirAction = 0;
  static unsigned long pirDelayStartTime = 0;
  static bool pirDelayActive = false;
  
  // Check if PIR control is enabled
  if (!sysConfig.enablePirControl) {
    return;
  }
  
  // Use configurable PIR delay (minimum 5 seconds to prevent spam)
  unsigned long pirDelay = max(5000UL, sysConfig.pirRetractDelay);
  
  // Only act if user presence status has changed
  if (userPresent != lastUserPresence) {
    Serial.println("PIR: User presence changed");
    Serial.print("From: "); Serial.print(lastUserPresence ? "PRESENT" : "ABSENT");
    Serial.print(" to: "); Serial.println(userPresent ? "PRESENT" : "ABSENT");
    Serial.printf("PIR Mode: %s\n", sysConfig.retractOnUserPresent ? "Privacy Mode (retract on user)" : "Access Mode (extend on user)");
    
    // Handle PIR delay for actions
    if (pirDelay > 0 && userPresent) {
      pirDelayActive = true;
      pirDelayStartTime = millis();
      Serial.printf("PIR: Starting %lu ms delay before action\n", pirDelay);
      lastUserPresence = userPresent;
      return;
    }
    
    // Execute immediate action (no delay or user left)
    executePirAction();
    lastUserPresence = userPresent;
    lastPirAction = millis();
  }
  
  // Handle delayed PIR action
  if (pirDelayActive && (millis() - pirDelayStartTime >= pirDelay)) {
    Serial.println("PIR: Delay completed, executing action");
    executePirAction();
    pirDelayActive = false;
    lastPirAction = millis();
  }
}

// Execute PIR action based on configuration
void executePirAction() {
  if (sysConfig.retractOnUserPresent) {
    // Privacy Mode: Retract when user present, extend when absent
    if (userPresent && currentState == EXTENDED) {
      Serial.println("PIR Privacy Mode: User present - Retracting hanger");
      retractHanger();
      sendAlertToFirebase("INFO", "User detected - Hanger retracted for privacy");
    } 
    else if (!userPresent && currentState == RETRACTED && isGoodDryingConditions()) {
      Serial.println("PIR Privacy Mode: User absent + good conditions - Extending hanger");
      extendHanger();
      sendAlertToFirebase("INFO", "User left + good conditions - Hanger extended");
    }
  } else {
    // Access Mode: Extend when user present, retract when absent
    if (userPresent && currentState == RETRACTED && isGoodDryingConditions()) {
      Serial.println("PIR Access Mode: User present + good conditions - Extending hanger");
      extendHanger();
      sendAlertToFirebase("INFO", "User present - Hanger extended for easy access");
    }
    else if (!userPresent && currentState == EXTENDED) {
      Serial.println("PIR Access Mode: User absent - Retracting hanger");
      retractHanger();
      sendAlertToFirebase("INFO", "User left - Hanger retracted");
    }
  }
}

// Emergency retraction (always active, even in manual mode)
void handleEmergencyRetraction() {
  Serial.println("EMERGENCY: Rain detected! Immediate retraction initiated");
  retractHanger();
  sendAlertToFirebase("EMERGENCY", "Rain detected! Clothes automatically retracted.");
}

// Main control logic
void executeControlLogic() {
  Serial.println("=== CONTROL LOGIC ===");
  Serial.print("Manual mode: "); Serial.println(manualMode ? "ON" : "OFF");
  Serial.print("PIR control: "); Serial.println(sysConfig.enablePirControl ? "ON" : "OFF");
  Serial.print("Rain detected: "); Serial.println(rainDetected ? "YES" : "NO");
  Serial.print("User present: "); Serial.println(userPresent ? "YES" : "NO");
  Serial.print("Current state: "); Serial.println(currentState == EXTENDED ? "EXTENDED" : "RETRACTED");
  
  // PRIORITY 1: Emergency rain protection (ALWAYS ACTIVE)
  if (rainDetected && currentState == EXTENDED) {
    Serial.println(">>> RAIN EMERGENCY - Retracting hanger");
    handleEmergencyRetraction();
    return;
  }
  
  // MANUAL MODE: Stop all automatic operations (except rain emergency)
  if (manualMode) {
    Serial.println(">>> MANUAL MODE - Automatic control disabled");
    Serial.println(">>> Only rain emergency protection is active");
    return;
  }
  
  // AUTO MODE: Automatic control active
  Serial.println(">>> AUTO MODE - Automatic control active");
  
  // PRIORITY 2: PIR-based control (if enabled)
  if (sysConfig.enablePirControl) {
    Serial.println(">>> Checking PIR control");
    handlePirControl();
    // PIR may take action, but continue to check weather conditions too
  }
  
  // PRIORITY 3: Auto extend when conditions are good and hanger is retracted
  if (currentState == RETRACTED && isGoodDryingConditions()) {
    // If PIR is enabled, only extend when user is absent
    if (sysConfig.enablePirControl && userPresent) {
      Serial.println(">>> Good conditions but user present - PIR prevents extension");
    } else {
      Serial.println(">>> Good conditions detected - Extending hanger");
      extendHanger();
      sendAlertToFirebase("INFO", "Good drying conditions - Hanger extended automatically");
    }
    return;
  }
  
  // PRIORITY 4: Auto retract when conditions are poor and hanger is extended
  if (currentState == EXTENDED && !isGoodDryingConditions()) {
    Serial.println(">>> Poor conditions detected - Retracting hanger");
    retractHanger();
    sendAlertToFirebase("WARNING", "Poor drying conditions - Hanger retracted automatically");
    return;
  }
  
  Serial.println(">>> No action needed");
}

// Initialize motor control
void initializeMotorControl() {
  Serial.println("Initializing Motor Control...");
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Ensure motor is stopped
  
  // Check initial position using configurable distances
  hangerDistance = readUltrasonicDistance();
  Serial.printf("Initial distance reading: %.1f cm\n", hangerDistance);
  Serial.printf("Extended target: %.1f ± %.1f cm\n", sysConfig.distanceExtended, sysConfig.distanceTolerance);
  Serial.printf("Retracted target: %.1f ± %.1f cm\n", sysConfig.distanceRetracted, sysConfig.distanceTolerance);
  
  if (hangerDistance >= sysConfig.distanceExtended - sysConfig.distanceTolerance && 
      hangerDistance <= sysConfig.distanceExtended + sysConfig.distanceTolerance) {
    currentState = EXTENDED;
    Serial.println("Initial position: EXTENDED");
  } else if (hangerDistance >= sysConfig.distanceRetracted - sysConfig.distanceTolerance && 
             hangerDistance <= sysConfig.distanceRetracted + sysConfig.distanceTolerance) {
    currentState = RETRACTED;
    Serial.println("Initial position: RETRACTED");
  } else {
    currentState = RETRACTED; // Default to retracted for safety
    Serial.println("Initial position: UNKNOWN - defaulting to RETRACTED for safety");
  }
  
  Serial.println("Motor Control initialized successfully");
}

// Stop motor function
void stopMotor() {
  digitalWrite(RELAY_PIN, LOW);
  motorRunning = false;
  Serial.println("Motor stopped");
}

// Check limit distances function
void checkLimitDistances() {
  // Safety timeout check
  if (motorRunning && (millis() - motorStartTime) > sysConfig.motorTimeout) {
    Serial.println("ERROR: Motor timeout - stopping motor");
    stopMotor();
    currentState = HANGER_ERROR;
  }
}

#endif