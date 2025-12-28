/*
  Yacht Wind Direction Display - ESP32-C6 + ILI9341 + LVGL
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
static lv_obj_t *wind_speed_label;
static lv_obj_t *wind_dir_label;
static lv_obj_t *wind_arrow;
static lv_obj_t *compass_base;
static lv_point_precise_t arrow_points[4];

// Simulated wind data
float wind_speed = 12.5;  // knots
int wind_direction = 45;  // degrees

void my_disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);
  
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.writePixels((uint16_t *)px_map, w * h);
  tft.endWrite();
  
  lv_display_flush_ready(display);
}

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

void draw_compass_marks(lv_obj_t *parent) {
  // Get parent dimensions
  int cx = lv_obj_get_width(parent) / 2;
  int cy = lv_obj_get_height(parent) / 2;
  
  // Draw tick marks around compass
  for (int i = 0; i < 36; i++) {
    int angle = i * 10;
    float rad = angle * 3.14159 / 180.0;
    
    // Different lengths for different marks
    int r1, r2, width;
    if (i % 9 == 0) {  // 0, 90, 180, 270 - cardinal directions
      r1 = 65;
      r2 = 85;
      width = 3;
    } else if (i % 3 == 0) {  // 30, 60, 120, etc - major marks
      r1 = 72;
      r2 = 85;
      width = 2;
    } else {  // Minor marks
      r1 = 78;
      r2 = 85;
      width = 1;
    }
    
    lv_obj_t *line = lv_line_create(parent);
    
    static lv_point_precise_t points[2];
    points[0].x = cx + r1 * sin(rad);
    points[0].y = cy - r1 * cos(rad);
    points[1].x = cx + r2 * sin(rad);
    points[1].y = cy - r2 * cos(rad);
    
    lv_line_set_points(line, points, 2);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_color(line, lv_color_white(), 0);
  }
}

void update_wind_display() {
  // Update wind speed
  lv_label_set_text_fmt(wind_speed_label, "%.1f kts", wind_speed);
  
  // Update wind direction text
  const char* cardinal[] = {"N", "NNE", "NE", "ENE", "E", "ESE", "SE", "SSE",
                           "S", "SSW", "SW", "WSW", "W", "WNW", "NW", "NNW"};
  int idx = ((wind_direction + 11) / 22) % 16;
  lv_label_set_text_fmt(wind_dir_label, "%d° %s", wind_direction, cardinal[idx]);
  
  // Calculate triangle
  int cx = 90, cy = 90;
  float rad = wind_direction * 3.14159 / 180.0;
  float perpRad = rad + 3.14159 / 2;
  
  arrow_points[0].x = cx + 60 * sin(rad);
  arrow_points[0].y = cy - 60 * cos(rad);
  
  arrow_points[1].x = cx + 4 * sin(perpRad);
  arrow_points[1].y = cy - 4 * cos(perpRad);
  
  arrow_points[2].x = cx - 4 * sin(perpRad);
  arrow_points[2].y = cy + 4 * cos(perpRad);
  
  arrow_points[3] = arrow_points[0];
  
  lv_line_set_points(wind_arrow, arrow_points, 4);
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
  
  disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(disp, my_disp_flush);
  
  static lv_color_t buf1[SCREEN_WIDTH * 10];
  lv_display_set_buffers(disp, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  
  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_touchpad_read);
  lv_indev_set_display(indev, disp);
  
  // Create UI
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_hex(0x001020), 0);
  
  // Title
  lv_obj_t *title = lv_label_create(lv_screen_active());
  lv_label_set_text(title, "WIND");
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
  
  // Compass base
  compass_base = lv_obj_create(lv_screen_active());
  lv_obj_set_size(compass_base, 180, 180);
  lv_obj_center(compass_base);
  lv_obj_set_style_bg_color(compass_base, lv_color_hex(0x001530), 0);
  lv_obj_set_style_border_color(compass_base, lv_color_hex(0x4080FF), 0);
  lv_obj_set_style_border_width(compass_base, 3, 0);
  lv_obj_set_style_radius(compass_base, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_pad_all(compass_base, 0, 0);
  lv_obj_set_style_shadow_width(compass_base, 10, 0);
  lv_obj_set_style_shadow_color(compass_base, lv_color_black(), 0);  // Remove all padding
  
  // Draw compass tick marks
  draw_compass_marks(compass_base);
  
  // Cardinal direction labels on compass
  lv_obj_t *n_label = lv_label_create(compass_base);
  lv_label_set_text(n_label, "N");
  lv_obj_set_style_text_color(n_label, lv_color_hex(0xFF4040), 0);
  lv_obj_set_style_text_font(n_label, &lv_font_montserrat_14, 0);
  lv_obj_align(n_label, LV_ALIGN_TOP_MID, 0, 8);
  
  lv_obj_t *e_label = lv_label_create(compass_base);
  lv_label_set_text(e_label, "E");
  lv_obj_set_style_text_color(e_label, lv_color_hex(0xC0C0C0), 0);
  lv_obj_align(e_label, LV_ALIGN_RIGHT_MID, -8, 0);
  
  lv_obj_t *s_label = lv_label_create(compass_base);
  lv_label_set_text(s_label, "S");
  lv_obj_set_style_text_color(s_label, lv_color_hex(0xC0C0C0), 0);
  lv_obj_align(s_label, LV_ALIGN_BOTTOM_MID, 0, -8);
  
  lv_obj_t *w_label = lv_label_create(compass_base);
  lv_label_set_text(w_label, "W");
  lv_obj_set_style_text_color(w_label, lv_color_hex(0xC0C0C0), 0);
  lv_obj_align(w_label, LV_ALIGN_LEFT_MID, 8, 0);
  
  // Wind arrow (outlined triangle)
  wind_arrow = lv_line_create(compass_base);
  lv_obj_set_style_line_width(wind_arrow, 4, 0);
  lv_obj_set_style_line_color(wind_arrow, lv_color_hex(0xFF0000), 0);
  
  // Wind speed label
  wind_speed_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_speed_label, lv_color_hex(0x00FF00), 0);
  lv_obj_align(wind_speed_label, LV_ALIGN_BOTTOM_MID, 0, -60);
  
  // Wind direction label
  wind_dir_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_dir_label, lv_color_white(), 0);
  lv_obj_align(wind_dir_label, LV_ALIGN_BOTTOM_MID, 0, -30);
  
  update_wind_display();
  
  Serial.println("Wind display initialized");
}

void loop() {
  // Simulate changing wind (for demo)
  static unsigned long last_update = 0;
  if (millis() - last_update > 200) {  // Update every 200ms (5 times per second)
    wind_direction = (wind_direction + 1) % 360;
    wind_speed = 10.0 + random(0, 50) / 10.0;
    update_wind_display();
    last_update = millis();
  }
  
  lv_timer_handler();
  lv_tick_inc(5);
  delay(5);
}
