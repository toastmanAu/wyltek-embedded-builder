/*
 * drivers/WyBH1750.h — BH1750 ambient light sensor (I2C)
 * ========================================================
 * Datasheet: https://www.mouser.com/datasheet/2/348/bh1750fvi-e-186247.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x23 (ADDR pin LOW) or 0x5C (ADDR pin HIGH)
 * Registered via WySensors::addI2C<WyBH1750>("name", sda, scl, 0x23)
 *
 * Measurement modes:
 *   BH1750_MODE_CONT_HIGH  — 1 lux resolution, 120ms, continuous (default)
 *   BH1750_MODE_CONT_HIGH2 — 0.5 lux resolution, 120ms, continuous
 *   BH1750_MODE_CONT_LOW   — 4 lux resolution, 16ms, continuous (fast)
 *   BH1750_MODE_ONE_HIGH   — one-shot high res (sensor powers down after)
 */

#pragma once
#include "../WySensors.h"

/* BH1750 instruction set (sent directly as 1-byte commands) */
#define BH1750_POWER_DOWN       0x00
#define BH1750_POWER_ON         0x01
#define BH1750_RESET            0x07  /* clears data register */
#define BH1750_MODE_CONT_HIGH   0x10  /* continuous high res — 1 lux, 120ms */
#define BH1750_MODE_CONT_HIGH2  0x11  /* continuous high res 2 — 0.5 lux, 120ms */
#define BH1750_MODE_CONT_LOW    0x13  /* continuous low res — 4 lux, 16ms */
#define BH1750_MODE_ONE_HIGH    0x20  /* one-shot high res */
#define BH1750_MODE_ONE_HIGH2   0x21
#define BH1750_MODE_ONE_LOW     0x23

class WyBH1750 : public WySensorBase {
public:
    WyBH1750(WyI2CPins pins, uint8_t mode = BH1750_MODE_CONT_HIGH)
        : _pins(pins), _mode(mode) {}

    const char* driverName() override { return "BH1750"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);

        _sendCmd(BH1750_POWER_ON);
        delay(10);
        _sendCmd(BH1750_RESET);
        delay(10);
        _sendCmd(_mode);

        /* Verify by reading a plausible value (all 0xFF = not connected) */
        delay(180);  /* wait for first measurement */
        Wire.requestFrom(_pins.addr, (uint8_t)2);
        if (!Wire.available()) return false;
        uint8_t hi = Wire.read();
        uint8_t lo = Wire.read();
        uint16_t raw = (hi << 8) | lo;
        return (raw != 0xFFFF);
    }

    WySensorData read() override {
        WySensorData d;

        /* For one-shot modes, trigger before read */
        bool oneShot = (_mode == BH1750_MODE_ONE_HIGH  ||
                        _mode == BH1750_MODE_ONE_HIGH2 ||
                        _mode == BH1750_MODE_ONE_LOW);
        if (oneShot) {
            _sendCmd(_mode);
            delay(_mode == BH1750_MODE_ONE_LOW ? 24 : 180);
        }

        Wire.requestFrom(_pins.addr, (uint8_t)2);
        if (Wire.available() < 2) { d.error = "no data"; return d; }

        uint16_t raw = ((uint16_t)Wire.read() << 8) | Wire.read();

        /* Convert: lux = raw / 1.2 (basic factor from datasheet)
         * For HIGH2 mode: lux = raw / 2.4 (0.5 lux resolution) */
        if (_mode == BH1750_MODE_CONT_HIGH2 || _mode == BH1750_MODE_ONE_HIGH2)
            d.light = raw / 2.4f;
        else
            d.light = raw / 1.2f;

        d.ok = true;
        return d;
    }

    /* Change measurement mode on the fly */
    void setMode(uint8_t mode) {
        _mode = mode;
        _sendCmd(_mode);
    }

private:
    WyI2CPins _pins;
    uint8_t   _mode;

    void _sendCmd(uint8_t cmd) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(cmd);
        Wire.endTransmission();
    }
};
