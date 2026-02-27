/*
 * drivers/WyHCSR04.h — HC-SR04 / JSN-SR04T Ultrasonic Distance Sensor (GPIO)
 * ============================================================================
 * Datasheet: https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf
 * Bundled driver — no external library needed.
 * Two GPIO pins: TRIG (output) + ECHO (input)
 * Registered via WySensors::addGPIO<WyHCSR04>("name", TRIG_PIN, ECHO_PIN)
 *
 * Sensor variants (same protocol):
 *   HC-SR04          — standard, 2–400cm, indoor use, 5V supply
 *   HC-SR04P         — 3.3V compatible version (direct ESP32 use)
 *   JSN-SR04T        — waterproof probe, 20–600cm, outdoor / liquid use
 *   AJ-SR04M         — waterproof, auto/R1 output modes, 20–700cm
 *   DYP-ME007Y       — waterproof, serial or analog output variant
 *
 * ═══════════════════════════════════════════════════════════════════
 * HOW IT WORKS
 * ═══════════════════════════════════════════════════════════════════
 * 1. Pull TRIG HIGH for 10µs → sensor fires 8× 40kHz ultrasonic bursts
 * 2. ECHO pin goes HIGH for exactly as long as the sound takes to
 *    travel to the object and back
 * 3. distance = echo_duration_µs × speed_of_sound / 2
 *
 * Speed of sound varies with temperature: ~343 m/s at 20°C.
 * Error without compensation: ~0.17% per °C → ~0.5cm at 1m per 3°C.
 * For indoor use this is usually negligible. For precision, pair with
 * a BME280/DHT22 and call setTemperature().
 *
 * ═══════════════════════════════════════════════════════════════════
 * ⚠️ VOLTAGE WARNING — HC-SR04 (classic) needs 5V
 * ═══════════════════════════════════════════════════════════════════
 * Classic HC-SR04: VCC = 5V, ECHO output = 5V → damages ESP32 GPIO.
 *
 * Solutions:
 *   1. Use HC-SR04P (3.3V version) — direct connection, no divider
 *   2. Voltage divider on ECHO: 1kΩ + 2kΩ (ratio 0.667 → 3.3V max)
 *   3. Level shifter module (clean solution for production)
 *   4. TRIG is fine at 3.3V on classic HC-SR04 (input threshold ~2V)
 *
 * JSN-SR04T: usually 5V supply, ECHO is 5V → same issue. Use divider.
 *
 * ═══════════════════════════════════════════════════════════════════
 * ⚠️ MINIMUM RANGE BLIND SPOT
 * ═══════════════════════════════════════════════════════════════════
 * HC-SR04:   minimum ~2cm (sensor can't hear echo of very close objects)
 * JSN-SR04T: minimum ~20cm (waterproof design has longer ring-down time)
 * AJ-SR04M:  minimum ~20cm
 *
 * Objects closer than minimum range cause unpredictable readings — often
 * returns maximum range value (echo never detected) or previous reading.
 * The driver returns -1 for readings outside valid range.
 *
 * ═══════════════════════════════════════════════════════════════════
 * MEDIAN FILTERING — recommended
 * ═══════════════════════════════════════════════════════════════════
 * Ultrasonic sensors are noisy. Spurious echoes from room reflections,
 * moving objects, vibration, and sensor warm-up cause occasional spikes.
 * Taking the median of N readings (default 3) eliminates most spikes.
 *
 * setSamples(5) — 5-reading median, ~125ms per call (5 × 25ms)
 * setSamples(1) — raw single reading, fastest but noisiest
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 * HC-SR04P (3.3V version — simplest):
 *   VCC  → 3.3V
 *   GND  → GND
 *   TRIG → ESP32 GPIO (any digital output)
 *   ECHO → ESP32 GPIO (any digital input)
 *
 * HC-SR04 classic (5V, with voltage divider on ECHO):
 *   VCC  → 5V
 *   GND  → GND
 *   TRIG → ESP32 GPIO (3.3V output is fine — threshold is ~2V)
 *   ECHO → 1kΩ → ESP32 GPIO
 *              → 2kΩ → GND   (gives 3.3V max into GPIO ✓)
 *
 * JSN-SR04T (waterproof):
 *   VCC  → 5V
 *   GND  → GND
 *   TRIG → ESP32 GPIO
 *   ECHO → voltage divider as above (1kΩ + 2kΩ)
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   // Basic (pin = TRIG, pin2 = ECHO):
 *   auto* us = sensors.addGPIO<WyHCSR04>("distance", TRIG_PIN, ECHO_PIN);
 *   us->setSamples(3);           // median of 3 (default)
 *   us->setMinRange(20.0f);      // JSN-SR04T: 20cm min
 *   us->setTemperature(25.0f);   // for accurate speed-of-sound
 *   sensors.begin();
 *
 *   WySensorData d = sensors.read("distance");
 *   if (d.ok) {
 *       Serial.printf("Distance: %.1f cm  (%.1f mm)\n",
 *           d.raw, d.distance);
 *   }
 *
 * WySensorData:
 *   d.distance = distance in mm (primary output)
 *   d.raw      = distance in cm
 *   d.rawInt   = echo pulse duration in µs (raw timing)
 *   d.ok       = true when reading within valid range
 *   d.error    = "out of range" or "timeout" on failure
 */

