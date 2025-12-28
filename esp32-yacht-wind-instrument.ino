/*
  Yacht Wind Direction Display - ESP32-C6 + ILI9341 + LVGL
  
  A marine wind instrument that displays apparent wind speed and direction.
  Connects to Signal K servers via WiFi for real-time data.
*/
#define VERSION "0.1.0"

#include <lvgl.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

// Wind data source abstraction
#include "WindDataSource.h"
#include "WindDataSourceManager.h"
#include "DemoWindDataSource.h"
#include "SignalKWindDataSource.h"
#include "WindConfig.h"
#include "ConfigScreen.h"

// Declare custom fonts (defined in roboto_mono_semibold_*.c)
LV_FONT_DECLARE(roboto_mono_semibold_24);
LV_FONT_DECLARE(roboto_mono_semibold_32);
LV_FONT_DECLARE(dejavu_sans_bold_24_dots);

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

// Test mode - set to 1 to run tests instead of normal display
// #define RUN_TESTS 0  // Removed to save flash space



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
static lv_obj_t *menu_btn;
static lv_obj_t *status_label;  // Connection status
static lv_obj_t *main_screen;  // Store main screen reference
static lv_point_precise_t arrow_points[2];  // Just 2 points for a line

// Wind data source
WindDataSourceManager sourceManager;
DemoWindDataSource* demoSource = nullptr;
SignalKWindDataSource* signalKSource = nullptr;
WindConfig windConfig;
ConfigScreen *configScreen = nullptr;

// Current wind data (in internal units: m/s and degrees)
float wind_speed_ms = 0.0;   // m/s
float wind_direction = 0.0;  // degrees

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
  // Get data from current source
  WindDataSource* dataSource = sourceManager.getCurrentSource();
  if (dataSource && dataSource->isConnected()) {
    wind_speed_ms = dataSource->getWindSpeed();
    wind_direction = dataSource->getWindAngle();
  }
  
  // Convert speed using configured units
  float wind_speed_display = windConfig.convertSpeed(wind_speed_ms);
  
  // Update wind speed with fixed-width formatting (right-aligned)
  char speed_buf[32];
  snprintf(speed_buf, sizeof(speed_buf), "%4.1f", wind_speed_display);
  lv_label_set_text(wind_speed_label, speed_buf);
  
  // Update units label
  lv_label_set_text(wind_speed_units_label, windConfig.getUnitsLabel());
  
  // Update wind direction angle with fixed-width formatting
  lv_label_set_text_fmt(wind_dir_label, "%3d°", (int)wind_direction);
  
  // Calculate arrow line - center at 120,120 in the 240x240 container
  int cx = 120, cy = 120;
  float rad = wind_direction * 3.14159 / 180.0;
  
  // Line from center to edge (70px length)
  arrow_points[0].x = cx;
  arrow_points[0].y = cy;
  
  arrow_points[1].x = cx + 70 * sin(rad);
  arrow_points[1].y = cy - 70 * cos(rad);
  
  lv_line_set_points(wind_arrow, arrow_points, 2);
}

// Restart data source after config change
void restartDataSource() {
  Serial.println("[Restart] Starting data source restart");
  DataSourceType sourceType = windConfig.getDataSource();
  Serial.printf("[Restart] Target source type: %d\n", sourceType);
  
  // Reset display values
  wind_speed_ms = 0.0;
  wind_direction = 0.0;
  
  if (sourceType == SOURCE_WIFI_SIGNALK) {
    Serial.println("[Restart] Switching to SignalK");
    // Clean up demo source if it exists
    if (demoSource) {
      Serial.println("[Restart] Cleaning up demo source");
      if (sourceManager.getCurrentSource() == demoSource) {
        sourceManager.switchSource(nullptr, SOURCE_DEMO);
      }
      delete demoSource;
      demoSource = nullptr;
    }
    
    // Clean up old Signal K source
    if (signalKSource) {
      Serial.println("[Restart] Cleaning up old SignalK source");
      if (sourceManager.getCurrentSource() == signalKSource) {
        sourceManager.switchSource(nullptr, SOURCE_WIFI_SIGNALK);
      }
      delete signalKSource;
    }
    
    // Create new Signal K source with updated settings
    Serial.println("[Restart] Creating new SignalK source");
    signalKSource = new SignalKWindDataSource(
      windConfig.getWifiSSID(),
      windConfig.getWifiPassword(),
      windConfig.getSignalKHost(),
      windConfig.getSignalKPort()
    );
    
    if (!sourceManager.switchSource(signalKSource, SOURCE_WIFI_SIGNALK)) {
      Serial.println("[Restart] SignalK failed, falling back to demo");
      // Fallback to demo
      demoSource = new DemoWindDataSource();
      sourceManager.switchSource(demoSource, SOURCE_DEMO);
    }
  } else {
    Serial.println("[Restart] Switching to Demo");
    // Clean up Signal K source if it exists
    if (signalKSource) {
      Serial.println("[Restart] Cleaning up SignalK source");
      if (sourceManager.getCurrentSource() == signalKSource) {
        sourceManager.switchSource(nullptr, SOURCE_WIFI_SIGNALK);
      }
      delete signalKSource;
      signalKSource = nullptr;
    }
    
    // Create demo source if needed
    if (!demoSource) {
      Serial.println("[Restart] Creating new demo source");
      demoSource = new DemoWindDataSource();
    }
    sourceManager.switchSource(demoSource, SOURCE_DEMO);
  }
  Serial.println("[Restart] Data source restart complete");
}

