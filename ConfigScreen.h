/*
  ConfigScreen.h - Configuration UI screen for wind display
  
  Provides a touch-based menu for changing settings.
*/

#ifndef CONFIG_SCREEN_H
#define CONFIG_SCREEN_H

#include <lvgl.h>
#include "WindConfig.h"
#include "WindDataSourceManager.h"

class ConfigScreen {
private:
  lv_obj_t *screen;
  lv_obj_t *main_screen;
  WindConfig *config;
  WindDataSourceManager *sourceManager;
  bool isVisible;
  void (*restartCallback)();  // Callback to restart data source
  
  // UI elements
  lv_obj_t *title_label;
  lv_obj_t *source_dropdown;
  lv_obj_t *units_dropdown;
  lv_obj_t *wifi_ssid_input;
  lv_obj_t *wifi_pass_input;
  lv_obj_t *signalk_host_input;
  lv_obj_t *signalk_port_input;
  lv_obj_t *save_btn;
  lv_obj_t *cancel_btn;
  lv_obj_t *keyboard;  // On-screen keyboard
  
  static void textarea_focused(lv_event_t *e) {
    ConfigScreen *self = (ConfigScreen*)lv_event_get_user_data(e);
    lv_obj_t *ta = (lv_obj_t*)lv_event_get_target(e);
    if (self->keyboard) {
      lv_keyboard_set_textarea(self->keyboard, ta);
      lv_obj_clear_flag(self->keyboard, LV_OBJ_FLAG_HIDDEN);
    }
  }
  
  static void keyboard_ready(lv_event_t *e) {
    ConfigScreen *self = (ConfigScreen*)lv_event_get_user_data(e);
    if (self->keyboard) {
      lv_obj_add_flag(self->keyboard, LV_OBJ_FLAG_HIDDEN);
      lv_keyboard_set_textarea(self->keyboard, NULL);
    }
  }
  
  static void save_clicked(lv_event_t *e) {
    ConfigScreen *self = (ConfigScreen*)lv_event_get_user_data(e);
    self->saveAndClose();
  }
  
  static void cancel_clicked(lv_event_t *e) {
    ConfigScreen *self = (ConfigScreen*)lv_event_get_user_data(e);
    self->hide();
  }
  
  void saveAndClose() {
    // Get selected data source
    uint16_t source_idx = lv_dropdown_get_selected(source_dropdown);
    DataSourceType sources[] = {SOURCE_DEMO, SOURCE_WIFI_SIGNALK, SOURCE_NMEA};
    config->setDataSource(sources[source_idx]);
    
    // Get selected units
    uint16_t units_idx = lv_dropdown_get_selected(units_dropdown);
    config->setUnits((WindUnits)units_idx);
    
    // Get WiFi settings
    const char* ssid = lv_textarea_get_text(wifi_ssid_input);
    const char* pass = lv_textarea_get_text(wifi_pass_input);
    config->setWifiSSID(ssid ? ssid : "");
    config->setWifiPassword(pass ? pass : "");
    
    // Get Signal K settings
    config->setSignalKHost(lv_textarea_get_text(signalk_host_input));
    const char *port_str = lv_textarea_get_text(signalk_port_input);
    config->setSignalKPort(atoi(port_str));
    
    hide();
    
    config->save();
    
    // Restart data source with new settings
    if (restartCallback) {
      restartCallback();
    }
  }
  
public:
  ConfigScreen(lv_obj_t *main_scr, WindConfig *cfg, WindDataSourceManager *mgr, void (*restart)() = nullptr) 
    : main_screen(main_scr), config(cfg), sourceManager(mgr), restartCallback(restart), isVisible(false), screen(nullptr), keyboard(nullptr) {}
  
  void create() {
    // Create config screen
    screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_white(), 0);
    
