#define GRAPHICS_ENABLE

/*******************************************************************************
 * Please configure graphics in gfx.h
 ******************************************************************************/
#ifdef GRAPHICS_ENABLE
#include "gfx.h"
#endif
#include <vector>
#include "ESPNowUtils.h"
/*******************************************************************************
 * Please configure the touch panel in touch.h
 ******************************************************************************/

#ifdef GRAPHICS_ENABLE

lv_obj_t * sensorBasedControlPanel = NULL;
lv_obj_t * sensorBasedControlSpinbox = NULL;
lv_obj_t * sensorBasedControlDescription = NULL;

lv_style_t style;
lv_style_t styleNoPadding;
lv_style_t stylePadding;
lv_style_t styleHugeFont;

lv_obj_t * statusLabel = NULL;
lv_obj_t * bannerLabel = NULL;
lv_obj_t * timeLabel = NULL;
std::vector<lv_obj_t *> controlButtons;
std::vector<struct_status_control *> controlStatuses;
lv_obj_t * controlButtonContainer = NULL;
unsigned long statusDisplayTime = 0;

// how long to keep last status message on the screen
#define STATUS_DISPLAY_TIMEOUT 5000
// how long to show the network activity symbols after network activity
#define NET_ACTIVITY_SYMBOL_AGE 500
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
    char msg[100];
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

void clearStatusMessage() {
#ifdef GRAPHICS_ENABLE
    if ((statusDisplayTime + STATUS_DISPLAY_TIMEOUT) < millis()) {
        if (statusLabel != NULL) { // already initialized
            lv_label_set_text(statusLabel, "");
        }
//        Serial.printf("Clearing status message because %d + TIMEOUT < %d\n", statusDisplayTime, millis());
        statusDisplayTime = LONG_MAX; // max long, don't keep clearing it.
    }
#endif
}


