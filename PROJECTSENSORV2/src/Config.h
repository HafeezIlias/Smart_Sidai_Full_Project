#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Preferences.h>
#include <ArduinoJson.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Wire.h>

// PIN DEFINITIONS
#define PIR_PIN 19
#define RAIN_PIN 34
#define DHT_PIN 4
#define RELAY_PIN 2
#define ULTRASONIC_TRIG_PIN 25
#define ULTRASONIC_ECHO_PIN 26
#define CONFIG_PIN 0

// OLED Display
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

// DHT Sensor
#define DHT_TYPE DHT11

// SYSTEM TIMING CONFIGURATION
#define MOTOR_TIMEOUT 30000              // 30 seconds max motor run time
#define SENSOR_READ_INTERVAL 2000        // 2 seconds

// ULTRASONIC SENSOR CONFIGURATION
#define DISTANCE_RETRACTED 20            // Distance in cm when fully retracted
#define DISTANCE_EXTENDED 50             // Distance in cm when fully extended
#define DISTANCE_TOLERANCE 5            // Tolerance in cm for position detection

// PREFERENCES CONFIGURATION
#define CONFIG_VERSION 1
#define CONFIG_NAMESPACE "smart_hanger"

// ENUMERATIONS
enum HangerState {
    EXTENDED,
    RETRACTED,
    HANGER_ERROR
};

// CONFIGURATION STRUCTURE
struct SystemConfig {
    int version;
    char deviceName[32];
    char deviceLocation[32];
    char ssid[32];
    char password[64];
    char firebaseApiKey[128];
    char firebaseDatabaseUrl[128];
    char firebaseUserEmail[64];
    char firebaseUserPassword[64];
    unsigned long motorTimeout;
    unsigned long sensorReadInterval;
    float distanceRetracted;
    float distanceExtended;
    float distanceTolerance;
    bool autoMode;
    // Configurable drying conditions
    float tempMinThreshold;          // Minimum temperature for good drying
    float tempMaxThreshold;          // Maximum temperature for good drying
    float humidityMinThreshold;      // Minimum humidity for good drying
    float humidityMaxThreshold;      // Maximum humidity for good drying
    bool useCustomThresholds;        // Whether to use custom thresholds or defaults
    // PIR-based control settings
    bool enablePirControl;           // Enable PIR-based automatic control
    bool retractOnUserPresent;       // Retract when user is detected (true), or extend when user present (false)
    unsigned long pirRetractDelay;   // Delay in ms before retracting when user detected
    // Motor command requests
    char requestedState[16];         // "extend", "retract", or "none" for immediate motor commands
};

// GLOBAL VARIABLES DECLARATION
extern SystemConfig sysConfig;

// Network Configuration
extern const char* ssid;
extern const char* password;

// Firebase Configuration
extern const char* FIREBASE_API_KEY;
extern const char* FIREBASE_DATABASE_URL;
extern const char* FIREBASE_USER_EMAIL;
extern const char* FIREBASE_USER_PASSWORD;

// Device Configuration
extern String deviceID;
extern const char* deviceLocation;
extern const char* deviceName;

// System State Variables
extern HangerState currentState;
extern bool motorRunning;
extern bool rainDetected;
extern bool userPresent;
extern bool manualMode;
extern bool manualCommandInProgress;
extern HangerState targetState;
extern bool needToClearFirebaseState;
extern float temperature;
extern float humidity;
extern float hangerDistance;
extern unsigned long motorStartTime;

// Hardware Objects
extern Adafruit_SSD1306 display;
extern DHT dht;

// Preferences Object
extern Preferences preferences;

// FUNCTION DECLARATIONS
void initializeConfig();
void loadConfig();
void saveConfig();
void resetConfig();
bool isConfigValid();
String getConfigJson();
void updateConfigFromJson(const String& json);
String getStateString();
void checkManualCommandCompletion();

#ifdef SMART_HANGER_CONFIG_IMPLEMENTATION

SystemConfig sysConfig;

// Network credentials
const char* ssid = "SS2DM02";
const char* password = "passwordwifi";

// Firebase Configuration
const char* FIREBASE_API_KEY = "AIzaSyCP64DJPw4kyxU57hDFUZ7c4IgNNha6ljM";
const char* FIREBASE_DATABASE_URL = "https://smartsidai-default-rtdb.asia-southeast1.firebasedatabase.app/";
const char* FIREBASE_USER_EMAIL = "";
const char* FIREBASE_USER_PASSWORD = "";

