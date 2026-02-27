/*
 * drivers/WyENS160.h — ENS160 digital MOX air quality sensor (I2C)
 * ==================================================================
 * Datasheet: https://www.sciosense.com/wp-content/uploads/2023/12/ENS160-Datasheet.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x52 (ADDR pin LOW) or 0x53 (ADDR pin HIGH)
 * Registered via WySensors::addI2C<WyENS160>("air", sda, scl, 0x52)
 *
 * Measures:
 *   - AQI:  Air Quality Index 1–5 (1=Excellent, 2=Good, 3=Moderate, 4=Poor, 5=Unhealthy)
 *   - TVOC: Total Volatile Organic Compounds (ppb), 0–65000
 *   - eCO2: Equivalent CO2 (ppm), 400–65000
 *
 * Unlike the SGP30 (which models gas via reference resistance),
 * the ENS160 has an on-chip AI/ML algorithm — it handles baseline
 * compensation internally. No 15-second warm-up flag needed.
 * However a ~3 minute initial conditioning period applies on first power.
 *
 * Temperature & humidity compensation:
 *   Strongly recommended — write T+RH to ENS160 before each read.
 *   Use WyAHT20 or WyBME280 on same I2C bus.
 *   See compensate(tempC, rhPct) method.
 *
 * Operating modes:
 *   ENS160_MODE_SLEEP   — 0 — lowest power, no measurement
 *   ENS160_MODE_IDLE    — 1 — ready, no measurement
 *   ENS160_MODE_STANDARD — 2 — normal operation (1 reading/second)
 *   ENS160_MODE_RESET   — 0xF0 — software reset
 *
 * Status register flags:
 *   STATAS (bit 7): high = new data available
 *   STATVOC (bit 3): 1 = initial startup phase (first ~3 min)
 *   VALIDITY (bits 2:1): 00=normal, 01=warm-up, 10=initial start, 11=invalid
 */

#pragma once
#include "../WySensors.h"

/* ENS160 register addresses */
#define ENS160_REG_PART_ID       0x00  /* should read 0x0160 (little-endian) */
#define ENS160_REG_OPMODE        0x10  /* operating mode */
#define ENS160_REG_CONFIG        0x11  /* interrupt config */
#define ENS160_REG_COMMAND       0x12  /* command register */
#define ENS160_REG_TEMP_IN       0x13  /* temperature compensation input (2 bytes) */
#define ENS160_REG_RH_IN         0x15  /* humidity compensation input (2 bytes) */
#define ENS160_REG_DEVICE_STATUS 0x20  /* status byte */
#define ENS160_REG_DATA_AQI      0x21  /* AQI UBA (1 byte) */
#define ENS160_REG_DATA_TVOC     0x22  /* TVOC ppb (2 bytes, little-endian) */
#define ENS160_REG_DATA_ECO2     0x24  /* eCO2 ppm (2 bytes, little-endian) */
#define ENS160_REG_DATA_T        0x30  /* last used temp compensation (read-back) */
#define ENS160_REG_DATA_RH       0x32  /* last used RH compensation (read-back) */
#define ENS160_REG_DATA_MISR     0x38  /* rolling checksum */
#define ENS160_REG_GPR_WRITE     0x40  /* general purpose write (8 bytes) */
#define ENS160_REG_GPR_READ      0x48  /* general purpose read (8 bytes) */

/* Operating modes */
#define ENS160_MODE_SLEEP    0x00
#define ENS160_MODE_IDLE     0x01
#define ENS160_MODE_STANDARD 0x02
#define ENS160_MODE_RESET    0xF0

/* Status validity field (bits 2:1 of DEVICE_STATUS) */
#define ENS160_VALIDITY_NORMAL    0x00
#define ENS160_VALIDITY_WARMUP    0x01
#define ENS160_VALIDITY_INITSTART 0x02
#define ENS160_VALIDITY_INVALID   0x03

/* AQI UBA labels */
static const char* const ENS160_AQI_LABELS[] = {
    "", "Excellent", "Good", "Moderate", "Poor", "Unhealthy"
};

class WyENS160 : public WySensorBase {
public:
    WyENS160(WyI2CPins pins) : _pins(pins) {}

    const char* driverName() override { return "ENS160"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);
        delay(10);

        /* Reset */
        _writeReg(ENS160_REG_OPMODE, ENS160_MODE_RESET);
        delay(10);

        /* Verify part ID: should be 0x0160 */
        uint8_t id[2];
        _readRegBuf(ENS160_REG_PART_ID, id, 2);
        uint16_t partId = (uint16_t)(id[1] << 8) | id[0];
        if (partId != 0x0160) {
            Serial.printf("[ENS160] wrong part ID: 0x%04X (expected 0x0160)\n", partId);
            return false;
        }

