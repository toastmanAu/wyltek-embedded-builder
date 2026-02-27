/*
 * drivers/WyAHT20.h — AHT20 / AHT21 / AHT10 temperature + humidity (I2C)
 * =========================================================================
 * Datasheet: https://asairsensors.com/wp-content/uploads/2021/09/Data-Sheet-AHT20-ASAIR-V1.0.03.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x38 (fixed, not selectable)
 * Registered via WySensors::addI2C<WyAHT20>("name", sda, scl, 0x38)
 *
 * Also compatible with: AHT21, AHT10, AM2301B
 */

#pragma once
#include "../WySensors.h"

#define AHT20_ADDR          0x38
#define AHT20_CMD_INIT      0xBE  /* AHT20 init command */
#define AHT10_CMD_INIT      0xE1  /* AHT10 uses different init */
#define AHT20_CMD_TRIGGER   0xAC  /* trigger measurement */
#define AHT20_CMD_RESET     0xBA  /* soft reset */
#define AHT20_STATUS_BUSY   0x80  /* bit 7 = busy */
#define AHT20_STATUS_CAL    0x08  /* bit 3 = calibrated */

class WyAHT20 : public WySensorBase {
public:
    WyAHT20(WyI2CPins pins, bool isAHT10 = false)
        : _pins(pins), _isAHT10(isAHT10) {}

    const char* driverName() override { return _isAHT10 ? "AHT10" : "AHT20"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);
        delay(40);  /* AHT20 needs 40ms after power-on */

        /* Check status — if not calibrated, send init command */
        uint8_t status = _readStatus();
        if (!(status & AHT20_STATUS_CAL)) {
            _sendInit();
            delay(10);
            status = _readStatus();
            if (!(status & AHT20_STATUS_CAL)) {
                Serial.println("[AHT20] calibration failed");
                return false;
            }
        }
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        /* Trigger measurement: 0xAC 0x33 0x00 */
        Wire.beginTransmission(_pins.addr);
        Wire.write(AHT20_CMD_TRIGGER);
        Wire.write(0x33);
        Wire.write(0x00);
        Wire.endTransmission();

        /* Wait for measurement (max 80ms) */
        delay(80);
        uint32_t t = millis();
        while (_readStatus() & AHT20_STATUS_BUSY) {
            if (millis() - t > 100) { d.error = "timeout"; return d; }
            delay(5);
        }

        /* Read 6 bytes: status, hum[20bit], temp[20bit] */
        Wire.requestFrom(_pins.addr, (uint8_t)6);
        if (Wire.available() < 6) { d.error = "no data"; return d; }

        uint8_t buf[6];
        for (uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();

        /* Parse humidity: bits 19:4 of bytes 1-3 */
        uint32_t raw_h = ((uint32_t)(buf[1]) << 12)
                       | ((uint32_t)(buf[2]) << 4)
                       | (buf[3] >> 4);

        /* Parse temperature: bits 19:0 of bytes 3-5 */
        uint32_t raw_t = ((uint32_t)(buf[3] & 0x0F) << 16)
                       | ((uint32_t)(buf[4]) << 8)
                       |  buf[5];

        d.humidity    = (raw_h * 100.0f) / 1048576.0f;       /* / 2^20 * 100 */
        d.temperature = (raw_t * 200.0f / 1048576.0f) - 50.0f; /* / 2^20 * 200 - 50 */
        d.ok = true;
        return d;
    }

private:
    WyI2CPins _pins;
    bool      _isAHT10;

    uint8_t _readStatus() {
        Wire.requestFrom(_pins.addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0xFF;
    }

    void _sendInit() {
        Wire.beginTransmission(_pins.addr);
        Wire.write(_isAHT10 ? AHT10_CMD_INIT : AHT20_CMD_INIT);
        Wire.write(0x08);
        Wire.write(0x00);
        Wire.endTransmission();
    }
};

/* Aliases */
using WyAHT21   = WyAHT20;
using WyAM2301B = WyAHT20;
class WyAHT10 : public WyAHT20 {
public:
    WyAHT10(WyI2CPins pins) : WyAHT20(pins, true) {}
};
