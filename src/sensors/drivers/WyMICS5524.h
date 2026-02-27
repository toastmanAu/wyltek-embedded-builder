/*
 * drivers/WyMICS5524.h — SGX MICS-5524 MEMS Gas Sensor (Analog)
 * ==============================================================
 * Datasheet: https://sgx.cdistore.com/datasheets/sgx/mics-5524.pdf
 * Manufactured by: SGX Sensortech (formerly e2v / MiCS)
 * Bundled driver — no external library needed.
 * Registered via WySensors::addGPIO<WyMICS5524>("gas", AOUT_PIN)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT DETECTS
 * ═══════════════════════════════════════════════════════════════════
 * The MICS-5524 is a MEMS metal oxide gas sensor sensitive to:
 *   CO   (carbon monoxide)      10–500 ppm
 *   Ethanol (alcohol)           10–500 ppm
 *   Hydrogen                    1–1000 ppm
 *   Ammonia                     1–500 ppm
 *   Methane (partial)           > 1000 ppm (less sensitive than dedicated CH4 sensor)
 *
 * It cannot distinguish between these gases — it measures combined
 * reducing gas concentration. Use alongside other sensors (SGP30,
 * ENS160, or dedicated CO/gas sensors) for gas identification.
 *
 * ═══════════════════════════════════════════════════════════════════
 * MICS-5524 vs MQ SERIES
 * ═══════════════════════════════════════════════════════════════════
 *   MQ series:   large ceramic bead, 5V heater, 150–950mW power draw
 *   MICS-5524:   MEMS chip, 1.8–5V heater, 35–70mW power draw
 *   MICS-5524:   faster response (< 30s vs 60–120s for MQ series)
 *   MICS-5524:   smaller form factor, better for portable/battery use
 *   Both:        analog output proportional to Rs (sensor resistance)
 *   Both:        require warm-up and R0 calibration in clean air
 *
 * ═══════════════════════════════════════════════════════════════════
 * HOW IT WORKS
 * ═══════════════════════════════════════════════════════════════════
 * Internal heater (on-chip) maintains metal oxide film at ~300–500°C.
 * Target gases adsorb onto the metal oxide surface and change its
 * resistance (reducing gases lower resistance).
 *
 * Most breakout boards use a voltage divider:
 *   VCC --- RL (load resistor, typically 10kΩ) --- AOUT --- Rs (sensor) --- GND
 *
 * Vout = VCC × Rs / (Rs + RL)
 * Rs = RL × (VCC/Vout - 1)
 * Ratio = Rs/R0 → ppm via power law curve from datasheet
 *
 * ppm = A × (Rs/R0)^B  (gas-specific A, B coefficients)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WARM-UP AND STABILITY
 * ═══════════════════════════════════════════════════════════════════
 * ⚠️ CRITICAL: The sensor requires warm-up time before stable readings.
 *
 *   Cold start (first power-on):  ~3 minutes to settle
 *   Subsequent power cycles:      ~30 seconds to settle
 *   After exposure to high gas:   ~5 minutes to clear and recover
 *
 * Do NOT read the sensor before warm-up completes. The driver tracks
 * elapsed time since begin() and returns d.ok=false during warm-up.
 * Default warm-up: 30s (subsequent power cycle). Override with
 * setWarmupSeconds(180) for first-time/cold-start calibration.
 *
 * ═══════════════════════════════════════════════════════════════════
 * R0 CALIBRATION (MANDATORY FOR PPM)
 * ═══════════════════════════════════════════════════════════════════
 * R0 = sensor resistance in clean fresh air (used as reference).
 * Without calibration, ppm values are meaningless.
 *
 * To calibrate:
 *   1. Power sensor outdoors or in fresh well-ventilated air
 *   2. Allow 3+ minutes warm-up (setWarmupSeconds(180))
 *   3. Call calibrateR0() — measures Rs 10 times, averages, stores as R0
 *   4. Save R0 value to NVS and restore on boot
 *
 * The driver uses R0 = 10.0 kΩ as default (typical clean-air value).
 * This is wrong for your specific unit — always calibrate.
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 * Most MICS-5524 breakout boards expose: VCC, GND, AOUT, (optional DOUT)
 *
 * Standard breakout (with onboard 10kΩ load resistor):
 *   VCC  → 3.3V or 5V (check your board — some are 5V only)
 *   GND  → GND
 *   AOUT → ESP32 ADC1 pin (GPIO32–39)
 *
 * ⚠️ If powered from 5V: AOUT can reach up to 5V.
 *    Use a voltage divider (100kΩ + 100kΩ) before the ESP32 ADC pin,
 *    and call setDividerRatio(0.5f).
 *
 * ⚠️ If powered from 3.3V: AOUT stays within 3.3V. Direct connection OK.
 *    Call setSupplyVoltage(3.3f).
 *
 * ⚠️ ADC1 ONLY: GPIO32–39. ADC2 is corrupted by WiFi.
 *
 * ⚠️ The MICS-5524 heater draws ~35–70mW. Use a separate power rail
 *    or ensure your 3.3V regulator can handle the extra load. The
 *    ESP32 onboard LDO on dev boards is usually adequate (150–300mA rated).
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   auto* gas = sensors.addGPIO<WyMICS5524>("gas", GPIO34);
 *   gas->setSupplyVoltage(3.3f);
 *   gas->setLoadResistance(10.0f);    // kΩ — check your board (usually 10kΩ)
 *   gas->setR0(10.0f);                // kΩ — CALIBRATE THIS!
 *   sensors.begin();
 *
 *   WySensorData d = sensors.read("gas");
 *   if (d.ok) {
 *       Serial.printf("CO: %.0f ppm  Ethanol: %.0f ppm  Rs: %.2f kΩ\n",
 *           d.co2, d.raw, d.voltage);
 *   }
 *
 * WySensorData:
 *   d.co2     = estimated CO ppm (from CO curve)
 *   d.raw     = estimated Ethanol ppm (from ethanol curve)
 *   d.rawInt  = Rs/R0 ratio × 100 (integer, for logging without floats)
 *   d.voltage = sensor resistance Rs (kΩ)
 *   d.ok      = true after warm-up, when reading is valid
 *   d.error   = "warming up" during warm-up period
 */

