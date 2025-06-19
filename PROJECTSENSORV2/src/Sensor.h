#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include "Config.h"

// Function declarations
void initializeSensors();
void readDHT11();
void readPIR();
void readRainSensor();
float readUltrasonicDistance();
void readSensors();
bool isGoodDryingConditions();
bool isHangerRetracted();
bool isHangerExtended();

void initializeSensors() {
  Serial.println("Initializing sensors...");
  
  // Initialize pin modes first
  pinMode(PIR_PIN, INPUT);
  pinMode(RAIN_PIN, INPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Give DHT sensor time to initialize
  delay(2000);
  
  // Initialize default values
  temperature = 25.0;
  humidity = 50.0;
  hangerDistance = DISTANCE_RETRACTED;
  
  // Initial sensor reading (with error handling)
  Serial.println("Taking initial sensor readings...");
  readSensors();
  
  Serial.println("Sensors initialized successfully");
}

float readUltrasonicDistance() {
  // Send trigger pulse
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  
  // Read echo pulse duration
  long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000); // 30ms timeout
  
  // Calculate distance in cm (speed of sound = 34320 cm/s)
  float distance = (duration * 0.034) / 2; //in cm
  
  // Debug output
  Serial.print("   Ultrasonic - Duration: "); Serial.print(duration);
  Serial.print(" μs, Distance: "); Serial.print(distance); Serial.println(" cm");
  
  // Return valid distance or previous value if invalid
  if (distance > 0 && distance < 400) {
    Serial.println("   Ultrasonic: OK");
    return distance;
  } else {
    Serial.println("   Ultrasonic: FAILED - Invalid reading!");
    return hangerDistance; // Return previous value if reading failed
  }
}

// DHT11 Sensor
void readDHT11() {
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();
  
  // Check if readings are valid
  if (!isnan(newTemp) && !isnan(newHum)) {
    temperature = newTemp;
    humidity = newHum;
    Serial.println("   DHT11: OK");
  } else {
    Serial.println("   DHT11: FAILED - NaN values detected!");
    Serial.print("   Raw temp: "); Serial.println(newTemp);
    Serial.print("   Raw humidity: "); Serial.println(newHum);
  }
}


unsigned long motionDetectedTime = 0;
const unsigned long MOTION_TIMEOUT = 10000; // 10 seconds in milliseconds
void readPIR() {
  int pirValue = digitalRead(PIR_PIN);
  
  // If motion is detected, update the timer
  if (pirValue == HIGH) {
    motionDetectedTime = millis();
    userPresent = true;
    Serial.print("   PIR: MOTION DETECTED");
  } else {
    // Check if 10 seconds have passed since last motion
    if (millis() - motionDetectedTime > MOTION_TIMEOUT) {
      userPresent = false;
      Serial.print("   PIR: No motion (timeout)");
    } else {
      // Still within timeout period, keep userPresent true
      userPresent = true;
      Serial.print("   PIR: Have Motion (still active)");
    }
  }
  
  Serial.println(userPresent ? " - USER PRESENT" : " - USER NOT PRESENT");
}

// Rain Sensor
  void readRainSensor() {
    int rainValue = analogRead(RAIN_PIN);
    rainDetected = (rainValue > 0);   // rain detected when analog value > 0
    Serial.print("   Rain sensor: "); 
    Serial.print(rainDetected ? "RAIN DETECTED" : "No rain");
    Serial.print(" (raw: "); Serial.print(rainValue); Serial.println(")");
  }

void readSensors() {
  readDHT11();
  readPIR();
  readRainSensor();
  hangerDistance = readUltrasonicDistance();
}

bool isGoodDryingConditions() {
  // Always use the configured thresholds (which have proper defaults)
  float tempMin = sysConfig.tempMinThreshold;
  float tempMax = sysConfig.tempMaxThreshold;
  float humidityMin = sysConfig.humidityMinThreshold;
  float humidityMax = sysConfig.humidityMaxThreshold;
  
  Serial.println("   Using configured drying thresholds:");
  Serial.print("   Temperature range: "); Serial.print(tempMin); Serial.print("°C - "); Serial.print(tempMax); Serial.println("°C");
  Serial.print("   Humidity range: "); Serial.print(humidityMin); Serial.print("% - "); Serial.print(humidityMax); Serial.println("%");
  
  // Define optimal drying conditions using configured thresholds
  bool tempOK = (temperature > tempMin && temperature < tempMax);
  bool humidityOK = (humidity > humidityMin && humidity < humidityMax);
  bool noRain = !rainDetected;
  
  Serial.print("   Current temp: "); Serial.print(temperature); Serial.print("°C ("); Serial.print(tempOK ? "OK" : "BAD"); Serial.println(")");
  Serial.print("   Current humidity: "); Serial.print(humidity); Serial.print("% ("); Serial.print(humidityOK ? "OK" : "BAD"); Serial.println(")");
  Serial.print("   Rain status: "); Serial.println(noRain ? "No rain (OK)" : "Rain detected (BAD)");
  
  bool result = (tempOK && humidityOK && noRain);
  Serial.print("   Overall drying conditions: "); Serial.println(result ? "GOOD" : "POOR");
  
  return result;
}

bool isHangerRetracted() {
  return (hangerDistance <= (DISTANCE_RETRACTED + DISTANCE_TOLERANCE) && 
          hangerDistance >= (DISTANCE_RETRACTED - DISTANCE_TOLERANCE));
}

bool isHangerExtended() {
  return (hangerDistance >= (DISTANCE_EXTENDED - DISTANCE_TOLERANCE) && 
          hangerDistance <= (DISTANCE_EXTENDED + DISTANCE_TOLERANCE));
}

#endif