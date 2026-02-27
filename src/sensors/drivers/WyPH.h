/*
 * drivers/WyPH.h — Analog pH sensor module (BNC probe + signal board)
 * =====================================================================
 * Compatible with: PH4502C, SEN0161, DFRobot pH meter v1/v2, and most
 * AliExpress "pH 0-14 BNC electrode" analog modules.
 *
 * These modules have an op-amp that amplifies the glass electrode signal
 * to an ADC-readable voltage. The output is linear:
 *   ~2.5V at pH 7  (neutral — isopotential point)
 *   ~3.4V at pH 4  (acidic)
 *   ~1.7V at pH 10 (alkaline)
 *
 * Registered via WySensors::addGPIO<WyPH>("ph_sensor", AOUT_PIN)
 *
 * Wiring:
 *   Module VCC → 5V (module runs on 5V, but AOUT is typically 0-3.3V safe)
 *   Module GND → GND
 *   Module AOUT/Po → ESP32 ADC pin (any ADC-capable GPIO)
 *   BNC socket → pH probe
 *
 * IMPORTANT — 3.3V vs 5V:
 *   The PH4502C module's op-amp is powered at 5V.
 *   AOUT can reach ~3.4V at very low pH — exceeds ESP32 3.3V ADC limit!
 *   Solutions:
 *     a) Voltage divider on AOUT (e.g. 3.3kΩ + 6.8kΩ → 0.49× scaling)
 *     b) Use the module's 3.3V supply pin if it has one
 *     c) Set WY_PH_VREF_MV to your actual supply voltage
 *     d) Accept slight clipping at pH < 5 (ok for many use cases)
 *
 * Calibration (required — probes vary):
 *   1. Prepare pH 7.0 and pH 4.0 buffer solutions (sachets usually included)
 *   2. Place probe in pH 7 buffer, wait 1 minute, call calibrate(7.0)
 *   3. Rinse, place in pH 4 buffer, wait 1 minute, call calibrate(4.0)
 *   4. Two-point calibration gives accurate slope + offset
 *   5. Single-point (offset only) works if you only need relative readings
 *
 * Temperature compensation:
 *   pH reading shifts ~0.002 pH/°C from the reference (25°C).
 *   Pass water temperature to read(tempC) for corrected output.
 *   A DS18B20 in the same water gives accurate compensation.
 *
 * Usage:
 *   auto* ph = sensors.addGPIO<WyPH>("water_ph", 34);
 *   sensors.begin();
 *   ph->calibrate(7.0);   // with probe in pH 7 buffer
 *   // ... then in pH 4 buffer:
 *   ph->calibrate(4.0);
 *   WySensorData d = sensors.read("water_ph");
 *   Serial.printf("pH = %.2f\n", d.raw);  // raw = pH value
 */

#pragma once
#include "../WySensors.h"

/* ADC config — override via build flags */
#ifndef WY_PH_ADC_BITS
#define WY_PH_ADC_BITS   12
#endif

#ifndef WY_PH_VREF_MV
#define WY_PH_VREF_MV    3300.0f   /* set to 5000.0 if using 5V ADC reference */
#endif

/* Number of samples to average per reading (reduces noise) */
#ifndef WY_PH_SAMPLES
#define WY_PH_SAMPLES    32
#endif

/* Neutral voltage (mV) — ~2500mV at pH 7 on 5V module, less on 3.3V */
#ifndef WY_PH_NEUTRAL_MV
#define WY_PH_NEUTRAL_MV  2500.0f
#endif

class WyPH : public WySensorBase {
public:
    WyPH(WyGPIOPins pins) : _pin(pins.pin) {}

    const char* driverName() override { return "pH-BNC"; }

    bool begin() override {
        pinMode(_pin, INPUT);
        analogReadResolution(WY_PH_ADC_BITS);
        delay(100);

        /* Quick sanity: if pin reads 0 or max constantly, probably not wired */
        int32_t raw = _readADC();
        if (raw < 10 || raw > ((1 << WY_PH_ADC_BITS) - 50)) {
            Serial.printf("[pH] ADC pin %d reading suspicious (%ld) — check wiring\n",
                _pin, (long)raw);
            /* Don't return false — sensor might just have no probe yet */
        }

        _calibrated = false;
        return true;
    }

    WySensorData read() override {
        return readTemp(25.0f);
    }

    /* Read with temperature compensation (preferred) */
    WySensorData readTemp(float waterTempC) {
        WySensorData d;

        float vMV = _readVoltageMV();
        float pH  = _voltageToPH(vMV, waterTempC);

        /* Sanity bounds */
        if (pH < 0.0f || pH > 14.0f) {
            d.error = "out of range — check probe / calibration";
            d.raw = pH;  /* still report the value */
            return d;
        }

        d.raw         = pH;          /* pH value in raw field */
        d.voltage     = vMV;
        d.temperature = waterTempC;
        d.ok          = true;
        return d;
    }

