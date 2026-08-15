/*******************************************************************************
 * Touch libraries:
 * FT6X36: https://github.com/strange-v/FT6X36.git
 * GT911: https://github.com/TAMCTec/gt911-arduino.git
 * XPT2046: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
 ******************************************************************************/

/* uncomment for FT6X36 */
// #define TOUCH_FT6X36
// #define TOUCH_FT6X36_SCL 19
// #define TOUCH_FT6X36_SDA 18
// #define TOUCH_FT6X36_INT 39
// #define TOUCH_SWAP_XY
// #define TOUCH_MAP_X1 480
// #define TOUCH_MAP_X2 0
// #define TOUCH_MAP_Y1 0
// #define TOUCH_MAP_Y2 320

/* uncomment for GT911 */
// #define TOUCH_GT911
// #define TOUCH_GT911_SCL 20
// #define TOUCH_GT911_SDA 19
// #define TOUCH_GT911_INT -1
// #define TOUCH_GT911_RST 38
// #define TOUCH_GT911_ROTATION ROTATION_NORMAL
// #define TOUCH_MAP_X1 480
// #define TOUCH_MAP_X2 0
// #define TOUCH_MAP_Y1 272
// #define TOUCH_MAP_Y2 0

/* uncomment for XPT2046 */
 #define TOUCH_XPT2046
 #define TOUCH_XPT2046_SCK 12
 #define TOUCH_XPT2046_MISO 13
 #define TOUCH_XPT2046_MOSI 11
 #define TOUCH_XPT2046_CS 38
 #define TOUCH_XPT2046_INT 18
 #define TOUCH_XPT2046_ROTATION 0
// 100-4000 => 11 - 407 (original)
// 0-4100 => 33-363
// 0-4500 => 63-400  (sharp pointer)
// 500-4500 => 81-455
// 500-5000 => 116 - 460
// 0 - 5000 => 105 - 410
// Calibrated 2026-08-12 from corner touches on this panel (raw X 181..2303, Y 658..3589).
// X1=left edge (high raw x), X2=right edge; Y1=top edge (low raw y), Y2=bottom edge.
 #define TOUCH_MAP_X1 1800
 #define TOUCH_MAP_X2 600
 #define TOUCH_MAP_Y1 660
 #define TOUCH_MAP_Y2 3590

int touch_last_x = 0, touch_last_y = 0;

#if defined(TOUCH_FT6X36)
#include <Wire.h>
#include "FT6X36.h"
FT6X36 ts(&Wire, TOUCH_FT6X36_INT);
bool touch_touched_flag = true, touch_released_flag = true;

#elif defined(TOUCH_GT911)
#include <Wire.h>
#include "TAMC_GT911.h"
TAMC_GT911 ts = TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

#elif defined(TOUCH_XPT2046)
#include "XPT2046_Touchscreen.h"
#include <SPI.h>
XPT2046_Touchscreen ts(TOUCH_XPT2046_CS, TOUCH_XPT2046_INT);

// --- Rolling-median filter -----------------------------------------------------
// This resistive panel occasionally returns a wildly off raw reading (a single
// sample can jump 30%+, e.g. raw X 1304 among neighbours around 850), which then
// maps onto the wrong control. Each physical touch yields several samples, so keep
// a short window of raw readings and report their per-axis MEDIAN, which discards
// the odd outlier (a mean would still be dragged toward it). A gap since the last
// sample starts a fresh window (= a new, separate touch). We withhold the press
// until a few samples are in, so the very first (possibly bad) sample can't land
// the press on the wrong control.
#define TOUCH_MEDIAN_WINDOW     7    // raw samples retained
#define TOUCH_MEDIAN_MIN        3    // samples for the normal path (median rejects 1 outlier)
#define TOUCH_MEDIAN_FLOOR      2    // fewest samples the time fallback will accept
#define TOUCH_SETTLE_MS         45   // commit a short touch by now even with < MIN samples
#define TOUCH_NEW_TOUCH_GAP_MS  120  // gap that marks the start of a new touch
static int touchRawX[TOUCH_MEDIAN_WINDOW];
static int touchRawY[TOUCH_MEDIAN_WINDOW];
static uint8_t touchSampleCount = 0;
static uint8_t touchSampleHead = 0;
static unsigned long touchLastSampleMs = 0;
static unsigned long touchStartMs = 0;   // time of the first sample of the current touch

static int touch_median(const int *src, uint8_t n) {
    int tmp[TOUCH_MEDIAN_WINDOW];
    for (uint8_t i = 0; i < n; i++) tmp[i] = src[i];
    for (uint8_t i = 1; i < n; i++) { // insertion sort; n is tiny (<= window)
        int v = tmp[i], j = i;
        while (j > 0 && tmp[j - 1] > v) { tmp[j] = tmp[j - 1]; j--; }
        tmp[j] = v;
    }
    return tmp[n / 2];
}
#endif

