#ifndef SYSTEM_ERROR_H
#define SYSTEM_ERROR_H

#include "Config.h"
#include "Firebase.h"
#include "Display.h"
#include "Sensor.h"

void handleSystemError() {
  static unsigned long errorStartTime = 0;
  
  if (errorStartTime == 0) {
    errorStartTime = millis();
    Serial.println("System entered ERROR state");
    
    // Send error alert to Firebase
    sendAlertToFirebase("ERROR", "System error detected: Motor timeout or hardware failure. Manual intervention may be required.");
  }
  
  // Show error on display
  showErrorScreen("Motor timeout or\nhardware failure");
  
  // Ensure motor is stopped
  digitalWrite(RELAY_PIN, LOW);
  motorRunning = false;
  
  // Auto-recovery attempt after 30 seconds
  if (millis() - errorStartTime > 30000) {
    Serial.println("Attempting error recovery...");
    
    // Reset error state
    currentState = RETRACTED;
    errorStartTime = 0;
    
    // Try to determine actual position
    checkLimitDistances();
    
    // Send recovery notification to Firebase
    sendAlertToFirebase("INFO", "System error recovery completed. System is now operational.");
    
    Serial.println("Error recovery completed");
  }
}

#endif // SYSTEM_ERROR_H