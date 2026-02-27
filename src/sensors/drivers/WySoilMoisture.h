/*
 * drivers/WySoilMoisture.h — Resistive Soil Moisture Sensor (Analog + Digital)
 * ==============================================================================
 * Compatible with: the ubiquitous blue/green Arduino soil moisture modules
 * (YL-69, HL-69, resistive probe variants — all the same circuit)
 *
 * Bundled driver — no external library needed.
 * Registered via WySensors::addGPIO<WySoilMoisture>("soil", AOUT_PIN)
 * Or with digital threshold:
 *   auto* s = sensors.addGPIO<WySoilMoisture>("soil", AOUT_PIN);
 *   s->setDOPin(DOUT_PIN);
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT IS
 * ═══════════════════════════════════════════════════════════════════
 * Two exposed metal probes. When inserted in soil, the soil completes
 * an electrical circuit between them. Moist soil = lower resistance =
 * more current = lower analog voltage output.
 *
 * The module has two outputs:
 *   AO (Analog Out) — continuous voltage proportional to moisture
 *   DO (Digital Out) — HIGH/LOW threshold, set by onboard potentiometer
 *
 * ═══════════════════════════════════════════════════════════════════
 * ⚠️ CORROSION — THE CRITICAL PROBLEM ⚠️
 * ═══════════════════════════════════════════════════════════════════
 * These sensors pass DC current through the soil continuously.
 * This causes ELECTROLYTIC CORROSION of the probes via electrolysis.
 * In a few weeks of continuous use, the probes dissolve away.
 *
 * THE FIX: Only power the sensor when taking a reading.
 * - Connect VCC through a GPIO pin (max 12mA draw — fine for these modules)
 * - Drive the pin HIGH just before reading, LOW immediately after
 * - Reading takes ~100ms including ADC settle time
 * - This extends probe life from weeks to years
 *
 * The driver does this automatically via setPowerPin().
 * If no power pin is set, sensor is always-on (shorter probe life).
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 * Basic (always-on):
 *   Module VCC  → 3.3V
 *   Module GND  → GND
 *   Module AO   → ESP32 analog GPIO (ADC1 only — see note)
 *   Module DO   → ESP32 digital GPIO (optional)
 *
 * Power-switched (recommended):
 *   Module VCC  → ESP32 GPIO (PWR_PIN) — drive HIGH to power, LOW to save
 *   Module GND  → GND
 *   Module AO   → ESP32 analog GPIO (ADC1)
 *   Module DO   → ESP32 digital GPIO (optional)
 *
 * ⚠️ ADC1 ONLY on ESP32:
 *   ADC2 is shared with WiFi. When WiFi is active, ADC2 returns garbage.
 *   Pins 32–39 are ADC1 — always use these for analog sensors with WiFi.
 *   Recommended: GPIO32, GPIO33, GPIO34, GPIO35, GPIO36, GPIO39
 *
 * ⚠️ 3.3V INPUT RANGE:
 *   ESP32 ADC is 0–3.3V, 12-bit (0–4095).
 *   Most soil moisture modules are designed for 5V Arduino but work at 3.3V
 *   with reduced output swing. Dry soil output may not reach full 3.3V —
 *   calibrate at your actual dry/wet endpoints.
 *
 * ═══════════════════════════════════════════════════════════════════
 * CALIBRATION
 * ═══════════════════════════════════════════════════════════════════
 * Raw ADC values are inverted: HIGH raw = DRY, LOW raw = WET.
 * This is because dry soil = high resistance = less current = higher voltage.
 *
 * Default calibration (approximate):
 *   Dry air:     ~3000–4095 raw
 *   Dry soil:    ~2200–3000 raw
 *   Moist soil:  ~1200–2200 raw
 *   Saturated:   ~0–1200 raw
 *
 * Calibrate for YOUR soil:
 *   1. Hold sensor in dry air → call rawValue() → this is your DRY point
 *   2. Submerge probe tips in water → call rawValue() → this is your WET point
 *   3. Call setCalibration(wetRaw, dryRaw)
 *   4. read().raw will then return 0.0–100.0 percent moisture
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   auto* soil = sensors.addGPIO<WySoilMoisture>("soil", AOUT_PIN);
 *   soil->setPowerPin(PWR_PIN);            // recommended!
 *   soil->setDOPin(DOUT_PIN);             // optional threshold output
 *   soil->setCalibration(1200, 3200);     // wet raw, dry raw
 *   sensors.begin();
 *
 *   WySensorData d = sensors.read("soil");
 *   if (d.ok) {
 *       Serial.printf("Moisture: %.1f%%  Raw: %u  Dry: %s\n",
 *           d.raw, (uint16_t)d.rawInt, d.rawInt > 3000 ? "YES" : "no");
 *   }
 *
 * Multiple sensors (different locations in same plot):
 *   auto* bed1 = sensors.addGPIO<WySoilMoisture>("bed1", A1_PIN);
 *   auto* bed2 = sensors.addGPIO<WySoilMoisture>("bed2", A2_PIN);
 *   bed1->setPowerPin(PWR1_PIN);
 *   bed2->setPowerPin(PWR2_PIN);
 *   // each powers independently — no cross-contamination
 *
 * WySensorData:
 *   d.raw    = moisture percentage (0.0–100.0) — 0=dry, 100=saturated
 *   d.rawInt = raw ADC value (0–4095) — higher = drier
 *   d.ok     = true when reading is valid
 *   d.error  = "dry threshold" if above dry calibration point
 */