// Device Configuration
String deviceID = "SS01";
const char* deviceLocation = "Backyard";
const char* deviceName = "Smart_Hanger_1";

// System State Variables
HangerState currentState = RETRACTED;
bool motorRunning = false;
bool rainDetected = false;
bool userPresent = false;
bool manualMode = false;
bool manualCommandInProgress = false;
HangerState targetState = RETRACTED;
bool needToClearFirebaseState = false;
float temperature = 0.0;
float humidity = 0.0;
float hangerDistance = 0.0;
unsigned long motorStartTime = 0;

// Hardware Objects
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT dht(DHT_PIN, DHT_TYPE);

// Preferences Object
Preferences preferences;

// FUNCTION IMPLEMENTATIONS

void initializeConfig() {
    preferences.begin(CONFIG_NAMESPACE, false);
    loadConfig();
    
    if (!isConfigValid()) {
        Serial.println("Invalid configuration detected, resetting to defaults");
        resetConfig();
    }
}

void loadConfig() {
    // Load individual configuration values from Preferences
    sysConfig.version = preferences.getInt("version", 0);
    
    Serial.println("Loading configuration from Preferences...");
    Serial.print("Config version found: ");
    Serial.println(sysConfig.version);
    
    if (sysConfig.version == CONFIG_VERSION) {
        Serial.println("Valid config version found, loading settings...");
        
        // Load all configuration values
        preferences.getString("deviceName", sysConfig.deviceName, sizeof(sysConfig.deviceName));
        preferences.getString("deviceLocation", sysConfig.deviceLocation, sizeof(sysConfig.deviceLocation));
        preferences.getString("ssid", sysConfig.ssid, sizeof(sysConfig.ssid));
        preferences.getString("password", sysConfig.password, sizeof(sysConfig.password));
        preferences.getString("fbApiKey", sysConfig.firebaseApiKey, sizeof(sysConfig.firebaseApiKey));
        preferences.getString("fbDbUrl", sysConfig.firebaseDatabaseUrl, sizeof(sysConfig.firebaseDatabaseUrl));
        preferences.getString("fbUserEmail", sysConfig.firebaseUserEmail, sizeof(sysConfig.firebaseUserEmail));
        preferences.getString("fbUserPass", sysConfig.firebaseUserPassword, sizeof(sysConfig.firebaseUserPassword));
        
        sysConfig.motorTimeout = preferences.getULong("motorTimeout", MOTOR_TIMEOUT);
        sysConfig.sensorReadInterval = preferences.getULong("sensorInterval", SENSOR_READ_INTERVAL);
        sysConfig.distanceRetracted = preferences.getFloat("distRetract", DISTANCE_RETRACTED);
        sysConfig.distanceExtended = preferences.getFloat("distExtend", DISTANCE_EXTENDED);
        sysConfig.distanceTolerance = preferences.getFloat("distTol", DISTANCE_TOLERANCE);
        sysConfig.autoMode = preferences.getBool("autoMode", true);
        
        // Load configurable drying conditions
        sysConfig.tempMinThreshold = preferences.getFloat("tempMin", 25.0);
        sysConfig.tempMaxThreshold = preferences.getFloat("tempMax", 40.0);
        sysConfig.humidityMinThreshold = preferences.getFloat("humidMin", 30.0);
        sysConfig.humidityMaxThreshold = preferences.getFloat("humidMax", 70.0);
        sysConfig.useCustomThresholds = preferences.getBool("useCustom", true);
        
        // Load PIR-based control settings
        sysConfig.enablePirControl = preferences.getBool("pirEnable", false);
        sysConfig.retractOnUserPresent = preferences.getBool("pirRetract", true);
        sysConfig.pirRetractDelay = preferences.getULong("pirDelay", 5000);
        
        // Load motor command requests
        preferences.getString("requestedState", sysConfig.requestedState, sizeof(sysConfig.requestedState));
        if (strlen(sysConfig.requestedState) == 0) {
            strcpy(sysConfig.requestedState, "none");
        }
        
        // Debug output for WiFi credentials
        Serial.print("Loaded WiFi SSID: '");
        Serial.print(sysConfig.ssid);
        Serial.println("'");
        Serial.print("Loaded WiFi password length: ");
        Serial.println(strlen(sysConfig.password));

        if (sysConfig.useCustomThresholds) {
            Serial.println("   - Using CUSTOM drying thresholds:");
            Serial.print("     Temperature: "); Serial.print(sysConfig.tempMinThreshold); 
            Serial.print("°C - "); Serial.print(sysConfig.tempMaxThreshold); Serial.println("°C");
            Serial.print("     Humidity: "); Serial.print(sysConfig.humidityMinThreshold);
            Serial.print("% - "); Serial.print(sysConfig.humidityMaxThreshold); Serial.println("%");
        } else {
            Serial.println("   - Using DEFAULT drying thresholds");
        }
    } else {
        Serial.println("No valid config found or version mismatch, will use defaults");
    }
}

