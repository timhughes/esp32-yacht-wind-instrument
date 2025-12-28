/*
  WindDataSource.h - Abstract interface for wind data sources
  
  All wind data sources (Demo, WiFi/Signal K, NMEA, BLE, NMEA2000, etc.)
  must implement this interface.
*/

#ifndef WIND_DATA_SOURCE_H
#define WIND_DATA_SOURCE_H

class WindDataSource {
public:
  virtual ~WindDataSource() {}
  
  // Initialize the data source
  virtual bool begin() = 0;
  
  // Update/process data (called in main loop)
  virtual void update() = 0;
  
  // Check if source is connected and providing data
  virtual bool isConnected() = 0;
  
  // Get wind speed in m/s (internal standard unit)
  virtual float getWindSpeed() = 0;
  
  // Get wind angle in degrees (0-359, relative to bow)
  virtual float getWindAngle() = 0;
  
  // Get human-readable source name
  virtual const char* getSourceName() = 0;
  
  // Clean shutdown
  virtual void stop() = 0;
};

#endif // WIND_DATA_SOURCE_H