#pragma once
#include "../WySensors.h"

/* Timeout: 25ms = sound at 343m/s travelling ~4.3m one way (8.6m round trip) */
#define HCSR04_TIMEOUT_US   25000UL

/* Minimum time between trigger pulses — sensor needs ~60ms to recover */
#define HCSR04_MIN_INTERVAL_MS  60

class WyHCSR04 : public WySensorBase {
public:
    /* WyGPIOPins: pin = TRIG, pin2 = ECHO */
    WyHCSR04(WyGPIOPins pins) : _trig(pins.pin), _echo(pins.pin2) {}

    const char* driverName() override { return "HC-SR04"; }

    /* Temperature for speed-of-sound correction (°C, default 20°C) */
    void setTemperature(float tempC) { _tempC = tempC; }

    /* Number of readings to median-filter (1 = raw, 3 = default, 5 = smooth) */
    void setSamples(uint8_t n)       { _samples = (n > 7) ? 7 : (n ? n : 1); }

    /* Sensor minimum detection range (cm). HC-SR04=2, JSN-SR04T=20 */
    void setMinRange(float cm)       { _minCm = cm; }

    /* Sensor maximum detection range (cm). HC-SR04=400, JSN-SR04T=600 */
    void setMaxRange(float cm)       { _maxCm = cm; }

    bool begin() override {
        if (_trig < 0 || _echo < 0) {
            Serial.println("[HC-SR04] TRIG pin = pin, ECHO pin = pin2");
            return false;
        }
        pinMode(_trig, OUTPUT);
        pinMode(_echo, INPUT);
        digitalWrite(_trig, LOW);
        delay(100);  /* sensor power-on settle */

        /* Test read */
        float d = _singleMeasure();
        if (d < 0) {
            Serial.println("[HC-SR04] begin: first read timed out — check wiring");
            /* Not fatal — sensor may just have nothing in range */
        } else {
            Serial.printf("[HC-SR04] online — %.1f cm (%.0f–%.0f cm range)\n",
                d, _minCm, _maxCm);
        }
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        if (_samples == 1) {
            float cm = _singleMeasure();
            if (cm < 0) { d.error = "out of range"; return d; }
            d.distance = cm * 10.0f;
            d.raw      = cm;
            d.ok       = true;
        } else {
            /* Median filter: take N readings, sort, return middle value */
            float buf[7] = {};
            uint8_t valid = 0;
            for (uint8_t i = 0; i < _samples; i++) {
                float cm = _singleMeasure();
                if (cm > 0) buf[valid++] = cm;
                delay(HCSR04_MIN_INTERVAL_MS);  /* sensor recovery time */
            }
            if (valid == 0) { d.error = "all samples failed"; return d; }

            /* Simple insertion sort on small array */
            for (uint8_t i = 1; i < valid; i++) {
                float key = buf[i];
                int8_t j = i - 1;
                while (j >= 0 && buf[j] > key) { buf[j+1] = buf[j]; j--; }
                buf[j+1] = key;
            }

            float median = buf[valid / 2];
            d.distance = median * 10.0f;  /* mm */
            d.raw      = median;          /* cm */
            d.rawInt   = valid;           /* how many valid samples */
            d.ok       = true;
        }
        return d;
    }

