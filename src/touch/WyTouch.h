/*
 * WyTouch.h — Unified touch abstraction for wyltek-embedded-builder
 * ==================================================================
 * Reads board config from boards.h. Supports:
 *   GT911  — I2C capacitive (Guition 4848S040, Sunton 8048S043)
 *   XPT2046 — SPI resistive (CYD, most budget ESP32 boards)
 *   FT5X06 — I2C capacitive (WT32-SC01 Plus)
 *   CST816S — I2C capacitive (XIAO round display)
 *
 * Usage:
 *   WyTouch touch;
 *   touch.begin();
 *   if (touch.update()) { Serial.printf("%d %d\n", touch.x, touch.y); }
 */

#pragma once
#include "boards.h"

#if WY_HAS_TOUCH

#include <Arduino.h>

/* ══════════════════════════════════════════════════════════════════
 * GT911 — I2C capacitive
 * ══════════════════════════════════════════════════════════════════ */
#if defined(WY_TOUCH_GT911)

#include <Wire.h>
#include "GT911.h"

/* Override GT911 default pins from board config */
#undef  GT911_SDA
#undef  GT911_SCL
#undef  GT911_INT
#undef  GT911_RST
#undef  GT911_ADDR
#define GT911_SDA   WY_TOUCH_SDA
#define GT911_SCL   WY_TOUCH_SCL
#define GT911_INT   WY_TOUCH_INT
#define GT911_RST   WY_TOUCH_RST
#define GT911_ADDR  WY_TOUCH_ADDR

class WyTouch {
public:
    int     x = 0, y = 0;
    bool    pressed = false;
    uint8_t points  = 0;

    bool begin() {
        bool ok = _gt.begin();
        return ok;
    }
    bool update() {
        bool r = _gt.update();
        x = _gt.x; y = _gt.y;
        pressed = _gt.pressed;
        points  = _gt.points;
        return r;
    }
private:
    GT911 _gt;
};

/* ══════════════════════════════════════════════════════════════════
 * XPT2046 — SPI resistive
 * ══════════════════════════════════════════════════════════════════ */
#elif defined(WY_TOUCH_XPT2046)

#include <SPI.h>
#include <XPT2046_Touchscreen.h>

class WyTouch {
public:
    int     x = 0, y = 0;
    bool    pressed = false;
    uint8_t points  = 0;

    bool begin() {
        _spi.begin(WY_TOUCH_SCK, WY_TOUCH_MISO, WY_TOUCH_MOSI, WY_TOUCH_CS);
        _ts.begin(_spi);
        _ts.setRotation(WY_DISPLAY_ROT);
        return true;
    }
    bool update() {
        if (!_ts.tirqTouched() || !_ts.touched()) {
            pressed = false; return false;
        }
        TS_Point p = _ts.getPoint();
        x = map(p.x, WY_TOUCH_X_MIN, WY_TOUCH_X_MAX, 0, WY_SCREEN_W);
        y = map(p.y, WY_TOUCH_Y_MIN, WY_TOUCH_Y_MAX, 0, WY_SCREEN_H);
        x = constrain(x, 0, WY_SCREEN_W - 1);
        y = constrain(y, 0, WY_SCREEN_H - 1);
        pressed = true;
        points  = 1;
        return true;
    }
private:
    SPIClass _spi{HSPI};
    XPT2046_Touchscreen _ts{WY_TOUCH_CS, WY_TOUCH_IRQ};
};

/* ══════════════════════════════════════════════════════════════════
 * FT5X06 — I2C capacitive (WT32-SC01 Plus)
 * ══════════════════════════════════════════════════════════════════ */
#elif defined(WY_TOUCH_FT5X06)

#include <Wire.h>

#define FT5X06_REG_TOUCH_NUM  0x02
#define FT5X06_REG_POINT1_XH  0x03

class WyTouch {
public:
    int     x = 0, y = 0;
    bool    pressed = false;
    uint8_t points  = 0;

    bool begin() {
        Wire.begin(WY_TOUCH_SDA, WY_TOUCH_SCL);
        Wire.setClock(400000);
        Wire.beginTransmission(WY_TOUCH_ADDR);
        return Wire.endTransmission() == 0;
    }
    bool update() {
        Wire.beginTransmission(WY_TOUCH_ADDR);
        Wire.write(FT5X06_REG_TOUCH_NUM);
        if (Wire.endTransmission(false) != 0) { pressed = false; return false; }
        Wire.requestFrom((uint8_t)WY_TOUCH_ADDR, (uint8_t)5);
        uint8_t n   = Wire.read() & 0x0F;
        uint8_t xh  = Wire.read();
        uint8_t xl  = Wire.read();
        uint8_t yh  = Wire.read();
        uint8_t yl  = Wire.read();
        if (n == 0) { pressed = false; points = 0; return false; }
        x = ((xh & 0x0F) << 8) | xl;
        y = ((yh & 0x0F) << 8) | yl;
        points = n;
        pressed = true;
        return true;
    }
};

/* ══════════════════════════════════════════════════════════════════
 * CST816S — I2C capacitive (XIAO round display)
 * ══════════════════════════════════════════════════════════════════ */
#elif defined(WY_TOUCH_CST816S)

#include <Wire.h>
#define CST816S_REG_GESTURE   0x01
#define CST816S_REG_TOUCH_NUM 0x02
#define CST816S_REG_XH        0x03

class WyTouch {
public:
    int     x = 0, y = 0;
    bool    pressed = false;
    uint8_t points  = 0;
    uint8_t gesture = 0;

    bool begin() {
        Wire.begin(WY_TOUCH_SDA, WY_TOUCH_SCL);
        Wire.setClock(400000);
        Wire.beginTransmission(WY_TOUCH_ADDR);
        return Wire.endTransmission() == 0;
    }
    bool update() {
        Wire.beginTransmission(WY_TOUCH_ADDR);
        Wire.write(CST816S_REG_GESTURE);
        if (Wire.endTransmission(false) != 0) { pressed = false; return false; }
        Wire.requestFrom((uint8_t)WY_TOUCH_ADDR, (uint8_t)6);
        gesture     = Wire.read();
        uint8_t n   = Wire.read() & 0x0F;
        uint8_t xh  = Wire.read();
        uint8_t xl  = Wire.read();
        uint8_t yh  = Wire.read();
        uint8_t yl  = Wire.read();
        if (n == 0) { pressed = false; points = 0; return false; }
        x = ((xh & 0x0F) << 8) | xl;
        y = ((yh & 0x0F) << 8) | yl;
        points = n;
        pressed = true;
        return true;
    }
};

#endif /* touch backend */

#else  /* WY_HAS_TOUCH == 0 */

class WyTouch {
public:
    int x = 0, y = 0;
    bool pressed = false;
    uint8_t points = 0;
    bool begin()  { return false; }
    bool update() { return false; }
};

#endif /* WY_HAS_TOUCH */
