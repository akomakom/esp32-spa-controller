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
lv_obj_t * statusLabel = NULL;
std::vector<lv_obj_t *> controlButtons;
std::vector<struct_status_control *> controlStatuses;
lv_obj_t * controlButtonContainer = NULL;
#endif

void initUI() {
    static lv_style_t style;
    lv_style_init(&style);
    lv_style_set_flex_flow(&style, LV_FLEX_FLOW_ROW_WRAP);
    lv_style_set_flex_main_place(&style, LV_FLEX_ALIGN_SPACE_EVENLY);
    lv_style_set_layout(&style, LV_LAYOUT_FLEX);



    // Top thing
    lv_obj_t * top_thing = lv_obj_create(lv_scr_act());
    lv_obj_set_size(top_thing, lv_pct(100), lv_pct(15));
    lv_obj_align(top_thing, LV_ALIGN_TOP_MID, 0, 5);
    lv_obj_set_flex_flow(top_thing, LV_FLEX_FLOW_ROW);
    // TODO: do something with the top thing
    
    controlButtonContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(controlButtonContainer, lv_pct(100), lv_pct(70));
    lv_obj_center(controlButtonContainer);
    lv_obj_align(controlButtonContainer, LV_ALIGN_CENTER, 0, 5);
    lv_obj_add_style(controlButtonContainer, &style, 0);

    // bottom thing
    lv_obj_t * bottom_thing = lv_obj_create(lv_scr_act());
    lv_obj_set_size(bottom_thing, lv_pct(100), lv_pct(15));
    lv_obj_align_to(bottom_thing, controlButtonContainer, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
    lv_obj_set_flex_flow(bottom_thing, LV_FLEX_FLOW_ROW);

    statusLabel = lv_label_create(bottom_thing);
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
    lv_label_set_text(statusLabel, "Starting");
    lv_obj_set_width(statusLabel, lv_pct(100));  /*Set smaller width to make the lines wrap*/
//    lv_obj_set_height(statusLabel, lv_pct(100));
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    //lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -40);
    lv_obj_center(statusLabel);

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

    ESPNowUtils::registerDataCallBackHandler((ESPNowUtils::hot_tub_control_status_recv_callback)dataReceivedControlStatus);
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
    delay(5);
}


void dataReceivedControlStatus(struct_status_control *status) {
//    Serial.print("Control ");
//    Serial.print(status->name);
//    Serial.print(" val ");
//    Serial.print(status->value);
//    Serial.println();
    showStatusMessage("Status: %s value %d min %d max %d.  Up %d", status->name, status->value, status->min, status->max, now());

    // is this a new control? Pick them up in order, starting with 0:
    if (controlButtons.size() == status->control_id) {
        // add it

        // make a struct for our copy of current info
        struct_status_control * status_copy = (struct_status_control*) malloc(sizeof(struct_status_control));
        controlStatuses.push_back(status_copy);


        lv_obj_t * btn = lv_btn_create(controlButtonContainer);     /*Add a button the current screen*/
//    lv_obj_set_pos(btn, 10, 10);                            /*Set its position*/
        lv_obj_set_size(btn, 120, LV_SIZE_CONTENT);                          /*Set its size*/
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, status_copy);           /*Assign a callback to the button*/

        lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
        lv_label_set_text_fmt(label, "%s", status->name);                     /*Set the labels text*/
        lv_obj_center(label);

        controlButtons.push_back(btn);
    }

    // this control (now?) exists
    if (controlButtons.size() > status->control_id) {
        // update our copy of the data
        memcpy(controlStatuses[status->control_id], status, sizeof(struct_status_control));
        //        controlButtons[status->control_id]
//        showStatusMessage("Updating copy of status, max orig %d new %d", status->max, controlStatuses[status->control_id]->max);

        lv_obj_t * label = lv_obj_get_child(controlButtons[status->control_id], 0);
        lv_label_set_text_fmt(label, "%s (%s)", status->name, status->e_value > 0 ? "ON" : "OFF");
    }

}

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
    Serial.println(msg);
#ifdef GRAPHICS_ENABLE
    if (statusLabel != NULL) { // already initialized
        lv_label_set_text(statusLabel, msg);
    }
#endif
    va_end(args);
}

#ifdef GRAPHICS_ENABLE
static void btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);


    if(code == LV_EVENT_CLICKED) {
        struct_status_control * status = (struct_status_control*) lv_event_get_user_data(e);
        showStatusMessage("Ctrl %s (%d), from %d to %d, min %d max %d ", status->name, status->control_id, status->value, status->value > status->min ? status->min : status->max, status->min, status->max);
        ESPNowUtils::sendOverrideCommand(
                status->control_id,
                0, // start now
                status->ORT > 0 ? 0 : status->DO, // if we're in override, cancel it
                status->value > status->min ? status->min : status->max // simplistic "just do the opposite"
        );


//        static uint8_t cnt = 0;
//        cnt++;
//
//        /*Get the first child of the button which is the label and change its text*/
//        lv_obj_t * label = lv_obj_get_child(btn, 0);
//        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }

}

#endif