#if defined(TOUCH_FT6X36)
void touch(TPoint p, TEvent e)
{
  if (e != TEvent::Tap && e != TEvent::DragStart && e != TEvent::DragMove && e != TEvent::DragEnd)
  {
    return;
  }
  // translation logic depends on screen rotation
#if defined(TOUCH_SWAP_XY)
  touch_last_x = map(p.y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width());
  touch_last_y = map(p.x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height());
#else
  touch_last_x = map(p.x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width());
  touch_last_y = map(p.y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height());
#endif
  switch (e)
  {
  case TEvent::Tap:
    Serial.println("Tap");
    touch_touched_flag = true;
    touch_released_flag = true;
    break;
  case TEvent::DragStart:
    Serial.println("DragStart");
    touch_touched_flag = true;
    break;
  case TEvent::DragMove:
    Serial.println("DragMove");
    touch_touched_flag = true;
    break;
  case TEvent::DragEnd:
    Serial.println("DragEnd");
    touch_released_flag = true;
    break;
  default:
    Serial.println("UNKNOWN");
    break;
  }
}
#endif

void touch_init()
{
#if defined(TOUCH_FT6X36)
  Wire.begin(TOUCH_FT6X36_SDA, TOUCH_FT6X36_SCL);
  ts.begin();
  ts.registerTouchHandler(touch);

#elif defined(TOUCH_GT911)
  Wire.begin(TOUCH_GT911_SDA, TOUCH_GT911_SCL);
  ts.begin();
  ts.setRotation(TOUCH_GT911_ROTATION);

#elif defined(TOUCH_XPT2046)
  SPI.begin(TOUCH_XPT2046_SCK, TOUCH_XPT2046_MISO, TOUCH_XPT2046_MOSI, TOUCH_XPT2046_CS);
  ts.begin();
  ts.setRotation(TOUCH_XPT2046_ROTATION);

#endif
}

bool touch_has_signal()
{
#if defined(TOUCH_FT6X36)
  ts.loop();
  return touch_touched_flag || touch_released_flag;

#elif defined(TOUCH_GT911)
  return true;

#elif defined(TOUCH_XPT2046)
  // Gate on the real PENIRQ pin level (LOW = touched). The library's latched
  // tirqTouched() gets stuck set under this toolchain, causing phantom touches.
  return digitalRead(TOUCH_XPT2046_INT) == LOW;

#else
  return false;
#endif
}

bool touch_touched()
{
#if defined(TOUCH_FT6X36)
  if (touch_touched_flag)
  {
    touch_touched_flag = false;
    return true;
  }
  else
  {
    return false;
  }

#elif defined(TOUCH_GT911)
  ts.read();
  if (ts.isTouched)
  {
#if defined(TOUCH_SWAP_XY)
    touch_last_x = map(ts.points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touch_last_y = map(ts.points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#else
    touch_last_x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touch_last_y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#endif
    return true;
  }
  else
  {
    return false;
  }

#elif defined(TOUCH_XPT2046)
  if (ts.touched())
  {
    TS_Point p = ts.getPoint();

    unsigned long nowMs = millis();
    if (nowMs - touchLastSampleMs > TOUCH_NEW_TOUCH_GAP_MS) {
      // Long gap since the last sample -> this is a new touch; drop the old window.
      touchSampleCount = 0;
      touchSampleHead = 0;
      touchStartMs = nowMs;
    }
    touchLastSampleMs = nowMs;

    touchRawX[touchSampleHead] = p.x;
    touchRawY[touchSampleHead] = p.y;
    touchSampleHead = (touchSampleHead + 1) % TOUCH_MEDIAN_WINDOW;
    if (touchSampleCount < TOUCH_MEDIAN_WINDOW) touchSampleCount++;

    // Commit the press once we have enough samples for a solid median, OR after a
    // short settle time with at least a couple of samples (so a brief tap that only
    // produces 2 reads still registers instead of being silently dropped).
    bool ready = (touchSampleCount >= TOUCH_MEDIAN_MIN) ||
                 (touchSampleCount >= TOUCH_MEDIAN_FLOOR && (nowMs - touchStartMs) >= TOUCH_SETTLE_MS);
    if (!ready) {
      return false;
    }

    int mx = touch_median(touchRawX, touchSampleCount);
    int my = touch_median(touchRawY, touchSampleCount);
    Serial.printf("Touch raw %d/%d @%d -> median %d/%d (n=%d)", p.x, p.y, p.z, mx, my, touchSampleCount);
#if defined(TOUCH_SWAP_XY)
    touch_last_x = map(my, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touch_last_y = map(mx, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#else
    touch_last_x = map(mx, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, gfx->width() - 1);
    touch_last_y = map(my, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, gfx->height() - 1);
#endif
    return true;
  }
  else
  {
    return false;
  }

#else
  return false;
#endif
}

bool touch_released()
{
#if defined(TOUCH_FT6X36)
  if (touch_released_flag)
  {
    touch_released_flag = false;
    return true;
  }
  else
  {
    return false;
  }

#elif defined(TOUCH_GT911)
  return true;

#elif defined(TOUCH_XPT2046)
  return true;

#else
  return false;
#endif
}