    // Title (fixed at top)
    title_label = lv_label_create(screen);
    lv_label_set_text(title_label, "Configuration");
    lv_obj_set_style_text_color(title_label, lv_color_black(), 0);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create scrollable container for settings
    lv_obj_t *scroll_container = lv_obj_create(screen);
    lv_obj_set_size(scroll_container, 240, 230);  // Leave room for title and buttons
    lv_obj_set_pos(scroll_container, 0, 40);
    lv_obj_set_style_bg_color(scroll_container, lv_color_white(), 0);
    lv_obj_set_style_border_width(scroll_container, 0, 0);
    lv_obj_set_style_pad_all(scroll_container, 10, 0);
    lv_obj_set_scroll_dir(scroll_container, LV_DIR_VER);  // Vertical scrolling only
    
    // Data Source dropdown
    lv_obj_t *source_label = lv_label_create(scroll_container);
    lv_label_set_text(source_label, "Data Source:");
    lv_obj_set_style_text_color(source_label, lv_color_black(), 0);
    lv_obj_set_pos(source_label, 0, 0);
    
    source_dropdown = lv_dropdown_create(scroll_container);
    lv_dropdown_set_options(source_dropdown, "Demo\nWiFi/Signal K\nNMEA 0183");
    lv_obj_set_width(source_dropdown, 200);
    lv_obj_set_pos(source_dropdown, 0, 25);
    
    // Units dropdown
    lv_obj_t *units_label = lv_label_create(scroll_container);
    lv_label_set_text(units_label, "Speed Units:");
    lv_obj_set_style_text_color(units_label, lv_color_black(), 0);
    lv_obj_set_pos(units_label, 0, 65);
    
    units_dropdown = lv_dropdown_create(scroll_container);
    lv_dropdown_set_options(units_dropdown, "Knots\nm/s\nMPH\nkm/h");
    lv_obj_set_width(units_dropdown, 200);
    lv_obj_set_pos(units_dropdown, 0, 90);
    
    // WiFi SSID
    lv_obj_t *ssid_label = lv_label_create(scroll_container);
    lv_label_set_text(ssid_label, "WiFi SSID:");
    lv_obj_set_style_text_color(ssid_label, lv_color_black(), 0);
    lv_obj_set_pos(ssid_label, 0, 130);
    
    wifi_ssid_input = lv_textarea_create(scroll_container);
    lv_obj_set_size(wifi_ssid_input, 200, 30);
    lv_obj_set_pos(wifi_ssid_input, 0, 150);
    lv_textarea_set_one_line(wifi_ssid_input, true);
    lv_textarea_set_max_length(wifi_ssid_input, 31);
    lv_textarea_set_placeholder_text(wifi_ssid_input, "WiFi Network");
    lv_obj_add_event_cb(wifi_ssid_input, textarea_focused, LV_EVENT_FOCUSED, this);
    
    // WiFi Password
    lv_obj_t *pass_label = lv_label_create(scroll_container);
    lv_label_set_text(pass_label, "WiFi Password:");
    lv_obj_set_style_text_color(pass_label, lv_color_black(), 0);
    lv_obj_set_pos(pass_label, 0, 185);
    
    wifi_pass_input = lv_textarea_create(scroll_container);
    lv_obj_set_size(wifi_pass_input, 200, 30);
    lv_obj_set_pos(wifi_pass_input, 0, 205);
    lv_textarea_set_one_line(wifi_pass_input, true);
    lv_textarea_set_max_length(wifi_pass_input, 63);
    lv_textarea_set_placeholder_text(wifi_pass_input, "Password");
    lv_textarea_set_password_mode(wifi_pass_input, true);  // Show as dots
    lv_obj_add_event_cb(wifi_pass_input, textarea_focused, LV_EVENT_FOCUSED, this);
    
    // Signal K Host
    lv_obj_t *host_label = lv_label_create(scroll_container);
    lv_label_set_text(host_label, "Signal K Host:");
    lv_obj_set_style_text_color(host_label, lv_color_black(), 0);
    lv_obj_set_pos(host_label, 0, 240);
    
