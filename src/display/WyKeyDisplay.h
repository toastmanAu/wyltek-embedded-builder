/*
 * WyKeyDisplay.h — Multi-display driver for LilyGo T-Keyboard S3
 * ================================================================
 * The T-Keyboard S3 has 4 mechanical keys, each with a 0.85"
 * GC9107 128×128 display embedded in the keycap.
 *
 * Hardware architecture (from LilyGo official source):
 *   - ONE shared SPI bus (SCK=47, MOSI=48, DC=45, RST=38, BL=39)
 *   - CS is NOT driven by the SPI controller — it's plain GPIO
 *   - Select a display by pulling its CS GPIO LOW, others HIGH
 *   - All 4 share the same Arduino_GFX instance; caller selects
 *     which display receives the data before each draw call
 *
 * CS pins: CS1=12, CS2=13, CS3=14, CS4=21
 * Key pins: KEY1=10, KEY2=9, KEY3=46, KEY4=3
 *
 * The GC9107 uses Arduino_GC9107 driver (distinct from GC9A01,
 * though the silicon is related — GC9107 has different init params).
 *
 * Usage:
 *   #include <display/WyKeyDisplay.h>
 *   WyKeyDisplay keys;
 *   keys.begin();
 *
 *   keys.select(0);                    // point bus at key 0
 *   keys.gfx->fillScreen(WY_BLACK);   // draw to key 0
 *
 *   keys.select(2);
 *   keys.gfx->drawString("CKB", 20, 50, 2);
 *
 *   keys.selectAll();                  // broadcast to all 4
 *   keys.gfx->fillScreen(WY_BLACK);   // clear all at once
 *
 * Key layout (USB-C at bottom):
 *   [0/KEY1] [1/KEY2]
 *   [2/KEY3] [3/KEY4]
 *
 * ⚠️ Only compiled when WY_BOARD_LILYGO_TKEYBOARD_S3 is defined.
 * ⚠️ License note: this is an independent implementation.
 *    LilyGo's official drive code (T-Keyboard_S3_Drive) is GPL 3.0
 *    and is NOT used here. Pin mapping sourced from public hardware docs.
 * ⚠️ Requires Arduino_GFX_Library with GC9107 support.
 *    lib_deps: moononournation/GFX Library for Arduino (>=1.4.7)
 *
 * Ref: github.com/Xinyuan-LilyGO/T-Keyboard-S3
 */

#pragma once
#include "boards.h"

#ifdef WY_BOARD_LILYGO_TKEYBOARD_S3

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

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

#define WY_KDISP_W   128
#define WY_KDISP_H   128
#define WY_KDISP_NUM 4

/* ── WyKeyDisplay class ─────────────────────────────────────────── */
class WyKeyDisplay {
public:
    Arduino_GFX *gfx = nullptr;  /* Single GFX instance — shared bus */
    uint16_t width  = WY_KDISP_W;
    uint16_t height = WY_KDISP_H;

    void begin() {
        /* Init CS pins as outputs, all HIGH (deselected) */
        for (int i = 0; i < WY_KDISP_NUM; i++) {
            pinMode(_cs[i], OUTPUT);
            digitalWrite(_cs[i], HIGH);
        }

        /* Shared SPI bus — CS managed manually via select() */
        auto *bus = new Arduino_ESP32SPI(
            WY_KDISP_DC,          /* DC  */
            GFX_NOT_DEFINED,      /* CS  — managed by select() */
            WY_KDISP_SCK,         /* SCK */
            WY_KDISP_MOSI,        /* MOSI */
            GFX_NOT_DEFINED       /* MISO */
        );

        /* GC9107: 128×128, col_offset1=2, row_offset1=1 (from LilyGo source) */
        gfx = new Arduino_GC9107(
            bus, WY_KDISP_RST, 0 /* rotation */, true /* IPS */,
            WY_KDISP_W, WY_KDISP_H,
            2 /* col_offset1 */, 1 /* row_offset1 */
        );

        /* Pull all CS low for simultaneous init (reset + begin all displays) */
        selectAll();
        if (WY_KDISP_RST >= 0) {
            pinMode(WY_KDISP_RST, OUTPUT);
            digitalWrite(WY_KDISP_RST, LOW);
            delay(10);
            digitalWrite(WY_KDISP_RST, HIGH);
            delay(120);
        }
        gfx->begin();
        gfx->fillScreen(WY_BLACK);

        /* Deselect all */
        for (int i = 0; i < WY_KDISP_NUM; i++) {
            digitalWrite(_cs[i], HIGH);
        }

        /* Backlight on */
        ledcSetup(WY_KDISP_BL_CHAN, 2000, 8);
        ledcAttachPin(WY_KDISP_BL, WY_KDISP_BL_CHAN);
        ledcWrite(WY_KDISP_BL_CHAN, 255);
    }

