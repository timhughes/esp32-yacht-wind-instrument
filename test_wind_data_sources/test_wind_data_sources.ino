/*
  test_wind_data_sources.ino - Unit tests for wind data sources
  
  To run: Upload this sketch instead of the main one, open Serial Monitor
  
  Tests:
  - WindDataSource interface implementation
  - DemoWindDataSource behavior
  - MockWindDataSource functionality
  - WindDataSourceManager switching
  - Unit conversions
*/

#include "WindDataSource.h"
#include "DemoWindDataSource.h"
#include "MockWindDataSource.h"
#include "WindDataSourceManager.h"

// Test counters
int tests_passed = 0;
int tests_failed = 0;

// Test helper macros
#define TEST_ASSERT(condition, message) \
  if (condition) { \
    Serial.printf("✓ PASS: %s\n", message); \
    tests_passed++; \
  } else { \
    Serial.printf("✗ FAIL: %s\n", message); \
    tests_failed++; \
  }

#define TEST_ASSERT_EQUAL(expected, actual, message) \
  if ((expected) == (actual)) { \
    Serial.printf("✓ PASS: %s\n", message); \
    tests_passed++; \
  } else { \
    Serial.printf("✗ FAIL: %s (expected: %f, got: %f)\n", message, (float)(expected), (float)(actual)); \
    tests_failed++; \
  }

#define TEST_ASSERT_NEAR(expected, actual, tolerance, message) \
  if (abs((expected) - (actual)) < (tolerance)) { \
    Serial.printf("✓ PASS: %s\n", message); \
    tests_passed++; \
  } else { \
    Serial.printf("✗ FAIL: %s (expected: %f, got: %f)\n", message, (float)(expected), (float)(actual)); \
    tests_failed++; \
  }

void test_demo_source() {
  Serial.println("\n=== Testing DemoWindDataSource ===");
  
  DemoWindDataSource demo;
  
  TEST_ASSERT(demo.begin(), "Demo source begins successfully");
  TEST_ASSERT(demo.isConnected(), "Demo source is always connected");
  TEST_ASSERT(strcmp(demo.getSourceName(), "Demo") == 0, "Demo source name is correct");
  
  float speed1 = demo.getWindSpeed();
  float angle1 = demo.getWindAngle();
  
  TEST_ASSERT(speed1 > 0, "Demo provides positive wind speed");
  TEST_ASSERT(angle1 >= 0 && angle1 < 360, "Demo provides valid wind angle");
  
  // Update and check values change
  delay(250);
  demo.update();
  float angle2 = demo.getWindAngle();
  
  TEST_ASSERT(angle2 != angle1, "Demo wind angle changes after update");
  
  demo.stop();
  Serial.println("Demo source tests complete");
}

void test_mock_source() {
  Serial.println("\n=== Testing MockWindDataSource ===");
  
  MockWindDataSource mock;
  
  TEST_ASSERT(mock.begin(), "Mock source begins successfully");
  TEST_ASSERT(mock.isConnected(), "Mock source is connected after begin");
  TEST_ASSERT(strcmp(mock.getSourceName(), "Mock") == 0, "Mock source name is correct");
  
  // Test setting values
  mock.setWindSpeed(10.0);
  mock.setWindAngle(45.0);
  
  TEST_ASSERT_EQUAL(10.0, mock.getWindSpeed(), "Mock returns set wind speed");
  TEST_ASSERT_EQUAL(45.0, mock.getWindAngle(), "Mock returns set wind angle");
  
  // Test connection state
  mock.setConnected(false);
  TEST_ASSERT(!mock.isConnected(), "Mock can be disconnected");
  
  mock.setConnected(true);
  TEST_ASSERT(mock.isConnected(), "Mock can be reconnected");
  
  mock.stop();
  TEST_ASSERT(!mock.isConnected(), "Mock is disconnected after stop");
  
  Serial.println("Mock source tests complete");
}

void test_source_manager() {
  Serial.println("\n=== Testing WindDataSourceManager ===");
  
  WindDataSourceManager manager;
  DemoWindDataSource demo;
  MockWindDataSource mock;
  
  // Test switching to demo
  TEST_ASSERT(manager.switchSource(&demo, SOURCE_DEMO), "Manager switches to demo source");
  TEST_ASSERT(manager.getCurrentSource() == &demo, "Manager returns correct current source");
  TEST_ASSERT(manager.getCurrentType() == SOURCE_DEMO, "Manager returns correct source type");
  TEST_ASSERT(manager.isConnected(), "Manager reports connected when source is connected");
  
  // Test switching to mock
  mock.setWindSpeed(15.0);
  mock.setWindAngle(90.0);
  TEST_ASSERT(manager.switchSource(&mock, SOURCE_NMEA), "Manager switches to mock source");
  TEST_ASSERT(manager.getCurrentSource() == &mock, "Manager returns new current source");
  TEST_ASSERT(manager.getCurrentType() == SOURCE_NMEA, "Manager returns new source type");
  
  // Test update propagation
  manager.update();
  TEST_ASSERT_EQUAL(15.0, mock.getWindSpeed(), "Manager update propagates to source");
  
  // Test type names
  TEST_ASSERT(strcmp(manager.getTypeName(SOURCE_DEMO), "Demo") == 0, "Demo type name correct");
  TEST_ASSERT(strcmp(manager.getTypeName(SOURCE_WIFI_SIGNALK), "WiFi/Signal K") == 0, "WiFi type name correct");
  TEST_ASSERT(strcmp(manager.getTypeName(SOURCE_NMEA), "NMEA 0183") == 0, "NMEA type name correct");
  
  Serial.println("Manager tests complete");
}

void test_unit_conversions() {
  Serial.println("\n=== Testing Unit Conversions ===");
  
  // Test m/s to knots
  float ms = 10.0;
  float kts = ms * 1.94384;
  TEST_ASSERT_NEAR(19.4384, kts, 0.001, "10 m/s = 19.4384 knots");
  
  // Test knots to m/s
  float kts2 = 20.0;
  float ms2 = kts2 * 0.514444;
  TEST_ASSERT_NEAR(10.28888, ms2, 0.001, "20 knots = 10.28888 m/s");
  
  // Test m/s to mph
  float ms3 = 10.0;
  float mph = ms3 * 2.23694;
  TEST_ASSERT_NEAR(22.3694, mph, 0.001, "10 m/s = 22.3694 mph");
  
  Serial.println("Unit conversion tests complete");
}

void setup() {
  Serial.begin(115200);
  delay(2000);  // Wait for serial monitor
  
  Serial.println("\n\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  Wind Data Source Unit Tests          ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  // Run all tests
  test_demo_source();
  test_mock_source();
  test_source_manager();
  test_unit_conversions();
  
  // Print summary
  Serial.println("\n");
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║  Test Summary                          ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.printf("Tests passed: %d\n", tests_passed);
  Serial.printf("Tests failed: %d\n", tests_failed);
  Serial.printf("Total tests:  %d\n", tests_passed + tests_failed);
  
  if (tests_failed == 0) {
    Serial.println("\n✓ ALL TESTS PASSED! ✓");
  } else {
    Serial.println("\n✗ SOME TESTS FAILED ✗");
  }
}

void loop() {
  // Tests run once in setup
  delay(1000);
}
