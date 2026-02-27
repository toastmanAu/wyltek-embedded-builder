/*
 * drivers/WyVEML7700.h — VEML7700 high dynamic range ambient light sensor (I2C)
 * ===============================================================================
 * Datasheet: https://www.vishay.com/docs/84286/veml7700.pdf
 * Application note: AN84333 (lux calculation from raw counts)
 * Bundled driver — no external library needed.
 * I2C address: 0x10 (fixed)
 * Registered via WySensors::addI2C<WyVEML7700>("light", sda, scl, 0x10)
 *
 * Measures:
 *   Ambient light (lux) — 0 to 120,000 lux
 *   White channel — broadband light (unfiltered)
 *   ALS channel   — filtered for human eye response (lux calibrated)
 *
 * vs BH1750:
 *   BH1750:   0–65535 lux, simple, fixed resolution
 *   VEML7700: 0–120,000 lux, auto-ranging, much better low-light sensitivity,
 *             non-linear correction required above ~1000 lux
 *
 * Gain + integration time — both configurable:
 *   Gain:  ×1 (default), ×2, ×1/4, ×1/8
 *   IT:    25ms, 50ms, 100ms (default), 200ms, 400ms, 800ms
 *   Higher gain + longer IT = more sensitive (lower light)
 *   Lower gain + shorter IT = wider range (bright sunlight)
 *
 * Auto-range:
 *   The driver can automatically adjust gain + IT to keep the ADC
 *   in range (not saturated, not in noise floor). Enable with setAutoRange(true).
 *
 * NON-LINEAR CORRECTION:
 *   Above ~1000 lux, the VEML7700 response becomes non-linear.
 *   Vishay AN84333 provides a correction formula — the driver applies it
 *   automatically. Raw counts are also available if you need them.
 *
 * IMPORTANT — 3.3V only:
 *   VEML7700 is a 3.3V device. Do not use 5V supply or 5V I2C.
 *
 * WySensorData:
 *   d.light  = ambient light (lux, corrected)
 *   d.raw    = raw ALS count (uncorrected)
 *   d.rawInt = white channel count
 *   d.ok     = true when reading is valid (not saturated)
 */

#pragma once
#include "../WySensors.h"

/* VEML7700 register addresses */
#define VEML7700_REG_ALS_CONF   0x00  /* configuration (gain, IT, interrupt) */
#define VEML7700_REG_ALS_WH     0x01  /* high threshold window setting */
#define VEML7700_REG_ALS_WL     0x02  /* low threshold window setting */
#define VEML7700_REG_POWER_SAVE 0x03  /* power save mode */
#define VEML7700_REG_ALS        0x04  /* ALS output (lux channel) */
#define VEML7700_REG_WHITE      0x05  /* white output (broadband channel) */
#define VEML7700_REG_INT_FLAG   0x06  /* interrupt status */

/* Gain settings (ALS_CONF bits 12:11) */
#define VEML7700_GAIN_1    0x00  /* ×1   — default */
#define VEML7700_GAIN_2    0x01  /* ×2   — more sensitive */
#define VEML7700_GAIN_1_8  0x02  /* ×1/8 — bright light */
#define VEML7700_GAIN_1_4  0x03  /* ×1/4 */

/* Integration time (ALS_CONF bits 9:6) */
#define VEML7700_IT_25MS   0x0C  /* 25ms  — widest range */
#define VEML7700_IT_50MS   0x08  /* 50ms */
#define VEML7700_IT_100MS  0x00  /* 100ms — default */
#define VEML7700_IT_200MS  0x01  /* 200ms */
#define VEML7700_IT_400MS  0x02  /* 400ms */
#define VEML7700_IT_800MS  0x03  /* 800ms — most sensitive */

/* Resolution (lux per count) — from datasheet Table 1
 * resolution = 0.0036 × (800/IT_ms) × (1/gain)
 * Pre-computed for each gain/IT combination */