void initUI() {
    // style for button panel
    lv_style_init(&style);
    lv_style_set_flex_flow(&style, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_flex_main_place(&style, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_layout(&style, LV_LAYOUT_FLEX);


//    lv_style_set_pad_inner(&style, 1);
    lv_style_set_pad_top(&style, 10);
    lv_style_set_pad_bottom(&style, 5);
    lv_style_set_pad_left(&style, 1);
    lv_style_set_pad_right(&style, 1);
    lv_style_set_pad_row(&style, 10);
    lv_style_set_pad_column(&style, 10);
    lv_style_set_text_font(&style, &lv_font_montserrat_20);

    lv_style_init(&styleNoPadding);
    lv_style_set_pad_top(&styleNoPadding, 0);
    lv_style_set_pad_bottom(&styleNoPadding, 0);
    lv_style_set_pad_left(&styleNoPadding, 0);
    lv_style_set_pad_right(&styleNoPadding, 0);
    lv_style_set_pad_row(&styleNoPadding, 0);
    lv_style_set_pad_column(&styleNoPadding, 0);
    // requires enabling this font in lv_conf.h
    lv_style_set_text_font(&styleNoPadding, &lv_font_montserrat_20);


    lv_style_init(&stylePadding);
    lv_style_set_pad_top(&stylePadding, 6);
    lv_style_set_pad_bottom(&stylePadding, 3);
    lv_style_set_pad_left(&stylePadding, 5);
    lv_style_set_pad_right(&stylePadding, 5);
    lv_style_set_pad_row(&stylePadding, 5);
    lv_style_set_pad_column(&stylePadding, 5);

    lv_style_init(&styleHugeFont);
    lv_style_set_text_font(&styleHugeFont, &lv_font_montserrat_40);

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
    TRACE("UI 2.1");
    lv_label_set_text(bannerLabel, "Connecting");
    TRACE("UI 2.2");
    lv_obj_set_width(bannerLabel, LV_SIZE_CONTENT);  /*Set smaller width to make the lines wrap*/
//    lv_obj_set_height(bannerLabel, lv_pct(100));
    TRACE("UI 2.3");
    lv_obj_align(bannerLabel, LV_ALIGN_LEFT_MID, 0, 0);
    TRACE("UI 2.4");
    lv_obj_set_style_text_align(bannerLabel, LV_TEXT_ALIGN_CENTER, 0);
    //lv_obj_align(bannerLabel, LV_ALIGN_CENTER, 0, -40);
//    lv_obj_center(bannerLabel);

    TRACE("UI 3");
    timeLabel = lv_label_create(top_thing);
//    lv_label_set_long_mode(timeLabel, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
    lv_label_set_text(timeLabel, "?");
    lv_obj_set_width(timeLabel, LV_SIZE_CONTENT);  /*Set smaller width to make the lines wrap*/
//    lv_obj_set_height(timeLabel, lv_pct(100));
    lv_obj_align(timeLabel, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_align(timeLabel, LV_TEXT_ALIGN_RIGHT, 0);
    //lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, -40);
//    lv_obj_center(timeLabel);

    TRACE("UI 4");
    controlButtonContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(controlButtonContainer, lv_pct(100), LV_SIZE_CONTENT);
//    lv_obj_center(controlButtonContainer);
    lv_obj_align(controlButtonContainer, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(controlButtonContainer, &style, 0);

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
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    //lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -40);
//    lv_obj_center(statusLabel);
    TRACE("UI 6");

    createSensorBasedDialog();


}

void createSensorBasedDialog() {


    static lv_coord_t col_dsc[] = {lv_pct(30), lv_pct(30), lv_pct(30), LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {lv_pct(10), lv_pct(40), lv_pct(40), LV_GRID_TEMPLATE_LAST};

    // dialog (can it be created on the active screen?  I want it to be a background layer)
    sensorBasedControlPanel = lv_obj_create(lv_scr_act());
    lv_obj_set_size(sensorBasedControlPanel, lv_pct(90), lv_pct(90));
    lv_obj_add_style(sensorBasedControlPanel, &style, 0);
    lv_obj_add_style(sensorBasedControlPanel, &stylePadding, 0);
    lv_obj_center(sensorBasedControlPanel);

    lv_obj_set_style_grid_column_dsc_array(sensorBasedControlPanel, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(sensorBasedControlPanel, row_dsc, 0);
    lv_obj_set_layout(sensorBasedControlPanel, LV_LAYOUT_GRID);


    sensorBasedControlDescription = lv_label_create(sensorBasedControlPanel);
    lv_obj_set_grid_cell(sensorBasedControlDescription, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 0, 1);


    TRACE("mbox 2");

    sensorBasedControlSpinbox = lv_spinbox_create(sensorBasedControlPanel);
    lv_spinbox_set_range(sensorBasedControlSpinbox, 0, 100); // TODO: adjust range when displaying
    lv_spinbox_set_digit_format(sensorBasedControlSpinbox, 3, 0);
    lv_obj_add_style(sensorBasedControlSpinbox, &styleHugeFont, 0);
//    lv_spinbox_step_prev(sensorBasedControlSpinbox);
//    lv_obj_set_size(sensorBasedControlSpinbox, 100, lv_pct(40));
//    lv_obj_center(sensorBasedControlSpinbox);
    lv_obj_set_grid_cell(sensorBasedControlSpinbox, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    lv_coord_t h = lv_pct(40);

    lv_obj_t * btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
//    lv_obj_align_to(btn, sensorBasedControlSpinbox, LV_ALIGN_OUT_RIGHT_MID, 5, 0);
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_PLUS, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_increment_event_cb, LV_EVENT_ALL,  NULL);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
//    lv_obj_align_to(btn, sensorBasedControlSpinbox, LV_ALIGN_OUT_LEFT_MID, -5, 0);
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_MINUS, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_decrement_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);

    btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
//    lv_obj_align_to(btn, sensorBasedControlSpinbox, LV_ALIGN_OUT_BOTTOM_MID, -5, 0);
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_OK, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_done_event_cb, LV_EVENT_ALL, NULL);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

    lv_obj_move_background(sensorBasedControlPanel);
    TRACE("mbox 5");
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    // while (!Serial);
    Serial.println("Hot Tub Display Init");
//    showStatusMessage("Test: %s", "hello");



    showStatusMessage("Total heap: %d", ESP.getHeapSize());
    showStatusMessage("Free heap: %d", ESP.getFreeHeap());
    showStatusMessage("Total PSRAM: %d", ESP.getPsramSize());
    showStatusMessage("Free PSRAM: %d", ESP.getFreePsram());


#ifdef GRAPHICS_ENABLE
    showStatusMessage("gfx_init begin: %d", esp_get_free_heap_size());
    gfx_init();
//    Serial.println("gfx_init done");
    showStatusMessage("gfx_init done: %d", esp_get_free_heap_size());
#endif

    ESPNowUtils::registerDataCallbackControlHandler((ESPNowUtils::hot_tub_control_status_recv_callback)dataReceivedControlStatus);
    ESPNowUtils::registerDataCallbackServerHandler((ESPNowUtils::hot_tub_server_status_recv_callback)dataReceivedServerStatus);
    ESPNowUtils::registerEspCommStatusCallBackHandler((ESPNowUtils::esp_comm_status_callback)showStatusMessage);
    ESPNowUtils::setup();

#ifdef GRAPHICS_ENABLE


    initUI();
#endif
    Serial.println("Setup done");
}


void loop()
{
#ifdef GRAPHICS_ENABLE
    lv_timer_handler(); /* let the GUI do its work */
#endif
    ESPNowUtils::loop();
    gfx_loop();
    clearStatusMessage();
    updateUI();
    delay(5);
}

void updateUI() {
    static unsigned long lastUpdate;
    if (lastUpdate + 100 < millis()) {
        lastUpdate = millis();
        lv_label_set_text_fmt(
                timeLabel,
                "%s%s %02d:%02d",
                ESPNowUtils::lastMessageReceivedTime + NET_ACTIVITY_SYMBOL_AGE > millis() ? LV_SYMBOL_DOWN : " ",
                ESPNowUtils::lastMessageSentTime + NET_ACTIVITY_SYMBOL_AGE > millis() ? LV_SYMBOL_UP : " ",
                hour(now()),
                minute(now()));
    }
}

void dataReceivedServerStatus(struct_status_server *status) {
//    showStatusMessage("Status: %s value %d min %d max %d.  Up %d", status->name, status->value, status->min, status->max, now());
    // set time from server
    setTime(status->time + status->tz_offset);
    TRACE("Set Time");

    if (bannerLabel != NULL && timeLabel != NULL) { // already initialized
        lv_label_set_text_fmt(bannerLabel, "%s @%.1fF ", status->server_name, status->water_temp);
        TRACE("Updated server status label 1");
    }
    TRACE("Updated server status labels");
}

void dataReceivedControlStatus(struct_status_control *status) {
//    Serial.print("Control ");
//    Serial.print(status->name);
//    Serial.print(" val ");
//    Serial.print(status->value);
//    Serial.println();
//    showStatusMessage("Status: %s value %d min %d max %d.  Up %ds", status->name, status->value, status->min, status->max, esp_timer_get_time() / 1000000);

    // is this a new control? Pick them up in order, starting with 0:
    if (controlButtons.size() == status->control_id) {
        // add it
        TRACE("Control Status 1");

        // make a struct for our copy of current info
        struct_status_control * status_copy = (struct_status_control*) malloc(sizeof(struct_status_control));
        controlStatuses.push_back(status_copy);
        TRACE("Control Status 2");

        lv_obj_t * singleControlContainer = lv_obj_create(controlButtonContainer);
        lv_obj_add_style(singleControlContainer, &style, 0);
        lv_obj_add_style(singleControlContainer, &stylePadding, 0);
        lv_obj_set_size(singleControlContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

        lv_obj_t * btn = lv_btn_create(singleControlContainer);     /*Add a button the current screen*/
        TRACE("Control Status 2.1");
        lv_obj_set_size(btn, 140, 50);                          /*Set its size*/
        lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);
        TRACE("Control Status 2.2");
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, status_copy);           /*Assign a callback to the button*/
        TRACE("Control Status 2.3");

        lv_obj_t * led  = lv_led_create(singleControlContainer);
        lv_obj_set_size(led, 15, 15);
        lv_obj_align(led, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_led_set_brightness(led, 150);
        lv_led_set_color(led, lv_palette_main(LV_PALETTE_RED));
        lv_led_off(led);

        lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
        lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
        TRACE("Control Status 2.4");
        lv_label_set_text_fmt(label, "%s", status->name);                     /*Set the labels text*/
        TRACE("Control Status 2.5");
        lv_obj_center(label);
        TRACE("Control Status 3");

        controlButtons.push_back(singleControlContainer);
    }
    TRACE("Control Status 4");

    // this control (now?) exists
    if (controlButtons.size() > status->control_id) {
        // update our copy of the data
        memcpy(controlStatuses[status->control_id], status, sizeof(struct_status_control));
        //        controlButtons[status->control_id]
//        showStatusMessage("Updating copy of status, max orig %d new %d", status->max, controlStatuses[status->control_id]->max);
        TRACE("Control Status 5");

        // 1st child
        lv_obj_t * btn = lv_obj_get_child(controlButtons[status->control_id], 0);
        // 2nd child
        lv_obj_t * led = lv_obj_get_child(controlButtons[status->control_id], 1);

        if (status->e_value) {
            lv_led_on(led);
        } else {
            lv_led_off(led);
        }
        // 2nd child
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        if (strcmp(status->type, "off-low-high") == 0 && status->e_value > 0) {
            lv_label_set_text_fmt(label, "%s (%s)", status->name, status->e_value == 1 ? "LOW" : "HIGH");
            lv_led_set_color(led, lv_palette_main(status->e_value == 1 ? LV_PALETTE_RED : LV_PALETTE_YELLOW));
        } else if (strcmp(status->type, "sensor-based") == 0) {
            lv_label_set_text_fmt(label, "%s (%d)", status->name, status->value);
            lv_led_set_color(led, lv_palette_main(LV_PALETTE_RED));
        } else {
            lv_label_set_text(label, status->name);
            lv_led_set_color(led, lv_palette_main(LV_PALETTE_RED));
        }
    }
    TRACE("Control Status 6");

}


#ifdef GRAPHICS_ENABLE
static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);


    if(code == LV_EVENT_CLICKED) {
        struct_status_control * status = (struct_status_control*) lv_event_get_user_data(e);
        if (strcmp(status->type, "off-on") == 0 || strcmp(status->type, "off-low-high") == 0) {
            // if we're in override, cancel it
            u_int32_t overrideTime = status->ORT > 0 ? 0 : status->DO;
            // toggle increment
            u_int8_t newValue = status->value == status->max ? status->min : status->value + 1;
            if (strcmp(status->type, "off-low-high") == 0) {
                overrideTime = status->DO; // since we have three positions, toggle and stay in override.
            }
            showStatusMessage("Ctrl %s (%d), from %d to %d, min %d max %d ",
                              status->name,
                              status->control_id,
                              status->value,
                              newValue,
                              status->min,
                              status->max);
            ESPNowUtils::sendOverrideCommand(
                    status->control_id,
                    0, // start now
                    overrideTime,
                    newValue
            );
        } else if (strcmp(status->type, "sensor-based") == 0) {
            lv_obj_set_user_data(sensorBasedControlSpinbox, status);
            lv_spinbox_set_range(sensorBasedControlSpinbox, status->min, status->max);
            lv_spinbox_set_value(sensorBasedControlSpinbox, status->value);
            lv_label_set_text_fmt(sensorBasedControlDescription, "Adjust setpoint for %s", status->name);
            lv_obj_move_foreground(sensorBasedControlPanel);
        }// otherwise we don't support click.  maybe open a dialog?
    }

}


void sensorBasedSetpointCallback(lv_event_t * e)
{
    lv_obj_t * messageBox = lv_event_get_current_target(e);
    const char* buttonText = lv_msgbox_get_active_btn_text(messageBox);
    LV_LOG_USER("Button %s clicked", buttonText);
    struct_status_control * status = (struct_status_control*) lv_obj_get_user_data(messageBox);
    Serial.printf("Clicked a dialog button for control %s", status->name);

    lv_obj_t * text = lv_msgbox_get_text(messageBox);
    lv_label_set_text_fmt(text, "Clicked %s", buttonText);

    if (strcmp(buttonText, "Apply") == 0) {
        // TODO
        if (messageBox != NULL) {
            lv_msgbox_close(messageBox);
        }
        messageBox = NULL;
    }
}

static void lv_spinbox_increment_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SHORT_CLICKED || code  == LV_EVENT_LONG_PRESSED_REPEAT) {
        lv_spinbox_set_value(sensorBasedControlSpinbox, lv_spinbox_get_value(sensorBasedControlSpinbox) + 1);
    }
}

static void lv_spinbox_decrement_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT) {
//        lv_spinbox_decrement(sensorBasedControlSpinbox);
        lv_spinbox_set_value(sensorBasedControlSpinbox, lv_spinbox_get_value(sensorBasedControlSpinbox) - 1);
    }
}
static void lv_spinbox_done_event_cb(lv_event_t * e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) {
        struct_status_control *status = (struct_status_control *) lv_obj_get_user_data(sensorBasedControlSpinbox);
        u_int8_t newValue = lv_spinbox_get_value(sensorBasedControlSpinbox);
        showStatusMessage("Ctrl %s (%d), from %d to %d, min %d max %d ",
                          status->name,
                          status->control_id,
                          status->value,
                          newValue,
                          status->min,
                          status->max);
        ESPNowUtils::sendOverrideCommand(
                status->control_id,
                0, // start now
                status->DO,
                newValue
        );
        lv_obj_move_background(sensorBasedControlPanel);
    }
}

#endif
