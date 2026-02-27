/*
 * drivers/WyTurbidity.h — Optical Turbidity Sensor (Analog + Digital)
 * =====================================================================
 * Compatible with: SEN0189 (DFRobot), the common blue Arduino turbidity
 * module, and most analog optical turbidity sensors with similar circuit.
 *
 * Bundled driver — no external library needed.
 * Registered via WySensors::addGPIO<WyTurbidity>("turbidity", AOUT_PIN)
 *
 * ═══════════════════════════════════════════════════════════════════
 * HOW IT WORKS
 * ═══════════════════════════════════════════════════════════════════
 * An IR LED shines through the liquid sample. A phototransistor on the
 * other side measures how much light arrives. Clear liquid = lots of
 * light transmitted = high voltage. Murky liquid = light scattered by
 * suspended particles = less light reaching sensor = lower voltage.
 *
 * This is a TRANSMISSION type sensor (light straight through).
 * More turbid = more scatter = less transmission = lower analog voltage.
 *
 * Output relationship (approximate, varies by sensor):
 *   Clear water:  ~4.1–4.5V (near rail at 5V supply)
 *   Slightly murky: ~3.0–4.1V
 *   Turbid:       ~1.5–3.0V
 *   Very turbid:  ~0–1.5V
 *
 * NTU (Nephelometric Turbidity Units) — the standard measurement:
 *   0 NTU    = perfectly clear (distilled water)
 *   1 NTU    = EU drinking water standard
 *   4 NTU    = US EPA drinking water limit
 *   10 NTU   = slightly hazy (mildly dirty fish tank)
 *   100 NTU  = noticeably turbid (river water after rain)
 *   1000 NTU = very turbid (muddy water)
 *   3000 NTU = near opaque
 *
 * ═══════════════════════════════════════════════════════════════════
 * VOLTAGE / NTU CONVERSION
 * ═══════════════════════════════════════════════════════════════════
 * The SEN0189 datasheet provides a V→NTU lookup table (at 5V supply).
 * This driver implements a piecewise linear interpolation of that table.
 *
 * ⚠️ SUPPLY VOLTAGE MATTERS:
 *   The sensor is calibrated for 5V supply. At 3.3V supply, the output
 *   voltage range compresses. You have two options:
 *   1. Power sensor from 5V, use a voltage divider on AO before ESP32
 *      (100kΩ + 100kΩ gives ~50% → max ~2.5V into ADC, safe)
 *   2. Power from 3.3V and use setSupplyVoltage(3.3f) to adjust the curve
 *      (less accurate, but workable for relative measurements)
 *   Option 1 is better for absolute NTU readings.
 *
 * ⚠️ ESP32 ADC INPUT MAX: 3.3V — never connect 5V sensor output directly.
 *    Use a voltage divider: 100kΩ from AO, then 100kΩ to GND, tap middle.
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING (5V sensor, 3.3V ESP32)
 * ═══════════════════════════════════════════════════════════════════
 *   Sensor VCC → 5V
 *   Sensor GND → GND
 *   Sensor AO  → 100kΩ → ESP32 ADC1 pin
 *                100kΩ → GND  (voltage divider at junction)
 *   Sensor DO  → ESP32 digital GPIO (optional threshold)
 *
 * ⚠️ ADC1 ONLY: GPIO32–39 on ESP32. ADC2 interferes with WiFi.
 *
 * WIRING (3.3V sensor — if your module supports it):
 *   Sensor VCC → 3.3V
 *   Sensor GND → GND
 *   Sensor AO  → ESP32 ADC1 pin (direct, no divider needed)
 *   Call setSupplyVoltage(3.3f) in firmware.
 *
 * ═══════════════════════════════════════════════════════════════════
 * IMPORTANT PRACTICAL NOTES
 * ═══════════════════════════════════════════════════════════════════
 * 1. TEMPERATURE SENSITIVITY:
 *    The IR photodiode is slightly temperature sensitive.
 *    For accurate NTU readings, pair with a temperature sensor and
 *    call setTemperature() before reading — enables temp compensation.
 *    Without compensation, expect ~5% drift per 10°C.
 *
 * 2. CALIBRATION:
 *    Factory NTU curve is approximate. For best accuracy:
 *    - Measure known-NTU standards (e.g. Formazin solutions or
 *      StablCal standards from Hach)
 *    - Or use tap water (~0.1 NTU) and a secondary reference turbidimeter
 *    - Or just use relative readings (is this batch cloudier than last?)
 *
 * 3. BUBBLES:
 *    Air bubbles give false high turbidity readings. De-gas sample first.
 *    Let liquid settle in the sensor for 30+ seconds before reading.
 *
 * 4. PROBE FOULING:
 *    The optical window fouls with algae, mineral deposits, and biofilm
 *    over time. Clean with soft cloth + dilute vinegar regularly.
 *    Fouling makes it read higher NTU than actual (more apparent scatter).
 *
 * 5. STRAY LIGHT:
 *    The sensor should be shielded from ambient light, especially sunlight.
 *    The IR LED is relatively strong, but direct sunlight on the detector
 *    overwhelms it. Use in a housing or measure in darkness.
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   auto* turb = sensors.addGPIO<WyTurbidity>("turbidity", AOUT_PIN);
 *   turb->setDividerRatio(0.5f);     // if using 100kΩ+100kΩ voltage divider
 *   turb->setSupplyVoltage(5.0f);    // sensor supply (default 5V)
 *   turb->setDOPin(DOUT_PIN);        // optional threshold output
 *   sensors.begin();
 *
 *   WySensorData d = sensors.read("turbidity");
 *   if (d.ok) {
 *       Serial.printf("Turbidity: %.1f NTU  Voltage: %.3fV\n",
 *           d.raw, d.voltage);
 *   }
 *
 * WySensorData:
 *   d.raw         = turbidity in NTU (0–3000)
 *   d.voltage     = sensor analog voltage (V, after divider correction)
 *   d.rawInt      = raw ADC value (0–4095)
 *   d.humidity    = water quality category (0=clear,1=good,2=fair,3=poor)
 *   d.ok          = true when reading valid
 */

