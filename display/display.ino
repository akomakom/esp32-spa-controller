#define GRAPHICS_ENABLE

// debug:
#include "esp_task_wdt.h"

#include "core.h"
/*******************************************************************************
 * Please configure graphics in gfx.h
 ******************************************************************************/
#ifdef GRAPHICS_ENABLE
#include "gfx.h"
#endif
#include <vector>
#include "ESPNowUtils.h"

// Card colours for the dark theme (0xRRGGBB): background, border, name/value text.
struct CardColors { uint32_t bg; uint32_t border; uint32_t nameC; uint32_t valueC; };
/*******************************************************************************
 * Please configure the touch panel in touch.h
 ******************************************************************************/

#ifdef GRAPHICS_ENABLE

const char *        MODE_MAPPING[] = { "OFF", "ON", "HIGH" };
const lv_palette_t  MODE_COLORS[]  = { LV_PALETTE_BLUE, LV_PALETTE_RED, LV_PALETTE_ORANGE };

// Store timestamps to check which task is stuck
#define WATCHDOG_TIMEOUT 1  // Timeout in seconds
volatile unsigned long lastTask1Time = 0;
volatile unsigned long lastTask2Stage = 0;

lv_obj_t * mainScreen = NULL;

// These get passed as pointers to lv events
int8_t sensorBasedControlPanelIncrement = 1;
int8_t sensorBasedControlPanelDecrement = -1;
int8_t sensorBasedControlPanelReset = 0;
int8_t sensorBasedControlPanelClose = 2;

lv_obj_t * sensorBasedControlPanel = NULL;
lv_obj_t * sensorBasedControlSetpointLabel = NULL;
lv_obj_t * sensorBasedControlDescription = NULL;
u_int8_t   sensorBasedControlSetpointValue = 0;

lv_style_t style;
lv_style_t styleNoPadding;
lv_style_t stylePadding;
lv_style_t styleSmallFont;
lv_style_t styleHugeFont;
lv_style_t styleArcMain;
lv_style_t styleArcIndicator;
lv_style_t styleArcKnob;

lv_obj_t * statusLabel = NULL;
lv_obj_t * bannerLabel = NULL;
lv_obj_t * timeLabel = NULL;
struct_status_server * lastServerStatus;
std::vector<lv_obj_t *> controlButtons;
std::vector<struct_status_control *> controlStatuses;
lv_obj_t * controlButtonContainer = NULL;

// Optimistic press feedback: acknowledge a press immediately (white border) until
// the controller confirms via a status update whose value matches, or we time out.
#define MAX_CONTROLS 16
#define PRESS_PENDING_TIMEOUT 3000
struct PressPending { bool active = false; unsigned long since = 0; u_int8_t expected = 0; };
PressPending pressPending[MAX_CONTROLS];
unsigned long statusDisplayTime = 0;

// how long to keep last status message on the screen
#define STATUS_DISPLAY_TIMEOUT 5000
// how long to show the network activity symbols after network activity
#define NET_ACTIVITY_SYMBOL_AGE 100
#endif


// Macro definition
#define TRACE(message) Serial.print("TRACE: ") ; Serial.println(message)

/**
 * Accepts printf-like args and prints to both
 * Serial and GUI (when appropriate)
 * @param messageFormat
 * @param ...
 */
void showStatusMessage(const char *messageFormat, ...) {
    va_list args;
    va_start(args, messageFormat);
    static char msg[100];
    vsprintf(msg, messageFormat, args);
    Serial.printf("Status Message: %s\n", msg);
#ifdef GRAPHICS_ENABLE
    if (statusLabel != NULL) { // already initialized
        lv_label_set_text(statusLabel, msg);
    }
    statusDisplayTime = millis();
#endif
    va_end(args);
}

void clearStatusMessage(unsigned long frequency) {

#ifdef GRAPHICS_ENABLE
  static unsigned long last_update = 0;
    if ((millis() - last_update) > frequency) {
      last_update = millis();
      if ((statusDisplayTime + STATUS_DISPLAY_TIMEOUT) < millis()) {
        if (statusLabel != NULL) { // already initialized
            lv_label_set_text(statusLabel, "");
        }
//        Serial.printf("Clearing status message because %d + TIMEOUT < %d\n", statusDisplayTime, millis());
        statusDisplayTime = LONG_MAX; // max long, don't keep clearing it.
      }
    }
#endif
}


