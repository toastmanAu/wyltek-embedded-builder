/*
 * drivers/WyPAJ7620.h — PAJ7620U2 gesture sensor (I2C)
 * ======================================================
 * Datasheet: PixArt PAJ7620U2 (GDS v1.0)
 * Application note: PAJ7620U2 Register List v1.0
 * Bundled driver — no external library needed.
 * I2C address: 0x73 (fixed)
 * Registered via WySensors::addI2C<WyPAJ7620>("gesture", sda, scl, 0x73)
 *
 * Detects 9 gestures:
 *   Up, Down, Left, Right — directional swipes
 *   Forward, Backward     — push toward / pull away from sensor
 *   Clockwise, Anti-CW    — circular motion in front of sensor
 *   Wave                  — fast left-right oscillation
 *
 * How it works:
 *   PAJ7620 has an internal 8×8 IR image array and onboard DSP.
 *   It processes motion internally and outputs a gesture code via interrupt.
 *   No raw image data is exposed — just the gesture result register.
 *   Works in complete darkness (IR LED built in).
 *   Reliable sensing range: ~5–15cm from sensor face.
 *
 * INIT SEQUENCE:
 *   The PAJ7620 requires ~67 initialisation register writes from the
 *   PixArt application note ("initialise sequence table"). These are
 *   magic values that configure the internal DSP — do not skip them.
 *   Bank must be switched to BANK0 before the sequence and BANK1 after.
 *
 * BANK SWITCHING:
 *   PAJ7620 has two register banks:
 *   BANK0 — gesture results, interrupt flags (normal operation)
 *   BANK1 — configuration registers (init sequence only)
 *   Switch via: write 0xEF → 0x00 (BANK0) or 0xEF → 0x01 (BANK1)
 *
 * INTERRUPT:
 *   INT pin goes LOW when a gesture is detected.
 *   Polling mode (no INT pin) is supported — just read REG_INT_FLAG_1 periodically.
 *   For reliable gesture capture, poll at ≥10ms intervals.
 *
 * WIRING:
 *   VCC  → 3.3V (do NOT use 5V — PAJ7620 is 3.3V only)
 *   GND  → GND
 *   SDA  → I2C SDA (4.7kΩ pull-up)
 *   SCL  → I2C SCL (4.7kΩ pull-up)
 *   INT  → GPIO (optional, active LOW — enables interrupt-driven mode)
 *
 * IMPORTANT — Reliable detection tips:
 *   1. Move hand deliberately — fast swipes (0.1–0.5s) work best
 *   2. Stay 5–15cm from sensor — too close or too far = misread
 *   3. No strong ambient IR (direct sunlight causes false triggers)
 *   4. After gesture read, clear the interrupt flags or it latches
 *   5. 100kHz I2C is more reliable than 400kHz for this chip
 *   6. Add 10µF cap on VCC — sensor draws burst current on LED pulse
 *
 * WySensorData:
 *   d.rawInt = gesture code (WY_GESTURE_*)
 *   d.raw    = same as rawInt
 *   d.ok     = true when a gesture was detected (false = no new gesture)
 *
 * Usage:
 *   WySensorData d = sensors.read("gesture");
 *   if (d.ok) {
 *     switch (d.rawInt) {
 *       case WY_GESTURE_UP:    ...; break;
 *       case WY_GESTURE_DOWN:  ...; break;
 *       case WY_GESTURE_LEFT:  ...; break;
 *       case WY_GESTURE_RIGHT: ...; break;
 *     }
 *   }
 */

#pragma once
#include "../WySensors.h"

/* Gesture result codes */
#define WY_GESTURE_NONE         0x00
#define WY_GESTURE_RIGHT        0x01
#define WY_GESTURE_LEFT         0x02
#define WY_GESTURE_UP           0x04
#define WY_GESTURE_DOWN         0x08
#define WY_GESTURE_FORWARD      0x10
#define WY_GESTURE_BACKWARD     0x20
#define WY_GESTURE_CLOCKWISE    0x40
#define WY_GESTURE_ANTICLOCKWISE 0x80
#define WY_GESTURE_WAVE         0x01  /* from INT_FLAG_2 register */

/* Key registers (BANK0) */
#define PAJ7620_REG_BANK_SEL    0xEF  /* 0x00=BANK0, 0x01=BANK1 */
#define PAJ7620_REG_INT_FLAG_1  0x43  /* gesture result bits 7:0 */
#define PAJ7620_REG_INT_FLAG_2  0x44  /* gesture result bits 8 (wave) */
#define PAJ7620_REG_STATE       0x45  /* gesture state machine */
#define PAJ7620_REG_OBJ_BRIGHT  0xB0  /* object brightness (approach detect) */
#define PAJ7620_REG_OBJ_SIZE_H  0xB2  /* object size high byte */
#define PAJ7620_REG_OBJ_SIZE_L  0xB3  /* object size low byte */