static const float VEML7700_RESOLUTIONS[4][6] = {
    /* Gain×2,  ×1,    ×1/4,  ×1/8  (inner: IT 25,50,100,200,400,800ms) */
    {0.0288f, 0.0144f, 0.0072f, 0.0036f, 0.0018f, 0.0009f},  /* gain ×1 */
    {0.0144f, 0.0072f, 0.0036f, 0.0018f, 0.0009f, 0.00045f}, /* gain ×2 */
    {0.1152f, 0.0576f, 0.0288f, 0.0144f, 0.0072f, 0.0036f},  /* gain ×1/8 */
    {0.0576f, 0.0288f, 0.0144f, 0.0072f, 0.0036f, 0.0018f},  /* gain ×1/4 */
};

/* IT in ms for each IT code (indices match VEML7700_IT_* >> 1 ordering) */
static const uint16_t VEML7700_IT_MS[]  = {100, 200, 400, 800, 0, 0, 0, 0, 50, 0, 0, 0, 25};
static const uint8_t  VEML7700_IT_IDX[] = {2,   3,   4,   5,   0, 0, 0, 0, 1,  0, 0, 0, 0};

class WyVEML7700 : public WySensorBase {
public:
    WyVEML7700(WyI2CPins pins,
               uint8_t gain = VEML7700_GAIN_1,
               uint8_t it   = VEML7700_IT_100MS)
        : _pins(pins), _gain(gain), _it(it) {}

    const char* driverName() override { return "VEML7700"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);
        delay(5);

        /* Write config: gain + IT + interrupt disable + power on */
        _applyConfig();
        delay(_itMs() + 10);  /* wait one integration period */

        /* Verify by reading ALS — 0xFFFF with no light source means missing */
        uint16_t als = _readReg16(VEML7700_REG_ALS);
        /* 0xFFFF in total darkness usually means sensor not present */
        if (als == 0xFFFF) {
            /* Try again after a moment — could be first read */
            delay(200);
            als = _readReg16(VEML7700_REG_ALS);
            if (als == 0xFFFF) {
                Serial.println("[VEML7700] sensor may not be connected (0xFFFF)");
                /* Don't fail — 0xFFFF is also valid at very high illuminance */
            }
        }
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        if (_autoRange) _adjustRange();

        uint16_t als   = _readReg16(VEML7700_REG_ALS);
        uint16_t white = _readReg16(VEML7700_REG_WHITE);

        /* Check saturation — if ADC at max, we need lower gain/IT */
        if (als == 0xFFFF) {
            if (!_autoRange) d.error = "saturated — reduce gain or IT";
            d.rawInt = 0xFFFF;
            d.ok     = false;
            return d;
        }

        /* Convert to lux using resolution factor */
        float lux = als * _resolution();

        /* Apply non-linear correction for high lux (Vishay AN84333) */
        lux = _correctLux(lux);

        d.light  = lux;
        d.raw    = als;
        d.rawInt = white;
        d.ok     = true;
        return d;
    }

    /* ── Configuration ───────────────────────────────────────────── */

    void setGain(uint8_t gain) { _gain = gain & 0x03; _applyConfig(); }
    void setIT(uint8_t it)     { _it   = it;           _applyConfig(); }

    /* Auto-ranging: adjust gain + IT to keep ADC in useful range */
    void setAutoRange(bool en) { _autoRange = en; }

    /* Power save mode: 1=50ms, 2=100ms, 3=500ms, 4=1000ms measurement interval */
    void setPowerSave(uint8_t mode) {
        uint16_t ps = (mode & 0x03) << 1;
        if (mode > 0) ps |= 0x01;  /* enable power save */
        _writeReg16(VEML7700_REG_POWER_SAVE, ps);
    }

    /* Raw channel reads */
    uint16_t rawALS()   { return _readReg16(VEML7700_REG_ALS); }
    uint16_t rawWhite() { return _readReg16(VEML7700_REG_WHITE); }
    float    resolution() { return _resolution(); }