void initUI() {

    mainScreen = lv_scr_act();

    createSensorBasedDialog();

    lv_scr_load(mainScreen);

    // ---- Dark theme ----
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *th = lv_theme_default_init(dispp,
        lv_color_hex(0x33bdef) /* primary (cyan) */, lv_color_hex(0xc23a30) /* secondary (red) */,
        true /* dark */, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, th);
    // Slate page background on both screens
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0x0f1620), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(mainScreen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(sensorBasedControlPanel, lv_color_hex(0x0f1620), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(sensorBasedControlPanel, LV_OPA_COVER, LV_PART_MAIN);

    // style for button panel
    lv_style_init(&style);
    lv_style_set_flex_flow(&style, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_flex_main_place(&style, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_layout(&style, LV_LAYOUT_FLEX);


//    lv_style_set_pad_inner(&style, 1);
//    lv_style_set_pad_top(&style, 10);
//    lv_style_set_pad_bottom(&style, 5);
//    lv_style_set_pad_left(&style, 1);
//    lv_style_set_pad_right(&style, 1);
//    lv_style_set_pad_row(&style, 10);
//    lv_style_set_pad_column(&style, 10);
    lv_style_set_text_align(&style, LV_TEXT_ALIGN_CENTER);
    lv_style_set_text_font(&style, &lv_font_montserrat_24);

    lv_style_init(&styleNoPadding);
    lv_style_set_pad_top(&styleNoPadding, 0);
    lv_style_set_pad_bottom(&styleNoPadding, 0);
    lv_style_set_pad_left(&styleNoPadding, 0);
    lv_style_set_pad_right(&styleNoPadding, 0);
    lv_style_set_pad_row(&styleNoPadding, 6);
    lv_style_set_pad_column(&styleNoPadding, 6);
    // transparent, borderless containers (let the slate page show through)
    lv_style_set_bg_opa(&styleNoPadding, LV_OPA_TRANSP);
    lv_style_set_border_width(&styleNoPadding, 0);
    lv_style_set_radius(&styleNoPadding, 0);
    // requires enabling this font in lv_conf.h
    lv_style_set_text_font(&styleNoPadding, &lv_font_montserrat_24);


    lv_style_init(&stylePadding);
    lv_style_set_pad_top(&stylePadding, 3);
    lv_style_set_pad_bottom(&stylePadding, 3);
    lv_style_set_pad_left(&stylePadding, 5);
    lv_style_set_pad_right(&stylePadding, 5);
    lv_style_set_pad_row(&stylePadding, 5);
    lv_style_set_pad_column(&stylePadding, 5);

    lv_style_init(&styleSmallFont);
    lv_style_set_text_font(&styleSmallFont, &lv_font_montserrat_20);

    lv_style_init(&styleHugeFont);
    lv_style_set_text_font(&styleHugeFont, &lv_font_montserrat_40);

    lv_style_init(&styleArcIndicator);
    lv_style_set_arc_width(&styleArcIndicator,3);
    lv_style_set_arc_color(&styleArcIndicator,lv_color_hex(0xC71585));
    
    lv_style_init(&styleArcMain);
    lv_style_set_arc_width(&styleArcMain,3);

    lv_style_init(&styleArcKnob);
    lv_style_set_arc_width(&styleArcKnob,3); // does nothing
    lv_style_set_bg_opa(&styleArcKnob, 30);
    lv_style_set_bg_color(&styleArcKnob,lv_color_hex(0xFF4500));

    TRACE("UI 1");

    // Top thing
    lv_obj_t * top_thing = lv_obj_create(lv_scr_act());
    lv_obj_set_size(top_thing, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align(top_thing, LV_ALIGN_TOP_MID, 0, 0);
//    lv_obj_set_flex_flow(top_thing, LV_FLEX_FLOW_ROW);
    lv_obj_add_style(top_thing, &styleNoPadding, 0);

    TRACE("UI 2");
    bannerLabel = lv_label_create(top_thing);
    lv_label_set_long_mode(bannerLabel, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
    lv_label_set_recolor(bannerLabel, true);                     /*allow #RRGGBB inline colour*/
    lv_obj_set_style_text_font(bannerLabel, &lv_font_montserrat_24, LV_PART_MAIN);
    TRACE("UI 2.1");
    lv_label_set_text(bannerLabel, "Connecting");
    TRACE("UI 2.2");
    lv_obj_set_width(bannerLabel, LV_SIZE_CONTENT);  /*Set smaller width to make the lines wrap*/
//    lv_obj_set_height(bannerLabel, lv_pct(100));
    TRACE("UI 2.3");
    lv_obj_align(bannerLabel, LV_ALIGN_LEFT_MID, 0, 0);
    TRACE("UI 2.4");
    //lv_obj_set_style_text_align(bannerLabel, LV_TEXT_ALIGN_CENTER, 0);
    //lv_obj_align(bannerLabel, LV_ALIGN_CENTER, 0, -40);
//    lv_obj_center(bannerLabel);

    TRACE("UI 3");
    timeLabel = lv_label_create(top_thing);
//    lv_label_set_long_mode(timeLabel, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
    lv_label_set_text(timeLabel, "?");
    lv_obj_set_width(timeLabel, LV_SIZE_CONTENT);  /*Set smaller width to make the lines wrap*/
//    lv_obj_set_height(timeLabel, lv_pct(100));
    lv_obj_align(timeLabel, LV_ALIGN_RIGHT_MID, 0, 0);
    //lv_obj_set_style_text_align(timeLabel, LV_TEXT_ALIGN_RIGHT, 0);
    //lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -40);
//    lv_obj_center(timeLabel);

    TRACE("UI 4");
    controlButtonContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(controlButtonContainer, lv_pct(100), LV_SIZE_CONTENT);
//    lv_obj_center(controlButtonContainer);
    lv_obj_align(controlButtonContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(controlButtonContainer, &style, 0);
    lv_obj_add_style(controlButtonContainer, &styleNoPadding, 0);

    // bottom thing
    lv_obj_t * bottom_thing = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bottom_thing, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_align(bottom_thing, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_add_style(bottom_thing, &styleNoPadding, 0);
    //lv_obj_align_to(bottom_thing, controlButtonContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
//    lv_obj_set_flex_flow(bottom_thing, LV_FLEX_FLOW_ROW);

    TRACE("UI 5");
    statusLabel = lv_label_create(bottom_thing);
//    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
    lv_label_set_text(statusLabel, "Starting");
    lv_obj_set_width(statusLabel, LV_SIZE_CONTENT);  /*Set smaller width to make the lines wrap*/
//    lv_obj_set_height(statusLabel, lv_pct(100));
    //lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    //lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -40);
//    lv_obj_center(statusLabel);
    TRACE("UI 6");

//    lv_timer_create(updateUI, 500,  NULL);

    TRACE("UI 7");
}

void updateUI(lv_timer_t * timer) {

  updateStatusBar(200);
  updateButtons(500);
  clearStatusMessage(1000);

}

void createSensorBasedDialog() {


    static lv_coord_t col_dsc[] = {lv_pct(20), lv_pct(20), lv_pct(20), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {lv_pct(10), lv_pct(40), lv_pct(40), LV_GRID_TEMPLATE_LAST};

    // dialog (can it be created on the active screen?  I want it to be a background layer)
    sensorBasedControlPanel = lv_obj_create(NULL);

    TRACE("mbox 1");
    lv_scr_load(sensorBasedControlPanel);
    TRACE("mbox 1.1");
//    lv_obj_add_flag(sensorBasedControlPanel, LV_OBJ_FLAG_HIDDEN);
    TRACE("mbox 1.2");
//    lv_obj_move_background(sensorBasedControlPanel);
    TRACE("mbox 1.3");
//    lv_obj_set_size(sensorBasedControlPanel, lv_pct(90), lv_pct(90));
    lv_obj_add_style(sensorBasedControlPanel, &style, 0);
//    lv_obj_add_style(sensorBasedControlPanel, &styleNoPadding, 0);
    lv_obj_center(sensorBasedControlPanel);
    TRACE("mbox 1.4");
    lv_obj_set_style_grid_column_dsc_array(sensorBasedControlPanel, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(sensorBasedControlPanel, row_dsc, 0);
    lv_obj_set_layout(sensorBasedControlPanel, LV_LAYOUT_GRID);
    lv_obj_set_grid_align(sensorBasedControlPanel, LV_GRID_ALIGN_SPACE_BETWEEN, LV_GRID_ALIGN_SPACE_BETWEEN);
    TRACE("mbox 1.5");

    sensorBasedControlDescription = lv_label_create(sensorBasedControlPanel);
    //lv_obj_set_style_text_align(sensorBasedControlDescription, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(sensorBasedControlDescription, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 0, 1);


    TRACE("mbox 2");

    sensorBasedControlSetpointLabel = lv_label_create(sensorBasedControlPanel);
//    lv_spinbox_set_range(sensorBasedControlSetpointLabel, 0, 100); // TODO: adjust range when displaying
//    lv_spinbox_set_digit_format(sensorBasedControlSetpointLabel, 3, 0);
    lv_obj_add_style(sensorBasedControlSetpointLabel, &styleHugeFont, 0);
//    lv_spinbox_step_prev(sensorBasedControlSetpointLabel);
//    lv_obj_set_size(sensorBasedControlSetpointLabel, 100, lv_pct(40));
    lv_obj_center(sensorBasedControlSetpointLabel);
    //lv_obj_set_style_text_align(sensorBasedControlSetpointLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(sensorBasedControlSetpointLabel, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
    TRACE("mbox 3");

    lv_coord_t h = lv_pct(40);

    // + Button
    lv_obj_t * btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_SHORT_CLICKED,        &sensorBasedControlPanelIncrement);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_LONG_PRESSED_REPEAT,  &sensorBasedControlPanelIncrement);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);


    TRACE("mbox 4");

    // - Button
    btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_SHORT_CLICKED,        &sensorBasedControlPanelDecrement);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_LONG_PRESSED_REPEAT,  &sensorBasedControlPanelDecrement);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);


    TRACE("mbox 5");
    // Apply Button
    btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
//    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_OK, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

    lv_obj_t * btnLabel = lv_label_create(btn);
    lv_label_set_text(btnLabel, "OK");
    //lv_obj_set_style_text_align(btnLabel, LV_TEXT_ALIGN_CENTER, 0);



    TRACE("mbox 6");
    // Revert Button
    btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
//    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_CLOSE, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_CLICKED, &sensorBasedControlPanelReset);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

    btnLabel = lv_label_create(btn);
    lv_label_set_text(btnLabel, "Revert");
    //lv_obj_set_style_text_align(btnLabel, LV_TEXT_ALIGN_CENTER, 0);


    TRACE("mbox 7");
    // Reset Button
    btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
//    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_CLOSE, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_CLICKED, &sensorBasedControlPanelClose);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

    btnLabel = lv_label_create(btn);
    lv_label_set_text(btnLabel, "Close");
    //lv_obj_set_style_text_align(btnLabel, LV_TEXT_ALIGN_CENTER, 0);

    TRACE("mbox 8");
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    // while (!Serial);
    Serial.printf("Hot Tub Display Init, reset reason: %s\n", resetReasonName(esp_reset_reason()));

    esp_task_wdt_config_t twdt_config = {
        .timeout_ms = 2000,
        .idle_core_mask = (1 << CONFIG_FREERTOS_NUMBER_OF_CORES) - 1,    // Bitmask of all cores
        .trigger_panic = false,
    };
//    ESP_ERROR_CHECK(esp_task_wdt_init(&twdt_config));
    esp_task_wdt_init(&twdt_config);
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL)); // Apply to the current task

    showStatusMessage("Total heap: %d", ESP.getHeapSize());
    showStatusMessage("Free heap: %d", ESP.getFreeHeap());
    showStatusMessage("Total PSRAM: %d", ESP.getPsramSize());
    showStatusMessage("Free PSRAM: %d", ESP.getFreePsram());

#ifdef GRAPHICS_ENABLE
    showStatusMessage("gfx_init begin: %d", esp_get_free_heap_size());
    gfx_init();
    initUI();
//    Serial.println("gfx_init done");
    showStatusMessage("gfx_init done: %d", esp_get_free_heap_size());
#endif

    ESPNowUtils::registerDataCallbackControlHandler((ESPNowUtils::hot_tub_control_status_recv_callback)dataReceivedControlStatus);
    ESPNowUtils::registerDataCallbackServerHandler((ESPNowUtils::hot_tub_server_status_recv_callback)dataReceivedServerStatus);
    ESPNowUtils::registerEspCommStatusCallBackHandler((ESPNowUtils::esp_comm_status_callback)showStatusMessage);
    ESPNowUtils::setup();

    lastServerStatus = (struct_status_server *)malloc(sizeof(struct_status_server));

    // custom watchdog
    xTaskCreate(watchdogMonitorTask, "WDTMonitor", 4096, NULL, 1, NULL);

    Serial.println("Setup done");
}

// Custom Watchdog Monitor Task
void watchdogMonitorTask(void *parameter) {
  while (1) {
    unsigned long now = millis();

    // Check if Task 1 is stuck
    if ((now - lastTask1Time) > (WATCHDOG_TIMEOUT * 1000)) {
      Serial.printf("🚨 WARNING: Task 1 is STUCK at stage %d", lastTask2Stage);
    }

    // Reset the watchdog for this monitoring task
//    esp_task_wdt_reset();

    delay(1000);
  }
}

void debug_lvgl_mem() {
  lv_mem_monitor_t mem_mon;
  lv_mem_monitor(&mem_mon);
  Serial.printf("LVGL Free: %d, Frag: %d%%, Biggest Block: %d\n",
                mem_mon.free_size, mem_mon.frag_pct, mem_mon.free_biggest_size);
  Serial.printf("LVGL Objects: %d\n", lv_obj_get_child_cnt(lv_scr_act()));
}



void loop()
{
  static unsigned long last_debug = millis();
  static bool debug;

  debug = (millis() - last_debug) > 30000;
  esp_task_wdt_reset();  // Keep resetting the watchdog
  lastTask1Time = millis();

  lastTask2Stage = 0;

#ifdef GRAPHICS_ENABLE
    updateUI(NULL);
    lastTask2Stage = 10;
    lv_timer_handler(); /* let the GUI do its work */
    lastTask2Stage = 20;
#endif
    ESPNowUtils::loop();
    lastTask2Stage = 30;
    gfx_loop();
    lastTask2Stage = 40;
    delay(5);
    lastTask2Stage = 50;

    lastTask2Stage = 60;

    if (debug) {
      Serial.printf("Free Heap: %d b, up %d\n", ESP.getFreeHeap(), millis() / 1000);
      Serial.printf("Stack High Water Mark: %d bytes\n", uxTaskGetStackHighWaterMark(NULL));
      debug_lvgl_mem();
      last_debug = millis();
    }
}

// The controller stores/sends temperatures in Celsius and pushes the desired
// display unit (0=F, 1=C) in the server status. Convert at presentation time only.
bool displayInCelsius() {
    return lastServerStatus != NULL && lastServerStatus->temp_unit != 0;
}
// Native unit is Fahrenheit; convert to Celsius only when that display unit is selected.
float toDisplayTemp(float fahrenheit) {
    return displayInCelsius() ? (fahrenheit - 32.0f) * 5.0f / 9.0f : fahrenheit;
}
char displayUnitChar() {
    return displayInCelsius() ? 'C' : 'F';
}

void updateStatusBar(unsigned long frequency) {
    static unsigned long last_update = 0;
    if ((millis() - last_update) > frequency) {
      last_update = millis();

      // LVGL's printf has float formatting disabled (LV_SPRINTF_USE_FLOAT 0), so build
      // the string with the standard library's snprintf (which supports %f) instead.
      char banner[72];
      snprintf(banner, sizeof(banner), "#8fa6b6 %s#   #33bdef %.1f%c#", lastServerStatus->server_name,
               toDisplayTemp(lastServerStatus->water_temp), displayUnitChar());
      lv_label_set_text(bannerLabel, banner);

      lv_label_set_text_fmt(
              timeLabel,
              "%s%s %02d:%02d",
              ESPNowUtils::lastMessageReceivedTime + NET_ACTIVITY_SYMBOL_AGE > millis() ? LV_SYMBOL_DOWNLOAD : " ",
              ESPNowUtils::lastMessageSentTime + NET_ACTIVITY_SYMBOL_AGE > millis() ? LV_SYMBOL_UPLOAD : " ",
              hour(now()),
              minute(now()));
    }
}

void dataReceivedServerStatus(struct_status_server *status) {
//    showStatusMessage("Status: %s value %d min %d max %d.  Up %d", status->name, status->value, status->min, status->max, now());
    // set time from server
    setTime(status->time + status->tz_offset);
    gfx_set_screen_timeout(status->touchscreen_timeout * 1000);
//    TRACE("Set Time");
//    Serial.printf("Received touchscreen timeout %d, last touch time %d, millis %d, remaining time %d, bright %d", status->touchscreen_timeout, last_gfx_touch_time, millis(), (last_gfx_touch_time + gfx_screen_timeout) - millis(), displayBrightness);

    // save it and let graphics update from the right thread later
    memcpy(lastServerStatus, status, sizeof(struct_status_server));

//    TRACE("Updated server status labels");
}

void dataReceivedControlStatus(struct_status_control *status) {
//    Serial.print("Control ");
//    Serial.print(status->name);
//    Serial.print(" val ");
//    Serial.print(status->value);
//    Serial.println();
//    showStatusMessage("Status: %s value %d min %d max %d.  Up %ds", status->name, status->value, status->min, status->max, esp_timer_get_time() / 1000000);

    // In an attempt at stability, don't update UI elements in the callback, let
    // an LVGL timer handle that.

    // is this a new control? Pick them up in order, starting with 0:
    if (controlStatuses.size() == status->control_id) {
        // add it
        TRACE("Control Status 1");
        // make a struct for our copy of current info
        struct_status_control *status_copy = (struct_status_control *) malloc(sizeof(struct_status_control));
        controlStatuses.push_back(status_copy);
        TRACE("Control Status 2");
    }

    // we must have this one already, update it.
    if (controlStatuses.size() > status->control_id) {
        // update our copy of the data
        memcpy(controlStatuses[status->control_id], status, sizeof(struct_status_control));
        //        controlButtons[status->control_id]
//        showStatusMessage("Updating copy of status, max orig %d new %d", status->max, controlStatuses[status->control_id]->max);
//        TRACE("Control Status 4");
    }

}

// Card colours by control state (dark theme). e_value = getOnState (0/1/2).
static CardColors cardColorsFor(struct_status_control *s) {
    const uint32_t OFF_BG = 0x1a2431, OFF_BD = 0x2c3a4d, OFF_NAME = 0xc7d5e2, OFF_VAL = 0x5f7183;
    if (strcmp(s->type, "sensor-based") == 0) {
        bool heating = s->e_value > 0;
        return { 0x1a2431, (uint32_t)(heating ? 0x5a3420 : 0x3a2a1e), 0xc7d5e2, 0xff7a45 };
    }
    if (strcmp(s->type, "off-low-high") == 0) {
        if (s->e_value == 1) return { 0xe0952b, 0xe0952b, 0xffffff, 0xffffff }; // LOW  (amber)
        if (s->e_value >= 2) return { 0xe0652b, 0xe0652b, 0xffffff, 0xffffff }; // HIGH (orange)
        return { OFF_BG, OFF_BD, OFF_NAME, OFF_VAL };
    }
    // off-on
    if (s->e_value > 0) return { 0xc23a30, 0xc23a30, 0xffffff, 0xffffff };       // ON (red)
    return { OFF_BG, OFF_BD, OFF_NAME, OFF_VAL };                                // OFF
}

void updateButtons(unsigned long frequency) {
    static unsigned long last_update = 0;
    if ((millis() - last_update) > frequency) {
      last_update = millis();
      if (controlButtons.size() < controlStatuses.size()) {
          for (u_int8_t i = 0; i < controlStatuses.size(); i++) {
              // do we have this control already created?
              if (controlButtons.size() <= i) {
                  // add this control
                  lv_obj_t *singleControlContainer = lv_obj_create(controlButtonContainer);
                  lv_obj_set_user_data(singleControlContainer, controlStatuses[i]);
  //                TRACE("Update Buttons 2.01");
                  lv_obj_add_style(singleControlContainer, &style, 0);
                  //TRACE("Update Buttons 2.02");
                  lv_obj_add_style(singleControlContainer, &styleNoPadding, 0);
                  //TRACE("Update Buttons 2.03");
                  lv_obj_set_size(singleControlContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                  //TRACE("Update Buttons 2.04");

                  lv_obj_t *btn = lv_btn_create(singleControlContainer);
                  lv_obj_set_size(btn, 150, 82);
                  lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);
                  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, controlStatuses[i]);
                  lv_obj_set_style_radius(btn, 14, LV_PART_MAIN);
                  lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
                  lv_obj_set_style_shadow_width(btn, 0, LV_PART_MAIN);
                  lv_obj_set_style_pad_all(btn, 11, LV_PART_MAIN);

                  // child 0: name (top-left)
                  lv_obj_t *label = lv_label_create(btn);
                  lv_obj_add_style(label, &styleSmallFont, LV_PART_MAIN);
                  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 0, 0);
                  lv_label_set_text(label, controlStatuses[i]->name);

                  // child 1: override-remaining chip (bottom-right, small)
                  lv_obj_t *labelORT = lv_label_create(btn);
                  lv_obj_add_style(labelORT, &styleSmallFont, LV_PART_MAIN);
                  lv_obj_align(labelORT, LV_ALIGN_BOTTOM_RIGHT, 0, 2);

                  // child 2: value (bottom-left, large)
                  lv_obj_t *labelValue = lv_label_create(btn);
                  lv_obj_set_style_text_font(labelValue, &lv_font_montserrat_24, LV_PART_MAIN);
                  lv_obj_align(labelValue, LV_ALIGN_BOTTOM_LEFT, 0, 0);
                  lv_label_set_text(labelValue, "");

                  controlButtons.push_back(singleControlContainer);
              }
          }
      }

      // Now the two vectors are the same size so it's safe to loop and update
      for(lv_obj_t *controlButton : controlButtons){
        struct_status_control *status = (struct_status_control*)lv_obj_get_user_data(controlButton);
        //TRACE("Update Buttons 5");

        // 1st child
        lv_obj_t * btn = lv_obj_get_child(controlButton, 0);
        //TRACE("Update Buttons 5.3");
        // 1 child (label is second)
//          lv_obj_t * led = lv_obj_get_child(btn, 0);
        // 2nd child
        lv_obj_t * label = lv_obj_get_child(btn, 0);
//        TRACE("Update Buttons 5.1");
        lv_obj_t * labelORT  = lv_obj_get_child(btn, 1);

        lv_obj_t * labelValue = lv_obj_get_child(btn, 2);

        if (status->ORT > 0) {
          if (status->ORT > 3600) {
            lv_label_set_text_fmt(labelORT, "%dh", status->ORT / 3600);
          } else if (status->ORT > 60) {
            lv_label_set_text_fmt(labelORT, "%dm", status->ORT / 60);
          } else {
            lv_label_set_text_fmt(labelORT, "%ds", status->ORT);
          }
//            lv_arc_set_value(arc, 100 *  status->EL / (status->ORT + status->EL));
          lv_obj_clear_flag(labelORT, LV_OBJ_FLAG_HIDDEN);
        } else {
//            lv_label_set_text(labelORT, "");
          lv_obj_add_flag(labelORT, LV_OBJ_FLAG_HIDDEN);
        }

        // Optimistic feedback: white border while a press is unconfirmed.
        u_int8_t cid = status->control_id;
        bool pending = (cid < MAX_CONTROLS) && pressPending[cid].active;
        if (pending) {
          if (status->value != pressPending[cid].expected) {
            pressPending[cid].active = false; pending = false;         // confirmed (value changed)
          } else if (millis() - pressPending[cid].since > PRESS_PENDING_TIMEOUT) {
            pressPending[cid].active = false; pending = false;         // gave up
            showStatusMessage("No response for %s", status->name);
          }
        }
        CardColors cc = cardColorsFor(status);
        lv_obj_set_style_bg_color(btn, lv_color_hex(cc.bg), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, pending ? lv_color_white() : lv_color_hex(cc.border), LV_PART_MAIN);
        lv_obj_set_style_border_width(btn, pending ? 3 : 1, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(cc.nameC), LV_PART_MAIN);
        lv_obj_set_style_text_color(labelValue, lv_color_hex(cc.valueC), LV_PART_MAIN);
        lv_obj_set_style_text_color(labelORT, lv_color_hex(cc.valueC), LV_PART_MAIN);
        //TRACE("Update Buttons 5.2");


      if (strcmp(status->type, "off-low-high") == 0) {
        lv_label_set_text(labelValue, status->e_value == 0 ? "OFF" : (status->e_value == 1 ? "LOW" : "HIGH"));
      } else if (strcmp(status->type, "sensor-based") == 0) {
        lv_label_set_text_fmt(labelValue, "%d%c", (int)lroundf(toDisplayTemp((float)status->value)), displayUnitChar());
      } else {
        // off-on
        lv_label_set_text(labelValue, status->e_value > 0 ? "ON" : "OFF");
      }
      lv_obj_clear_flag(labelValue, LV_OBJ_FLAG_HIDDEN);
    }
  //    TRACE("Update Buttons 6");
  }

}