/* Partid verification registers (BANK0) */
#define PAJ7620_REG_PARTID_L    0x00  /* should read 0x20 */
#define PAJ7620_REG_PARTID_H    0x01  /* should read 0x76 */

#define PAJ7620_PARTID_L        0x20
#define PAJ7620_PARTID_H        0x76

/* Operating modes */
#define PAJ7620_MODE_GESTURE    0x00  /* gesture detection */
#define PAJ7620_MODE_PROXIMITY  0x01  /* proximity detection */

/*
 * Initialisation sequence from PixArt application note.
 * These are {register, value} pairs written to BANK1 then BANK0.
 * DO NOT modify — these configure internal DSP parameters.
 */
static const uint8_t PAJ7620_INIT_SEQ[][2] = {
    /* BANK1 init */
    {0x00, 0x1E}, {0x01, 0x1E}, {0x02, 0x0F}, {0x03, 0x10},
    {0x04, 0x02}, {0x05, 0x00}, {0x06, 0xB0}, {0x07, 0x04},
    {0x08, 0x01}, {0x09, 0x07}, {0x0A, 0x08}, {0x0C, 0x01},
    {0x0D, 0x00}, {0x0E, 0x00}, {0x0F, 0x01}, {0x10, 0x01},
    {0x11, 0x00}, {0x13, 0x01}, {0x14, 0x01}, {0x15, 0x1C},
    {0x16, 0x00}, {0x17, 0x01}, {0x18, 0x00}, {0x19, 0x00},
    {0x1A, 0x00}, {0x1B, 0x00}, {0x1C, 0x00}, {0x1D, 0x00},
    {0x1E, 0x00}, {0x21, 0x00}, {0x22, 0x00}, {0x23, 0x00},
    {0x25, 0x01}, {0x26, 0x00}, {0x27, 0x39}, {0x28, 0x7F},
    {0x29, 0x08}, {0x30, 0x03}, {0x31, 0x00}, {0x32, 0xA9},
    {0x33, 0x00}, {0x34, 0x00}, {0x35, 0x01}, {0x40, 0x02},
    {0x41, 0x01}, {0x42, 0x02}, {0x43, 0x03}, {0x44, 0x00},
    {0x45, 0x7C}, {0x46, 0x00}, {0x47, 0x7C}, {0x48, 0x07},
    {0x49, 0x00}, {0x4A, 0x00}, {0x4C, 0x01}, {0x4D, 0x00},
    {0x51, 0x10}, {0x5E, 0x10}, {0x60, 0x27}, {0x80, 0x42},
    {0x81, 0x44}, {0x82, 0x04}, {0x8B, 0x01}, {0x90, 0x06},
    {0x95, 0x0A}, {0x96, 0x0C}, {0x97, 0x05}, {0x9A, 0x14},
    {0x9C, 0x3F},
};
#define PAJ7620_INIT_SEQ_LEN  (sizeof(PAJ7620_INIT_SEQ) / sizeof(PAJ7620_INIT_SEQ[0]))

/* BANK0 gesture mode registers */
static const uint8_t PAJ7620_GESTURE_SEQ[][2] = {
    {0x41, 0x00}, {0x42, 0x00},
};

static const char* const PAJ7620_GESTURE_NAMES[] = {
    "None", "Right", "Left", "?", "Up", "?", "?", "?",
    "Down", "?", "?", "?", "?", "?", "?", "?",
    "Forward", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "Backward", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "Clockwise", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "?", "?", "?", "?", "?", "?", "?", "?",
    "AntiCW"
};

class WyPAJ7620 : public WySensorBase {
public:
    WyPAJ7620(WyI2CPins pins, int8_t intPin = -1)
        : _pins(pins), _intPin(intPin) {}

    const char* driverName() override { return "PAJ7620"; }