    signalk_host_input = lv_textarea_create(scroll_container);
    lv_obj_set_size(signalk_host_input, 200, 30);
    lv_obj_set_pos(signalk_host_input, 0, 260);
    lv_textarea_set_one_line(signalk_host_input, true);
    lv_textarea_set_placeholder_text(signalk_host_input, "192.168.1.100");
    lv_obj_add_event_cb(signalk_host_input, textarea_focused, LV_EVENT_FOCUSED, this);
    
    // Signal K Port
    lv_obj_t *port_label = lv_label_create(scroll_container);
    lv_label_set_text(port_label, "Port:");
    lv_obj_set_style_text_color(port_label, lv_color_black(), 0);
    lv_obj_set_pos(port_label, 0, 295);
    
    signalk_port_input = lv_textarea_create(scroll_container);
    lv_obj_set_size(signalk_port_input, 80, 30);
    lv_obj_set_pos(signalk_port_input, 0, 315);
    lv_textarea_set_one_line(signalk_port_input, true);
    lv_textarea_set_max_length(signalk_port_input, 5);
    lv_textarea_set_placeholder_text(signalk_port_input, "3000");
    lv_obj_add_event_cb(signalk_port_input, textarea_focused, LV_EVENT_FOCUSED, this);
    
    // Create keyboard (hidden by default)
    keyboard = lv_keyboard_create(screen);
    lv_obj_set_size(keyboard, 240, 120);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_event_cb(keyboard, keyboard_ready, LV_EVENT_READY, this);
    lv_obj_add_event_cb(keyboard, keyboard_ready, LV_EVENT_CANCEL, this);
    
    // Save button
    save_btn = lv_button_create(screen);
    lv_obj_set_size(save_btn, 100, 35);
    lv_obj_align(save_btn, LV_ALIGN_BOTTOM_LEFT, 10, -5);
    lv_obj_set_style_bg_color(save_btn, lv_color_hex(0x00AA00), 0);
    lv_obj_add_event_cb(save_btn, save_clicked, LV_EVENT_CLICKED, this);
    
    lv_obj_t *save_label = lv_label_create(save_btn);
    lv_label_set_text(save_label, "SAVE");
    lv_obj_set_style_text_color(save_label, lv_color_white(), 0);
    lv_obj_center(save_label);
    
    // Cancel button
    cancel_btn = lv_button_create(screen);
    lv_obj_set_size(cancel_btn, 100, 35);
    lv_obj_align(cancel_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -5);
    lv_obj_set_style_bg_color(cancel_btn, lv_color_hex(0xAA0000), 0);
    lv_obj_add_event_cb(cancel_btn, cancel_clicked, LV_EVENT_CLICKED, this);
    
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "CANCEL");
    lv_obj_set_style_text_color(cancel_label, lv_color_white(), 0);
    lv_obj_center(cancel_label);
  }
  
  void show() {
    if (!screen) create();
    
    // Load current values
    lv_dropdown_set_selected(source_dropdown, config->getDataSource());
    lv_dropdown_set_selected(units_dropdown, config->getUnits());
    lv_textarea_set_text(wifi_ssid_input, config->getWifiSSID());
    lv_textarea_set_text(wifi_pass_input, config->getWifiPassword());
    lv_textarea_set_text(signalk_host_input, config->getSignalKHost());
    
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", config->getSignalKPort());
    lv_textarea_set_text(signalk_port_input, port_str);
    
    lv_screen_load(screen);
    isVisible = true;
  }
  
  void hide() {
    if (main_screen) {
      lv_screen_load(main_screen);
    }
    isVisible = false;
    
    // Don't delete the screen, just hide it
    // This prevents issues with screen switching
  }
  
  bool visible() { return isVisible; }
};

#endif // CONFIG_SCREEN_H
