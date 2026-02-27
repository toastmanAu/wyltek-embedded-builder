/*
 * drivers/WyX9C.h — X9C digital potentiometer (3-wire GPIO)
 * ===========================================================
 * Datasheet: https://www.renesas.com/us/en/document/dst/x9c102-x9c103-x9c503-x9c104-datasheet
 * Bundled driver — no external library needed.
 * Uses 3 GPIO pins: CS, INC, U_D (not standard SPI).
 * Registered via WySensors::addGPIO<WyX9C103>("pot", CS_PIN, INC_PIN)
 * + setUDPin(UD_PIN) before begin()
 *
 * VARIANTS (same driver, different resistance):
 *   X9C102  —  1kΩ  (model=102)
 *   X9C103  — 10kΩ  (model=103, default)
 *   X9C503  — 50kΩ  (model=503)
 *   X9C104  —100kΩ  (model=104)
 *   All: 100 wiper positions (0–99), ~1% step resolution
 *
 * PIN FUNCTIONS:
 *   CS   (Chip Select)    — active LOW, enables the chip
 *   INC  (Increment)      — falling edge steps the wiper
 *   U/_D (Up/Down)        — HIGH = increment (wiper up), LOW = decrement
 *   VH   — high terminal of resistor
 *   VL   — low terminal of resistor
 *   VW   — wiper output
 *
 * WIPER MOVEMENT:
 *   Each falling edge on INC moves the wiper one step in the U/_D direction.
 *   Direction must be set BEFORE the falling edge (setup time = 1µs min).
 *   Minimum pulse width: 1µs HIGH, 1µs LOW on INC.
 *
 * NV MEMORY:
 *   The X9C has non-volatile memory — the wiper position is stored to NV
 *   when CS rises while INC is LOW. This survives power cycles.
 *   store() triggers the NV write. Do NOT call on every adjustment —
 *   NV write endurance is ~100,000 cycles.
 *
 * WIRING:
 *   CS   → GPIO (any)
 *   INC  → GPIO (any)
 *   U/_D → GPIO (any)
 *   VH   → high voltage reference (e.g. 3.3V or signal input)
 *   VL   → low voltage reference (e.g. GND)
 *   VW   → wiper output (variable voltage divider or resistance)
 *   VCC  → 5V (2.7V–5.5V range)
 *   GND  → GND
 *
 * TYPICAL USE — variable voltage divider:
 *   VH = 3.3V, VL = GND → VW = 3.3V × (position / 99)
 *
 * TYPICAL USE — variable gain/bias on op-amp:
 *   Wire wiper between feedback resistors to vary gain digitally.
 *
 * Registered:
 *   auto* pot = sensors.addGPIO<WyX9C103>("volume", CS_PIN, INC_PIN);
 *   pot->setUDPin(UD_PIN);
 *   sensors.begin();
 *   pot->set(50);   // set to midpoint
 *
 * WySensorData:
 *   d.raw    = current wiper position (0–99)
 *   d.voltage = position as percentage (0.0–1.0)
 *   d.ok     = true
 */

#pragma once
#include "../WySensors.h"

/* How many µs to wait between INC pulses (datasheet min = 1µs, use 2 for safety) */
#ifndef WY_X9C_PULSE_US
#define WY_X9C_PULSE_US  2
#endif

class WyX9C : public WySensorBase {
public:
    /* pin = CS, pin2 = INC */
    WyX9C(WyGPIOPins pins, const char* name = "X9C")
        : _cs(pins.pin), _inc(pins.pin2), _name(name) {}

    void setUDPin(int8_t ud) { _ud = ud; }

    const char* driverName() override { return _name; }

    bool begin() override {
        if (_inc < 0 || _ud < 0) {
            Serial.printf("[%s] INC (pin2) and UD pins required\n", _name);
            return false;
        }
        pinMode(_cs,  OUTPUT);
        pinMode(_inc, OUTPUT);
        pinMode(_ud,  OUTPUT);

        /* Idle state: CS HIGH, INC HIGH */
        digitalWrite(_cs,  HIGH);
        digitalWrite(_inc, HIGH);
        digitalWrite(_ud,  HIGH);

        /* We don't know the current wiper position after power-on
         * (it restores from NV to wherever it was saved).
         * Drive to position 0 to establish a known state. */
        _position = 99;  /* assume worst case: at top */
        _move(-99);      /* drive to 0 */
        _position = 0;

        return true;
    }