#pragma once
#include "../WySensors.h"

/* Number of ADC samples to average */
#ifndef WY_TURB_SAMPLES
#define WY_TURB_SAMPLES  16
#endif

/* NTU lookup table — V (at 5V supply) → NTU
 * From SEN0189 datasheet + community measurements.
 * Piecewise linear between breakpoints.
 * Sensor output DECREASES as turbidity increases. */
static const float WY_TURB_V[]   = {4.20f, 3.80f, 3.00f, 2.50f, 2.00f, 1.50f, 1.00f, 0.50f};
static const float WY_TURB_NTU[] = {0.0f,  100.f, 400.f, 700.f, 1100.f, 1700.f, 2600.f, 3000.f};
static const uint8_t WY_TURB_LUT_SIZE = 8;

class WyTurbidity : public WySensorBase {
public:
    WyTurbidity(WyGPIOPins pins) : _aoPin(pins.pin) {}

    const char* driverName() override { return "Turbidity"; }

    /* Voltage divider ratio (output / input). Default 1.0 (no divider).
     * For 100kΩ+100kΩ divider: 0.5f
     * For 68kΩ+100kΩ divider: 0.595f */
    void setDividerRatio(float ratio)   { _divRatio = (ratio > 0) ? ratio : 1.0f; }

    /* Sensor supply voltage. Default 5.0V. Use 3.3f if powering from 3.3V.
     * Scales the NTU lookup table proportionally. */
    void setSupplyVoltage(float vcc)    { _supplyV = vcc; }

    /* Optional digital threshold output pin (LOW = above threshold = turbid) */
    void setDOPin(int8_t pin)           { _doPin = pin; }

    /* Temperature compensation (°C). Call before read() for best accuracy.
     * ~0.5% NTU change per °C — matters for precision, ignorable for casual use. */
    void setTemperature(float tempC)    { _tempC = tempC; }

    /* Number of ADC samples to average per reading */
    void setSamples(uint8_t n)          { _samples = n ? n : 1; }

    /* Override NTU lookup table with your own calibration points.
     * v[] and ntu[] must be in descending voltage order (clear→turbid).
     * count: number of points (max 16). */
    void setCalibration(const float* v, const float* ntu, uint8_t count) {
        _calV   = v;
        _calNTU = ntu;
        _calN   = count;
    }

