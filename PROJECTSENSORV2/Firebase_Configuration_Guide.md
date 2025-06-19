# Smart Clothes Hanger - Firebase Remote Configuration Guide

## Overview
The Smart Clothes Hanger system now supports remote configuration through Firebase Realtime Database. Users can modify device settings remotely, and the device will automatically apply these changes.

## Firebase Database Structure

### Device Configuration Path
```
/devices/{deviceID}/config/
```

### Configurable Settings

#### 1. Auto Mode Control
```json
{
  "autoMode": true
}
```
- **Type**: Boolean
- **Description**: Enables/disables automatic operation based on weather conditions
- **Default**: true
- **Effect**: When set to false, device operates in manual mode only

#### 2. Motor Timeout
```json
{
  "motorTimeout": 30000
}
```
- **Type**: Number (milliseconds)
- **Range**: 1000 - 300000 (1 second to 5 minutes)
- **Description**: Maximum time motor can run before timeout
- **Default**: 30000 (30 seconds)

#### 3. Sensor Read Interval
```json
{
  "sensorReadInterval": 5000
}
```
- **Type**: Number (milliseconds)
- **Range**: 1000 - 60000 (1 second to 1 minute)
- **Description**: How often sensors are read and main loop executes
- **Default**: 2000 (2 seconds)

#### 4. Device Information
```json
{
  "deviceName": "Kitchen_Hanger",
  "deviceLocation": "Kitchen Balcony"
}
```
- **Type**: String
- **Max Length**: 31 characters each
- **Description**: Human-readable device identification
- **Default**: "Smart_Hanger_1", "Backyard"

#### 5. Custom Drying Thresholds
```json
{
  "useCustomThresholds": true,
  "tempMinThreshold": 20.0,
  "tempMaxThreshold": 35.0,
  "humidityMinThreshold": 25.0,
  "humidityMaxThreshold": 65.0
}
```
- **useCustomThresholds**: Boolean - Enable custom thresholds
- **tempMinThreshold**: Number (°C) - Minimum temperature for good drying (0-50°C)
- **tempMaxThreshold**: Number (°C) - Maximum temperature for good drying (0-60°C)
- **humidityMinThreshold**: Number (%) - Minimum humidity for good drying (0-100%)
- **humidityMaxThreshold**: Number (%) - Maximum humidity for good drying (0-100%)

> **Note**: Humidity thresholds now work as a range. Very low humidity (<25%) might be too harsh for delicate fabrics and can cause static buildup, while high humidity (>70%) prevents effective drying.

#### 6. PIR-Based Motion Control
```json
{
  "enablePirControl": true,
  "retractOnUserPresent": true,
  "pirRetractDelay": 5000
}
```
- **enablePirControl**: Boolean - Enable PIR motion sensor based control
- **retractOnUserPresent**: Boolean - true = retract when user detected, false = extend when user detected
- **pirRetractDelay**: Number (milliseconds) - Delay before PIR action (0-60000ms)

> **PIR Control Modes**:
> - **Privacy Mode** (`retractOnUserPresent: true`): Retract when user present for privacy/space
> - **Access Mode** (`retractOnUserPresent: false`): Extend when user present for easy access

#### 7. State Control Commands
```json
{
  "requestedState": "extend"
}
```
- **requestedState**: String - Request immediate state change
- **Valid values**: `"extend"`, `"retract"`, `"none"`
- **Behavior**: Device executes command immediately and switches to manual mode
- **Auto-clear**: Value automatically resets to `"none"` after execution

> **Usage**: Set `requestedState` to `"extend"` or `"retract"` in your app to immediately control the hanger position. The device will execute the command and clear the request.

### Complete Configuration Example
```json
{
  "devices": {
    "SS01": {
      "config": {
        "autoMode": true,
        "motorTimeout": 45000,
        "sensorReadInterval": 3000,
        "deviceName": "Bedroom_Hanger",
        "deviceLocation": "Master Bedroom Balcony",
        "useCustomThresholds": true,
        "tempMinThreshold": 22.0,
        "tempMaxThreshold": 38.0,
        "humidityMinThreshold": 25.0,
        "humidityMaxThreshold": 60.0,
        "enablePirControl": true,
        "retractOnUserPresent": true,
        "pirRetractDelay": 5000,
        "requestedState": "none",
        "lastUpdated": "1234567890"
      }
    }
  }
}
```

## Manual Commands

### Command Path
```
/devices/{deviceID}/commands/action
```

### Available Commands
- `"extend"` - Manually extend the hanger (switches to manual mode)
- `"retract"` - Manually retract the hanger (switches to manual mode)
- `"auto"` - Switch back to automatic mode
- `"stop"` - Emergency stop motor

### Command Example
```json
{
  "devices": {
    "SS01": {
      "commands": {
        "action": "extend"
      }
    }
  }
}
```

## How It Works

1. **Configuration Monitoring**: Every loop cycle, the device checks for configuration changes in Firebase
2. **Validation**: All changes are validated for proper ranges and types
3. **Local Application**: Valid changes are immediately applied to the device
4. **Persistence**: Changes are saved to device flash memory for persistence across reboots
5. **Confirmation**: Device sends an alert to Firebase confirming successful configuration update

