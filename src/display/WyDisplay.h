/*
 * WyDisplay.h — Board-specific display abstraction for wyltek-embedded-builder
 * =============================================================================
 * Select board via build flag:
 *   -DWY_BOARD_GUITION4848S040  — Guition ESP32-S3-4848S040, ST7701S 480×480 RGB
 *   -DWY_BOARD_CYD              — ESP32-2432S028R CYD, ILI9341 320×240 SPI
 *
 * Usage:
 *   WyDisplay display;
 *   display.begin();
 *   display.gfx->fillScreen(BLACK);   // access underlying GFX object directly
 */

#pragma once
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

/* ══════════════════════════════════════════════════════════════════
 * Guition ESP32-S3-4848S040
 * ST7701S RGB panel, 480×480, backlight GPIO38
 * ══════════════════════════════════════════════════════════════════ */
#if defined(WY_BOARD_GUITION4848S040)

#define WY_SCREEN_W 480
#define WY_SCREEN_H 480
#define WY_BL_PIN   38

class WyDisplay {
public:
    Arduino_ST7701_RGBPanel *gfx = nullptr;
    uint16_t width  = WY_SCREEN_W;
    uint16_t height = WY_SCREEN_H;

    void begin() {
        auto *bus = new Arduino_ESP32RGBPanel(
            39, 48, 47,
            18, 17, 16, 21,
            11,12,13,14,0,
            8,20,3,46,9,10,
            4,5,6,7,15);
        gfx = new Arduino_ST7701_RGBPanel(
            bus, GFX_NOT_DEFINED, 0, true, WY_SCREEN_W, WY_SCREEN_H,
            st7701_type1_init_operations, sizeof(st7701_type1_init_operations), true,
            10,8,50, 10,8,20);
        gfx->begin();
        pinMode(WY_BL_PIN, OUTPUT);
        digitalWrite(WY_BL_PIN, HIGH);
    }

    void setBrightness(bool on) { digitalWrite(WY_BL_PIN, on ? HIGH : LOW); }
};

/* ══════════════════════════════════════════════════════════════════
 * CYD — ESP32-2432S028R
 * ILI9341 SPI, 320×240, backlight GPIO21
 * ══════════════════════════════════════════════════════════════════ */
#elif defined(WY_BOARD_CYD)

#define WY_SCREEN_W 320
#define WY_SCREEN_H 240
#define WY_BL_PIN   21

#ifndef WY_CYD_CS
  #define WY_CYD_CS   15
#endif
#ifndef WY_CYD_DC
  #define WY_CYD_DC    2
#endif
#ifndef WY_CYD_RST
  #define WY_CYD_RST  -1
#endif
#ifndef WY_CYD_SCK
  #define WY_CYD_SCK  14
#endif
#ifndef WY_CYD_MOSI
  #define WY_CYD_MOSI 13
#endif
#ifndef WY_CYD_MISO
  #define WY_CYD_MISO 12
#endif

class WyDisplay {
public:
    Arduino_ILI9341 *gfx = nullptr;
    uint16_t width  = WY_SCREEN_W;
    uint16_t height = WY_SCREEN_H;

    void begin() {
        auto *bus = new Arduino_ESP32SPI(
            WY_CYD_DC, WY_CYD_CS, WY_CYD_SCK, WY_CYD_MOSI, WY_CYD_MISO);
        gfx = new Arduino_ILI9341(bus, WY_CYD_RST, 1 /* rotation */);
        gfx->begin();
        pinMode(WY_BL_PIN, OUTPUT);
        ledcSetup(0, 5000, 8);
        ledcAttachPin(WY_BL_PIN, 0);
        ledcWrite(0, 255);
    }

    void setBrightness(uint8_t val) { ledcWrite(0, val); }
};

/* ══════════════════════════════════════════════════════════════════
 * Stub — no board selected
 * ══════════════════════════════════════════════════════════════════ */
#else
  #warning "WyDisplay: no board selected — define WY_BOARD_GUITION4848S040 or WY_BOARD_CYD"
  class WyDisplay {
  public:
      void *gfx = nullptr;
      uint16_t width = 0, height = 0;
      void begin() {}
      void setBrightness(uint8_t) {}
  };
#endif
