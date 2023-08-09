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

const char * MODE_MAPPING[] = { "OFF", "ON", "HIGH" };

lv_obj_t * mainScreen = NULL;

// These get passed as pointers to lv events
int8_t sensorBasedControlPanelIncrement = 1;
int8_t sensorBasedControlPanelDecrement = -1;
int8_t sensorBasedControlPanelReset = 0;

lv_obj_t * sensorBasedControlPanel = NULL;
lv_obj_t * sensorBasedControlSetpointLabel = NULL;
lv_obj_t * sensorBasedControlDescription = NULL;
u_int8_t   sensorBasedControlSetpointValue = 0;

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

    mainScreen = lv_scr_act();

    createSensorBasedDialog();

    lv_scr_load(mainScreen);

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

    lv_timer_create(updateStatusBar, 100,  NULL);
    lv_timer_create(updateButtons, 500,  NULL);

    TRACE("UI 7");
}

void createSensorBasedDialog() {


    static lv_coord_t col_dsc[] = {lv_pct(30), lv_pct(30), lv_pct(30), LV_GRID_TEMPLATE_LAST};
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
    lv_obj_add_style(sensorBasedControlPanel, &stylePadding, 0);
    lv_obj_center(sensorBasedControlPanel);
    TRACE("mbox 1.4");
    lv_obj_set_style_grid_column_dsc_array(sensorBasedControlPanel, col_dsc, 0);
    lv_obj_set_style_grid_row_dsc_array(sensorBasedControlPanel, row_dsc, 0);
    lv_obj_set_layout(sensorBasedControlPanel, LV_LAYOUT_GRID);
    TRACE("mbox 1.5");

    sensorBasedControlDescription = lv_label_create(sensorBasedControlPanel);
    lv_obj_set_grid_cell(sensorBasedControlDescription, LV_GRID_ALIGN_STRETCH, 0, 3, LV_GRID_ALIGN_STRETCH, 0, 1);


    TRACE("mbox 2");

    sensorBasedControlSetpointLabel = lv_label_create(sensorBasedControlPanel);
//    lv_spinbox_set_range(sensorBasedControlSetpointLabel, 0, 100); // TODO: adjust range when displaying
//    lv_spinbox_set_digit_format(sensorBasedControlSetpointLabel, 3, 0);
    lv_obj_add_style(sensorBasedControlSetpointLabel, &styleHugeFont, 0);
//    lv_spinbox_step_prev(sensorBasedControlSetpointLabel);
//    lv_obj_set_size(sensorBasedControlSetpointLabel, 100, lv_pct(40));
    lv_obj_center(sensorBasedControlSetpointLabel);
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
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_OK, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

    TRACE("mbox 6");
    // Reset Button
    btn = lv_btn_create(sensorBasedControlPanel);
    lv_obj_set_size(btn, h, h);
    lv_obj_set_style_bg_img_src(btn, LV_SYMBOL_CLOSE, 0);
    lv_obj_add_event_cb(btn, lv_spinbox_event_cb, LV_EVENT_CLICKED, &sensorBasedControlPanelReset);
    lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_STRETCH, 2, 1);

    TRACE("mbox 7");
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    // while (!Serial);
    Serial.println("Hot Tub Display Init");

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
    delay(5);
}

void updateStatusBar(lv_timer_t * timer) {
    lv_label_set_text_fmt(
            timeLabel,
            "%s%s %02d:%02d",
            ESPNowUtils::lastMessageReceivedTime + NET_ACTIVITY_SYMBOL_AGE > millis() ? LV_SYMBOL_DOWN : " ",
            ESPNowUtils::lastMessageSentTime + NET_ACTIVITY_SYMBOL_AGE > millis() ? LV_SYMBOL_UP : " ",
            hour(now()),
            minute(now()));
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

    // In an attempt at stability, don't update UI elements in the callback, let
    // an LVGL timer handle that.

    // is this a new control? Pick them up in order, starting with 0:
    if (controlButtons.size() == status->control_id) {
        // add it
        TRACE("Control Status 1");
        // make a struct for our copy of current info
        struct_status_control *status_copy = (struct_status_control *) malloc(sizeof(struct_status_control));
        controlStatuses.push_back(status_copy);
        TRACE("Control Status 2");
    }

    // we must have this one already, update it.
    if (controlButtons.size() > status->control_id) {
        // update our copy of the data
        memcpy(controlStatuses[status->control_id], status, sizeof(struct_status_control));
        //        controlButtons[status->control_id]
//        showStatusMessage("Updating copy of status, max orig %d new %d", status->max, controlStatuses[status->control_id]->max);
        TRACE("Control Status 4");
    }

}