void saveConfig() {
    // Save individual configuration values to Preferences
    preferences.putInt("version", sysConfig.version);
    preferences.putString("deviceName", sysConfig.deviceName);
    preferences.putString("deviceLocation", sysConfig.deviceLocation);
    preferences.putString("ssid", sysConfig.ssid);
    preferences.putString("password", sysConfig.password);
    preferences.putString("fbApiKey", sysConfig.firebaseApiKey);
    preferences.putString("fbDbUrl", sysConfig.firebaseDatabaseUrl);
    preferences.putString("fbUserEmail", sysConfig.firebaseUserEmail);
    preferences.putString("fbUserPass", sysConfig.firebaseUserPassword);
    
    preferences.putULong("motorTimeout", sysConfig.motorTimeout);
    preferences.putULong("sensorInterval", sysConfig.sensorReadInterval);
    preferences.putFloat("distRetract", sysConfig.distanceRetracted);
    preferences.putFloat("distExtend", sysConfig.distanceExtended);
    preferences.putFloat("distTol", sysConfig.distanceTolerance);
    preferences.putBool("autoMode", sysConfig.autoMode);
    
    // Save configurable drying conditions
    preferences.putFloat("tempMin", sysConfig.tempMinThreshold);
    preferences.putFloat("tempMax", sysConfig.tempMaxThreshold);
    preferences.putFloat("humidMin", sysConfig.humidityMinThreshold);
    preferences.putFloat("humidMax", sysConfig.humidityMaxThreshold);
    preferences.putBool("useCustom", sysConfig.useCustomThresholds);
    
    // Save PIR-based control settings
    preferences.putBool("pirEnable", sysConfig.enablePirControl);
    preferences.putBool("pirRetract", sysConfig.retractOnUserPresent);
    preferences.putULong("pirDelay", sysConfig.pirRetractDelay);
    
    // Save motor command requests
    preferences.putString("requestedState", sysConfig.requestedState);
    
    Serial.println("Configuration saved to Preferences");
}

void resetConfig() {
    sysConfig.version = CONFIG_VERSION;
    strcpy(sysConfig.deviceName, "Smart_Hanger_1");
    strcpy(sysConfig.deviceLocation, "Backyard");
    strcpy(sysConfig.ssid, "SS2DM02");
    strcpy(sysConfig.password, "passwordwifi");
    strcpy(sysConfig.firebaseApiKey, "AIzaSyCP64DJPw4kyxU57hDFUZ7c4IgNNha6ljM");
    strcpy(sysConfig.firebaseDatabaseUrl, "https://smartsidai-default-rtdb.asia-southeast1.firebasedatabase.app/");
    strcpy(sysConfig.firebaseUserEmail, "");
    strcpy(sysConfig.firebaseUserPassword, "");
    sysConfig.motorTimeout = MOTOR_TIMEOUT;
    sysConfig.sensorReadInterval = SENSOR_READ_INTERVAL;
    sysConfig.distanceRetracted = DISTANCE_RETRACTED;
    sysConfig.distanceExtended = DISTANCE_EXTENDED;
    sysConfig.distanceTolerance = DISTANCE_TOLERANCE;
    sysConfig.autoMode = true;
    // Set default drying condition thresholds
    sysConfig.tempMinThreshold = 25.0;
    sysConfig.tempMaxThreshold = 40.0;
    sysConfig.humidityMinThreshold = 30.0;
    sysConfig.humidityMaxThreshold = 70.0;
    sysConfig.useCustomThresholds = false;
    
    // Set default PIR-based control settings
    sysConfig.enablePirControl = false;
    
    // Set default motor command requests
    strcpy(sysConfig.requestedState, "none");
    
    saveConfig();
    Serial.println("Configuration reset to defaults");
}