#ifdef GRAPHICS_ENABLE
static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);


    if(code == LV_EVENT_CLICKED) {
        struct_status_control * status = (struct_status_control*) lv_event_get_user_data(e);

        // Debounce: the resistive panel can report a tap as press-release-press,
        // firing two CLICKED events that toggle the control on then off. Ignore a
        // second click on the same control within a short window.
        static unsigned long lastClickMillis[MAX_CONTROLS] = {0};
        #define TOUCH_DEBOUNCE_MS 350
        if (status->control_id < MAX_CONTROLS) {
            if (millis() - lastClickMillis[status->control_id] < TOUCH_DEBOUNCE_MS) {
                return; // bounce — ignore
            }
            lastClickMillis[status->control_id] = millis();
        }

        if (strcmp(status->type, "off-on") == 0 || strcmp(status->type, "off-low-high") == 0) {
            // if we're in override, cancel it
            u_int32_t overrideTime = status->ORT > 0 ? 0 : status->DO;
            // toggle increment
            u_int8_t newValue = status->value == status->max ? status->min : status->value + 1;
            if (strcmp(status->type, "off-low-high") == 0) {
                overrideTime = status->DO; // since we have three positions, toggle and stay in override.
            }
//            showStatusMessage("Ctrl %s (%d), from %d to %d, min %d max %d ",
//                              status->name,
//                              status->control_id,
//                              status->value,
//                              newValue,
//                              status->min,
//                              status->max);

            if (overrideTime > 0) {
                showStatusMessage("Set %s to %s for %d minutes", status->name, MODE_MAPPING[newValue], overrideTime / 60 );
            } else {
                showStatusMessage("Set %s to %s (on normal schedule)", status->name, MODE_MAPPING[newValue]);
            }

            ESPNowUtils::sendOverrideCommand(
                    status->control_id,
                    0, // start now
                    overrideTime,
                    newValue
            );
            // Acknowledge the press instantly, before the ESP-NOW round-trip completes.
            // Confirm on value CHANGE (not a specific target) so cancel/revert also clears.
            if (status->control_id < MAX_CONTROLS) {
                pressPending[status->control_id].active = true;
                pressPending[status->control_id].since = millis();
                pressPending[status->control_id].expected = status->value; // value before the press
            }
            lv_obj_set_style_border_color(btn, lv_color_white(), LV_PART_MAIN | LV_STATE_DEFAULT);
        } else if (strcmp(status->type, "sensor-based") == 0) {
            TRACE("SENS DISP 1");
            lv_obj_set_user_data(sensorBasedControlSetpointLabel, status);
            sensorBasedControlSetpointValue = status->value;
            TRACE("SENS DISP 2");
//            lv_spinbox_set_range(sensorBasedControlSetpointLabel, status->min, status->max);
//            lv_spinbox_set_value(sensorBasedControlSetpointLabel, status->value);
            lv_label_set_text_fmt(sensorBasedControlSetpointLabel, "%d%c", (int)lroundf(toDisplayTemp((float)status->value)), displayUnitChar());
            TRACE("SENS DISP 2.1");
            lv_label_set_text_fmt(sensorBasedControlDescription, "Adjust setpoint for %s", status->name);
            TRACE("SENS DISP 3");
            lv_scr_load_anim(sensorBasedControlPanel, LV_SCR_LOAD_ANIM_OVER_TOP, 500, 10, false);
//            lv_obj_move_foreground(sensorBasedControlPanel);
//            lv_obj_clear_flag(sensorBasedControlPanel, LV_OBJ_FLAG_HIDDEN);

            TRACE("SENS DISP END");
        }// otherwise we don't support click.  maybe open a dialog?
    }

}