void updateButtons(lv_timer_t * timer) {
    if (controlButtons.size() < controlStatuses.size()) {
        for (u_int8_t i = 0; i < controlStatuses.size(); i++) {
            // do we have this control already created?
            if (controlButtons.size() <= i) {
                // add this control
                lv_obj_t *singleControlContainer = lv_obj_create(controlButtonContainer);
                lv_obj_set_user_data(singleControlContainer, controlStatuses[i]);
                TRACE("Update Buttons 2.01");
                lv_obj_add_style(singleControlContainer, &style, 0);
                TRACE("Update Buttons 2.02");
                lv_obj_add_style(singleControlContainer, &stylePadding, 0);
                TRACE("Update Buttons 2.03");
                lv_obj_set_size(singleControlContainer, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                TRACE("Update Buttons 2.04");

                lv_obj_t *btn = lv_btn_create(singleControlContainer);
                TRACE("Update Buttons 2.1");
                lv_obj_set_size(btn, 140, 50);
                lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, 0);
                TRACE("Update Buttons 2.2");
                lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED,
                                    controlStatuses[i]);
                TRACE("Update Buttons 2.3");

                lv_obj_t *led = lv_led_create(singleControlContainer);
                lv_obj_set_size(led, 15, 15);
                lv_obj_align(led, LV_ALIGN_TOP_RIGHT, 0, 0);
                lv_led_set_brightness(led, 150);
                lv_led_set_color(led, lv_palette_main(LV_PALETTE_RED));
                lv_led_off(led);

                lv_obj_t *label = lv_label_create(btn);
                lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 0);
                TRACE("Update Buttons 2.4");
                lv_label_set_text_fmt(label, "%s", controlStatuses[i]->name);
                TRACE("Update Buttons 2.5");
                lv_obj_center(label);
                TRACE("Update Buttons 3");

                controlButtons.push_back(singleControlContainer);
            }
        }
    }

    // Now the two vectors are the same size so it's safe to loop and update
    for(lv_obj_t *controlButton : controlButtons){
        struct_status_control *status = (struct_status_control*)lv_obj_get_user_data(controlButton);
        TRACE("Update Buttons 5");

        // 1st child
        lv_obj_t * btn = lv_obj_get_child(controlButton, 0);
        // 2nd child
        lv_obj_t * led = lv_obj_get_child(controlButton, 1);
        TRACE("Update Buttons 5.1");

        if (status->e_value) {
            lv_led_on(led);
        } else {
            lv_led_off(led);
        }
        TRACE("Update Buttons 5.2");

        // 2nd child
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        TRACE("Update Buttons 5.3");

        if (strcmp(status->type, "off-low-high") == 0 && status->e_value > 0) {
            TRACE("Update Buttons 5.4");
            lv_label_set_text_fmt(label, "%s (%s)", status->name, status->e_value == 1 ? "LOW" : "HIGH");
            TRACE("Update Buttons 5.5");
            lv_led_set_color(led, lv_palette_main(status->e_value == 1 ? LV_PALETTE_RED : LV_PALETTE_YELLOW));
        } else if (strcmp(status->type, "sensor-based") == 0) {
            lv_label_set_text_fmt(label, "%s (%d)", status->name, status->value);
        } else {
            lv_label_set_text(label, status->name);
        }
    }
    TRACE("Update Buttons 6");

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
        } else if (strcmp(status->type, "sensor-based") == 0) {
            TRACE("SENS DISP 1");
            lv_obj_set_user_data(sensorBasedControlSetpointLabel, status);
            sensorBasedControlSetpointValue = status->value;
            TRACE("SENS DISP 2");
//            lv_spinbox_set_range(sensorBasedControlSetpointLabel, status->min, status->max);
//            lv_spinbox_set_value(sensorBasedControlSetpointLabel, status->value);
            lv_label_set_text_fmt(sensorBasedControlSetpointLabel, "%d", status->value);
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
        lv_label_set_text_fmt(sensorBasedControlSetpointLabel, "%d", sensorBasedControlSetpointValue);
    }
    TRACE("SENS CB 3");

    if (code == LV_EVENT_CLICKED) {
        struct_status_control *status = (struct_status_control *) lv_obj_get_user_data(
                sensorBasedControlSetpointLabel);
        if (lv_event_get_user_data(e) == NULL) {
            // this is a confirm
            TRACE("SENS CB 3.1");
            TRACE("SENS CB 3.2");
            showStatusMessage("Set %s to %d for %d minutes", status->name, sensorBasedControlSetpointValue,
                              status->DO / 60);
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
