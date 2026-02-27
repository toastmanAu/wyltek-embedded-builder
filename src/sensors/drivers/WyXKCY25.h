/*
 * drivers/WyXKCY25.h — XKC-Y25-V / XKC-Y23-V non-contact liquid level sensor (GPIO)
 * ====================================================================================
 * Datasheet: XKC-Y25/Y23 series, Shenzhen Xingkong Tech
 * Bundled driver — no external library needed.
 * Registered via WySensors::addGPIO<WyXKCY25>("tank_level", SIGNAL_PIN)
 *
 * HOW IT WORKS:
 *   Capacitive sensing through non-conductive tank walls (plastic, glass, ceramic).
 *   The sensor detects the change in capacitance when liquid is present at its level.
 *   Output is a simple digital HIGH or LOW — no calibration needed.
 *   Does NOT work through metal walls.
 *
 * VARIANTS (all use this driver):
 *   XKC-Y25-V   — NPN output, signal LOW when liquid present  (most common)
 *   XKC-Y25-PNP — PNP output, signal HIGH when liquid present
 *   XKC-Y23-V   — smaller body, same electrical interface
 *   XKC-Y24-V   — longer sensing range version
 *
 * OUTPUT LOGIC (XKC-Y25-V NPN, default):
 *   Liquid detected:    signal pin → LOW  (NPN pulls to GND)
 *   No liquid present:  signal pin → HIGH (pulled up to Vcc via internal resistor)
 *
 * OUTPUT LOGIC (XKC-Y25-PNP):
 *   Liquid detected:    signal pin → HIGH
 *   No liquid present:  signal pin → LOW
 *   Use: sensors.addGPIO<WyXKCY25PNP>("level", pin)
 *
 * WIRING:
 *   Brown  → 5V–12V supply (5V from ESP32 VIN works)
 *   Blue   → GND
 *   Black  → signal output → ESP32 GPIO (add 10kΩ pull-up to 3.3V for NPN)
 *
 *   ┌─────────────────────────────────┐
 *   │  ESP32 3.3V ──┐                 │
 *   │              10kΩ               │
 *   │               │                 │
 *   │  GPIO ────────┴──── sensor OUT  │
 *   └─────────────────────────────────┘
 *   Pull-up is REQUIRED for NPN variant — without it pin floats when no liquid.
 *
 * MOUNTING:
 *   Stick flat face of sensor to outside of tank, at the level you want to detect.
 *   Use the included mounting pad or double-sided tape.
 *   Works through walls up to ~14mm thick (adjust with sensitivity screw if present).
 *   Keep away from metal objects — they interfere with capacitive field.
 *
 * WySensorData:
 *   d.ok       = true always (sensor always operational)
 *   d.rawInt   = 1 if liquid present, 0 if not
 *   d.raw      = same as rawInt (float)
 *   d.distance = 0.0 (no distance — level is binary)
 *
 * Multiple level sensors (e.g. low/mid/high):
 *   sensors.addGPIO<WyXKCY25>("level_low",  PIN_LOW);
 *   sensors.addGPIO<WyXKCY25>("level_high", PIN_HIGH);
 *   bool low  = sensors.read("level_low").rawInt;
 *   bool high = sensors.read("level_high").rawInt;
 */

#pragma once
#include "../WySensors.h"

/* Debounce — how many consecutive matching reads before state is accepted */
#ifndef WY_XKCY25_DEBOUNCE
#define WY_XKCY25_DEBOUNCE  3
#endif

class WyXKCY25 : public WySensorBase {
public:
    /* activeLevel: LOW for NPN (default), HIGH for PNP */
    WyXKCY25(WyGPIOPins pins, uint8_t activeLevel = LOW)
        : _pin(pins.pin), _activeLevel(activeLevel) {}

    const char* driverName() override { return "XKC-Y25"; }

    bool begin() override {
        /* NPN: needs pull-up. INPUT_PULLUP uses ESP32 internal ~45kΩ.
         * For long wire runs, add external 10kΩ pull-up for reliability. */
        pinMode(_pin, _activeLevel == LOW ? INPUT_PULLUP : INPUT_PULLDOWN);
        delay(100);  /* sensor settles on power-on */
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        bool detected = _readDebounced();

        d.rawInt = detected ? 1 : 0;
        d.raw    = (float)d.rawInt;
        d.ok     = true;
        return d;
    }

    /* Direct bool read — most convenient */
    bool liquidPresent() { return _readDebounced(); }

    /* Register a callback for state changes (call poll() in loop) */
    void onStateChange(void (*cb)(bool present)) { _callback = cb; }

    /* Call in loop() to fire state-change callbacks */
    void poll() {
        bool current = _readDebounced();
        if (current != _lastState) {
            _lastState = current;
            if (_callback) _callback(current);
        }
    }

private:
    int8_t   _pin;
    uint8_t  _activeLevel;
    bool     _lastState = false;
    void     (*_callback)(bool) = nullptr;

    /* Debounce: read N times with small gap, only accept if all agree */
    bool _readDebounced() {
        uint8_t count = 0;
        for (uint8_t i = 0; i < WY_XKCY25_DEBOUNCE; i++) {
            if (digitalRead(_pin) == _activeLevel) count++;
            delayMicroseconds(500);
        }
        return count >= (WY_XKCY25_DEBOUNCE + 1) / 2;  /* majority vote */
    }
};

/* PNP variant alias */
class WyXKCY25PNP : public WyXKCY25 {
public:
    WyXKCY25PNP(WyGPIOPins pins) : WyXKCY25(pins, HIGH) {}
    const char* driverName() override { return "XKC-Y25-PNP"; }
};

/* Y23 variant aliases (same electrical interface) */
using WyXKCY23    = WyXKCY25;
using WyXKCY23PNP = WyXKCY25PNP;
using WyXKCY24    = WyXKCY25;
