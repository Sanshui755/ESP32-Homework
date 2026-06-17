# SinricPro ESP-IDF Examples Guide

## Completed Examples

### ✅ 1. Switch
- **Location:** `examples/switch/`
- **Features:** Basic on/off control, physical button
- **Complexity:** ⭐ Simple

### ✅ 2. TV
- **Location:** `examples/tv/`
- **Features:** Power, volume, mute, media control, input selection, channel control
- **Complexity:** ⭐⭐⭐ Complex

### ✅ 3. Speaker
- **Location:** `examples/speaker/`
- **Features:** Power, volume, mute, media control, equalizer, mode control
- **Complexity:** ⭐⭐⭐ Complex

### ✅ 4. Motion Sensor
- **Location:** `examples/motion_sensor/`
- **Features:** Motion detection with PIR sensor
- **Complexity:** ⭐ Simple

### ✅ 5. Smart Light
- **Location:** `examples/light/`
- **Features:** RGB color, brightness, color temperature
- **Complexity:** ⭐⭐⭐ Complex

### ✅ 6. Thermostat
- **Location:** `examples/thermostat/`
- **Features:** HVAC control, temperature modes (AUTO/COOL/HEAT/ECO/OFF), target temperature
- **Complexity:** ⭐⭐⭐ Complex

### ✅ 7. Lock
- **Location:** `examples/lock/`
- **Features:** Servo-based lock/unlock control, manual button
- **Complexity:** ⭐⭐ Medium

### ✅ 8. Fan
- **Location:** `examples/fan/`
- **Features:** Variable speed control 0-100%, PWM motor control
- **Complexity:** ⭐⭐ Medium

### ✅ 9. DimSwitch
- **Location:** `examples/dimswitch/`
- **Features:** Dimmable light control, brightness 0-100%
- **Complexity:** ⭐⭐ Medium

### ✅ 10. Contact Sensor
- **Location:** `examples/contact_sensor/`
- **Features:** Door/window sensor with magnetic reed switch
- **Complexity:** ⭐ Simple

### ✅ 11. Temperature Sensor
- **Location:** `examples/temperature_sensor/`
- **Features:** Temperature and humidity monitoring (DHT22/DHT11)
- **Complexity:** ⭐⭐ Medium

### ✅ 12. Garage Door
- **Location:** `examples/garage_door/`
- **Features:** Garage door control with position detection
- **Complexity:** ⭐⭐⭐ Complex

### ✅ 13. Blinds
- **Location:** `examples/blinds/`
- **Features:** Motorized blinds/curtains, position control 0-100%
- **Complexity:** ⭐⭐⭐ Complex

## Remaining Examples (To Be Implemented)

The following examples still need implementation:

### Sensors (Read-only devices)
1. **AirQualitySensor** - `examples/air_quality_sensor/`
   - PM2.5 sensor (e.g., PMS5003)
   - Reports PM1, PM2.5, PM10
   - Directory structure created, needs implementation

2. **PowerSensor** - `examples/power_sensor/`
   - Current/voltage sensor (e.g., INA219, ACS712)
   - Reports voltage, current, power
   - Directory structure created, needs implementation

### Actuators/Controllers
3. **WindowAC** - `examples/windowac/`
   - Window air conditioner
   - Temperature + fan speed control
   - Directory structure created, needs implementation

## Quick Example Template

Each example follows this structure:

```
examples/<device_name>/
├── CMakeLists.txt
├── README.md
└── main/
    ├── CMakeLists.txt
    ├── idf_component.yml
    └── <device_name>_example.c
```

### CMakeLists.txt Template
```cmake
cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(<device_name>_example)
```

### main/CMakeLists.txt Template
```cmake
idf_component_register(SRCS "<device_name>_example.c"
                    INCLUDE_DIRS ".")
```

### main/idf_component.yml Template
```yaml
dependencies:
  sinricpro/esp-idf:
    version: "*"
    override_path: "../../"
```

### main/<device_name>_example.c Template Structure

```c
#include "sinricpro.h"
#include "sinricpro_<device_name>.h"

/* WiFi + SinricPro credentials */
#define WIFI_SSID      "YOUR_SSID"
#define WIFI_PASS      "YOUR_PASS"
#define DEVICE_ID      "YOUR_DEVICE_ID"
#define APP_KEY        "YOUR_APP_KEY"
#define APP_SECRET     "YOUR_SECRET"

/* WiFi initialization code (same for all examples) */
/* GPIO initialization code */
/* SinricPro callbacks */
/* Main function with device creation */
```

## Device-Specific Callback Examples

### Sensor Devices (Event-based, no callbacks)

```c
/* Send sensor event */
sinricpro_<sensor>_send_event(device, value, SINRICPRO_CAUSE_PHYSICAL_INTERACTION);
```

### Actuator Devices (Callback-based)

```c
/* Register callback */
sinricpro_<device>_on_<action>(device, callback_function, NULL);

/* Callback implementation */
static bool callback_function(const char *device_id, <type> *value, void *user_data) {
    /* Handle action */
    return true;
}
```

## Voice Command Examples

### All Devices
- "Alexa, turn on/off <device_name>"

### Brightness/Level Control
- "Alexa, set <device> to 50%"
- "Alexa, dim the <device>"

### Temperature Control
- "Alexa, set thermostat to 72 degrees"
- "Alexa, make it warmer"

### Color Control
- "Alexa, set light to red"
- "Alexa, make light warm white"

## Building Examples

```bash
cd examples/<device_name>
idf.py set-target esp32  # or esp32s2, esp32s3, esp32c3, esp32c6
idf.py build
idf.py flash monitor
```

## Common GPIO Configurations

### Sensors
- **PIR Motion:** GPIO4 (digital input)
- **Reed Switch:** GPIO4 (digital input with pull-up)
- **DHT22:** GPIO4 (1-wire protocol)
- **Current Sensor:** ADC pin (analog input)

### Actuators
- **Relay:** GPIO5 (digital output)
- **Servo:** GPIO18 (PWM)
- **LED/PWM:** GPIO25-27 (PWM channels)

## Rate Limiting

- **Sensor events:** Max 1 event per 60 seconds
- **State changes:** Max 1 event per second
- Handle `SINRICPRO_ERR_RATE_LIMITED` error code

## Next Steps for Remaining Examples

1. Copy template files to each example directory
2. Update device-specific includes and functions
3. Implement GPIO initialization for hardware
4. Implement callbacks or event sending
5. Add README with wiring diagram and voice commands
6. Test build and functionality

## Reference

- **Existing Examples:** `switch/`, `tv/`, `speaker/`, `motion_sensor/`, `light/`
- **API Reference:** `docs/api-reference.md`
- **Arduino Examples:** Reference for functionality ideas
