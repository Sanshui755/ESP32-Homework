# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

This is an ESP-IDF component for integrating ESP32 devices with the SinricPro IoT platform. It enables voice control via Alexa and Google Home through WebSocket-based bidirectional communication with HMAC-SHA256 message signing.

## Build Commands

### Building the Component

The component uses ESP-IDF's standard build system:

```bash
# Build
idf.py build

# Flash to device
idf.py flash

# Monitor serial output
idf.py monitor

# Flash and monitor in one command
idf.py flash monitor

# Clean build
idf.py fullclean
```

### Working with Examples

```bash
cd examples/switch
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Configuration

Access menuconfig for configuration:

```bash
idf.py menuconfig
# Navigate to: Component config -> SinricPro Configuration
```

## Publishing to ESP Component Registry

Version is managed in `idf_component.yml`. To publish:

1. Update version in `idf_component.yml`
2. Commit and push changes
3. Create GitHub release with matching tag (e.g., `v1.0.1`)
4. GitHub Actions automatically uploads to ESP Component Registry

See `PUBLISHING.md` for details.

## Architecture

### Layered Design

The component follows a 4-layer architecture:

1. **Application Layer**: User code, callbacks, device initialization
2. **Device Layer**: Device type implementations (Switch, Light, etc.)
   - `src/devices/sinricpro_switch.c`
3. **Capability Layer**: Reusable capability controllers
   - `src/capabilities/power_state_controller.c` - On/Off state management
   - `src/capabilities/setting_controller.c` - Device settings
   - `src/capabilities/push_notification.c` - Notifications
4. **Core Layer**: Shared infrastructure
   - `src/core/sinricpro_core.c` - Main lifecycle and device registry
   - `src/core/sinricpro_websocket.c` - WebSocket client
   - `src/core/sinricpro_signature.c` - HMAC-SHA256 signing
   - `src/core/sinricpro_message_queue.c` - Outbound message queue
   - `src/core/sinricpro_event_limiter.c` - Rate limiting (1 event/sec)

### Device Registry System

Devices are registered as linked list nodes (`sinricpro_device_t`) in the core. Each device has:
- Device ID (24-character hex string from portal)
- Device type enum
- Request handler callback (routes incoming WebSocket messages)
- User data pointer

The registry uses `sinricpro_core_register_device()` and `sinricpro_core_unregister_device()` (internal APIs).

### Message Flow

**Incoming (Server → Device):**
1. WebSocket receives JSON message
2. Core validates signature
3. Core looks up device in registry by device_id
4. Device's request handler is called
5. Request handler delegates to capability controller
6. Capability controller invokes user callback
7. Response JSON is generated and sent back

**Outgoing (Device → Server):**
1. User calls device event function (e.g., `sinricpro_switch_send_power_state_event()`)
2. Event limiter checks rate limit
3. Event is added to message queue
4. Message queue task signs and sends via WebSocket

### Capability Controllers

Capabilities are reusable building blocks attached to devices:

- **PowerStateController**: Handles `setPowerState` actions and state events
- **SettingController**: Handles device-specific settings
- **PushNotification**: Sends notifications to SinricPro app

Each controller has:
- Handle/create/destroy lifecycle
- Callback registration (user code)
- Request handler (processes incoming actions)
- Event sender (sends state changes to server)

## Key Constraints

### Rate Limiting
- State change events: Max 1 per second per device
- Sensor events: Max 1 per 60 seconds
- Returns `SINRICPRO_ERR_RATE_LIMITED` if exceeded

### Thread Safety
- All public APIs are thread-safe (use FreeRTOS mutexes)
- Callbacks execute in separate callback task (4KB stack by default)
- WebSocket runs in dedicated task (8KB stack by default)

### Memory
- Heap usage: ~40KB (includes WebSocket buffers)
- Device registry: Linked list (max devices configurable in Kconfig)
- Message queue: FreeRTOS queue (size configurable)

### Callbacks
- Must complete quickly (< 100ms recommended)
- Must not block or call time-consuming functions
- Return `true` for success, `false` for failure

## Configuration (Kconfig)

Key settings in `Kconfig`:
- `CONFIG_SINRICPRO_MAX_DEVICES` - Max registered devices (default: 10)
- `CONFIG_SINRICPRO_EVENT_QUEUE_SIZE` - Event queue size (default: 10)
- `CONFIG_SINRICPRO_MESSAGE_QUEUE_SIZE` - Message queue size (default: 10)
- `CONFIG_SINRICPRO_ENABLE_DEBUG` - Enable verbose logging
- Stack sizes and task priorities for WebSocket and callback tasks

## Adding New Device Types

To add a new device type (e.g., Light):

1. Define device API in `include/sinricpro_light.h`
2. Implement device in `src/devices/sinricpro_light.c`
3. Create/reuse capability controllers as needed
4. Device must implement request handler that routes actions to capabilities
5. Provide event-sending functions for each capability
6. Register with core using `sinricpro_core_register_device()`
7. Update `CMakeLists.txt` to include new source files
8. Add device type enum to `sinricpro_types.h` if needed

## Dependencies

From `idf_component.yml`:
- ESP-IDF >= 4.4 (tested on 6.1)
- `espressif/esp_websocket_client ^1.2.0`
- `espressif/cjson *`
- Built-in: mbedtls, esp_event, nvs_flash, esp_netif, esp_wifi

## Code Style Notes

- Copyright header: Creative Commons CC BY-SA 4.0 license
- Use ESP-IDF logging macros (`ESP_LOGI`, `ESP_LOGE`, etc.)
- Error codes use `SINRICPRO_ERR_*` defines
- Function naming: `sinricpro_<component>_<action>()`
- Internal APIs have `_internal.h` headers
- Opaque handles use `typedef void*` pattern