// Button event handlers
void menu_button_clicked(lv_event_t * e) {
  if (configScreen) {
    configScreen->show();
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\n=== Yacht Wind Display v" VERSION " ===");
  Serial.println("Initializing...");
  
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
  
  // Store reference to main screen
  main_screen = lv_screen_active();
  
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
  
  // Wind arrow (fat line from center) - on the 240x240 container
  wind_arrow = lv_line_create(compass_base);
  lv_obj_set_style_line_width(wind_arrow, 8, 0);  // Thicker line
  lv_obj_set_style_line_color(wind_arrow, lv_color_hex(0xFF0000), 0);
  lv_obj_set_style_line_rounded(wind_arrow, true, 0);  // Rounded ends
  
  // Wind speed label (left side, 32px font, right-aligned)
  wind_speed_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_speed_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(wind_speed_label, &roboto_mono_semibold_32, 0);
  lv_label_set_text(wind_speed_label, "12.5");
  lv_obj_set_width(wind_speed_label, 90);  // Fixed width for number
  lv_obj_set_style_text_align(wind_speed_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_pos(wind_speed_label, 5, 280);  // Bottom aligned
  
  // Wind speed units label (14px font, baseline aligned with speed)
  wind_speed_units_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_speed_units_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(wind_speed_units_label, &lv_font_montserrat_14, 0);
  lv_label_set_text(wind_speed_units_label, "kts");
  lv_obj_set_pos(wind_speed_units_label, 98, 298);  // Baseline aligned with 32px font
  
  // Wind direction label (right side, 32px font, right-aligned)
  wind_dir_label = lv_label_create(lv_screen_active());
  lv_obj_set_style_text_color(wind_dir_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(wind_dir_label, &roboto_mono_semibold_32, 0);
  lv_label_set_text(wind_dir_label, "45°");
  lv_obj_align(wind_dir_label, LV_ALIGN_TOP_RIGHT, -10, 280);  // Bottom aligned
  
  // Status label (top left, small)
  status_label = lv_label_create(lv_screen_active());
  lv_label_set_text(status_label, "");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x808080), 0);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 5, 2);
  
  // Menu button (three dots in top right corner)
  menu_btn = lv_button_create(lv_screen_active());
  lv_obj_set_size(menu_btn, 35, 35);
  lv_obj_align(menu_btn, LV_ALIGN_TOP_RIGHT, -2, 5);
  lv_obj_set_style_bg_opa(menu_btn, LV_OPA_TRANSP, 0);  // Transparent background
  lv_obj_set_style_border_width(menu_btn, 0, 0);  // No border
  lv_obj_set_style_shadow_width(menu_btn, 0, 0);  // No shadow
  lv_obj_add_event_cb(menu_btn, menu_button_clicked, LV_EVENT_CLICKED, NULL);
  
  lv_obj_t *menu_label = lv_label_create(menu_btn);
  lv_label_set_text(menu_label, "⋮");  // Vertical ellipsis
  lv_obj_set_style_text_color(menu_label, lv_color_black(), 0);
  lv_obj_set_style_text_font(menu_label, &dejavu_sans_bold_24_dots, 0);
  lv_obj_center(menu_label);
  
  // Load configuration and start data source
  windConfig.load();
  restartDataSource();
  
  update_wind_display();
  
  // Create config screen
  configScreen = new ConfigScreen(main_screen, &windConfig, &sourceManager, restartDataSource);
}

void loop() {
  // Update data source
  sourceManager.update();
  
  // Update display
  static unsigned long last_display_update = 0;
  if (millis() - last_display_update > 100) {
    update_wind_display();
    
    // Update status
    if (sourceManager.getCurrentType() == SOURCE_WIFI_SIGNALK) {
      if (WiFi.status() == WL_CONNECTED) {
        if (sourceManager.isConnected()) {
          lv_label_set_text(status_label, "SignalK");
        } else {
          lv_label_set_text(status_label, "WiFi OK");
        }
      } else {
        lv_label_set_text(status_label, "WiFi...");
      }
    } else {
      lv_label_set_text(status_label, "Demo");
    }
    
    last_display_update = millis();
  }
  
  lv_timer_handler();
  lv_tick_inc(5);
  delay(5);
}
