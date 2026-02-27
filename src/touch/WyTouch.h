/*
 * WyTouch.h — Unified touch abstraction for wyltek-embedded-builder
 * ==================================================================
 * Select backend via build flag:
 *   -DWY_TOUCH_GT911    — Guition 4848S040, I2C capacitive
 *   -DWY_TOUCH_XPT2046  — CYD ESP32-2432S028R, SPI resistive
 *
 * Usage (same for all backends):
 *   WyTouch touch;
 *   touch.begin();
 *   if (touch.update()) { int x = touch.x; int y = touch.y; }
 */

#pragma once
#include <Arduino.h>

#if defined(WY_TOUCH_GT911)
  #include "GT911.h"

  class WyTouch {
  public:
      int x = 0, y = 0;
      bool pressed = false;

      bool begin() { return _gt.begin(); }
      bool update() {
          bool r = _gt.update();
          x = _gt.x; y = _gt.y; pressed = _gt.pressed;
          return r;
      }
  private:
      GT911 _gt;
  };

#elif defined(WY_TOUCH_XPT2046)
  #include <XPT2046_Touchscreen.h>
  #ifndef WY_XPT_CS
    #define WY_XPT_CS   33
  #endif
  #ifndef WY_XPT_IRQ
    #define WY_XPT_IRQ  36
  #endif
  #ifndef WY_XPT_X_MIN
    #define WY_XPT_X_MIN 200
  #endif
  #ifndef WY_XPT_X_MAX
    #define WY_XPT_X_MAX 3700
  #endif
  #ifndef WY_XPT_Y_MIN
    #define WY_XPT_Y_MIN 240
  #endif
  #ifndef WY_XPT_Y_MAX
    #define WY_XPT_Y_MAX 3800
  #endif
  #ifndef WY_SCREEN_W
    #define WY_SCREEN_W 320
  #endif
  #ifndef WY_SCREEN_H
    #define WY_SCREEN_H 240
  #endif

  class WyTouch {
  public:
      int x = 0, y = 0;
      bool pressed = false;

      bool begin() {
          _ts.begin();
          _ts.setRotation(1);
          return true;
      }
      bool update() {
          if (!_ts.touched()) { pressed = false; return false; }
          TS_Point p = _ts.getPoint();
          x = map(p.x, WY_XPT_X_MIN, WY_XPT_X_MAX, 0, WY_SCREEN_W);
          y = map(p.y, WY_XPT_Y_MIN, WY_XPT_Y_MAX, 0, WY_SCREEN_H);
          pressed = true;
          return true;
      }
  private:
      XPT2046_Touchscreen _ts{WY_XPT_CS, WY_XPT_IRQ};
  };

#else
  /* Stub — no touch backend selected */
  class WyTouch {
  public:
      int x = 0, y = 0;
      bool pressed = false;
      bool begin()  { return false; }
      bool update() { return false; }
  };
#endif
