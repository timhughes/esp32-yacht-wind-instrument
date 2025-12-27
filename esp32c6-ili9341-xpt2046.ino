/*
  ILI9341 + XPT2046 Touch Example for ESP32-C6
  Using separate SPI pins for touch
*/

#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// Display pins
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  4
#define TFT_MOSI 23
#define TFT_SCLK 18

// Touch pins (separate SPI)
#define TOUCH_CS   6
#define TOUCH_CLK  5
#define TOUCH_MOSI 7
#define TOUCH_MISO 8
#define TOUCH_IRQ  9

// Create display instance
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

// Create touch instance
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// Use FSPI for touch (ESP32-C6 only has FSPI)
SPIClass touchSPI(FSPI);

void setup() {
  Serial.begin(115200);
  
  // Initialize display SPI
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  
  // Initialize display
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  
  // Initialize touch with separate SPI
  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(touchSPI);
  touch.setRotation(2);  // 180 degrees
  
  Serial.println("Display and touch initialized");
}

void loop() {
  if (touch.tirqTouched() && touch.touched()) {
    TS_Point p = touch.getPoint();
    
    // Map touch coordinates
    int x = map(p.x, 200, 3800, 0, 240);
    int y = map(p.y, 200, 3800, 0, 320);
    
    // Bounds check
    if (x >= 0 && x < 240 && y >= 0 && y < 320) {
      tft.fillCircle(x, y, 5, ILI9341_RED);
      
      Serial.print("Touch: ");
      Serial.print(x);
      Serial.print(", ");
      Serial.println(y);
    }
    
    delay(100);
  }
}