private:
    WyI2CPins _pins;
    uint8_t   _gain;
    uint8_t   _it;
    bool      _autoRange = false;

    void _applyConfig() {
        /* ALS_CONF: [15:13]=0 | [12:11]=gain | [10]=0 | [9:6]=IT | [5:3]=0 | [2:1]=int | [0]=SD */
        uint16_t conf = (uint16_t)(_gain & 0x03) << 11
                      | (uint16_t)(_it   & 0x0F) << 6;
        /* SD=0 (power on), interrupts disabled */
        _writeReg16(VEML7700_REG_ALS_CONF, conf);
        delay(_itMs() + 5);  /* settle after config change */
    }

    float _resolution() {
        /* Map gain code to row index */
        uint8_t gi = (_gain == VEML7700_GAIN_1)   ? 0 :
                     (_gain == VEML7700_GAIN_2)   ? 1 :
                     (_gain == VEML7700_GAIN_1_8) ? 2 : 3;
        /* Map IT code to column index */
        uint8_t ii = (_it < 13) ? VEML7700_IT_IDX[_it] : 0;
        return VEML7700_RESOLUTIONS[gi][ii];
    }

    uint16_t _itMs() {
        return (_it < 13) ? VEML7700_IT_MS[_it] : 100;
    }

    /* Non-linear correction from Vishay AN84333
     * Correction polynomial: lux_corrected = 6.0135e-13×L^4 - 9.3924e-9×L^3
     *                                        + 8.1488e-5×L^2 + 1.0023×L
     * Only needed above ~1000 lux — skip for low-light applications */
    float _correctLux(float lux) {
        if (lux <= 1000.0f) return lux;
        double L = lux;
        double corrected = (6.0135e-13 * L*L*L*L)
                         - (9.3924e-9  * L*L*L)
                         + (8.1488e-5  * L*L)
                         + (1.0023     * L);
        return (float)corrected;
    }

    /* Auto-range: adjust gain/IT to keep ALS count in 100–60000 range */
    void _adjustRange() {
        uint16_t als = _readReg16(VEML7700_REG_ALS);

        if (als > 60000) {
            /* Too bright — decrease sensitivity */
            if (_gain == VEML7700_GAIN_2)        { _gain = VEML7700_GAIN_1;   _applyConfig(); return; }
            if (_gain == VEML7700_GAIN_1)        { _gain = VEML7700_GAIN_1_4; _applyConfig(); return; }
            if (_gain == VEML7700_GAIN_1_4)      { _gain = VEML7700_GAIN_1_8; _applyConfig(); return; }
            /* Gain at minimum — try shorter IT */
            if (_it == VEML7700_IT_800MS) { _it = VEML7700_IT_400MS; _applyConfig(); return; }
            if (_it == VEML7700_IT_400MS) { _it = VEML7700_IT_200MS; _applyConfig(); return; }
            if (_it == VEML7700_IT_200MS) { _it = VEML7700_IT_100MS; _applyConfig(); return; }
            if (_it == VEML7700_IT_100MS) { _it = VEML7700_IT_50MS;  _applyConfig(); return; }
            if (_it == VEML7700_IT_50MS)  { _it = VEML7700_IT_25MS;  _applyConfig(); return; }
        } else if (als < 100 && als > 0) {
            /* Too dim — increase sensitivity */
            if (_it == VEML7700_IT_25MS)  { _it = VEML7700_IT_50MS;  _applyConfig(); return; }
            if (_it == VEML7700_IT_50MS)  { _it = VEML7700_IT_100MS; _applyConfig(); return; }
            if (_it == VEML7700_IT_100MS) { _it = VEML7700_IT_200MS; _applyConfig(); return; }
            if (_it == VEML7700_IT_200MS) { _it = VEML7700_IT_400MS; _applyConfig(); return; }
            if (_it == VEML7700_IT_400MS) { _it = VEML7700_IT_800MS; _applyConfig(); return; }
            if (_gain == VEML7700_GAIN_1_8) { _gain = VEML7700_GAIN_1_4; _applyConfig(); return; }
            if (_gain == VEML7700_GAIN_1_4) { _gain = VEML7700_GAIN_1;   _applyConfig(); return; }
            if (_gain == VEML7700_GAIN_1)   { _gain = VEML7700_GAIN_2;   _applyConfig(); return; }
        }
    }

    void _writeReg16(uint8_t reg, uint16_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.write(val & 0xFF);   /* low byte first (little-endian) */
        Wire.write(val >> 8);
        Wire.endTransmission();
    }

    uint16_t _readReg16(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)2);
        if (Wire.available() < 2) return 0xFFFF;
        uint8_t lo = Wire.read();
        uint8_t hi = Wire.read();
        return ((uint16_t)hi << 8) | lo;
    }
};
