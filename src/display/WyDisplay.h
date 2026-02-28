/*
 * WyDisplay.h — Display abstraction for wyltek-embedded-builder
 * ==============================================================
 * Reads board config from boards.h. Supports:
 *   - SPI displays: ILI9341, ST7796, ST7789, GC9A01, GC9107
 *   - RGB parallel: ST7701S (Guition 4848), ST7262 (Sunton 8048)
 *   - 8-bit parallel: ST7796 (WT32-SC01 Plus)
 *
 * Note: GC9107 (T-Keyboard keycap displays) uses GC9A01 driver.
 *   For the T-Keyboard's 4-display setup, use WyKeyDisplay.h instead.
 *
 * Usage:
 *   #include <WyDisplay.h>
 *   WyDisplay display;
 *   display.begin();
 *   display.gfx->fillScreen(BLACK);
 *
 * Modules only compiled if WY_HAS_DISPLAY=1 (set by boards.h).
 */

#pragma once
#include "boards.h"

#if WY_HAS_DISPLAY

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

/* ── Common colours ─────────────────────────────────────────────── */
#define WY_BLACK   0x0000
#define WY_WHITE   0xFFFF
#define WY_RED     0xF800
#define WY_GREEN   0x07E0
#define WY_BLUE    0x001F
#define WY_YELLOW  0xFFE0
#define WY_CYAN    0x07FF
#define WY_MAGENTA 0xF81F
#define WY_ORANGE  0xFD20
#define WY_GRAY    0x8410
#define WY_DARKGRAY 0x4208

/* ── Backlight helpers ──────────────────────────────────────────── */
#if WY_DISPLAY_BL_PWM
  #define WY_BL_CHANNEL 0
  static inline void _wy_bl_init() {
      ledcSetup(WY_BL_CHANNEL, 5000, 8);
      ledcAttachPin(WY_DISPLAY_BL, WY_BL_CHANNEL);
      ledcWrite(WY_BL_CHANNEL, 255);
  }
  static inline void _wy_bl_set(uint8_t v) { ledcWrite(WY_BL_CHANNEL, v); }
#else
  static inline void _wy_bl_init() {
      if (WY_DISPLAY_BL >= 0) { pinMode(WY_DISPLAY_BL, OUTPUT); digitalWrite(WY_DISPLAY_BL, HIGH); }
  }
  static inline void _wy_bl_set(uint8_t v) {
      if (WY_DISPLAY_BL >= 0) digitalWrite(WY_DISPLAY_BL, v > 0 ? HIGH : LOW);
  }
#endif

/* ══════════════════════════════════════════════════════════════════
 * RGB parallel panel (Guition 4848S040, Sunton 8048S043, etc.)
 * ══════════════════════════════════════════════════════════════════ */
#if defined(WY_DISPLAY_BUS_RGB16)

class WyDisplay {
public:
    Arduino_GFX *gfx = nullptr;
    uint16_t width  = WY_SCREEN_W;
    uint16_t height = WY_SCREEN_H;

    void begin() {
        #if defined(WY_BOARD_GUITION4848S040)
        auto *rgbbus = new Arduino_ESP32RGBPanel(
            WY_RGB_DE, WY_RGB_VSYNC, WY_RGB_HSYNC,
            WY_RGB_PCLK,
            WY_RGB_R0, WY_RGB_R1, WY_RGB_R2, WY_RGB_R3, WY_RGB_R4,
            WY_RGB_G0, WY_RGB_G1, WY_RGB_G2, WY_RGB_G3, WY_RGB_G4, WY_RGB_G5,
            WY_RGB_B0, WY_RGB_B1, WY_RGB_B2, WY_RGB_B3, WY_RGB_B4);
        gfx = new Arduino_ST7701_RGBPanel(
            rgbbus, GFX_NOT_DEFINED, WY_DISPLAY_ROT, true,
            WY_DISPLAY_W, WY_DISPLAY_H,
            st7701_type1_init_operations, sizeof(st7701_type1_init_operations), true,
            10, 8, 50, 10, 8, 20);

        #elif defined(WY_BOARD_SUNTON_8048S043)
        auto *rgbbus = new Arduino_ESP32RGBPanel(
            WY_RGB_DE, WY_RGB_VSYNC, WY_RGB_HSYNC,
            WY_RGB_PCLK,
            WY_RGB_R0, WY_RGB_R1, WY_RGB_R2, WY_RGB_R3, WY_RGB_R4,
            WY_RGB_G0, WY_RGB_G1, WY_RGB_G2, WY_RGB_G3, WY_RGB_G4, WY_RGB_G5,
            WY_RGB_B0, WY_RGB_B1, WY_RGB_B2, WY_RGB_B3, WY_RGB_B4);
        gfx = new Arduino_RGB_Display(
            WY_DISPLAY_W, WY_DISPLAY_H, rgbbus, WY_DISPLAY_ROT, true);
        #endif

        if (gfx) gfx->begin();
        _wy_bl_init();
    }

