
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
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include "freertos/semphr.h"
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


Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
40 /* DE */, 41 /* VSYNC */, 39 /* HSYNC */, 42 /* PCLK */,
45 /* R0 */, 48 /* R1 */, 47 /* R2 */, 21 /* R3 */, 14 /* R4 */,
5 /* G0 */, 6 /* G1 */, 7 /* G2 */, 15 /* G3 */, 16 /* G4 */, 4 /* G5 */,
8 /* B0 */, 3 /* B1 */, 46 /* B2 */, 9 /* B3 */, 1 /* B4 */,
0 /* hsync_polarity */, 1 /* hsync_front_porch */, 1 /* hsync_pulse_width */, 43 /* hsync_back_porch */,
0 /* vsync_polarity */, 3 /* vsync_front_porch */, 1 /* vsync_pulse_width */, 12 /* vsync_back_porch */,
1 /* pclk_active_neg */, 10000000 /* prefer_speed */);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
480 /* width */, 272 /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */);

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

// ---- esp_lcd RGB panel driven directly with a DOUBLE framebuffer ----
// Two full-screen framebuffers in PSRAM are used as LVGL's draw buffers. The
// panel scans out one while LVGL renders into the other, swapping at vsync, so
// there is no PSRAM read/write contention -> the intermittent frame shift is gone.
static esp_lcd_panel_handle_t rgb_panel_handle = NULL;
static SemaphoreHandle_t sem_vsync_end = NULL;
static SemaphoreHandle_t sem_gui_ready = NULL;

static bool IRAM_ATTR rgb_vsync_cb(esp_lcd_panel_handle_t panel,
                                   const esp_lcd_rgb_panel_event_data_t *edata, void *user_data) {
    BaseType_t high_task_awoken = pdFALSE;
    if (sem_gui_ready && xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE) {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }
    return high_task_awoken == pdTRUE;
}

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    // With num_fbs=2 + full_refresh, color_p is one of the panel's framebuffers,
    // so draw_bitmap just swaps (no copy). Wait for the swap to take effect at the
    // next vsync before letting LVGL render into the now-free buffer (tear-free).
    if (sem_gui_ready) xSemaphoreGive(sem_gui_ready);
    if (sem_vsync_end) xSemaphoreTake(sem_vsync_end, portMAX_DELAY);
    esp_lcd_panel_draw_bitmap(rgb_panel_handle, area->x1, area->y1,
                              area->x2 + 1, area->y2 + 1, (void *)color_p);
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

    lv_init();

    // ---- Create the RGB panel directly via esp_lcd with two framebuffers ----
    // (pins/timings copied exactly from the Arduino_GFX config above; the `gfx`
    //  object is kept only for width()/height() and is intentionally not begin()'d.)
    esp_lcd_rgb_panel_config_t panel_config = {};
    panel_config.clk_src = LCD_CLK_SRC_DEFAULT;
    panel_config.timings.pclk_hz = 10000000;
    panel_config.timings.h_res = 480;
    panel_config.timings.v_res = 272;
    panel_config.timings.hsync_pulse_width = 1;
    panel_config.timings.hsync_back_porch  = 43;
    panel_config.timings.hsync_front_porch = 1;
    panel_config.timings.vsync_pulse_width = 1;
    panel_config.timings.vsync_back_porch  = 12;
    panel_config.timings.vsync_front_porch = 3;
    panel_config.timings.flags.hsync_idle_low  = 1; // hsync_polarity == 0
    panel_config.timings.flags.vsync_idle_low  = 1; // vsync_polarity == 0
    panel_config.timings.flags.de_idle_high    = 0;
    panel_config.timings.flags.pclk_active_neg = 1;
    panel_config.timings.flags.pclk_idle_high  = 0;
    panel_config.data_width = 16;
    panel_config.bits_per_pixel = 16;
    panel_config.num_fbs = 2;                  // <-- double framebuffer
    panel_config.bounce_buffer_size_px = 0;
    panel_config.dma_burst_size = 64;
    panel_config.hsync_gpio_num = 39;
    panel_config.vsync_gpio_num = 41;
    panel_config.de_gpio_num    = 40;
    panel_config.pclk_gpio_num  = 42;
    panel_config.disp_gpio_num  = GPIO_NUM_NC;
    // RGB565 data lines, non-big-endian order (must match Arduino_GFX's mapping):
    const int data_pins[16] = {
        8, 3, 46, 9, 1,      // B0..B4
        5, 6, 7, 15, 16, 4,  // G0..G5
        45, 48, 47, 21, 14   // R0..R4
    };
    for (int i = 0; i < 16; i++) panel_config.data_gpio_nums[i] = data_pins[i];
    panel_config.flags.fb_in_psram = 1;
    panel_config.flags.disp_active_low = 1;

    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &rgb_panel_handle));

    sem_vsync_end = xSemaphoreCreateBinary();
    sem_gui_ready = xSemaphoreCreateBinary();
    esp_lcd_rgb_panel_event_callbacks_t cbs = {};
    cbs.on_vsync = rgb_vsync_cb;
    esp_lcd_rgb_panel_register_event_callbacks(rgb_panel_handle, &cbs, NULL);

    ESP_ERROR_CHECK(esp_lcd_panel_reset(rgb_panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(rgb_panel_handle));

    void *fb0 = NULL, *fb1 = NULL;
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(rgb_panel_handle, 2, &fb0, &fb1));

    delay(10);
    touch_init();

#ifdef TFT_BL
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
#endif

    screenWidth = gfx->width();    // 480 (object not begun; returns declared size)
    screenHeight = gfx->height();  // 272

    // The two PSRAM framebuffers ARE LVGL's draw buffers. full_refresh is required
    // for direct-framebuffer double buffering.
    lv_disp_draw_buf_init(&draw_buf, (lv_color_t *)fb0, (lv_color_t *)fb1, screenWidth * screenHeight);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    /* Initialize the input device driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_t *indev = lv_indev_drv_register(&indev_drv);
    // debug pointer calibration:
    lv_obj_t *cursor_img = lv_img_create(lv_scr_act());
    lv_img_set_src(cursor_img, LV_SYMBOL_OK);
    lv_indev_set_cursor(indev, cursor_img);
}