    /* ── Calibration ─────────────────────────────────────────────── */

    /* Single-point calibration at a known pH buffer.
     * Call once with probe in pH 7.0 buffer for offset-only calibration. */
    void calibrate(float bufferPH, uint8_t samples = 20) {
        float vMV = 0;
        for (uint8_t i = 0; i < samples; i++) {
            vMV += _readVoltageMV();
            delay(100);
        }
        vMV /= samples;

        if (!_calibrated) {
            /* First calibration point — store it */
            _cal1_pH = bufferPH;
            _cal1_mV = vMV;
            _calibrated = true;
            /* Single-point: update offset only, keep default slope */
            _offsetMV = vMV - (WY_PH_NEUTRAL_MV + _slopeMV_pH * (7.0f - bufferPH));
            Serial.printf("[pH] cal point 1: pH %.1f → %.1f mV  offset=%.1f mV\n",
                bufferPH, vMV, _offsetMV);
        } else {
            /* Second calibration point — calculate slope */
            _cal2_pH = bufferPH;
            _cal2_mV = vMV;
            /* Slope: mV per pH unit */
            _slopeMV_pH = (_cal1_mV - _cal2_mV) / (_cal2_pH - _cal1_pH);
            /* Offset: voltage at pH 7 */
            _offsetMV = _cal1_mV - _slopeMV_pH * (7.0f - _cal1_pH);
            Serial.printf("[pH] cal point 2: pH %.1f → %.1f mV\n", bufferPH, vMV);
            Serial.printf("[pH] slope=%.2f mV/pH  neutral=%.1f mV\n",
                _slopeMV_pH, _offsetMV);
        }
    }

    /* Manual calibration if you know your module's slope and neutral voltage */
    void setCalib(float neutralMV, float slopeMVperPH = -59.16f) {
        _offsetMV   = neutralMV;
        _slopeMV_pH = slopeMVperPH;
        _calibrated = true;
    }

    /* Reset calibration to defaults */
    void resetCalib() {
        _offsetMV   = WY_PH_NEUTRAL_MV;
        _slopeMV_pH = -59.16f;
        _calibrated = false;
    }

    bool isCalibrated() { return _calibrated; }

    /* Get raw voltage in mV — useful for calibration debugging */
    float readMV() { return _readVoltageMV(); }

private:
    int8_t _pin;
    bool   _calibrated  = false;

    /* Calibration state */
    float _cal1_pH = 7.0f, _cal1_mV = 2500.0f;
    float _cal2_pH = 4.0f, _cal2_mV = 0.0f;

    /* Nernst equation: ~59.16 mV per pH unit at 25°C */
    /* Negative slope: higher voltage = lower pH (acidic) */
    float _offsetMV   = WY_PH_NEUTRAL_MV;   /* voltage at pH 7 */
    float _slopeMV_pH = -59.16f;             /* mV per pH unit (Nernst, 25°C) */

    /* Temperature-corrected Nernst slope:
     * slope(T) = -2.3026 × R × T / F
     * R = 8.314 J/mol·K, F = 96485 C/mol
     * ≈ -0.1984 × (T_kelvin) mV/pH
     * At 25°C (298.15K): -59.16 mV/pH
     * At 20°C (293.15K): -58.17 mV/pH
     * At 30°C (303.15K): -60.15 mV/pH
     */
    float _nernstSlope(float tempC) {
        float tK = tempC + 273.15f;
        return -0.1984f * tK;  /* mV/pH */
    }

    float _voltageToPH(float vMV, float tempC) {
        /* Adjust slope for temperature, keep offset (isopotential at pH 7) */
        float slope = _nernstSlope(tempC);
        /* Scale our calibrated slope to the temperature-corrected one */
        float tempCorr = slope / _slopeMV_pH;
        return 7.0f + (vMV - _offsetMV) / (_slopeMV_pH * tempCorr);
    }

    /* Average multiple ADC readings to reduce noise */
    int32_t _readADC() {
        int64_t sum = 0;
        for (uint8_t i = 0; i < WY_PH_SAMPLES; i++) {
            sum += analogRead(_pin);
            delayMicroseconds(200);
        }
        return (int32_t)(sum / WY_PH_SAMPLES);
    }

    float _readVoltageMV() {
        int32_t raw = _readADC();
        return (raw / (float)((1 << WY_PH_ADC_BITS) - 1)) * WY_PH_VREF_MV;
    }
};
