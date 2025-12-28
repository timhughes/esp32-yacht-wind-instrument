# Wind Display - Signal K Integration Plan

## Project Overview
Add Signal K connectivity to ESP32-C6 wind display with WiFi and NMEA 0183 support, configuration UI, and demo mode fallback. Designed with extensibility for future protocols (BLE, NMEA2000, etc.) and comprehensive testing.

## Architecture Design

### Core Abstraction: Data Source Interface
All data sources (Demo, WiFi/Signal K, NMEA, future BLE, NMEA2000) implement a common interface:

```cpp
class WindDataSource {
public:
  virtual bool begin() = 0;                    // Initialize the source
  virtual void update() = 0;                   // Called in loop to process data
  virtual bool isConnected() = 0;              // Connection status
  virtual float getWindSpeed() = 0;            // Get speed in m/s (internal unit)
  virtual float getWindAngle() = 0;            // Get angle in degrees
  virtual const char* getSourceName() = 0;     // "Demo", "WiFi", "NMEA", etc.
  virtual void stop() = 0;                     // Clean shutdown
};
```

### Benefits
- Easy to add new protocols (just implement the interface)
- Testable (can create mock sources)
- Clean separation of concerns
- Hot-swappable data sources

## Phase 0: Architecture & Testing Framework
- [x] 0.1 Create WindDataSource abstract base class
- [x] 0.2 Create DemoWindDataSource (current demo mode)
- [x] 0.3 Create MockWindDataSource for testing
- [x] 0.4 Refactor current code to use data source abstraction
- [x] 0.5 Add data source manager to switch between sources
- [x] 0.6 Add unit test framework and comprehensive tests

**Phase 0 Complete!** ✅ Architecture is now extensible and fully tested.

### Running Tests
To run the unit tests:
1. Open `test_wind_data_sources/test_wind_data_sources.ino` in Arduino IDE
2. Upload to ESP32-C6
3. Open Serial Monitor (115200 baud)
4. View test results

Tests cover:
- DemoWindDataSource functionality
- MockWindDataSource behavior
- WindDataSourceManager switching
- Unit conversions (m/s ↔ knots ↔ mph)

## Phase 1: Configuration System
- [x] 1.1 Create configuration structure for storing settings
  - WiFi SSID/password
  - Signal K server IP/port
  - Connection mode (Demo/WiFi/NMEA/BLE/NMEA2000)
  - Wind units preference (kts/m/s/mph)
  - Data source specific settings (extensible)
- [x] 1.2 Implement NVS storage for persistent configuration
- [x] 1.3 Add default configuration values
- [x] 1.4 Add unit conversion helpers
- [x] 1.5 Integrate config into main code

**Phase 1 Complete!** ✅ Configuration system with NVS persistence ready.

## Phase 2: Configuration UI
- [ ] 2.1 Create menu screen with LVGL
  - Connection mode selection
  - WiFi settings entry
  - Signal K server settings
  - Units selection
  - Back/Save buttons
- [ ] 2.2 Implement on-screen keyboard for text entry
- [ ] 2.3 Wire up MENU button to show/hide config screen
- [ ] 2.4 Add visual indicators for connection status

## Phase 3: WiFi Implementation
- [ ] 3.1 Add WiFi connection manager
  - Connect to configured network
  - Handle reconnection on disconnect
  - Show connection status on display
- [ ] 3.2 Implement Signal K WebSocket client
  - Connect to Signal K server
  - Subscribe to wind data paths:
    * `environment.wind.speedApparent`
    * `environment.wind.angleApparent`
  - Parse JSON delta messages
- [ ] 3.3 Update display with real-time wind data
- [ ] 3.4 Add error handling and timeout logic
- [ ] 3.5 Fallback to demo mode on connection failure

## Phase 4: NMEA 0183 Implementation
- [ ] 4.1 Configure UART for NMEA input
  - Use available GPIO pins
  - 4800 baud, 8N1 (standard NMEA)
- [ ] 4.2 Implement NMEA sentence parser
  - Parse MWV (Wind Speed and Angle)
  - Parse VWR (Relative Wind Speed and Angle)
  - Validate checksums
- [ ] 4.3 Update display with parsed NMEA data
- [ ] 4.4 Add sentence filtering and error handling
- [ ] 4.5 Fallback to demo mode on no data timeout

## Phase 5: Units Button Implementation
- [x] 5.1 Wire up UNITS button handler
- [x] 5.2 Implement unit conversion functions (already in WindConfig)
- [x] 5.3 Update display labels based on selected units
- [x] 5.4 Save unit preference to configuration

**Phase 5 Complete!** ✅ UNITS button cycles through kts/m/s/mph and persists choice.

## Phase 6: Connection Mode Management
- [ ] 6.1 Implement mode state machine
  - Demo mode (default)
  - WiFi mode
  - NMEA mode
- [ ] 6.2 Add mode switching logic
- [ ] 6.3 Clean up resources when switching modes
- [ ] 6.4 Add visual indicator for current mode

## Phase 7: Testing & Polish
- [ ] 7.1 Test WiFi connection and reconnection
- [ ] 7.2 Test Signal K data parsing
- [ ] 7.3 Test NMEA parsing with real data
- [ ] 7.4 Test configuration save/load
- [ ] 7.5 Test mode switching
- [ ] 7.6 Add connection timeout indicators
- [ ] 7.7 Optimize memory usage
- [ ] 7.8 Add debug logging (optional)

## Technical Notes

### Signal K WebSocket Format
```json
{
  "context": "vessels.self",
  "subscribe": [
    {
      "path": "environment.wind.speedApparent",
      "period": 1000
    },
    {
      "path": "environment.wind.angleApparent",
      "period": 1000
    }
  ]
}
```

### NMEA 0183 Sentences
- **MWV**: `$--MWV,x.x,a,x.x,a*hh`
  - Wind angle (0-359°)
  - Reference (R=Relative, T=True)
  - Wind speed
  - Units (K=kts, M=m/s, N=mph)
- **VWR**: `$--VWR,x.x,a,x.x,N,x.x,M,x.x,K*hh`
  - Relative wind direction and speed

### GPIO Pins Available for NMEA UART
- RX: GPIO 10 or 11 (check which are free)
- TX: Not needed for receive-only

### Unit Conversions
- 1 knot = 0.514444 m/s
- 1 knot = 1.15078 mph
- 1 m/s = 1.94384 knots
- 1 mph = 0.868976 knots

## Current Status
✅ Basic wind display with compass
✅ Touch input working
✅ LVGL UI framework
✅ Monospace fonts for values
✅ UNITS and MENU buttons (not yet functional)

## Next Steps
Start with Phase 1: Configuration System
