/*
  MockWindDataSource.h - Mock wind data source for testing
  
  Allows setting specific wind values for testing UI and conversions.
*/

#ifndef MOCK_WIND_DATA_SOURCE_H
#define MOCK_WIND_DATA_SOURCE_H

#include "WindDataSource.h"

class MockWindDataSource : public WindDataSource {
private:
  float wind_speed;      // m/s
  float wind_angle;      // degrees
  bool connected;
  
public:
  MockWindDataSource() : wind_speed(0.0), wind_angle(0.0), connected(false) {}
  
  bool begin() override {
    connected = true;
    return true;
  }
  
  void update() override {
    // Mock doesn't auto-update, values set manually
  }
  
  bool isConnected() override {
    return connected;
  }
  
  float getWindSpeed() override {
    return wind_speed;
  }
  
  float getWindAngle() override {
    return wind_angle;
  }
  
  const char* getSourceName() override {
    return "Mock";
  }
  
  void stop() override {
    connected = false;
  }
  
  // Test helper methods
  void setWindSpeed(float speed_ms) {
    wind_speed = speed_ms;
  }
  
  void setWindAngle(float angle_deg) {
    wind_angle = angle_deg;
  }
  
  void setConnected(bool state) {
    connected = state;
  }
};

#endif // MOCK_WIND_DATA_SOURCE_H
