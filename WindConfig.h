/*
  WindConfig.h - Configuration management for wind display
  
  Stores and persists configuration to ESP32 NVS (Non-Volatile Storage).
*/

#ifndef WIND_CONFIG_H
#define WIND_CONFIG_H

#include <Preferences.h>

enum WindUnits {
  UNITS_KNOTS,
  UNITS_MS,
  UNITS_MPH,
  UNITS_KPH
};

struct WindConfiguration {
  // Connection settings
  DataSourceType dataSource;
  
  // WiFi settings
  char wifiSSID[32];
  char wifiPassword[64];
  
  // Signal K settings
  char signalkHost[64];
  uint16_t signalkPort;
  
  // NMEA settings
  uint8_t nmeaRxPin;
  uint32_t nmeaBaudRate;
  
  // Display settings
  WindUnits units;
  
  // Version for future compatibility
  uint8_t configVersion;
};

class WindConfig {
private:
  Preferences prefs;
  WindConfiguration config;
  
  // Default values
  void setDefaults() {
    config.dataSource = SOURCE_DEMO;
    
    strcpy(config.wifiSSID, "");
    strcpy(config.wifiPassword, "");
    
    strcpy(config.signalkHost, "192.168.1.100");
    config.signalkPort = 3000;
    
    config.nmeaRxPin = 10;
    config.nmeaBaudRate = 4800;
    
    config.units = UNITS_KNOTS;
    config.configVersion = 1;
  }
  
public:
  WindConfig() {
    setDefaults();
  }
  
  // Load configuration from NVS
  bool load() {
    if (!prefs.begin("windconfig", false)) {
      return false;
    }
    
    if (!prefs.isKey("version")) {
      prefs.end();
      return false;
    }
    
    config.configVersion = prefs.getUChar("version", 1);
    config.dataSource = (DataSourceType)prefs.getUChar("dataSource", SOURCE_DEMO);
    config.units = (WindUnits)prefs.getUChar("units", UNITS_KNOTS);
    
    prefs.getString("wifiSSID", config.wifiSSID, sizeof(config.wifiSSID));
    prefs.getString("wifiPass", config.wifiPassword, sizeof(config.wifiPassword));
    
    prefs.getString("skHost", config.signalkHost, sizeof(config.signalkHost));
    config.signalkPort = prefs.getUShort("skPort", 3000);
    
    config.nmeaRxPin = prefs.getUChar("nmeaRx", 10);
    config.nmeaBaudRate = prefs.getUInt("nmeaBaud", 4800);
    
    prefs.end();
    return true;
  }
  
  // Save configuration to NVS
  bool save() {
    if (!prefs.begin("windconfig", false)) {
      return false;
    }
    
    prefs.putUChar("version", config.configVersion);
    prefs.putUChar("dataSource", config.dataSource);
    prefs.putUChar("units", config.units);
    
    prefs.putString("wifiSSID", config.wifiSSID);
    prefs.putString("wifiPass", config.wifiPassword);
    
    prefs.putString("skHost", config.signalkHost);
    prefs.putUShort("skPort", config.signalkPort);
    
    prefs.putUChar("nmeaRx", config.nmeaRxPin);
    prefs.putUInt("nmeaBaud", config.nmeaBaudRate);
    
    prefs.end();
    return true;
  }
  
  // Clear all saved configuration
  bool clear() {
    if (!prefs.begin("windconfig", false)) {
      return false;
    }
    prefs.clear();
    prefs.end();
    setDefaults();
    return true;
  }
  
  // Getters
  WindConfiguration& get() { return config; }
  DataSourceType getDataSource() { return config.dataSource; }
  WindUnits getUnits() { return config.units; }
  const char* getWifiSSID() { return config.wifiSSID; }
  const char* getWifiPassword() { return config.wifiPassword; }
  const char* getSignalKHost() { return config.signalkHost; }
  uint16_t getSignalKPort() { return config.signalkPort; }
  uint8_t getNMEARxPin() { return config.nmeaRxPin; }
  uint32_t getNMEABaudRate() { return config.nmeaBaudRate; }
  
  // Setters
  void setDataSource(DataSourceType source) { config.dataSource = source; }
  void setUnits(WindUnits u) { config.units = u; }
  void setWifiSSID(const char* ssid) { strncpy(config.wifiSSID, ssid, sizeof(config.wifiSSID) - 1); }
  void setWifiPassword(const char* pass) { strncpy(config.wifiPassword, pass, sizeof(config.wifiPassword) - 1); }
  void setSignalKHost(const char* host) { strncpy(config.signalkHost, host, sizeof(config.signalkHost) - 1); }
  void setSignalKPort(uint16_t port) { config.signalkPort = port; }
  void setNMEARxPin(uint8_t pin) { config.nmeaRxPin = pin; }
  void setNMEABaudRate(uint32_t baud) { config.nmeaBaudRate = baud; }
  
  // Unit conversion helpers
  float convertSpeed(float speed_ms) {
    switch (config.units) {
      case UNITS_KNOTS: return speed_ms * 1.94384;  // m/s to knots
      case UNITS_MS: return speed_ms;                // already m/s
      case UNITS_MPH: return speed_ms * 2.23694;     // m/s to mph
      case UNITS_KPH: return speed_ms * 3.6;         // m/s to km/h
      default: return speed_ms;
    }
  }
  
  const char* getUnitsLabel() {
    switch (config.units) {
      case UNITS_KNOTS: return "kts";
      case UNITS_MS: return "m/s";
      case UNITS_MPH: return "mph";
      case UNITS_KPH: return "km/h";
      default: return "kts";
    }
  }
  
  // Cycle through units
  void cycleUnits() {
    config.units = (WindUnits)((config.units + 1) % 4);
  }
  
  // Print current configuration (removed to save flash space)
  void print() {}
};

#endif // WIND_CONFIG_H