#pragma once
#include "../WySensors.h"

/* Number of ADC samples to average per reading */
#ifndef WY_SOIL_SAMPLES
#define WY_SOIL_SAMPLES  8
#endif

/* Default calibration (approximate for 3.3V supply, adjust with setCalibration) */
#ifndef WY_SOIL_WET_RAW
#define WY_SOIL_WET_RAW   1200
#endif
#ifndef WY_SOIL_DRY_RAW
#define WY_SOIL_DRY_RAW   3200
#endif

class WySoilMoisture : public WySensorBase {
public:
    WySoilMoisture(WyGPIOPins pins) : _aoPin(pins.pin) {}

    const char* driverName() override { return "SoilMoisture"; }

    /* GPIO to drive HIGH to power sensor (LOW = off = probes not corroding) */
    void setPowerPin(int8_t pin)    { _pwrPin = pin; }

    /* Digital threshold output pin (optional) */
    void setDOPin(int8_t pin)       { _doPin = pin; }

    /* Calibration: wetRaw = ADC value in water, dryRaw = ADC value in dry air
     * Higher raw = drier (inverted: dry soil = high resistance = high voltage) */
    void setCalibration(uint16_t wetRaw, uint16_t dryRaw) {
        _wetRaw = wetRaw;
        _dryRaw = dryRaw;
    }

    /* Number of ADC samples to average (more = less noise, slower) */
    void setSamples(uint8_t n)      { _samples = n ? n : 1; }

    bool begin() override {
        if (_aoPin < 0) {
            Serial.println("[SoilMoisture] analog pin required");
            return false;
        }
        if (_pwrPin >= 0) {
            pinMode(_pwrPin, OUTPUT);
            digitalWrite(_pwrPin, LOW);  /* start powered OFF */
        }
        if (_doPin >= 0) {
            pinMode(_doPin, INPUT);
        }
        /* Quick sanity check — power on, read, power off */
        uint16_t raw = _readRaw();
        Serial.printf("[SoilMoisture] online — raw:%u (wet=%u dry=%u)\n",
            raw, _wetRaw, _dryRaw);
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        uint16_t raw = _readRaw();

        d.rawInt = raw;

        /* Map raw to 0–100% moisture
         * raw == _dryRaw → 0%, raw == _wetRaw → 100%
         * Clamp to valid range */
        float pct = 0.0f;
        if (_dryRaw != _wetRaw) {
            pct = ((float)_dryRaw - (float)raw) /
                  ((float)_dryRaw - (float)_wetRaw) * 100.0f;
            pct = constrain(pct, 0.0f, 100.0f);
        }
        d.raw = pct;

        /* Digital threshold (if wired) */
        if (_doPin >= 0) {
            /* DO is LOW when moisture exceeds threshold (active low) */
            d.voltage = digitalRead(_doPin) ? 0.0f : 1.0f;
        }

        d.ok = true;
        return d;
    }

    /* Raw ADC value — useful for calibration */
    uint16_t rawValue() { return _readRaw(); }

    /* True if dry threshold output is triggered (DO pin required) */
    bool isDry() {
        if (_doPin < 0) return false;
        return digitalRead(_doPin) == HIGH;  /* DO HIGH = below moisture threshold = dry */
    }

private:
    int8_t   _aoPin    = -1;
    int8_t   _pwrPin   = -1;
    int8_t   _doPin    = -1;
    uint16_t _wetRaw   = WY_SOIL_WET_RAW;
    uint16_t _dryRaw   = WY_SOIL_DRY_RAW;
    uint8_t  _samples  = WY_SOIL_SAMPLES;

    uint16_t _readRaw() {
        /* Power on */
        if (_pwrPin >= 0) {
            digitalWrite(_pwrPin, HIGH);
            delay(80);  /* settle time: probe + ADC capacitance + RC filter */
        }

        /* Average multiple ADC readings */
        uint32_t sum = 0;
        for (uint8_t i = 0; i < _samples; i++) {
            sum += analogRead(_aoPin);
            delay(2);
        }
        uint16_t raw = (uint16_t)(sum / _samples);

        /* Power off immediately — stops corrosion */
        if (_pwrPin >= 0) {
            digitalWrite(_pwrPin, LOW);
        }

        return raw;
    }
};