    bool begin() override {
        /* PAJ7620 needs 700µs after power-on before I2C is ready */
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(100000);  /* 100kHz — more reliable than 400kHz for this chip */
        delay(5);

        /* Wake up: dummy read to transition from sleep to normal */
        Wire.beginTransmission(_pins.addr);
        Wire.write(0x00);
        Wire.endTransmission();
        delay(5);

        /* Verify part ID (BANK0) */
        _selectBank(0);
        uint8_t id_l = _readReg8(PAJ7620_REG_PARTID_L);
        uint8_t id_h = _readReg8(PAJ7620_REG_PARTID_H);
        if (id_l != PAJ7620_PARTID_L || id_h != PAJ7620_PARTID_H) {
            Serial.printf("[PAJ7620] wrong ID: 0x%02X%02X (expected 0x%02X%02X)\n",
                id_h, id_l, PAJ7620_PARTID_H, PAJ7620_PARTID_L);
            return false;
        }

        /* Write BANK1 init sequence */
        _selectBank(1);
        for (uint8_t i = 0; i < PAJ7620_INIT_SEQ_LEN; i++) {
            _writeReg(PAJ7620_INIT_SEQ[i][0], PAJ7620_INIT_SEQ[i][1]);
        }

        /* Switch back to BANK0 and configure for gesture mode */
        _selectBank(0);
        for (uint8_t i = 0; i < 2; i++) {
            _writeReg(PAJ7620_GESTURE_SEQ[i][0], PAJ7620_GESTURE_SEQ[i][1]);
        }

        /* Configure INT pin if provided */
        if (_intPin >= 0) {
            pinMode(_intPin, INPUT_PULLUP);
        }

        /* Clear any pending gesture flags */
        _clearFlags();

        delay(10);
        Serial.println("[PAJ7620] initialised — gesture mode ready");
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        /* If INT pin configured, only read on interrupt */
        if (_intPin >= 0 && digitalRead(_intPin) == HIGH) {
            d.ok = false;  /* no new gesture */
            return d;
        }

        /* Read gesture result registers */
        uint8_t flag1 = _readReg8(PAJ7620_REG_INT_FLAG_1);
        uint8_t flag2 = _readReg8(PAJ7620_REG_INT_FLAG_2);

        /* Clear flags immediately after read (prevents re-triggering) */
        _clearFlags();

        if (flag1 == 0 && (flag2 & 0x01) == 0) {
            d.ok = false;
            return d;
        }

        uint8_t gesture = WY_GESTURE_NONE;
        if (flag1)        gesture = flag1;
        else if (flag2 & 0x01) gesture = WY_GESTURE_WAVE;

        /* If multiple bits set, pick the highest priority (most significant) */
        if (__builtin_popcount(flag1) > 1) {
            /* Multiple gestures — pick highest priority bit */
            for (int8_t bit = 7; bit >= 0; bit--) {
                if (flag1 & (1 << bit)) { gesture = (1 << bit); break; }
            }
        }

        d.rawInt = gesture;
        d.raw    = (float)gesture;
        d.ok     = (gesture != WY_GESTURE_NONE);
        _lastGesture = gesture;
        return d;
    }

    /* ── Convenience ─────────────────────────────────────────────── */

    /* Human-readable gesture name */
    const char* gestureName(uint8_t g = 0) {
        uint8_t code = g ? g : _lastGesture;
        if (code == 0)    return "None";
        if (code == 0x01) return "Right";
        if (code == 0x02) return "Left";
        if (code == 0x04) return "Up";
        if (code == 0x08) return "Down";
        if (code == 0x10) return "Forward";
        if (code == 0x20) return "Backward";
        if (code == 0x40) return "Clockwise";
        if (code == 0x80) return "AntiClockwise";
        if (code == WY_GESTURE_WAVE) return "Wave";
        return "Unknown";
    }

    uint8_t lastGesture() { return _lastGesture; }

    /* Object proximity — brightness of nearest object (0=none, 255=close) */
    uint8_t proximity() {
        _selectBank(0);
        return _readReg8(PAJ7620_REG_OBJ_BRIGHT);
    }

    /* Object size in pixels (0–900 on 30×30 internal array) */
    uint16_t objectSize() {
        _selectBank(0);
        uint8_t h = _readReg8(PAJ7620_REG_OBJ_SIZE_H);
        uint8_t l = _readReg8(PAJ7620_REG_OBJ_SIZE_L);
        return ((uint16_t)h << 8) | l;
    }

    /* Switch between gesture and proximity modes */
    void setGestureMode() {
        _selectBank(0);
        _writeReg(PAJ7620_REG_STATE, 0x00);
    }
    void setProximityMode() {
        _selectBank(0);
        _writeReg(PAJ7620_REG_STATE, 0x01);
    }

private:
    WyI2CPins _pins;
    int8_t    _intPin;
    uint8_t   _lastGesture = 0;
    uint8_t   _currentBank = 0xFF;  /* unknown at start */

    void _selectBank(uint8_t bank) {
        if (_currentBank == bank) return;
        _writeReg(PAJ7620_REG_BANK_SEL, bank);
        _currentBank = bank;
        delayMicroseconds(100);
    }

    void _clearFlags() {
        _readReg8(PAJ7620_REG_INT_FLAG_1);
        _readReg8(PAJ7620_REG_INT_FLAG_2);
    }

    void _writeReg(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }

    uint8_t _readReg8(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0xFF;
    }
};
