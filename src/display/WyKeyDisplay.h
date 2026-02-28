/*
 * WyKeyDisplay.h — Multi-display driver for LilyGo T-Keyboard S3 Pro
 * ====================================================================
 * The T-Keyboard S3 Pro has 4 mechanical keys, each with its own
 * 0.85" GC9107 128×128 display embedded in the keycap. All 4 displays
 * share the same SPI bus (SCK/MOSI) but have individual CS pins.
 *
 * The GC9107 uses the GC9A01 driver (identical init sequence).
 *
 * Usage:
 *   #include <display/WyKeyDisplay.h>
 *   WyKeyDisplay keys;
 *   keys.begin();
 *   keys.key[0]->fillScreen(WY_BLACK);         // draw on key 0
 *   keys.key[2]->drawString("CKB", 20, 50, 2); // draw on key 2
 *   keys.key[3]->drawNumber(blockHeight, 10, 50, 4);
 *
 * Key layout (when USB-C is at bottom):
 *   [0] [1]
 *   [2] [3]
 *
 * Pin mapping (T-Keyboard S3 Pro default):
 *   SCK   = 3   MOSI  = 2   DC = 42
 *   CS0   = 10  CS1   = 11  CS2 = 12  CS3 = 13
 *   BL    = 38  RST   = -1
 *
 * ⚠️ Only compiled when WY_BOARD_LILYGO_TKEYBOARD_S3 is defined.
 * ⚠️ WyDisplay.h handles single-display boards — do NOT include both.
 * ⚠️ Requires Arduino_GFX_Library in lib_deps.
 *
 * Ref: github.com/Xinyuan-LilyGO/T-Keyboard-S3-Pro
 */

#pragma once
#include "boards.h"

#ifdef WY_BOARD_LILYGO_TKEYBOARD_S3

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

/* ── Pin definitions ────────────────────────────────────────────── */
/* Override these before including this header if your unit differs  */
#ifndef WY_KDISP_SCK
  #define WY_KDISP_SCK    3
#endif
#ifndef WY_KDISP_MOSI
  #define WY_KDISP_MOSI   2
#endif
#ifndef WY_KDISP_DC
  #define WY_KDISP_DC     42
#endif
#ifndef WY_KDISP_RST
  #define WY_KDISP_RST    GFX_NOT_DEFINED
#endif
#ifndef WY_KDISP_BL
  #define WY_KDISP_BL     38
#endif
/* CS pin for each key display */
#ifndef WY_KDISP_CS0
  #define WY_KDISP_CS0    10
#endif
#ifndef WY_KDISP_CS1
  #define WY_KDISP_CS1    11
#endif
#ifndef WY_KDISP_CS2
  #define WY_KDISP_CS2    12
#endif
#ifndef WY_KDISP_CS3
  #define WY_KDISP_CS3    13
#endif

#define WY_KDISP_W   128
#define WY_KDISP_H   128
#define WY_KDISP_NUM 4

/* ── Colours (shared with WyDisplay.h) ─────────────────────────── */
#ifndef WY_BLACK
  #define WY_BLACK    0x0000
  #define WY_WHITE    0xFFFF
  #define WY_RED      0xF800
  #define WY_GREEN    0x07E0
  #define WY_BLUE     0x001F
  #define WY_YELLOW   0xFFE0
  #define WY_CYAN     0x07FF
  #define WY_MAGENTA  0xF81F
  #define WY_ORANGE   0xFD20
  #define WY_GRAY     0x8410
  #define WY_DARKGRAY 0x4208
#endif

/* ── WyKeyDisplay class ─────────────────────────────────────────── */
class WyKeyDisplay {
public:
    /* One Arduino_GFX pointer per key — draw to each independently */
    Arduino_GFX *key[WY_KDISP_NUM] = {};
    uint16_t width  = WY_KDISP_W;
    uint16_t height = WY_KDISP_H;

    void begin() {
        static const int8_t cs_pins[WY_KDISP_NUM] = {
            WY_KDISP_CS0, WY_KDISP_CS1, WY_KDISP_CS2, WY_KDISP_CS3
        };

        for (int i = 0; i < WY_KDISP_NUM; i++) {
            auto *bus = new Arduino_ESP32SPI(
                WY_KDISP_DC, cs_pins[i],
                WY_KDISP_SCK, WY_KDISP_MOSI,
                GFX_NOT_DEFINED   /* MISO not needed */
            );
            /* GC9107 = GC9A01 with different part number — same init */
            key[i] = new Arduino_GC9A01(bus, WY_KDISP_RST, 0, true);
            key[i]->begin();
            key[i]->fillScreen(WY_BLACK);
        }

        /* Backlight on */
        if (WY_KDISP_BL >= 0) {
            pinMode(WY_KDISP_BL, OUTPUT);
            digitalWrite(WY_KDISP_BL, HIGH);
        }
    }

    /* Fill all 4 keys with same colour */
    void fillAll(uint16_t colour) {
        for (int i = 0; i < WY_KDISP_NUM; i++) {
            if (key[i]) key[i]->fillScreen(colour);
        }
    }

    /* Draw a centred label on one key (uses built-in GFX font) */
    void setLabel(uint8_t idx, const char *text,
                  uint16_t fg = WY_WHITE, uint16_t bg = WY_BLACK,
                  uint8_t textSize = 2) {
        if (idx >= WY_KDISP_NUM || !key[idx]) return;
        key[idx]->fillScreen(bg);
        key[idx]->setTextColor(fg);
        key[idx]->setTextSize(textSize);
        /* Rough centre — 6px per char per textSize unit */
        int16_t tw = strlen(text) * 6 * textSize;
        int16_t th = 8 * textSize;
        key[idx]->setCursor((WY_KDISP_W - tw) / 2, (WY_KDISP_H - th) / 2);
        key[idx]->print(text);
    }

    /* Draw a 2-line label (top = small, bottom = large value) */
    void setMetric(uint8_t idx, const char *label, const char *value,
                   uint16_t labelCol = WY_GRAY, uint16_t valueCol = WY_CYAN,
                   uint16_t bg = WY_BLACK) {
        if (idx >= WY_KDISP_NUM || !key[idx]) return;
        key[idx]->fillScreen(bg);
        /* Label — small, upper third */
        key[idx]->setTextSize(1);
        key[idx]->setTextColor(labelCol);
        int16_t lw = strlen(label) * 6;
        key[idx]->setCursor((WY_KDISP_W - lw) / 2, 30);
        key[idx]->print(label);
        /* Value — larger, lower half */
        key[idx]->setTextSize(2);
        key[idx]->setTextColor(valueCol);
        int16_t vw = strlen(value) * 12;
        if (vw > WY_KDISP_W) { key[idx]->setTextSize(1); vw = strlen(value)*6; }
        key[idx]->setCursor((WY_KDISP_W - vw) / 2, 65);
        key[idx]->print(value);
    }
};

#else
/* Stub for non-T-Keyboard builds */
class WyKeyDisplay {
public:
    void *key[4] = {};
    void begin() {}
    void fillAll(uint16_t) {}
    void setLabel(uint8_t, const char*, uint16_t=0, uint16_t=0, uint8_t=2) {}
    void setMetric(uint8_t, const char*, const char*, uint16_t=0, uint16_t=0, uint16_t=0) {}
};
#endif /* WY_BOARD_LILYGO_TKEYBOARD_S3 */