        /* Set standard operating mode */
        _writeReg(ENS160_REG_OPMODE, ENS160_MODE_STANDARD);
        delay(50);

        /* Default compensation: 25°C, 50% RH */
        compensate(25.0f, 50.0f);

        return true;
    }

    WySensorData read() override {
        WySensorData d;

        /* Check status */
        uint8_t status = _readReg8(ENS160_REG_DEVICE_STATUS);
        uint8_t validity = (status >> 2) & 0x03;

        if (validity == ENS160_VALIDITY_INVALID) {
            d.error = "invalid reading";
            return d;
        }

        /* Read AQI (1 byte) */
        uint8_t aqi = _readReg8(ENS160_REG_DATA_AQI) & 0x07;

        /* Read TVOC (2 bytes, little-endian) */
        uint8_t tvoc_buf[2];
        _readRegBuf(ENS160_REG_DATA_TVOC, tvoc_buf, 2);
        uint16_t tvoc = (uint16_t)(tvoc_buf[1] << 8) | tvoc_buf[0];

        /* Read eCO2 (2 bytes, little-endian) */
        uint8_t eco2_buf[2];
        _readRegBuf(ENS160_REG_DATA_ECO2, eco2_buf, 2);
        uint16_t eco2 = (uint16_t)(eco2_buf[1] << 8) | eco2_buf[0];

        d.co2    = eco2;            /* eCO2 ppm */
        d.raw    = tvoc;            /* TVOC ppb */
        d.rawInt = aqi;             /* AQI 1-5 */
        d.ok     = (validity == ENS160_VALIDITY_NORMAL || validity == ENS160_VALIDITY_WARMUP);

        if (validity == ENS160_VALIDITY_WARMUP)    d.error = "warming up";
        if (validity == ENS160_VALIDITY_INITSTART) d.error = "initial start (~3min)";

        _lastAQI  = aqi;
        _lastTVOC = tvoc;
        _lastECO2 = eco2;

        return d;
    }

    /* Write T+RH compensation before reading.
     * Call this every time you have a fresh temp/humidity reading.
     * T encoded as (tempC + 273.15) × 64  (Kelvin × 64, uint16 LE)
     * RH encoded as rhPct × 512           (uint16 LE) */
    void compensate(float tempC, float rhPct) {
        uint16_t t_raw = (uint16_t)((tempC + 273.15f) * 64.0f + 0.5f);
        uint16_t h_raw = (uint16_t)(rhPct * 512.0f + 0.5f);

        uint8_t t_buf[2] = {(uint8_t)(t_raw & 0xFF), (uint8_t)(t_raw >> 8)};
        uint8_t h_buf[2] = {(uint8_t)(h_raw & 0xFF), (uint8_t)(h_raw >> 8)};

        Wire.beginTransmission(_pins.addr);
        Wire.write(ENS160_REG_TEMP_IN);
        Wire.write(t_buf, 2);
        Wire.endTransmission();

        Wire.beginTransmission(_pins.addr);
        Wire.write(ENS160_REG_RH_IN);
        Wire.write(h_buf, 2);
        Wire.endTransmission();
    }

    /* Set operating mode directly */
    void setMode(uint8_t mode) { _writeReg(ENS160_REG_OPMODE, mode); delay(10); }

    /* Human-readable AQI label */
    const char* aqiLabel(uint8_t aqi = 0) {
        uint8_t a = aqi ? aqi : _lastAQI;
        if (a < 1 || a > 5) return "Unknown";
        return ENS160_AQI_LABELS[a];
    }

    uint8_t  lastAQI()  { return _lastAQI; }
    uint16_t lastTVOC() { return _lastTVOC; }
    uint16_t lastECO2() { return _lastECO2; }

    /* Status helpers */
    bool newDataReady() { return (_readReg8(ENS160_REG_DEVICE_STATUS) & 0x02) != 0; }
    uint8_t validity()  { return (_readReg8(ENS160_REG_DEVICE_STATUS) >> 2) & 0x03; }

private:
    WyI2CPins _pins;
    uint8_t   _lastAQI  = 0;
    uint16_t  _lastTVOC = 0;
    uint16_t  _lastECO2 = 0;

    void _writeReg(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.write(val);
        Wire.endTransmission();
    }
    uint8_t _readReg8(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0xFF;
    }
    void _readRegBuf(uint8_t reg, uint8_t* buf, uint8_t len) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, len);
        for (uint8_t i = 0; i < len && Wire.available(); i++) buf[i] = Wire.read();
    }
};
