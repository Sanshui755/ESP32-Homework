# SinricPro ESP-IDF Examples - Complete Summary

## Overview

This document provides a comprehensive overview of all 13 working examples included in the SinricPro ESP-IDF component.

## Examples by Category

### 🔌 Simple Devices (3 examples)

#### 1. Switch
- **Path:** `examples/switch/`
- **Hardware:** LED (GPIO 2), Button (GPIO 0)
- **Capabilities:** PowerState
- **Use Case:** Basic on/off control for lights, appliances
- **Voice:** "Alexa, turn on the switch"
- **Complexity:** ⭐ Beginner-friendly

#### 2. Motion Sensor
- **Path:** `examples/motion_sensor/`
- **Hardware:** PIR sensor (HC-SR501), LED status indicator
- **Capabilities:** Motion detection (read-only)
- **Use Case:** Security, automation triggers
- **Voice:** "Alexa, is there motion detected?"
- **Complexity:** ⭐ Beginner-friendly

#### 3. Contact Sensor
- **Path:** `examples/contact_sensor/`
- **Hardware:** Magnetic reed switch, status LED
- **Capabilities:** Contact detection (read-only)
- **Use Case:** Door/window monitoring, security
- **Voice:** "Alexa, is the door open?"
- **Complexity:** ⭐ Beginner-friendly

### 💡 Lighting (2 examples)

#### 4. Smart Light
- **Path:** `examples/light/`
- **Hardware:** RGB LED (common cathode), 3× 220Ω resistors
- **GPIO:** R=25, G=26, B=27
- **Capabilities:** PowerState, Brightness, Color (RGB), ColorTemperature
- **Features:**
  - RGB color control (16.7M colors)
  - Brightness 0-100%
  - Color temperature 2200-7000K (warm to cool white)
  - Kelvin-to-RGB conversion
  - PWM smooth transitions (5kHz)
- **Voice:** "Alexa, set light to red", "Alexa, set light to warm white"
- **Complexity:** ⭐⭐⭐ Advanced

#### 5. DimSwitch
- **Path:** `examples/dimswitch/`
- **Hardware:** High-power LED or LED strip, MOSFET (IRF520/IRLZ44N)
- **GPIO:** LED=25, Button=0
- **Capabilities:** PowerState, PowerLevel (brightness)
- **Features:**
  - Dimmable brightness 0-100%
  - PWM control (5kHz)
  - Auto turn-on when brightness set
- **Voice:** "Alexa, dim the light", "Alexa, set light to 50%"
- **Complexity:** ⭐⭐ Intermediate

### 🌡️ Climate Control (3 examples)

#### 6. Thermostat
- **Path:** `examples/thermostat/`
- **Hardware:** 3× relays (heat/cool/fan), DHT22 sensor (simulated)
- **GPIO:** Heat=18, Cool=19, Fan=21, DHT=4
- **Capabilities:** PowerState, ThermostatMode, TargetTemperature, CurrentTemperature
- **Features:**
  - 5 modes: AUTO, COOL, HEAT, ECO, OFF
  - Temperature control with hysteresis (0.5°C)
  - Simulated DHT22 (replaceable with real sensor)
  - Comfort level indicators
- **Voice:** "Alexa, set thermostat to 72 degrees", "Alexa, make it warmer"
- **Complexity:** ⭐⭐⭐ Advanced

#### 7. Fan
- **Path:** `examples/fan/`
- **Hardware:** DC motor, MOSFET/L298N motor driver
- **GPIO:** Motor=25, Button=0
- **Capabilities:** PowerState, PowerLevel (speed)
- **Features:**
  - Variable speed 0-100%
  - PWM control (25kHz, above audible range)
  - Speed presets (off/low/medium/high)
  - Minimum 20% startup threshold
- **Voice:** "Alexa, set fan speed to 75"
- **Complexity:** ⭐⭐ Intermediate

#### 8. Temperature Sensor
- **Path:** `examples/temperature_sensor/`
- **Hardware:** DHT22 or DHT11 sensor
- **GPIO:** DHT=4
- **Capabilities:** Temperature & Humidity reporting
- **Features:**
  - Simulated sensor (for testing without hardware)
  - Real DHT22/DHT11 support (with library)
  - 60-second reporting interval
  - Comfort level indicators
- **Voice:** "Alexa, what's the temperature?"
- **Complexity:** ⭐⭐ Intermediate

### 🔒 Security & Access (2 examples)

#### 9. Lock
- **Path:** `examples/lock/`
- **Hardware:** Servo motor (SG90), status LED
- **GPIO:** Servo=18, LED=2, Button=0
- **Capabilities:** LockState
- **Features:**
  - Servo-based mechanical lock (0-90°)
  - Configurable locked/unlocked angles
  - Manual button override
  - Status LED (off=locked, on=unlocked)
