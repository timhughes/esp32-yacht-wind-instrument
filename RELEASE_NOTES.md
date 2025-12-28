# Release Notes

## Version 0.1.0 - Initial Release (2025-12-29)

### Overview

First production release of the Yacht Wind Display - a marine wind instrument for ESP32-C6 with ILI9341 touchscreen display. Connects to Signal K servers via WiFi to show real-time apparent wind speed and direction.

### Features

#### Display

- 240x320 pixel touchscreen interface with compass rose
- Large, readable 32px Roboto Mono font for wind speed and direction
- Compass rose with cardinal points (N, E, S, W)
- Red/green port/starboard sectors (20-60°) for optimal sailing angles
- Thick red line indicator showing wind direction
- Real-time connection status indicator

#### Data Sources

- **Signal K WebSocket**: Connect to Signal K server via WiFi for real-time wind data
- **Demo Mode**: Simulated rotating wind data for testing and demonstration
- Extensible architecture ready for future data sources (NMEA 0183, Bluetooth LE, NMEA 2000)

#### Configuration

- Touch-based configuration menu with on-screen keyboard
- WiFi network setup (SSID and password)
- Signal K server configuration (IP address and port)
- Multiple unit options: Knots, m/s, mph, km/h
- Persistent storage in ESP32 NVS (survives reboots)

#### Connectivity

- WiFi 2.4GHz support
- WebSocket client for Signal K streaming
- Automatic reconnection on connection loss
- Clear status indicators (Demo, WiFi..., WiFi OK, SignalK)

### Hardware Requirements

- ESP32-C6 Development Board
- ILI9341 2.4" TFT LCD Display (240x320)
- XPT2046 Touch Controller
- 8MB flash partition required

### Software Requirements

- Arduino IDE 2.0+
- ESP32 board support v3.0.0+
- Libraries: LVGL 9.4.0+, Adafruit GFX, Adafruit ILI9341, XPT2046_Touchscreen, WebSockets, ArduinoJson

### Signal K Integration

- Subscribes to `environment.wind.speedApparent` (m/s)
- Subscribes to `environment.wind.angleApparent` (radians)
- 1-second update interval
- Automatic unit conversion from Signal K standard units

### Known Limitations

- WiFi 2.4GHz only (ESP32-C6 limitation)
- Single Signal K server connection
- No data logging or history
- No calibration options

### Installation

See README.md for complete installation and configuration instructions.

### Credits

- LVGL graphics library: <https://lvgl.io/>
- Adafruit libraries: <https://github.com/adafruit>
- Signal K: <https://signalk.org/>
- ESP32 Arduino Core: <https://github.com/espressif/arduino-esp32>

### Future Roadmap

- NMEA 0183 serial input support
- Bluetooth LE wind sensor support
- NMEA 2000 CAN bus support
- True wind calculation
- Wind history graphs
- Configurable display themes
- Multiple instrument pages

---

For support, issues, or feature requests, please visit the project repository.
