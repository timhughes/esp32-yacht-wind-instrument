/*
  SignalKWindDataSource.h - WiFi + Signal K WebSocket data source
  
  Connects to Signal K server via WiFi and subscribes to wind data.
*/

#ifndef SIGNALK_WIND_DATA_SOURCE_H
#define SIGNALK_WIND_DATA_SOURCE_H

#include "WindDataSource.h"
#include <WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

class SignalKWindDataSource : public WindDataSource {
private:
  WebSocketsClient webSocket;
  String host;
  uint16_t port;
  String ssid;
  String password;
  
  float wind_speed_ms;
  float wind_angle;
  bool connected;
  bool wifi_connected;
  unsigned long last_data_time;
  
  static SignalKWindDataSource* instance;  // For static callback
  
  static void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    if (instance) {
      instance->handleWebSocketEvent(type, payload, length);
    }
  }
  
  void handleWebSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
      case WStype_DISCONNECTED:
        Serial.println("[SignalK] WebSocket disconnected");
        connected = false;
        break;
        
      case WStype_CONNECTED:
        Serial.println("[SignalK] WebSocket connected");
        connected = true;
        subscribeToWindData();
        break;
        
      case WStype_TEXT:
        parseSignalKMessage((char*)payload);
        break;
        
      case WStype_ERROR:
        Serial.println("[SignalK] WebSocket error");
        connected = false;
        break;
        
      default:
        break;
    }
  }
  
  void subscribeToWindData() {
    Serial.println("[SignalK] Subscribing to wind data");
    StaticJsonDocument<512> doc;
    doc["context"] = "vessels.self";
    
    JsonArray subscribe = doc.createNestedArray("subscribe");
    
    JsonObject speed = subscribe.createNestedObject();
    speed["path"] = "environment.wind.speedApparent";
    speed["period"] = 1000;
    
    JsonObject angle = subscribe.createNestedObject();
    angle["path"] = "environment.wind.angleApparent";
    angle["period"] = 1000;
    
    String json;
    serializeJson(doc, json);
    webSocket.sendTXT(json);
  }
  
  void parseSignalKMessage(const char* payload) {
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
      return;
    }
    
    if (!doc.containsKey("updates")) {
      return;
    }
    
    JsonArray updates = doc["updates"];
    for (JsonObject update : updates) {
      JsonArray values = update["values"];
      for (JsonObject value : values) {
        const char* path = value["path"];
        
        if (strcmp(path, "environment.wind.speedApparent") == 0) {
          float val = value["value"];
          wind_speed_ms = val;
          last_data_time = millis();
        }
        else if (strcmp(path, "environment.wind.angleApparent") == 0) {
          float val = value["value"];
          // Signal K angle is in radians, convert to degrees
          wind_angle = val * 180.0 / PI;
          // Normalize to 0-360
          while (wind_angle < 0) wind_angle += 360;
          while (wind_angle >= 360) wind_angle -= 360;
          last_data_time = millis();
        }
      }
    }
  }
  
public:
  SignalKWindDataSource(const char* wifi_ssid, const char* wifi_pass, 
                        const char* sk_host, uint16_t sk_port)
    : ssid(wifi_ssid), password(wifi_pass), host(sk_host), port(sk_port),
      wind_speed_ms(0), wind_angle(0), connected(false), wifi_connected(false),
      last_data_time(0) {
    instance = this;
  }
  
  ~SignalKWindDataSource() {
    Serial.println("[SignalK] Stopped");
    webSocket.disconnect();
    WiFi.disconnect();
  }
  
  bool begin() override {
    Serial.printf("[SignalK] Connecting to WiFi '%s'...\n", ssid.c_str());
    
    // Connect to WiFi
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());
    
    // Wait up to 10 seconds for connection
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      attempts++;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[SignalK] WiFi connection failed");
      return false;
    }
    
    wifi_connected = true;
    Serial.printf("[SignalK] WiFi connected: %s\n", WiFi.localIP().toString().c_str());
    
    // Connect to Signal K WebSocket
    webSocket.begin(host.c_str(), port, "/signalk/v1/stream?subscribe=none");
    webSocket.onEvent(webSocketEvent);
    webSocket.setReconnectInterval(5000);
    
    Serial.printf("[SignalK] Connecting to server %s:%d\n", host.c_str(), port);
    
    return true;
  }
  
  void update() override {
    if (wifi_connected) {
      webSocket.loop();
      
      // Check for data timeout (no data for 10 seconds)
      if (connected && last_data_time > 0 && (millis() - last_data_time > 10000)) {
        Serial.println("[SignalK] Data timeout");
        connected = false;
      }
    }
  }
  
  bool isConnected() override {
    return wifi_connected && connected && (millis() - last_data_time < 10000);
  }
  
  float getWindSpeed() override {
    return wind_speed_ms;
  }
  
  float getWindAngle() override {
    return wind_angle;
  }
  
  const char* getSourceName() override {
    return "WiFi/Signal K";
  }
  
  void stop() override {
    webSocket.disconnect();
    WiFi.disconnect();
    connected = false;
    wifi_connected = false;
    Serial.println("[SignalK] Stopped");
  }
};

// Initialize static instance pointer
SignalKWindDataSource* SignalKWindDataSource::instance = nullptr;

#endif // SIGNALK_WIND_DATA_SOURCE_H