#pragma once
#include "../WySensors.h"
#include <math.h>

/* Gas curve coefficients: ppm = A × (Rs/R0)^B
 * From MICS-5524 datasheet sensitivity curves (log-log linear fit)
 * These are approximate — actual curves are non-linear, these are
 * linearised over the sensor's useful range */
struct MICS5524Curve {
    float A;  /* intercept */
    float B;  /* slope (negative — Rs decreases with more gas) */
};

/* Datasheet-derived coefficients (approximate) */
static const MICS5524Curve MICS5524_CO       = { 4.4638f, -1.1760f };
static const MICS5524Curve MICS5524_ETHANOL  = { 3.1813f, -1.0313f };
static const MICS5524Curve MICS5524_H2       = { 0.3934f, -1.8990f };
static const MICS5524Curve MICS5524_NH3      = { 0.7842f, -1.9019f };

/* Default warm-up time (seconds) */
#ifndef WY_MICS5524_WARMUP_S
#define WY_MICS5524_WARMUP_S  30
#endif

/* ADC samples to average */
#ifndef WY_MICS5524_SAMPLES
#define WY_MICS5524_SAMPLES  16
#endif

class WyMICS5524 : public WySensorBase {
public:
    WyMICS5524(WyGPIOPins pins) : _aoPin(pins.pin) {}

    const char* driverName() override { return "MICS-5524"; }

    /* Supply voltage to sensor (default 5.0V — check your board) */
    void setSupplyVoltage(float vcc)    { _vcc = vcc; }

    /* Load resistor on breakout board (kΩ, default 10kΩ) */
    void setLoadResistance(float rlKOhm) { _rlKOhm = rlKOhm; }

    /* R0 = sensor resistance in clean air (kΩ).
     * DEFAULT IS WRONG FOR YOUR UNIT — calibrate with calibrateR0() */
    void setR0(float r0KOhm)            { _r0KOhm = r0KOhm; }

    /* Voltage divider ratio (if 5V sensor on 3.3V ESP32 ADC) */
    void setDividerRatio(float ratio)   { _divRatio = (ratio > 0) ? ratio : 1.0f; }

    /* Override warm-up period. Use 180s for first-time cold start. */
    void setWarmupSeconds(uint16_t s)   { _warmupSec = s; }

    void setSamples(uint8_t n)          { _samples = n ? n : 1; }