## System Behavior

### Drying Conditions Logic
The device determines good drying conditions based on:
- **Temperature**: Between `tempMinThreshold` and `tempMaxThreshold`
- **Humidity**: Between `humidityMinThreshold` and `humidityMaxThreshold` (range-based)
- **Rain**: No rain detected

**Why Humidity Range Matters:**
- **Too Low Humidity** (<25%): Can cause static electricity, harsh on delicate fabrics, may indicate extreme heat
- **Too High Humidity** (>70%): Prevents effective drying, clothes may not dry properly
- **Optimal Range** (25-70%): Provides effective drying while being gentle on fabrics

### PIR Motion Control Behavior
The PIR (motion sensor) control adds intelligent user-presence awareness:

#### Privacy Mode (`retractOnUserPresent: true`)
- **User Detected** → Retract hanger (gives privacy/space)
- **User Leaves** → Extend if drying conditions are good
- **Use Case**: Bedroom, bathroom, or private balcony areas

#### Access Mode (`retractOnUserPresent: false`)  
- **User Detected** → Extend hanger (easy access to clothes)
- **User Leaves** → Retract hanger (out of the way)
- **Use Case**: Shared spaces, laundry rooms, or high-traffic areas

#### Priority System
1. **Emergency Rain** (highest priority) - Always retracts regardless of PIR
2. **PIR Control** (if enabled) - Overrides weather-based decisions
3. **Weather Conditions** (if PIR disabled) - Auto extend/retract based on temperature/humidity
4. **Manual Commands** - Always available via Firebase

#### PIR Delay Setting
- **Purpose**: Prevents rapid switching when user briefly enters/exits area
- **Range**: 0-60 seconds
- **Recommendation**: 5-10 seconds for most applications

### Auto Mode vs Manual Mode
- **Auto Mode** (`autoMode: true`): Device automatically extends/retracts based on conditions
- **Manual Mode** (`autoMode: false`): Device only responds to manual commands

#### Safety Override in Manual Mode
🚨 **Important**: Even in manual mode, **rain emergency detection remains active** for safety. If rain is detected while the hanger is extended, it will automatically retract to protect clothes from water damage, regardless of the current mode setting.

**Manual Mode Behavior**:
- ✅ **Rain Emergency**: Active (safety override)
- ❌ **PIR Control**: Disabled  
- ❌ **Weather Logic**: Disabled (temperature/humidity based actions)
- ❌ **Auto Extension/Retraction**: Disabled
- ✅ **Manual Commands**: Always available

### Safety Features
- All configuration changes are validated before application
- Motor timeout prevents mechanical damage
- Device continues operating with previous settings if Firebase is unavailable
- Invalid configurations are rejected and logged

## Monitoring

### Device Status
Monitor device status at:
```
/devices/{deviceID}/status/
```

### Alerts
System alerts are logged at:
```
/devices/{deviceID}/alerts/
```

### Sensor Data
Real-time sensor data at:
```
/devices/{deviceID}/sensors/logs/
```

## Usage Tips

1. **Test Changes Gradually**: Start with small adjustments to verify behavior
2. **Monitor Alerts**: Check the alerts path for configuration update confirmations
3. **Use Reasonable Values**: Extreme values may be rejected by validation
4. **Backup Settings**: Keep a record of working configurations
5. **Consider Environment**: Adjust thresholds based on your local climate

## Troubleshooting

### Configuration Not Applied
- Check Firebase connection status in device logs
- Verify JSON syntax is correct
- Ensure values are within acceptable ranges
- Check device alerts for error messages

### Device Not Responding
- Verify device is online and connected to WiFi
- Check Firebase authentication credentials
- Monitor serial output for error messages
- Try manual commands first

### Unexpected Behavior
- Review current configuration in Firebase
- Check sensor readings for accuracy
- Verify auto mode setting
- Monitor system alerts for clues

## Configuration Parameters

### Motor Control Commands
- **`requestedState`** (string): Immediate motor command requests
  - **Values**: `"extend"`, `"retract"`, `"none"`
  - **Default**: `"none"`
  - **Usage**: Set to `"extend"` or `"retract"` to immediately execute motor command
  - **Behavior**: Device automatically clears back to `"none"` after executing command and switches to manual mode
  - **Example**: `"requestedState": "extend"`

### Core System Settings

## Usage Examples

### Immediate Motor Commands (App → Device)
To execute an immediate motor command, set the `requestedState` in the config:

```json
{
  "devices": {
    "SS01": {
      "config": {
        "requestedState": "extend"
      }
    }
  }
}
```

**What happens:**
1. App sets `requestedState: "extend"` in `/devices/SS01/config/`
2. Device detects the change and immediately executes `extendHanger()`
3. Device switches to manual mode (`autoMode: false`)
4. Device automatically clears `requestedState` back to `"none"`
5. Device sends confirmation alert to `/devices/SS01/alerts/`

### Configuration Updates