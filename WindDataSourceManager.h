/*
  WindDataSourceManager.h - Manages switching between wind data sources
  
  Handles initialization, cleanup, and switching between different data sources.
*/

#ifndef WIND_DATA_SOURCE_MANAGER_H
#define WIND_DATA_SOURCE_MANAGER_H

#include "WindDataSource.h"

enum DataSourceType {
  SOURCE_DEMO,
  SOURCE_WIFI_SIGNALK,
  SOURCE_NMEA,
  SOURCE_BLE,
  SOURCE_NMEA2000
};

class WindDataSourceManager {
private:
  WindDataSource* currentSource;
  DataSourceType currentType;
  
public:
  WindDataSourceManager() : currentSource(nullptr), currentType(SOURCE_DEMO) {}
  
  // Switch to a new data source
  bool switchSource(WindDataSource* newSource, DataSourceType type) {
    // Stop current source (but don't delete it)
    if (currentSource) {
      currentSource->stop();
    }
    
    // Start new source
    currentSource = newSource;
    currentType = type;
    
    if (currentSource) {
      bool success = currentSource->begin();
      if (!success) {
        currentSource = nullptr;
        return false;
      }
      return true;
    }
    
    return false;
  }
  
  // Get current source
  WindDataSource* getCurrentSource() {
    return currentSource;
  }
  
  // Get current source type
  DataSourceType getCurrentType() {
    return currentType;
  }
  
  // Get source type name
  const char* getTypeName(DataSourceType type) {
    switch (type) {
      case SOURCE_DEMO: return "Demo";
      case SOURCE_WIFI_SIGNALK: return "WiFi/Signal K";
      case SOURCE_NMEA: return "NMEA 0183";
      case SOURCE_BLE: return "Bluetooth LE";
      case SOURCE_NMEA2000: return "NMEA 2000";
      default: return "Unknown";
    }
  }
  
  // Update current source
  void update() {
    if (currentSource) {
      currentSource->update();
    }
  }
  
  // Check if connected
  bool isConnected() {
    return currentSource && currentSource->isConnected();
  }
};

#endif // WIND_DATA_SOURCE_MANAGER_H
