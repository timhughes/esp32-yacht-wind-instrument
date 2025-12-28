/*
  DemoWindDataSource.h - Demo/simulation wind data source
  
  Generates simulated wind data for testing and demo purposes.
*/

#ifndef DEMO_WIND_DATA_SOURCE_H
#define DEMO_WIND_DATA_SOURCE_H

#include "WindDataSource.h"

class DemoWindDataSource : public WindDataSource {
private:
  float wind_speed;      // m/s
  float wind_angle;      // degrees
  unsigned long last_update;
  
public:
  DemoWindDataSource() : wind_speed(6.43), wind_angle(45.0), last_update(0) {}
  
  ~DemoWindDataSource() {}
  
  bool begin() override {
    Serial.println("[Demo] Started");
    wind_speed = 6.43;  // ~12.5 knots
    wind_angle = 45.0;
    last_update = millis();
    return true;
  }
  
  void update() override {
    // Update every 200ms for smooth animation
    if (millis() - last_update > 200) {
      // Rotate angle
      wind_angle = fmod(wind_angle + 1.0, 360.0);
      
      // Vary speed slightly (10-15 knots range)
      wind_speed = 5.14 + (random(0, 50) / 100.0);
      
      last_update = millis();
    }
  }
  
  bool isConnected() override {
    return true;  // Demo is always "connected"
  }
  
  float getWindSpeed() override {
    return wind_speed;  // m/s
  }
  
  float getWindAngle() override {
    return wind_angle;  // degrees
  }
  
  const char* getSourceName() override {
    return "Demo";
  }
  
  void stop() override {
    Serial.println("[Demo] Stopped");
  }
};

#endif // DEMO_WIND_DATA_SOURCE_H