    /* Select one display (0–3). Deselects all others. */
    void select(uint8_t idx) {
        for (int i = 0; i < WY_KDISP_NUM; i++) {
            digitalWrite(_cs[i], (i == idx) ? LOW : HIGH);
        }
    }

    /* Select all displays — next draw call hits all 4 simultaneously */
    void selectAll() {
        for (int i = 0; i < WY_KDISP_NUM; i++) {
            digitalWrite(_cs[i], LOW);
        }
    }

    /* Deselect all */
    void deselect() {
        for (int i = 0; i < WY_KDISP_NUM; i++) {
            digitalWrite(_cs[i], HIGH);
        }
    }

    /* Set backlight brightness 0–255 */
    void setBrightness(uint8_t v) {
        ledcWrite(WY_KDISP_BL_CHAN, v);
    }

    /* ── Convenience helpers ──────────────────────────────────────
     * Each helper selects the target key, draws, then deselects.   */

    /* Fill all 4 keys with one colour */
    void fillAll(uint16_t colour) {
        selectAll();
        gfx->fillScreen(colour);
        deselect();
    }

    /* Centred label on one key */
    void setLabel(uint8_t idx, const char *text,
                  uint16_t fg = WY_WHITE, uint16_t bg = WY_BLACK,
                  uint8_t textSize = 2) {
        if (idx >= WY_KDISP_NUM) return;
        select(idx);
        gfx->fillScreen(bg);
        gfx->setTextColor(fg);
        gfx->setTextSize(textSize);
        int16_t tw = strlen(text) * 6 * textSize;
        int16_t th = 8 * textSize;
        gfx->setCursor((WY_KDISP_W - tw) / 2, (WY_KDISP_H - th) / 2);
        gfx->print(text);
        deselect();
    }

    /* 2-line metric display: small label top, large value bottom */
    void setMetric(uint8_t idx, const char *label, const char *value,
                   uint16_t labelCol = WY_GRAY, uint16_t valueCol = WY_CYAN,
                   uint16_t bg = WY_BLACK) {
        if (idx >= WY_KDISP_NUM) return;
        select(idx);
        gfx->fillScreen(bg);
        /* Label — small, upper area */
        gfx->setTextSize(1);
        gfx->setTextColor(labelCol);
        int16_t lw = strlen(label) * 6;
        gfx->setCursor((WY_KDISP_W - lw) / 2, 28);
        gfx->print(label);
        /* Value — larger, centred vertically in lower half */
        uint8_t vSize = 3;
        int16_t vw = strlen(value) * 6 * vSize;
        if (vw > WY_KDISP_W - 4) { vSize = 2; vw = strlen(value) * 12; }
        if (vw > WY_KDISP_W - 4) { vSize = 1; vw = strlen(value) * 6; }
        gfx->setTextSize(vSize);
        gfx->setTextColor(valueCol);
        gfx->setCursor((WY_KDISP_W - vw) / 2, 62);
        gfx->print(value);
        deselect();
    }

    /* Read key state (active LOW) */
    bool keyPressed(uint8_t idx) {
        if (idx >= WY_KDISP_NUM) return false;
        return digitalRead(_keyPin[idx]) == LOW;
    }

    /* Attach interrupt to a key */
    void attachKeyInterrupt(uint8_t idx, void (*isr)(), int mode = FALLING) {
        if (idx >= WY_KDISP_NUM) return;
        pinMode(_keyPin[idx], INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(_keyPin[idx]), isr, mode);
    }

private:
    const int8_t _cs[WY_KDISP_NUM]     = { WY_KDISP_CS0, WY_KDISP_CS1,
                                             WY_KDISP_CS2, WY_KDISP_CS3 };
    const int8_t _keyPin[WY_KDISP_NUM] = { WY_KEY1, WY_KEY2, WY_KEY3, WY_KEY4 };
};

#else
/* Stub for non-T-Keyboard builds — zero overhead */
class WyKeyDisplay {
public:
    void *gfx = nullptr;
    uint16_t width = 0, height = 0;
    void begin() {}
    void select(uint8_t) {}
    void selectAll() {}
    void deselect() {}
    void setBrightness(uint8_t) {}
    void fillAll(uint16_t) {}
    void setLabel(uint8_t, const char*, uint16_t=0, uint16_t=0, uint8_t=2) {}
    void setMetric(uint8_t, const char*, const char*, uint16_t=0, uint16_t=0, uint16_t=0) {}
    bool keyPressed(uint8_t) { return false; }
    void attachKeyInterrupt(uint8_t, void(*)(), int=1) {}
};
#endif /* WY_BOARD_LILYGO_TKEYBOARD_S3 */