    WySensorData read() override {
        WySensorData d;
        d.raw     = (float)_position;
        d.voltage = _position / 99.0f;
        d.ok      = true;
        return d;
    }

    /* ── Wiper control ───────────────────────────────────────────── */

    /* Set absolute position 0–99 */
    void set(uint8_t pos) {
        pos = constrain(pos, 0, 99);
        int8_t delta = (int8_t)pos - (int8_t)_position;
        _move(delta);
    }

    /* Increment by N steps (positive = up, negative = down) */
    void move(int8_t steps) { _move(steps); }

    void increment() { _move(1);  }
    void decrement() { _move(-1); }

    uint8_t position() { return _position; }

    /* Set as percentage 0.0–1.0 */
    void setPercent(float pct) { set((uint8_t)(constrain(pct, 0.0f, 1.0f) * 99.0f)); }
    float percent()            { return _position / 99.0f; }

    /* Store current position to NV memory (survives power cycle).
     * NV write endurance: ~100,000 cycles — don't call on every adjustment. */
    void store() {
        /* NV store: CS rises while INC is LOW */
        digitalWrite(_cs,  LOW);
        delayMicroseconds(WY_X9C_PULSE_US);
        digitalWrite(_inc, LOW);
        delayMicroseconds(WY_X9C_PULSE_US);
        digitalWrite(_cs,  HIGH);   /* rising CS with INC LOW = store */
        delayMicroseconds(20000);   /* NV write time: max 20ms */
        digitalWrite(_inc, HIGH);
    }

private:
    int8_t      _cs, _inc, _ud = -1;
    uint8_t     _position = 0;
    const char* _name;

    /* Move wiper by delta steps (positive = up, negative = down) */
    void _move(int8_t delta) {
        if (delta == 0) return;

        bool up = (delta > 0);
        uint8_t steps = up ? delta : -delta;

        /* Clamp to valid range */
        if (up)  steps = min((int)steps, 99 - (int)_position);
        else     steps = min((int)steps, (int)_position);

        if (steps == 0) return;

        /* Set direction BEFORE enabling chip */
        digitalWrite(_ud, up ? HIGH : LOW);
        delayMicroseconds(WY_X9C_PULSE_US);

        /* Enable chip */
        digitalWrite(_cs, LOW);
        delayMicroseconds(WY_X9C_PULSE_US);

        /* Pulse INC 'steps' times — each falling edge = one wiper step */
        for (uint8_t i = 0; i < steps; i++) {
            digitalWrite(_inc, HIGH);
            delayMicroseconds(WY_X9C_PULSE_US);
            digitalWrite(_inc, LOW);
            delayMicroseconds(WY_X9C_PULSE_US);
        }

        /* Deselect — CS HIGH with INC LOW does NOT store (INC must be HIGH first) */
        digitalWrite(_inc, HIGH);
        delayMicroseconds(WY_X9C_PULSE_US);
        digitalWrite(_cs, HIGH);

        /* Update tracked position */
        _position = up ? _position + steps : _position - steps;
    }
};

/* Named aliases per resistance value */
class WyX9C102 : public WyX9C {
public: WyX9C102(WyGPIOPins p) : WyX9C(p, "X9C102") {}  /* 1kΩ  */
};
class WyX9C103 : public WyX9C {
public: WyX9C103(WyGPIOPins p) : WyX9C(p, "X9C103") {}  /* 10kΩ */
};
class WyX9C503 : public WyX9C {
public: WyX9C503(WyGPIOPins p) : WyX9C(p, "X9C503") {}  /* 50kΩ */
};
class WyX9C104 : public WyX9C {
public: WyX9C104(WyGPIOPins p) : WyX9C(p, "X9C104") {}  /* 100kΩ */
};
