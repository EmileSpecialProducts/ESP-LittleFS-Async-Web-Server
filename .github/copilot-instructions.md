# ESP-LittleFS-Async-Web-Server Developer Guide

## Project Overview
Multi-device ESP8266/ESP32 web server with LittleFS filesystem management. Provides a web-based file editor, OTA updates, and WiFi configuration.

## Architecture

### Core Components
- **Main firmware**: [src/ESP-LittleFS-Async-Web-Server.ino](src/ESP-LittleFS-Async-Web-Server.ino) - Arduino sketch (~730 lines)
  - Async web server on port 80 using ESPAsyncWebServer
  - REST API for filesystem operations
  - Built-in web UI (embedded HTML via `PROGMEM`)
  - OTA update support via ArduinoOTA
- **Web UI**: [web/](web/) - Editor interface (editor.js, editor.css, index.htm)
- **LittleFS data**: [littlefs/](littlefs/) - Default filesystem content (index.html, readme.txt)

### Multi-Device Support
- **Platforms**: ESP8266, ESP32, ESP32-S2/S3, ESP32-C2/C3/C5/C6/C61
- **Flash sizes**: 4MB, 8MB, 16MB, 32MB configurations in [platformio.ini](platformio.ini)
- **Naming convention**: Hostname set per-chip via preprocessor defines (e.g., `ESP-LittleFS-C3`, `ESP-LittleFS-S3`)
- **GPIO configuration**: Device-specific PIN_BOOT, PIN_LED, LED_BUILTIN defined per chip

## REST API Endpoints

| Method | Endpoint | Purpose |
|--------|----------|---------|
| GET | `/diskinfo` | Returns `{totalBytes, usedBytes, freeBytes}` as JSON |
| GET | `/list?dir=PATH` | Lists directory contents as JSON array |
| PUT | `/edit?arg=PATH` | Create file (if contains `.`) or directory |
| DELETE | `/edit?arg=PATH` | Delete file or directory |
| POST | `/edit` | Upload file (multipart form, saves with original filename) |
| GET | `/index.html` | Embedded default UI (Index_html PROGMEM array) |
| GET | `/*` | Serves files from LittleFS; 404.html fallback |

## Build & Deploy

### Build System
- **Tool**: PlatformIO (platformio.ini)
- **Framework**: Arduino
- **Common build flags**:
  - `CORE_DEBUG_LEVEL=0` - Disable core debug output
  - `CONFIG_ASYNC_TCP_*` - Tune async TCP stack
  - `DEBUG_PRINT` - Enable serial logging (serial.h macros)

### Key Commands
```bash
# List all environments
pio run -l

# Build for specific device
pio run -e esp32-c5-devkitc-1-4MB

# Build and flash
pio run -e esp32-c5-devkitc-1-4MB -t erase_upload

# Build LittleFS from littlefs/ directory
pio run -t buildfs

# Monitor serial output at 115200 baud (auto-configured)
pio device monitor
```

### Device Workflow
1. Flash firmware + LittleFS: `pio run -t erase_upload`
2. Device boots → WiFiManager portal starts if needed
3. Connect to `ESP-xxxxxxx` AP to configure WiFi (180s timeout)
4. Access web UI at `http://ESP-LittleFS-*.local` or IP address
5. OTA updates available via `arduclient` or ArduinoOTA protocol

## Code Patterns

### Platform-Specific Compilation
Uses `#if defined()` extensively for ESP8266 vs ESP32/variants:
```cpp
#if defined(ESP8266)
  // ESP8266-specific code
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
  // C3-specific code
#endif
```

### Debug Output
- Conditional via `DEBUG_PRINT` macro
- Maps to Serial.print/printf via macros in lines 78-88
- Disable with `-D CORE_DEBUG_LEVEL=0`

### LittleFS vs SPIFFS
- **Filesystem**: LittleFS on all targets (replaces SPIFFS)
- **File initialization**: `LittleFS.begin(true)` on ESP32 enables auto-formatting; manual on ESP8266
- **Data directory**: `Littlefs/` → mounts to `/` on device

### Web Server Pattern
- Async request handlers use lambda functions
- File upload callback: `[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)`
- CORS headers applied globally: `DefaultHeaders::Instance().addHeader(...)`

## Hardware Details

### Pin Assignments
- **PIN_BOOT**: BOOT button (press 5+ sec with WifiManager to reset WiFi)
- **PIN_LED**: Status LED (blinks at 1Hz when running)
- **ESP32-C5 exceptions**: PIN_BOOT=28, LED=27 (non-standard)

### WiFi & Network
- Uses WiFiManager for persistent AP configuration
- mDNS enabled: hostname resolves automatically (e.g., `ESP-LittleFS-C3.local`)
- WiFi sleep disabled (`WiFi.setSleep(false)`, `WIFI_PS_NONE`)

### OTA Updates
- Hostname-based: `ArduinoOTA.setHostname(host)`
- Timeout: 60 seconds max via `OTAUploadBusy` counter
- Prevents concurrent filesystem ops during OTA (`if (OTAUploadBusy == 0)`)

## Firmware Development

### Adding Endpoints


### Modifying LittleFS Content
1. Edit files in `littlefs/` directory
2. Run: `pio run -t buildfs`
3. Reflash: `pio run -t erase_upload`

### Serial Logging
- Baud: 115200 (configured in setup())
- Exception decoder active (platformio.ini monitor_filters)
- Log file written to LittleFS `/log.txt` by `Log()` function

## Important Files
- [platformio.ini](platformio.ini) - 630+ lines: defines 40+ build environments
- [managed_components/](managed_components/) - IDF component manager deps (don't edit)
- [partitions/spiffs_*.csv](partitions/) - Partition tables per flash size
- [web/editor.js](web/editor.js) - Frontend logic for file tree, edit, upload

## Notes
- Supports embedded HTML (Index_html, error404_html as PROGMEM arrays)
- URL encoding/decoding implemented (urlDecode function)
- Automatic chip identification at boot (logs chip ID, MAC, heap, PSRAM)
- Temperature sensor reading on ESP32 (lines 698-699)