    /* Measure R0 in clean air — call after full warm-up (3+ minutes outside).
     * Takes 10 averaged readings, stores result, returns R0 in kΩ. */
    float calibrateR0(uint8_t nReadings = 10) {
        Serial.println("[MICS5524] calibrating R0 — ensure clean air...");
        float rsSum = 0;
        for (uint8_t i = 0; i < nReadings; i++) {
            rsSum += _readRsKOhm();
            delay(500);
        }
        _r0KOhm = rsSum / nReadings;
        Serial.printf("[MICS5524] R0 = %.3f kΩ — save this value to NVS\n", _r0KOhm);
        return _r0KOhm;
    }

    float r0() { return _r0KOhm; }
    bool  isWarmedUp() { return (millis() - _startMs) >= (uint32_t)_warmupSec * 1000UL; }

    bool begin() override {
        if (_aoPin < 0) {
            Serial.println("[MICS5524] analog pin required");
            return false;
        }
        _startMs = millis();
        Serial.printf("[MICS5524] started — warm-up: %us  R0: %.2f kΩ\n",
            _warmupSec, _r0KOhm);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        if (!isWarmedUp()) {
            uint32_t remaining = _warmupSec - ((millis() - _startMs) / 1000);
            d.error = "warming up";
            d.raw   = (float)remaining;   /* seconds remaining */
            return d;
        }

        float rs = _readRsKOhm();
        if (rs <= 0) { d.error = "read error"; return d; }

        float ratio = rs / _r0KOhm;

        /* Clamp ratio to valid curve range (0.1–10) */
        if (ratio < 0.01f) ratio = 0.01f;
        if (ratio > 100.0f) ratio = 100.0f;

        float coPpm      = MICS5524_CO.A      * powf(ratio, MICS5524_CO.B);
        float ethanolPpm = MICS5524_ETHANOL.A * powf(ratio, MICS5524_ETHANOL.B);

        d.co2    = max(coPpm,      0.0f);   /* CO ppm */
        d.raw    = max(ethanolPpm, 0.0f);   /* Ethanol ppm */
        d.rawInt = (uint32_t)(ratio * 100); /* Rs/R0 × 100 (integer) */
        d.voltage = rs;                      /* Rs in kΩ */
        d.ok      = true;
        return d;
    }

    /* ppm for specific gas (uses gas curve) */
    float ppmCO()      { return _calcPpm(MICS5524_CO); }
    float ppmEthanol() { return _calcPpm(MICS5524_ETHANOL); }
    float ppmH2()      { return _calcPpm(MICS5524_H2); }
    float ppmNH3()     { return _calcPpm(MICS5524_NH3); }

    /* Raw sensor resistance (kΩ) */
    float readRs()     { return _readRsKOhm(); }

private:
    int8_t   _aoPin      = -1;
    float    _vcc        = 5.0f;
    float    _rlKOhm     = 10.0f;
    float    _r0KOhm     = 10.0f;   /* DEFAULT IS WRONG — calibrate! */
    float    _divRatio   = 1.0f;
    uint16_t _warmupSec  = WY_MICS5524_WARMUP_S;
    uint8_t  _samples    = WY_MICS5524_SAMPLES;
    uint32_t _startMs    = 0;

    float _readRsKOhm() {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < _samples; i++) {
            sum += analogRead(_aoPin);
            delay(2);
        }
        uint16_t raw = (uint16_t)(sum / _samples);

        /* ADC → voltage at ADC pin → actual sensor output voltage */
        float adcV  = (raw / 4095.0f) * 3.3f;
        float vout  = adcV / _divRatio;   /* actual Vout from sensor circuit */

        if (vout <= 0.0f || vout >= _vcc) return -1.0f;  /* out of range */

        /* Rs = RL × (VCC/Vout - 1) */
        float rs = _rlKOhm * ((_vcc / vout) - 1.0f);
        return rs;
    }

    float _calcPpm(const MICS5524Curve& curve) {
        if (!isWarmedUp()) return -1.0f;
        float rs    = _readRsKOhm();
        if (rs <= 0) return -1.0f;
        float ratio = rs / _r0KOhm;
        ratio = constrain(ratio, 0.01f, 100.0f);
        return max(curve.A * powf(ratio, curve.B), 0.0f);
    }
};