bool isConfigValid() {
    return sysConfig.version == CONFIG_VERSION;
}

String getConfigJson() {
    StaticJsonDocument<1024> doc;
    
    doc["version"] = sysConfig.version;
    doc["deviceName"] = sysConfig.deviceName;
    doc["deviceLocation"] = sysConfig.deviceLocation;
    doc["ssid"] = sysConfig.ssid;
    doc["password"] = "********"; // Don't expose actual password
    doc["firebaseApiKey"] = sysConfig.firebaseApiKey;
    doc["firebaseDatabaseUrl"] = sysConfig.firebaseDatabaseUrl;
    doc["firebaseUserEmail"] = sysConfig.firebaseUserEmail;
    doc["firebaseUserPassword"] = "********"; // Don't expose actual password
    doc["motorTimeout"] = sysConfig.motorTimeout;
    doc["sensorReadInterval"] = sysConfig.sensorReadInterval;
    doc["distanceRetracted"] = sysConfig.distanceRetracted;
    doc["distanceExtended"] = sysConfig.distanceExtended;
    doc["distanceTolerance"] = sysConfig.distanceTolerance;
    doc["autoMode"] = sysConfig.autoMode;
    
    // Add configurable drying conditions
    doc["tempMinThreshold"] = sysConfig.tempMinThreshold;
    doc["tempMaxThreshold"] = sysConfig.tempMaxThreshold;
    doc["humidityMinThreshold"] = sysConfig.humidityMinThreshold;
    doc["humidityMaxThreshold"] = sysConfig.humidityMaxThreshold;
    doc["useCustomThresholds"] = sysConfig.useCustomThresholds;
    
    // Add PIR-based control settings
    doc["enablePirControl"] = sysConfig.enablePirControl;
    doc["retractOnUserPresent"] = sysConfig.retractOnUserPresent;
    doc["pirRetractDelay"] = sysConfig.pirRetractDelay;
    
    // Add motor command requests
    doc["requestedState"] = sysConfig.requestedState;
    
    String output;
    serializeJson(doc, output);
    return output;
}

