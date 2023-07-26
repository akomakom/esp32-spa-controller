#define GRAPHICS_ENABLE

/*******************************************************************************
 * Please configure graphics in gfx.h
 ******************************************************************************/
#ifdef GRAPHICS_ENABLE
#include "gfx.h"
#endif

#include "ESPNowUtils.h"
/*******************************************************************************
 * Please configure the touch panel in touch.h
 ******************************************************************************/

bool graphicsReady = false;
#ifdef GRAPHICS_ENABLE
lv_obj_t * statusLabel;
#endif

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
    statusLabel = lv_label_create(lv_scr_act());
    lv_label_set_long_mode(statusLabel, LV_LABEL_LONG_WRAP);     /*Break the long lines*/
    lv_label_set_text(statusLabel, "Starting");
    lv_obj_set_width(statusLabel, 150);  /*Set smaller width to make the lines wrap*/
    lv_obj_set_style_text_align(statusLabel, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(statusLabel, LV_ALIGN_CENTER, 0, -40);

    lv_obj_t * btn = lv_btn_create(lv_scr_act());     /*Add a button the current screen*/
    lv_obj_set_pos(btn, 10, 10);                            /*Set its position*/
    lv_obj_set_size(btn, 120, 50);                          /*Set its size*/
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);           /*Assign a callback to the button*/

    lv_obj_t * label = lv_label_create(btn);          /*Add a label to the button*/
    lv_label_set_text(label, "Button");                     /*Set the labels text*/
    lv_obj_center(label);
    graphicsReady = true;
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
    Serial.print("Control ");
    Serial.print(status->name);
    Serial.print(" val ");
    Serial.print(status->value);
    Serial.println();
    showStatusMessage("Control update: %s value %d.  Uptime %d", status->name, status->value, now());
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
    if (graphicsReady) {
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
        static uint8_t cnt = 0;
        cnt++;

        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t * label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

#endif