static void lv_spinbox_event_cb(lv_event_t * e)
{
    TRACE("SENS CB 1");
    lv_event_code_t code = lv_event_get_code(e);
    TRACE("SENS CB 2");

    if(lv_event_get_user_data(e) != NULL && (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)) {
        struct_status_control *status = (struct_status_control *) lv_obj_get_user_data(sensorBasedControlSetpointLabel);
        int8_t *change = (int8_t*)(lv_event_get_user_data(e));
        switch(*change) {
            case 1:
                if (sensorBasedControlSetpointValue < status->max) {
                    sensorBasedControlSetpointValue++;
                }
                break;
            case -1:
                if (sensorBasedControlSetpointValue > status->min) {
                    sensorBasedControlSetpointValue--;
                }
                break;
        }
        lv_label_set_text_fmt(sensorBasedControlSetpointLabel, "%d%c", (int)lroundf(toDisplayTemp((float)sensorBasedControlSetpointValue)), displayUnitChar());
    }
    TRACE("SENS CB 3");

    if (code == LV_EVENT_CLICKED) {
        struct_status_control *status = (struct_status_control *) lv_obj_get_user_data(
                sensorBasedControlSetpointLabel);
        if (lv_event_get_user_data(e) == NULL) {
            // this is a confirm
            TRACE("SENS CB 3.1");
            TRACE("SENS CB 3.2");
            showStatusMessage("Set %s to %.0f%c", status->name,
                              toDisplayTemp((float)sensorBasedControlSetpointValue), displayUnitChar());
            TRACE("SENS CB 3.3");
            ESPNowUtils::sendOverrideCommand(
                    status->control_id,
                    0, // start now
                    status->DO,
                    sensorBasedControlSetpointValue
            );
            TRACE("SENS CB 3.4");
         } else if (lv_event_get_user_data(e) == &sensorBasedControlPanelReset) {
            showStatusMessage("Restoring %s to normal schedule", status->name);
            TRACE("SENS CB 4.3");
            ESPNowUtils::sendOverrideCommand(
                    status->control_id,
                    0, // start now
                    0,
                    status->value // doesn't matter what we pass
            );
        }
//        lv_obj_move_background(sensorBasedControlPanel);
//        lv_obj_add_flag(sensorBasedControlPanel, LV_OBJ_FLAG_HIDDEN);
//        lv_scr_load(mainScreen);
        lv_scr_load_anim(mainScreen, LV_SCR_LOAD_ANIM_OVER_BOTTOM, 500, 10, false);
    }
    TRACE("SENS CB END");

}

#endif