    void setBrightness(uint8_t v) { _wy_bl_set(v); }
};

/* ══════════════════════════════════════════════════════════════════
 * 8-bit parallel (WT32-SC01 Plus, etc.)
 * ══════════════════════════════════════════════════════════════════ */
#elif defined(WY_DISPLAY_BUS_PAR8)

class WyDisplay {
public:
    Arduino_GFX *gfx = nullptr;
    uint16_t width  = WY_SCREEN_W;
    uint16_t height = WY_SCREEN_H;

    void begin() {
        auto *bus = new Arduino_ESP32LCD8(
            WY_DISPLAY_DC, GFX_NOT_DEFINED,
            WY_DISPLAY_WR, GFX_NOT_DEFINED,
            WY_DISPLAY_D0, WY_DISPLAY_D1, WY_DISPLAY_D2, WY_DISPLAY_D3,
            WY_DISPLAY_D4, WY_DISPLAY_D5, WY_DISPLAY_D6, WY_DISPLAY_D7);

        #if defined(WY_DISPLAY_ST7796)
        gfx = new Arduino_ST7796(bus, GFX_NOT_DEFINED, WY_DISPLAY_ROT, true);
        #endif

        if (gfx) gfx->begin();
        _wy_bl_init();
    }

    void setBrightness(uint8_t v) { _wy_bl_set(v); }
};

/* ══════════════════════════════════════════════════════════════════
 * SPI displays (CYD ILI9341, ST7789, ST7796, GC9A01)
 * ══════════════════════════════════════════════════════════════════ */
#elif defined(WY_DISPLAY_BUS_SPI)

class WyDisplay {
public:
    Arduino_GFX *gfx = nullptr;
    uint16_t width  = WY_SCREEN_W;
    uint16_t height = WY_SCREEN_H;

    void begin() {
        auto *bus = new Arduino_ESP32SPI(
            WY_DISPLAY_DC, WY_DISPLAY_CS,
            WY_DISPLAY_SCK, WY_DISPLAY_MOSI,
        #ifdef WY_DISPLAY_MISO
            WY_DISPLAY_MISO
        #else
            GFX_NOT_DEFINED
        #endif
        );

        #if defined(WY_DISPLAY_ILI9341)
        gfx = new Arduino_ILI9341(bus, WY_DISPLAY_RST, WY_DISPLAY_ROT);
        #elif defined(WY_DISPLAY_ST7796)
        gfx = new Arduino_ST7796(bus, WY_DISPLAY_RST, WY_DISPLAY_ROT);
        #elif defined(WY_DISPLAY_ST7789)
        gfx = new Arduino_ST7789(bus, WY_DISPLAY_RST, WY_DISPLAY_ROT,
            false, WY_DISPLAY_W, WY_DISPLAY_H);
        #elif defined(WY_DISPLAY_GC9A01) || defined(WY_DISPLAY_GC9107)
        /* GC9107 is a stripped-down GC9A01 — same driver, same init sequence */
        gfx = new Arduino_GC9A01(bus, WY_DISPLAY_RST, WY_DISPLAY_ROT, true);
        #endif

        if (gfx) {
            gfx->begin();
            #ifdef WY_DISPLAY_INVERT
            gfx->invertDisplay(true);
            #endif
        }
        _wy_bl_init();
    }

    void setBrightness(uint8_t v) { _wy_bl_set(v); }
};

#endif  /* bus type */

#else   /* WY_HAS_DISPLAY == 0 */

/* Stub when no display defined */
class WyDisplay {
public:
    void *gfx = nullptr;
    uint16_t width = 0, height = 0;
    void begin() {}
    void setBrightness(uint8_t) {}
};

#endif  /* WY_HAS_DISPLAY */
