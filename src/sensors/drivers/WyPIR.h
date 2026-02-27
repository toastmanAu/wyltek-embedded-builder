/*
 * drivers/WyPIR.h — Generic PIR motion sensor (GPIO)
 * ====================================================
 * Covers all common PIR modules with digital HIGH/LOW output:
 *
 *   MH-SR602   — mini PIR, fixed sensitivity, 2.7V–12V, ultra small
 *   HC-SR501   — adjustable sensitivity + delay, 5V–20V, most common
 *   AM312      — 3.3V micro PIR, fixed settings, smallest package
 *   RCWL-0516  — microwave doppler (not PIR, but same digital interface)
 *   D203S      — TO-5 PIR element (needs external signal conditioning)
 *   E18-D80NK  — IR obstacle sensor (same output interface)
 *
 * All output: digital HIGH when motion detected, LOW when clear.
 *
 * Registered via WySensors::addGPIO<WyPIR>("motion", SIGNAL_PIN)
 *
 * HC-SR501 HARDWARE SETTINGS (trim pots on module):
 *   Left pot  (Sx) — sensitivity: clockwise = more sensitive (up to ~7m)
 *   Right pot (Tx) — delay time: clockwise = longer hold (0.5s–200s)
 *   Jumper H  — retriggerable: re-starts delay timer if motion continues
 *   Jumper L  — non-retriggerable: one pulse per trigger regardless of motion
 *
 * MH-SR602 HARDWARE:
 *   Fixed sensitivity (~3m), fixed delay (~2s), no pots
 *   Tiny — 1cm × 1cm, runs on 2.7V–12V, output 3.3V compatible
 *   Warm-up time: ~30 seconds after power-on (outputs garbage until then)
 *
 * AM312 HARDWARE:
 *   3.3V supply, fixed ~2m range, fixed ~2s delay
 *   No warm-up required (fresnel lens pre-installed, PIR element exposed)
 *
 * WIRING:
 *   VCC → 3.3V (AM312, some MH-SR602) or 5V (HC-SR501, standard MH-SR602)
 *   GND → GND
 *   OUT → ESP32 GPIO (3.3V signal level on all models)
 *   Note: HC-SR501 output is ~3.3V HIGH even from 5V supply — ESP32 safe
 *
 * WARM-UP:
 *   Most PIRs need 30–60 seconds after power-on before reliable detection.
 *   begin() documents the warm-up start time.
 *   Call isWarmedUp() before trusting motion() results.
 *
 * WySensorData:
 *   d.ok     = true (always — digital output is always valid signal)
 *   d.rawInt = 1 if motion, 0 if clear
 *   d.raw    = same as rawInt (float)
 *
 * Usage:
 *   auto* pir = sensors.addGPIO<WyPIR>("motion", PIR_PIN);
 *   pir->setWarmup(30000);  // 30s for HC-SR501, 0 for AM312
 *   sensors.begin();
 *
 *   // In loop:
 *   if (pir->motion()) { Serial.println("Motion!"); }
 *   // Or via registry:
 *   WySensorData d = sensors.read("motion");
 *   if (d.rawInt) { ... }
 *
 * Multiple zones:
 *   sensors.addGPIO<WyPIR>("hallway",  PIN_HALL);
 *   sensors.addGPIO<WyPIR>("entrance", PIN_ENTRY);
 *   sensors.addGPIO<WyPIR>("garden",   PIN_GARDEN);
 */

#pragma once
#include "../WySensors.h"

#ifndef WY_PIR_WARMUP_MS
#define WY_PIR_WARMUP_MS  30000UL  /* 30 seconds default */
#endif

#ifndef WY_PIR_DEBOUNCE_MS
#define WY_PIR_DEBOUNCE_MS  50  /* ignore glitches shorter than this */
#endif

class WyPIR : public WySensorBase {
public:
    WyPIR(WyGPIOPins pins, uint8_t activeLevel = HIGH)
        : _pin(pins.pin), _activeLevel(activeLevel) {}

    const char* driverName() override { return "PIR"; }

    bool begin() override {
        pinMode(_pin, INPUT);
        _warmupStart = millis();
        _lastState   = false;
        _lastChange  = millis();
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        bool m = motion();
        d.rawInt = m ? 1 : 0;
        d.raw    = (float)d.rawInt;
        d.ok     = true;
        if (!isWarmedUp()) d.error = "warming up";
        return d;
    }

    /* ── Motion detection ────────────────────────────────────────── */

    /* Returns true while motion is detected (HIGH signal from sensor) */
    bool motion() {
        bool current = (digitalRead(_pin) == _activeLevel);

        /* Debounce — only update state after signal stable for WY_PIR_DEBOUNCE_MS */
        if (current != _rawState) {
            _rawState   = current;
            _lastChange = millis();
        }
        if (millis() - _lastChange >= WY_PIR_DEBOUNCE_MS) {
            _lastState = _rawState;
        }

        return _lastState;
    }

    /* Returns true only on the rising edge (start of motion event) */
    bool motionStarted() {
        bool current = motion();
        bool edge    = current && !_prevState;
        _prevState   = current;
        return edge;
    }

    /* Returns true only on the falling edge (motion cleared) */
    bool motionEnded() {
        bool current = motion();
        bool edge    = !current && _prevState;
        _prevState   = current;
        return edge;
    }

    /* Register callback — called on state change, call poll() in loop() */
    void onMotion(void (*cb)(bool detected)) { _callback = cb; }

    void poll() {
        bool current = motion();
        if (current != _callbackState) {
            _callbackState = current;
            if (_callback) _callback(current);
        }
    }

    /* ── Warm-up ─────────────────────────────────────────────────── */

    bool isWarmedUp() { return millis() - _warmupStart >= _warmupMs; }
    uint32_t warmupRemaining() {
        uint32_t elapsed = millis() - _warmupStart;
        return elapsed >= _warmupMs ? 0 : _warmupMs - elapsed;
    }
    void setWarmup(uint32_t ms) { _warmupMs = ms; }

    /* Skip warm-up (AM312 or pre-warmed sensor) */
    void skipWarmup() { _warmupStart = millis() - _warmupMs - 1; }

private:
    int8_t   _pin;
    uint8_t  _activeLevel;
    uint32_t _warmupStart = 0;
    uint32_t _warmupMs    = WY_PIR_WARMUP_MS;
    bool     _lastState   = false;
    bool     _rawState    = false;
    bool     _prevState   = false;
    bool     _callbackState = false;
    uint32_t _lastChange  = 0;
    void     (*_callback)(bool) = nullptr;
};

/* Named aliases for clarity */
using WyHCSR501  = WyPIR;
using WyMHSR602  = WyPIR;
using WyAM312    = WyPIR;
using WyRCWL0516 = WyPIR;  /* microwave doppler — same interface */