- **Voice:** "Alexa, lock the door"
- **Complexity:** ⭐⭐ Intermediate

#### 10. Garage Door
- **Path:** `examples/garage_door/`
- **Hardware:** Relay module, 2× reed switches, 2× magnets
- **GPIO:** Relay=18, Open switch=4, Closed switch=5, Button=0
- **Capabilities:** DoorState
- **Features:**
  - Momentary relay pulse (500ms)
  - Position detection (fully open/closed)
  - Safety checks (prevents unnecessary triggers)
  - Manual button control
- **Voice:** "Alexa, open the garage door"
- **Complexity:** ⭐⭐⭐ Advanced

### 🏠 Home Automation (1 example)

#### 11. Blinds
- **Path:** `examples/blinds/`
- **Hardware:** DC motor, L298N motor driver, manual buttons
- **GPIO:** Motor IN1=25, IN2=26, Enable=27, Up=32, Down=33, Stop=0
- **Capabilities:** PowerState, RangeValue (position)
- **Features:**
  - Position control 0-100% (0=closed, 100=open)
  - Time-based position estimation
  - Configurable travel time calibration
  - Manual controls (up/down/stop buttons)
  - Position reporting every 10%
- **Voice:** "Alexa, set blinds to 50%"
- **Complexity:** ⭐⭐⭐ Advanced

### 📺 Media Devices (2 examples)

#### 12. TV
- **Path:** `examples/tv/`
- **Hardware:** Simulated (can add IR transmitter)
- **Capabilities:** PowerState, Volume, Mute, MediaControl, InputController, ChannelController
- **Features:**
  - 8 capabilities (most complex device)
  - Volume 0-100
  - Media control (play/pause/stop/next/previous/etc.)
  - Input selection (HDMI1/2/3, AV1/2, etc.)
  - Channel control (number + name)
  - Button simulation for testing
- **Voice:** "Alexa, change TV to HDMI 1", "Alexa, channel 5"
- **Complexity:** ⭐⭐⭐ Advanced

#### 13. Speaker
- **Path:** `examples/speaker/`
- **Hardware:** Simulated audio system
- **Capabilities:** PowerState, Volume, Mute, MediaControl, InputController, EqualizerController, ModeController
- **Features:**
  - 9 capabilities (most feature-rich)
  - Equalizer (Bass/Midrange/Treble: -10 to +10)
  - Mode control (MOVIE/MUSIC/SPORT/TV)
  - All TV features plus audio-specific controls
- **Voice:** "Alexa, set bass to 5", "Alexa, set speaker to movie mode"
- **Complexity:** ⭐⭐⭐ Advanced

## Hardware Requirements Summary

### Common Components (Used in Multiple Examples)
- **ESP32 Development Board** - All examples
- **LEDs** - Switch, Lock, Contact Sensor, Light, DimSwitch
- **Buttons** - All actuator examples
- **Resistors (220Ω-10kΩ)** - Various examples

### Specialized Components by Category

#### Sensors
- **PIR Sensor (HC-SR501)** - Motion Sensor
- **Reed Switches** - Contact Sensor, Garage Door
- **DHT22/DHT11** - Temperature Sensor, Thermostat

#### Motors & Actuators
- **Servo Motor** - Lock
- **DC Motor** - Fan, Blinds
- **Relay Modules** - Thermostat, Garage Door

#### Motor Drivers
- **MOSFET (IRF520/IRLZ44N)** - DimSwitch, Fan
- **L298N/L293D** - Blinds, Fan (high power)

#### LEDs
- **Single LEDs** - Switch, sensors
- **RGB LED** - Light
- **High-power LED/Strip** - DimSwitch

## Voice Command Reference

### Power Control (All Devices)
```
"Alexa, turn on/off the <device>"
"Alexa, is the <device> on?"
```

### Brightness/Level Control
```
"Alexa, set <device> to 50 percent"
"Alexa, dim the <device>"
"Alexa, brighten the <device>"
"Alexa, increase/decrease <device>"
```

### Temperature Control
```
"Alexa, set thermostat to 72 degrees"
"Alexa, make it warmer/cooler"
"Alexa, what's the temperature?"
```

### Color Control
```
"Alexa, set light to red/blue/green"
"Alexa, set light to warm white"
"Alexa, make the light cooler" (color temperature)
```

### Media Control
```
"Alexa, volume up/down"
"Alexa, mute the TV"
"Alexa, play/pause"
"Alexa, next/previous track"
```

### Input/Channel Control
```
"Alexa, change TV to HDMI 1"
"Alexa, channel 5"
"Alexa, next channel"
```

### Position Control
```
"Alexa, open/close the blinds"
"Alexa, set blinds to 50 percent"
```

### Security Control
```
"Alexa, lock/unlock the door"
"Alexa, open/close the garage door"
"Alexa, is the door locked?"
```

