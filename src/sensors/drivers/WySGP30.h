/*
 * drivers/WySGP30.h — SGP30 air quality sensor — eCO2 + TVOC (I2C)
 * ==================================================================
 * Datasheet: https://sensirion.com/media/documents/984E0DD5/61644B8B/Sensirion_Gas_Sensors_Datasheet_SGP30.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x58 (fixed)
 * Registered via WySensors::addI2C<WySGP30>("air", sda, scl, 0x58)
 *
 * Measures:
 *   - eCO2: equivalent CO2 (ppm) — 400–60000
 *   - TVOC: total volatile organic compounds (ppb) — 0–60000
 *
 * IMPORTANT: SGP30 requires a 15-second warm-up on first power.
 *            Outputs eCO2=400, TVOC=0 during warm-up (normal).
 *            For accurate readings, run continuously — algorithm improves over time.
 *            Store baseline registers in NVS and reload on boot (see setBaseline()).
 *
 * CRC: SGP30 uses CRC-8 (poly 0x31, init 0xFF) on every 2-byte word.
 */

#pragma once
#include "../WySensors.h"

/* SGP30 command words (16-bit, sent as 2 bytes MSB first) */
#define SGP30_CMD_INIT_AIR_QUALITY    0x2003
#define SGP30_CMD_MEASURE_AIR_QUALITY 0x2008
#define SGP30_CMD_GET_BASELINE        0x2015
#define SGP30_CMD_SET_BASELINE        0x201E
#define SGP30_CMD_SET_HUMIDITY        0x2061
#define SGP30_CMD_MEASURE_TEST        0x2032
#define SGP30_CMD_GET_FEATURE_SET     0x202F
#define SGP30_CMD_MEASURE_RAW         0x2050
#define SGP30_CMD_GET_SERIAL          0x3682

#define SGP30_ADDR  0x58

class WySGP30 : public WySensorBase {
public:
    WySGP30(WyI2CPins pins) : _pins(pins) {}

    const char* driverName() override { return "SGP30"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);
        delay(10);  /* SGP30 power-on delay */

        /* Read serial number to verify presence */
        _sendCmd(SGP30_CMD_GET_SERIAL);
        delay(1);
        Wire.requestFrom(_pins.addr, (uint8_t)9);
        if (!Wire.available()) return false;
        for (uint8_t i = 0; i < 9; i++) Wire.read();  /* 3× word+CRC */

        /* Init air quality measurement algorithm */
        _sendCmd(SGP30_CMD_INIT_AIR_QUALITY);
        delay(10);

        /* 15-second warm-up — caller can read() during this, gets 400/0 */
        _warmupStart = millis();
        _warmedUp = false;
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        _sendCmd(SGP30_CMD_MEASURE_AIR_QUALITY);
        delay(12);  /* measurement takes 12ms */

        /* Response: eCO2_H eCO2_L CRC TVOC_H TVOC_L CRC */
        Wire.requestFrom(_pins.addr, (uint8_t)6);
        if (Wire.available() < 6) { d.error = "no data"; return d; }

        uint8_t buf[6];
        for (uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();

        /* CRC check on each word */
        if (_crc8(buf, 2) != buf[2] || _crc8(buf+3, 2) != buf[5]) {
            d.error = "CRC fail"; return d;
        }

        uint16_t eco2 = ((uint16_t)buf[0] << 8) | buf[1];
        uint16_t tvoc = ((uint16_t)buf[3] << 8) | buf[4];

        if (!_warmedUp && millis() - _warmupStart > 15000) _warmedUp = true;

        d.co2 = eco2;
        d.raw = tvoc;        /* TVOC in ppb stored in raw field */
        d.ok  = _warmedUp;   /* mark ok only after warm-up */
        if (!_warmedUp) d.error = "warming up";
        return d;
    }

    bool warmedUp() { return _warmedUp; }

    /* Humidity compensation: pass absolute humidity in g/m³ (from BME280 etc.)
     * Formula: AH = 216.7 × (RH/100 × 6.112 × exp(17.62×T/(243.12+T))) / (273.15+T)
     * Value encoded as 8.8 fixed-point (e.g. 11.5 g/m³ → 0x0B80) */
    void setHumidity(float absHumidity_gm3) {
        uint16_t val = (uint16_t)(absHumidity_gm3 * 256.0f + 0.5f);
        uint8_t data[3] = {(uint8_t)(val >> 8), (uint8_t)(val & 0xFF), 0};
        data[2] = _crc8(data, 2);
        _sendCmd(SGP30_CMD_SET_HUMIDITY);
        Wire.beginTransmission(_pins.addr);
        Wire.write(data, 3);
        Wire.endTransmission();
        delay(1);
    }

    /* Store baseline to NVS and reload on next boot (prevents re-warm-up) */
    struct Baseline { uint16_t eco2; uint16_t tvoc; bool valid; };

    Baseline getBaseline() {
        Baseline bl = {0, 0, false};
        _sendCmd(SGP30_CMD_GET_BASELINE);
        delay(10);
        Wire.requestFrom(_pins.addr, (uint8_t)6);
        if (Wire.available() < 6) return bl;
        uint8_t buf[6];
        for (uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();
        if (_crc8(buf, 2) != buf[2] || _crc8(buf+3, 2) != buf[5]) return bl;
        bl.eco2  = ((uint16_t)buf[0] << 8) | buf[1];
        bl.tvoc  = ((uint16_t)buf[3] << 8) | buf[4];
        bl.valid = true;
        return bl;
    }

    void setBaseline(uint16_t eco2, uint16_t tvoc) {
        uint8_t buf[8];
        buf[0] = SGP30_CMD_SET_BASELINE >> 8;
        buf[1] = SGP30_CMD_SET_BASELINE & 0xFF;
        buf[2] = eco2 >> 8; buf[3] = eco2 & 0xFF; buf[4] = _crc8(buf+2, 2);
        buf[5] = tvoc >> 8; buf[6] = tvoc & 0xFF; buf[7] = _crc8(buf+5, 2);
        Wire.beginTransmission(_pins.addr);
        Wire.write(buf, 8);
        Wire.endTransmission();
        delay(10);
        _warmedUp = true;  /* baseline restores calibrated state */
    }

private:
    WyI2CPins _pins;
    uint32_t  _warmupStart = 0;
    bool      _warmedUp    = false;

    void _sendCmd(uint16_t cmd) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(cmd >> 8);
        Wire.write(cmd & 0xFF);
        Wire.endTransmission();
    }

    /* CRC-8: poly=0x31, init=0xFF (Sensirion standard) */
    uint8_t _crc8(uint8_t* data, uint8_t len) {
        uint8_t crc = 0xFF;
        for (uint8_t i = 0; i < len; i++) {
            crc ^= data[i];
            for (uint8_t b = 0; b < 8; b++)
                crc = (crc & 0x80) ? (crc << 1) ^ 0x31 : (crc << 1);
        }
        return crc;
    }
};
