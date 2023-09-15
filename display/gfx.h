
/*******************************************************************************
 * Dependent libraries:
 * LVGL: https://github.com/lvgl/lvgl.git

 * Touch libraries (choose one):
 * FT6X36: https://github.com/strange-v/FT6X36.git
 * GT911: https://github.com/TAMCTec/gt911-arduino.git
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 *
 * LVGL Configuration file:
 * Copy your_arduino_path/libraries/lvgl/lv_conf_template.h
 * to your_arduino_path/libraries/lv_conf.h
 * Then find and set:
 * #define LV_COLOR_DEPTH     16
 * #define LV_TICK_CUSTOM     1
 * If getting memory issues at runtime, try setting:
 * #define LV_MEM_CUSTOM      1
 *
 * For SPI display set color swap can be faster, parallel screen don't set!
 * #define LV_COLOR_16_SWAP   1
 *
 * Optional: Show CPU usage and FPS count
 * #define LV_USE_PERF_MONITOR 1
 ******************************************************************************/#include <lvgl.h>
/*******************************************************************************
 ******************************************************************************/
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#define TFT_BL 2
#define GFX_BL DF_GFX_BL // default backlight pin, you may replace DF_GFX_BL to actual backlight pin
#define GFX_FRAME_BUFFER_FRACTION 4 // divide size of width * height * color by this factor and allocate that many bytes

/* More dev device declaration: https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration */
#if defined(DISPLAY_DEV_KIT)
Arduino_GFX *gfx = create_default_Arduino_GFX();
#else /* !defined(DISPLAY_DEV_KIT) */

/* More data bus class: https://github.com/moononournation/Arduino_GFX/wiki/Data-Bus-Class */
//Arduino_DataBus *bus = create_default_Arduino_DataBus();

/* More display class: https://github.com/moononournation/Arduino_GFX/wiki/Display-Class */
//Arduino_GFX *gfx = new Arduino_ILI9341(bus, DF_GFX_RST, 0 /* rotation */, false /* IPS */);

Arduino_ESP32RGBPanel *bus = new Arduino_ESP32RGBPanel(
        GFX_NOT_DEFINED /* CS */, GFX_NOT_DEFINED /* SCK */, GFX_NOT_DEFINED /* SDA */,
        40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
        45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
        5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
        8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */
);
// option 1:
// ILI6485 LCD 480x272
Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
        bus,
        480 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
        272 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
        1 /* pclk_active_neg */, 9000000 /* prefer_speed */, true /* auto_flush */);

// option 2:
// ST7262 IPS LCD 800x480
// Arduino_RPi_DPI_RGBPanel *gfx = new Arduino_RPi_DPI_RGBPanel(
//   bus,
//   800 /* width */, 0 /* hsync_polarity */, 8 /* hsync_front_porch */, 4 /* hsync_pulse_width */, 8 /* hsync_back_porch */,
//   480 /* height */, 0 /* vsync_polarity */, 8 /* vsync_front_porch */, 4 /* vsync_pulse_width */, 8 /* vsync_back_porch */,
//   1 /* pclk_active_neg */, 14000000 /* prefer_speed */, true /* auto_flush */);
#endif /* !defined(DISPLAY_DEV_KIT) */
/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#include "touch.h"

#define BRIGHTNESS_FULL 255
#define BRIGHTNESS_DIM  40
#define BRIGHTNESS_OFF  0

// After waking up (backlight on after the first touch event),
// ignore touch event for this much time to avoid accidental
// control activation
#define GFX_IGNORE_TOUCH_ON_WAKE_MILLIS 1000

static uint32_t screenWidth;
static uint32_t screenHeight;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t *disp_draw_buf;
static lv_disp_drv_t disp_drv;
static unsigned long last_gfx_touch_time = 0;
static unsigned long gfx_ignore_touch_until = 0;
static unsigned long gfx_screen_timeout = 60000;
static u_int8_t displayBrightness = BRIGHTNESS_FULL;

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
    gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

    lv_disp_flush_ready(disp);
}


long gfx_screen_timeout_remaining_millis() {
    return (last_gfx_touch_time + gfx_screen_timeout) - millis();
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (touch_has_signal())
    {
        if (touch_touched())
        {
            if (gfx_screen_timeout_remaining_millis() < 0) {
                // screen was off when this touch event occurred, prevent touch to avoid accidental press
                gfx_ignore_touch_until = millis() + GFX_IGNORE_TOUCH_ON_WAKE_MILLIS;
            }
            last_gfx_touch_time = millis(); // wake up regardless
            if (gfx_ignore_touch_until > millis()) {
                Serial.printf("Ignoring touch for another %d millis", (gfx_ignore_touch_until - millis()));
                return;
            }

            data->state = LV_INDEV_STATE_PR;

            /*Set the coordinates*/
            data->point.x = touch_last_x;
            data->point.y = touch_last_y;
            Serial.printf("Touch at (%d,%d) \n", data->point.x, data->point.y);
        }
        else if (touch_released())
        {
            data->state = LV_INDEV_STATE_REL;
        }
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}


void gfx_loop() {
    // dim the screen after half the screen timeout
    long remaining_millis = gfx_screen_timeout_remaining_millis();
    if (remaining_millis > (long)(gfx_screen_timeout / 2)) {
        displayBrightness = BRIGHTNESS_FULL;
    } else if (remaining_millis > 0) {
        displayBrightness = BRIGHTNESS_DIM;
    } else {
        displayBrightness = BRIGHTNESS_OFF;
    }
    analogWrite(TFT_BL, displayBrightness);
}

void gfx_set_screen_timeout(unsigned long timeout) {
    gfx_screen_timeout = timeout;
}

void gfx_init() {

    // Init Display
    gfx->begin();
    lv_init();
    delay(10);
    touch_init();

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif

    screenWidth = gfx->width();
    screenHeight = gfx->height();
#ifdef ESP32
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(sizeof(lv_color_t) * screenWidth * screenHeight/GFX_FRAME_BUFFER_FRACTION , MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
#else
    disp_draw_buf = (lv_color_t *) malloc(sizeof(lv_color_t) * screenWidth * screenHeight / GFX_FRAME_BUFFER_FRACTION);
#endif
    if (!disp_draw_buf) {
        Serial.println("LVGL disp_draw_buf allocate failed!");
    } else {
        lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / GFX_FRAME_BUFFER_FRACTION);

        /* Initialize the display */
        lv_disp_drv_init(&disp_drv);
        /* Change the following line to your display resolution */
        disp_drv.hor_res = screenWidth;
        disp_drv.ver_res = screenHeight;
        disp_drv.flush_cb = my_disp_flush;
        disp_drv.draw_buf = &draw_buf;
        lv_disp_drv_register(&disp_drv);

        /* Initialize the (dummy) input device driver */
        static lv_indev_drv_t indev_drv;
        lv_indev_drv_init(&indev_drv);
        indev_drv.type = LV_INDEV_TYPE_POINTER;
        indev_drv.read_cb = my_touchpad_read;
        lv_indev_t *indev    = lv_indev_drv_register(&indev_drv);
        // debug pointer calibration:
        lv_obj_t *cursor_img = lv_img_create(lv_scr_act());
        lv_img_set_src(cursor_img, LV_SYMBOL_OK);
        lv_indev_set_cursor(indev, cursor_img);

    }
}