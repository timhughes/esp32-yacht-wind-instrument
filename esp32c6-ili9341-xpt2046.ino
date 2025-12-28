/*
  LVGL v9 UI Example for ESP32-C6 + ILI9341 + XPT2046
*/

#include <lvgl.h>
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

// Touch pins
#define TOUCH_CS   6
#define TOUCH_CLK  5
#define TOUCH_MOSI 7
#define TOUCH_MISO 8
#define TOUCH_IRQ  9

#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 320

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);
SPIClass touchSPI(FSPI);

static lv_display_t *disp;

// Display flush callback
void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((uint16_t *)px_map, w * h);
  tft.endWrite();
  
  lv_display_flush_ready(display);
}

// Touch read callback
void my_touchpad_read(lv_indev_t *indev_drv, lv_indev_data_t *data) {
  if (touch.touched()) {
    TS_Point p = touch.getPoint();
    data->point.x = map(p.x, 400, 3700, 0, SCREEN_WIDTH);
    data->point.y = map(p.y, 400, 3700, 0, SCREEN_HEIGHT);
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

// Button click event
void btn_event_cb(lv_event_t *e) {
  lv_obj_t *label = (lv_obj_t *)lv_event_get_user_data(e);
  static int count = 0;
  count++;
  lv_label_set_text_fmt(label, "Clicked: %d", count);
  Serial.print("Button clicked! Count: ");
  Serial.println(count);
}

void setup() {
  Serial.begin(115200);
  
  // Initialize display
  SPI.begin(TFT_SCLK, -1, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(0);
  tft.fillScreen(ILI9341_BLACK);
  
  // Initialize touch
  touchSPI.begin(TOUCH_CLK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(touchSPI);
  touch.setRotation(2);
  
  // Initialize LVGL
  lv_init();
  
  // Create display
  disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);
  
  // Allocate draw buffer
  static lv_color_t buf1[SCREEN_WIDTH * 10];
  lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  
  // Create touch input and link to display
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_display(indev, disp);
  
  // Create UI
  lv_obj_t *label = lv_label_create(lv_screen_active());
  lv_label_set_text(label, "ESP32-C6 LVGL");
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);
  
  lv_obj_t *counter_label = lv_label_create(lv_screen_active());
  lv_label_set_text(counter_label, "Clicked: 0");
  lv_obj_align(counter_label, LV_ALIGN_CENTER, 0, -20);
  
  lv_obj_t *btn = lv_button_create(lv_screen_active());
  lv_obj_set_size(btn, 120, 50);
  lv_obj_align(btn, LV_ALIGN_CENTER, 0, 40);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, counter_label);
  
  lv_obj_t *btn_label = lv_label_create(btn);
  lv_label_set_text(btn_label, "Click Me!");
  lv_obj_center(btn_label);
  
  Serial.println("LVGL initialized");
}

void loop() {
  lv_timer_handler();
  lv_tick_inc(5);
  delay(5);
}