    bool begin() override {
        if (_aoPin < 0) {
            Serial.println("[Turbidity] analog pin required");
            return false;
        }
        if (_doPin >= 0) pinMode(_doPin, INPUT);

        /* Quick sanity read */
        uint16_t raw = _readRaw();
        float voltage = _rawToVoltage(raw);
        float ntu = _voltageToNTU(voltage);
        Serial.printf("[Turbidity] online — raw:%u V:%.3f NTU:%.1f\n",
            raw, voltage, ntu);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        uint16_t raw = _readRaw();
        float voltage = _rawToVoltage(raw);
        float ntu = _voltageToNTU(voltage);

        /* Temperature compensation (if set) */
        if (_tempC > 0.0f) {
            /* Correction: ~0.5% per °C from reference 25°C */
            float correction = 1.0f + 0.005f * (_tempC - 25.0f);
            ntu /= correction;
        }

        ntu = max(ntu, 0.0f);

        d.rawInt   = raw;
        d.voltage  = voltage;
        d.raw      = ntu;

        /* Water quality category */
        if      (ntu < 1.0f)   d.humidity = 0;  /* clear (drinking water) */
        else if (ntu < 10.0f)  d.humidity = 1;  /* good (slightly hazy) */
        else if (ntu < 100.0f) d.humidity = 2;  /* fair (noticeably turbid) */
        else                   d.humidity = 3;  /* poor (very turbid) */

        /* Digital threshold if wired (LOW = turbid = threshold exceeded) */
        if (_doPin >= 0) {
            bool triggered = (digitalRead(_doPin) == LOW);
            /* Store in error field only if triggered — non-intrusive */
            if (triggered) d.error = "threshold";
        }

        d.ok = true;
        return d;
    }

    /* Human-readable water quality label */
    const char* qualityLabel() {
        WySensorData d = read();
        switch ((int)d.humidity) {
            case 0: return "Clear";
            case 1: return "Good";
            case 2: return "Fair";
            default: return "Poor";
        }
    }

    /* Raw voltage without NTU conversion — useful for calibration */
    float readVoltage() {
        return _rawToVoltage(_readRaw());
    }

private:
    int8_t   _aoPin     = -1;
    int8_t   _doPin     = -1;
    float    _divRatio  = 1.0f;
    float    _supplyV   = 5.0f;
    float    _tempC     = 0.0f;   /* 0 = compensation disabled */
    uint8_t  _samples   = WY_TURB_SAMPLES;

    /* Calibration table pointers — default to built-in LUT */
    const float* _calV   = WY_TURB_V;
    const float* _calNTU = WY_TURB_NTU;
    uint8_t      _calN   = WY_TURB_LUT_SIZE;

    uint16_t _readRaw() {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < _samples; i++) {
            sum += analogRead(_aoPin);
            delay(2);
        }
        return (uint16_t)(sum / _samples);
    }

    /* Convert raw ADC → actual sensor voltage (correcting for divider) */
    float _rawToVoltage(uint16_t raw) {
        /* ADC is 12-bit 0–4095 = 0–3.3V on ESP32 */
        float adcV = (raw / 4095.0f) * 3.3f;
        /* Undo voltage divider to get actual sensor output voltage */
        float sensorV = (_divRatio > 0) ? adcV / _divRatio : adcV;
        return sensorV;
    }

    /* Piecewise linear interpolation: voltage → NTU
     * Table must be in descending voltage order (high V = clear, low V = turbid) */
    float _voltageToNTU(float v) {
        /* Scale voltage for non-5V supplies */
        float vScaled = v * (5.0f / _supplyV);

        /* Above first table entry (very clear) */
        if (vScaled >= _calV[0]) return _calNTU[0];

        /* Below last table entry (off-scale turbid) */
        if (vScaled <= _calV[_calN - 1]) return _calNTU[_calN - 1];

        /* Find enclosing segment */
        for (uint8_t i = 1; i < _calN; i++) {
            if (vScaled >= _calV[i]) {
                /* Linear interpolate between [i-1] and [i] */
                float vSpan  = _calV[i-1]   - _calV[i];
                float ntuSpan= _calNTU[i]   - _calNTU[i-1];
                float t = (vScaled - _calV[i]) / vSpan;
                return _calNTU[i-1] + t * ntuSpan;
            }
        }
        return _calNTU[_calN - 1];
    }
};