    /* Convenience: direct unit reads (takes a fresh measurement each call) */
    float readCm()    { WySensorData d = read(); return d.ok ? d.raw      : -1.0f; }
    float readMm()    { WySensorData d = read(); return d.ok ? d.distance : -1.0f; }
    float readM()     { WySensorData d = read(); return d.ok ? d.raw/100.0f : -1.0f; }
    float readInch()  { WySensorData d = read(); return d.ok ? d.raw/2.54f : -1.0f; }

private:
    int8_t  _trig    = -1;
    int8_t  _echo    = -1;
    float   _tempC   = 20.0f;
    uint8_t _samples = 3;
    float   _minCm   = 2.0f;
    float   _maxCm   = 400.0f;

    float _singleMeasure() {
        /* Enforce minimum inter-measurement interval */
        static uint32_t _lastTrig = 0;
        uint32_t now = millis();
        if ((now - _lastTrig) < HCSR04_MIN_INTERVAL_MS) {
            delay(HCSR04_MIN_INTERVAL_MS - (now - _lastTrig));
        }

        /* Send 10µs trigger pulse */
        digitalWrite(_trig, LOW);
        delayMicroseconds(4);
        digitalWrite(_trig, HIGH);
        delayMicroseconds(10);
        digitalWrite(_trig, LOW);
        _lastTrig = millis();

        /* Wait for ECHO to go HIGH (start of return pulse) */
        uint32_t t0 = micros();
        while (digitalRead(_echo) == LOW) {
            if (micros() - t0 > 5000UL) return -1.0f;  /* no echo start */
        }

        /* Measure ECHO pulse duration */
        uint32_t pulseStart = micros();
        while (digitalRead(_echo) == HIGH) {
            if (micros() - pulseStart > HCSR04_TIMEOUT_US) return -1.0f;
        }
        uint32_t duration = micros() - pulseStart;

        /* Speed of sound: 331.4 + 0.606×T (m/s) → cm/µs = ÷10000 */
        float speedCmUs = (331.4f + 0.606f * _tempC) / 10000.0f;
        float cm = (duration * speedCmUs) / 2.0f;  /* ÷2 for round trip */

        if (cm < _minCm || cm > _maxCm) return -1.0f;
        return cm;
    }
};

/* ── Variant aliases ─────────────────────────────────────────────── */

/* JSN-SR04T — waterproof, 20–600cm */
class WyJSNSR04T : public WyHCSR04 {
public:
    WyJSNSR04T(WyGPIOPins pins) : WyHCSR04(pins) {
        setMinRange(20.0f);
        setMaxRange(600.0f);
    }
    const char* driverName() override { return "JSN-SR04T"; }
};

/* AJ-SR04M — waterproof, 20–700cm, auto mode */
class WyAJSR04M : public WyHCSR04 {
public:
    WyAJSR04M(WyGPIOPins pins) : WyHCSR04(pins) {
        setMinRange(20.0f);
        setMaxRange(700.0f);
    }
    const char* driverName() override { return "AJ-SR04M"; }
};