void updateConfigFromJson(const String& json) {
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, json);
    
    if (error) {
        Serial.print("Failed to parse JSON: ");
        Serial.println(error.c_str());
        return;
    }
    
    // Update configuration values with proper bounds checking
    if (doc.containsKey("deviceName")) {
        strncpy(sysConfig.deviceName, doc["deviceName"], sizeof(sysConfig.deviceName) - 1);
        sysConfig.deviceName[sizeof(sysConfig.deviceName) - 1] = '\0';
    }
    
    if (doc.containsKey("deviceLocation")) {
        strncpy(sysConfig.deviceLocation, doc["deviceLocation"], sizeof(sysConfig.deviceLocation) - 1);
        sysConfig.deviceLocation[sizeof(sysConfig.deviceLocation) - 1] = '\0';
    }
    
    if (doc.containsKey("ssid")) {
        strncpy(sysConfig.ssid, doc["ssid"], sizeof(sysConfig.ssid) - 1);
        sysConfig.ssid[sizeof(sysConfig.ssid) - 1] = '\0';
    }
    
    if (doc.containsKey("password")) {
        strncpy(sysConfig.password, doc["password"], sizeof(sysConfig.password) - 1);
        sysConfig.password[sizeof(sysConfig.password) - 1] = '\0';
    }
    
    if (doc.containsKey("firebaseApiKey")) {
        strncpy(sysConfig.firebaseApiKey, doc["firebaseApiKey"], sizeof(sysConfig.firebaseApiKey) - 1);
        sysConfig.firebaseApiKey[sizeof(sysConfig.firebaseApiKey) - 1] = '\0';
    }
    
    if (doc.containsKey("firebaseDatabaseUrl")) {
        strncpy(sysConfig.firebaseDatabaseUrl, doc["firebaseDatabaseUrl"], sizeof(sysConfig.firebaseDatabaseUrl) - 1);
        sysConfig.firebaseDatabaseUrl[sizeof(sysConfig.firebaseDatabaseUrl) - 1] = '\0';
    }
    
    if (doc.containsKey("firebaseUserEmail")) {
        strncpy(sysConfig.firebaseUserEmail, doc["firebaseUserEmail"], sizeof(sysConfig.firebaseUserEmail) - 1);
        sysConfig.firebaseUserEmail[sizeof(sysConfig.firebaseUserEmail) - 1] = '\0';
    }
    
    if (doc.containsKey("firebaseUserPassword")) {
        strncpy(sysConfig.firebaseUserPassword, doc["firebaseUserPassword"], sizeof(sysConfig.firebaseUserPassword) - 1);
        sysConfig.firebaseUserPassword[sizeof(sysConfig.firebaseUserPassword) - 1] = '\0';
    }
    
    if (doc.containsKey("motorTimeout")) sysConfig.motorTimeout = doc["motorTimeout"];
    if (doc.containsKey("sensorReadInterval")) sysConfig.sensorReadInterval = doc["sensorReadInterval"];
    if (doc.containsKey("distanceRetracted")) sysConfig.distanceRetracted = doc["distanceRetracted"];
    if (doc.containsKey("distanceExtended")) sysConfig.distanceExtended = doc["distanceExtended"];
    if (doc.containsKey("distanceTolerance")) sysConfig.distanceTolerance = doc["distanceTolerance"];
    if (doc.containsKey("autoMode")) sysConfig.autoMode = doc["autoMode"];
    
    // Update configurable drying conditions
    if (doc.containsKey("tempMinThreshold")) sysConfig.tempMinThreshold = doc["tempMinThreshold"];
    if (doc.containsKey("tempMaxThreshold")) sysConfig.tempMaxThreshold = doc["tempMaxThreshold"];
    if (doc.containsKey("humidityMinThreshold")) sysConfig.humidityMinThreshold = doc["humidityMinThreshold"];
    if (doc.containsKey("humidityMaxThreshold")) sysConfig.humidityMaxThreshold = doc["humidityMaxThreshold"];
    if (doc.containsKey("useCustomThresholds")) sysConfig.useCustomThresholds = doc["useCustomThresholds"];
    
    // Update PIR-based control settings
    if (doc.containsKey("enablePirControl")) sysConfig.enablePirControl = doc["enablePirControl"];
    if (doc.containsKey("retractOnUserPresent")) sysConfig.retractOnUserPresent = doc["retractOnUserPresent"];
    if (doc.containsKey("pirRetractDelay")) sysConfig.pirRetractDelay = doc["pirRetractDelay"];
    
    // Update motor command requests
    if (doc.containsKey("requestedState")) {
        strncpy(sysConfig.requestedState, doc["requestedState"], sizeof(sysConfig.requestedState) - 1);
        sysConfig.requestedState[sizeof(sysConfig.requestedState) - 1] = '\0';
    }
    
    // Save the updated configuration
    saveConfig();
}

String getStateString() {
    switch (currentState) {
        case EXTENDED: return "Extended";
        case RETRACTED: return "Retracted";
        case HANGER_ERROR: return "Error";
        default: return "Unknown";
    }
}

void checkManualCommandCompletion() {
    if (manualCommandInProgress) {
        // Check if target state has been achieved
        if (currentState == targetState) {
            Serial.println("   ✅ MANUAL COMMAND COMPLETED!");
            Serial.print("   Target state "); Serial.print(getStateString()); Serial.println(" achieved");
            Serial.print("   Clearing requestedState from '"); Serial.print(sysConfig.requestedState); Serial.println("' to 'none'");
            Serial.println("   🔄 RESUMING normal automatic operations");
            manualCommandInProgress = false;
            
            // Reset requestedState to "none" locally
            strcpy(sysConfig.requestedState, "none");
            saveConfig(); // Save the cleared state
            
            // Signal that Firebase needs to be cleared
            needToClearFirebaseState = true;
            
            Serial.println("   📝 Manual command flags cleared - system ready for automatic mode");
            Serial.println("   📡 Firebase requestedState will be cleared on next Firebase sync");
        } else {
            Serial.print("   ⏳ Manual command still in progress: Target=");
            Serial.print(targetState == EXTENDED ? "EXTENDED" : "RETRACTED");
            Serial.print(", Current="); Serial.println(getStateString());
        }
    }
}

#endif

#endif