/*
  Yacht Wind Direction Display - ESP32-C6 + ILI9341 + LVGL
*/

#include <lvgl.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// Declare custom fonts (defined in roboto_mono_semibold_*.c)
LV_FONT_DECLARE(roboto_mono_semibold_24);
LV_FONT_DECLARE(roboto_mono_semibold_28);
LV_FONT_DECLARE(roboto_mono_semibold_32);

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
static lv_obj_t *wind_speed_units_label;
static lv_obj_t *wind_dir_label;
static lv_obj_t *wind_arrow;
static lv_obj_t *compass_base;
static lv_obj_t *units_btn;
static lv_obj_t *menu_btn;
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

void draw_compass_marks(lv_obj_t *parent, lv_obj_t *circle) {
  // Circle is at (30, 30) within parent, with center at (90, 90) relative to circle
  // But we need absolute positions within parent, so center is at (120, 120)
  int cx = 120;
  int cy = 120;
  int circle_radius = 90;
  
  // Draw port (red) and starboard (green) sectors between 20° and 60°
  // With rotation=270, angle 0 is at top (north)
  // Starboard is 20°-60° clockwise from north
  // Port is 300°-340° (or -60° to -20°)
  
  // Starboard (green) sector from 20° to 60°
  lv_obj_t *stbd_arc = lv_arc_create(parent);
  lv_obj_set_size(stbd_arc, 180, 180);
  lv_obj_set_pos(stbd_arc, 30, 30);
  lv_arc_set_bg_angles(stbd_arc, 20, 60);
  lv_arc_set_angles(stbd_arc, 20, 60);  // Set the indicator angles
  lv_arc_set_rotation(stbd_arc, 270);
  lv_obj_set_style_arc_width(stbd_arc, 12, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(stbd_arc, lv_color_hex(0x00FF00), LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(stbd_arc, LV_OPA_80, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(stbd_arc, false, LV_PART_INDICATOR);  // Flat ends
  lv_obj_remove_style(stbd_arc, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(stbd_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(stbd_arc, 0, LV_PART_MAIN);
  
  // Port (red) sector from 300° to 340°
  lv_obj_t *port_arc = lv_arc_create(parent);
  lv_obj_set_size(port_arc, 180, 180);
  lv_obj_set_pos(port_arc, 30, 30);
  lv_arc_set_bg_angles(port_arc, 300, 340);
  lv_arc_set_angles(port_arc, 300, 340);  // Set the indicator angles
  lv_arc_set_rotation(port_arc, 270);
  lv_obj_set_style_arc_width(port_arc, 12, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(port_arc, lv_color_hex(0xFF0000), LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(port_arc, LV_OPA_80, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(port_arc, false, LV_PART_INDICATOR);  // Flat ends
  lv_obj_remove_style(port_arc, NULL, LV_PART_KNOB);
  lv_obj_clear_flag(port_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(port_arc, 0, LV_PART_MAIN);
  
  // Draw tick marks - major every 30°, minor every 10°
  for (int i = 0; i < 36; i++) {
    int angle = i * 10;
    float rad = angle * 3.14159 / 180.0;
    
    int r1, r2, width;
    if (i % 3 == 0) {  // Every 30° - major ticks
      r1 = 72;   // Inner radius (90 - 18)
      r2 = 90;   // Outer radius
      width = 3;
    } else {  // Every 10° - minor ticks
      r1 = 78;   // Inner radius (90 - 12)
      r2 = 90;   // Outer radius
      width = 1;
    }
    
    lv_obj_t *line = lv_line_create(parent);
    
    // Allocate new points for each line (not static!)
    lv_point_precise_t *line_points = (lv_point_precise_t*)malloc(2 * sizeof(lv_point_precise_t));
    line_points[0].x = cx + r1 * sin(rad);
    line_points[0].y = cy - r1 * cos(rad);
    line_points[1].x = cx + r2 * sin(rad);
    line_points[1].y = cy - r2 * cos(rad);
    
    lv_line_set_points(line, line_points, 2);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_color(line, lv_color_black(), 0);
  }
  
  // Draw angle labels at radius 105 (15px outside circle edge)
  // Formula: x = cx + label_radius*sin(θ), y = cy - label_radius*cos(θ)
  // Don't add half_font since we're centering with label_h/2
  int label_radius = 105;
  
  int angles[] = {0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330};
  const char* labels[] = {"0", "30", "60", "90", "120", "150", "180", "150", "120", "90", "60", "30"};
  
  for (int i = 0; i < 12; i++) {
    float rad = angles[i] * 3.14159 / 180.0;
    float x = cx + label_radius * sin(rad);
    float y = cy - label_radius * cos(rad) + 2;  // Add 2px to move labels down
    
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, labels[i]);
    lv_obj_set_style_text_color(label, lv_color_black(), 0);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
    
    // Get label size to center it properly
    lv_obj_update_layout(label);
    int label_w = lv_obj_get_width(label);
    int label_h = lv_obj_get_height(label);
    
    lv_obj_set_pos(label, x - label_w/2, y - label_h/2);
  }
}

void update_wind_display() {
  // Update wind speed with fixed-width formatting (right-aligned)
  char speed_buf[32];
  snprintf(speed_buf, sizeof(speed_buf), "%4.1f", wind_speed);
  lv_label_set_text(wind_speed_label, speed_buf);
  
  // Update wind direction angle with fixed-width formatting
  lv_label_set_text_fmt(wind_dir_label, "%3d°", wind_direction);
  
  // Calculate arrow triangle - center at 120,120 in the 240x240 container
  int cx = 120, cy = 120;
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
  
  // Create UI - white background
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_white(), 0);
  
  // Compass base - white with black border, positioned higher (center at y=120)
  // Make it larger (240x240) to accommodate labels outside the 180px circle
  compass_base = lv_obj_create(lv_screen_active());
  lv_obj_set_size(compass_base, 240, 240);
  lv_obj_set_pos(compass_base, 0, 0);  // Full width, starting at top
  lv_obj_set_style_bg_color(compass_base, lv_color_white(), 0);
  lv_obj_set_style_bg_opa(compass_base, LV_OPA_TRANSP, 0);  // Transparent background
  lv_obj_set_style_border_width(compass_base, 0, 0);  // No border on container
  lv_obj_set_style_pad_all(compass_base, 0, 0);
  lv_obj_set_style_clip_corner(compass_base, false, 0);  // Don't clip children
  lv_obj_clear_flag(compass_base, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_add_flag(compass_base, LV_OBJ_FLAG_OVERFLOW_VISIBLE);  // Allow overflow
  
  // Draw the actual compass circle inside - just the border, transparent bg
  lv_obj_t *circle = lv_obj_create(compass_base);
  lv_obj_set_size(circle, 180, 180);
  lv_obj_set_pos(circle, 30, 30);  // Center at (120, 120) within the 240x240 container
  lv_obj_set_style_bg_opa(circle, LV_OPA_TRANSP, 0);  // Transparent so ticks show through
  lv_obj_set_style_border_color(circle, lv_color_black(), 0);
  lv_obj_set_style_border_width(circle, 2, 0);
  lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_pad_all(circle, 0, 0);
  lv_obj_set_style_clip_corner(circle, false, 0);
  lv_obj_add_flag(circle, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
  
  // Draw compass tick marks and labels AFTER the circle so they're on top
  draw_compass_marks(compass_base, circle);
  
  // Wind arrow (outlined triangle) - now on the 240x240 container
  wind_arrow = lv_line_create(compass_base);
  lv_obj_set_style_line_width(wind_arrow, 4, 0);
  lv_obj_set_style_line_color(wind_arrow, lv_color_hex(0xFF0000), 0);
  
  // Wind speed label (left side, 28px font, right-aligned)
  wind_speed_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_speed_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(wind_speed_label, &roboto_mono_semibold_28, 0);
  lv_label_set_text(wind_speed_label, "12.5");
  lv_obj_set_width(wind_speed_label, 75);  // Fixed width for number only
  lv_obj_set_style_text_align(wind_speed_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(wind_speed_label, 5, 242);  // Adjusted position
  
  // Wind speed units label (14px font, positioned to align right edge with button at x=110)
  wind_speed_units_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_speed_units_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(wind_speed_units_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(wind_speed_units_label, "kts");
  lv_obj_set_pos(wind_speed_units_label, 82, 252);  // Adjusted for baseline alignment
  
  // Wind direction label (right side, 28px font, right-aligned to menu button at x=230)
  wind_dir_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_dir_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(wind_dir_label, &roboto_mono_semibold_28, 0);
  lv_label_set_text(wind_dir_label, "45°");
  lv_obj_align(wind_dir_label, LV_ALIGN_TOP_RIGHT, -10, 242);  // Adjusted position
  
  // Units button (bottom left)
  units_btn = lv_button_create(lv_screen_active());
  lv_obj_set_size(units_btn, 100, 30);
  lv_obj_set_pos(units_btn, 10, 280);
  lv_obj_set_style_bg_color(units_btn, lv_color_white(), 0);
  lv_obj_set_style_border_color(units_btn, lv_color_black(), 0);
  lv_obj_set_style_border_width(units_btn, 2, 0);
  
  lv_obj_t *units_label = lv_label_create(units_btn);
  lv_label_set_text(units_label, "UNITS");
  lv_obj_set_style_text_color(units_label, lv_color_black(), 0);
  lv_obj_center(units_label);
  
  // Menu button (bottom right)
  menu_btn = lv_button_create(lv_screen_active());
  lv_obj_set_size(menu_btn, 100, 30);
  lv_obj_set_pos(menu_btn, 130, 280);
  lv_obj_set_style_bg_color(menu_btn, lv_color_white(), 0);
  lv_obj_set_style_border_color(menu_btn, lv_color_black(), 0);
  lv_obj_set_style_border_width(menu_btn, 2, 0);
  
  lv_obj_t *menu_label = lv_label_create(menu_btn);
  lv_label_set_text(menu_label, "MENU");
  lv_obj_set_style_text_color(menu_label, lv_color_black(), 0);
  lv_obj_center(menu_label);
  
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