## Build Instructions

All examples follow the same build process:

```bash
# Navigate to example directory
cd examples/<example_name>

# Configure for your target
idf.py set-target esp32  # or esp32s2, esp32s3, esp32c3, esp32c6

# Configure WiFi and credentials
idf.py menuconfig  # Optional: configure component settings

# Edit credentials in main/<example>_example.c
# Update: WIFI_SSID, WIFI_PASS, DEVICE_ID, APP_KEY, APP_SECRET

# Build
idf.py build

# Flash and monitor
idf.py flash monitor
```

## Example File Structure

Each example includes:

```
examples/<device>/
├── CMakeLists.txt                 # Project configuration
├── README.md                      # Example-specific documentation
└── main/
    ├── CMakeLists.txt             # Component configuration
    ├── idf_component.yml          # Dependencies
    └── <device>_example.c         # Main application code
```

## Configuration Requirements

### Required (All Examples)
1. **WiFi Credentials**
   - WIFI_SSID
   - WIFI_PASS

2. **SinricPro Credentials**
   - DEVICE_ID (from SinricPro portal)
   - APP_KEY (from SinricPro portal)
   - APP_SECRET (from SinricPro portal)

### Optional (Example-Specific)
- GPIO pin assignments
- Timing constants (relay pulse, travel time, etc.)
- Sensor calibration values
- PWM frequencies

## Getting SinricPro Credentials

1. Sign up at [sinric.pro](https://sinric.pro)
2. Go to **Devices** → **Add Device**
3. Select device type (Switch, Light, Thermostat, etc.)
4. Note the **Device ID**
5. Go to **Credentials** to get **APP_KEY** and **APP_SECRET**

## Testing Without Hardware

Several examples support simulation mode for testing without physical hardware:

- **Temperature Sensor** - Simulated DHT22 sensor
- **Thermostat** - Simulated temperature readings
- **TV** - Simulated with button controls
- **Speaker** - Simulated audio system

Enable simulation by setting `USE_SIMULATED_SENSOR` or similar flags in the example code.

## Common Issues & Solutions

### Build Issues
- **Missing dependencies**: Run `idf.py reconfigure`
- **IDF version**: Ensure ESP-IDF v4.4 or higher
- **Target mismatch**: Run `idf.py set-target <chip>`

### Connection Issues
- **WiFi won't connect**: Check SSID/password, signal strength
- **SinricPro connection fails**: Verify credentials, check internet
- **Auto-reconnect not working**: Ensure `auto_reconnect = true`

### Hardware Issues
- **Motor doesn't move**: Check power supply, driver wiring
- **Sensor returns zeros**: Verify GPIO pin, sensor power
- **LED too dim**: Adjust PWM duty cycle, check resistor value

### Rate Limiting
- Events sent too frequently will be rate-limited
- Sensors: Max 1 event per 60 seconds
- Actuators: Max 1 event per second
- Handle `SINRICPRO_ERR_RATE_LIMITED` gracefully

## Performance Characteristics

### Memory Usage (Typical)
- **Flash:** ~250KB (component + dependencies)
- **RAM (heap):** ~40-60KB runtime
- **Stack:** 4-8KB per task

### Network Usage
- **Idle:** ~1KB/minute (heartbeat)
- **Active:** ~2-5KB per command
- **WebSocket:** Persistent connection, minimal overhead

### CPU Usage
- **Idle:** <1% (ESP32 @ 240MHz)
- **Processing command:** <5% burst
- **PWM control:** <2% continuous

## Next Steps

1. **Choose an example** based on your project needs
2. **Gather hardware** components (or use simulation mode)
3. **Get SinricPro credentials** from portal
4. **Build and flash** the example
5. **Test voice control** with Alexa/Google Home
6. **Customize** for your specific use case

## Contributing

Have a new device type or improvement?
1. Follow existing example patterns
2. Include comprehensive README
3. Test on multiple ESP32 variants
4. Submit pull request

## Support Resources

- **Examples Guide:** [EXAMPLES_GUIDE.md](EXAMPLES_GUIDE.md)
- **API Reference:** [docs/api-reference.md](docs/api-reference.md)
- **Discord Community:** [discord.gg/W5299EgB59](https://discord.gg/W5299EgB59)
- **GitHub Issues:** [github.com/sinricpro/esp-idf/issues](https://github.com/sinricpro/esp-idf/issues)

---

**Total Examples:** 13 complete working examples
**Total Complexity Levels:** 3 (Simple, Intermediate, Advanced)
**Hardware Platforms:** ESP32, ESP32-S2, ESP32-S3, ESP32-C3, ESP32-C6
**ESP-IDF Versions:** v4.4, v5.0, v5.1, v5.2+

Built with ❤️ by the SinricPro Team
