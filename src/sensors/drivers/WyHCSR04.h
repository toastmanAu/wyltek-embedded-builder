/*
 * drivers/WyHCSR04.h — HC-SR04 / JSN-SR04T ultrasonic distance sensor (GPIO)
 * ===========================================================================
 * Datasheet: https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf
 * Bundled driver — no external library needed.
 * Uses TRIG (output) + ECHO (input) — 2 GPIO pins.
 * Registered via WySensors::addGPIO<WyHCSR04>("name", trig_pin, echo_pin)
 *
 * Range: 2cm – 400cm, resolution ~3mm
 * Timeout: ~25ms per measurement (38ms pulse = no object)
 *
 * JSN-SR04T (waterproof variant): same protocol, 20cm min range
 */

#pragma once
#include "../WySensors.h"

#define HCSR04_TIMEOUT_US   25000  /* 25ms = ~4.3m max distance */
#define HCSR04_TEMP_COEFF   0.0343f  /* cm per us at 20°C (343 m/s) */

class WyHCSR04 : public WySensorBase {
public:
    /* pin = TRIG, pin2 = ECHO */
    WyHCSR04(WyGPIOPins pins, float tempC = 20.0f)
        : _trig(pins.pin), _echo(pins.pin2), _tempC(tempC) {}

    const char* driverName() override { return "HC-SR04"; }

    bool begin() override {
        if (_trig < 0 || _echo < 0) {
            Serial.println("[HC-SR04] both trig and echo pins required");
            return false;
        }
        pinMode(_trig, OUTPUT);
        pinMode(_echo, INPUT);
        digitalWrite(_trig, LOW);
        delay(100);
        /* Test measurement */
        float d = _measure();
        return (d > 0 && d < 5000);  /* 0–5000mm plausible */
    }

    WySensorData read() override {
        WySensorData d;
        float dist_mm = _measure();
        if (dist_mm < 0) { d.error = "timeout / out of range"; return d; }
        d.distance  = dist_mm;
        d.raw       = dist_mm / 10.0f;  /* also provide cm in raw */
        d.ok        = true;
        return d;
    }

    /* Update temperature compensation (speed of sound changes with temp) */
    void setTemperature(float tempC) { _tempC = tempC; }

    /* Get last reading in specific units */
    float readCm()  { return _measure() / 10.0f; }
    float readM()   { return _measure() / 1000.0f; }
    float readInch(){ return _measure() / 25.4f; }

private:
    int8_t _trig, _echo;
    float  _tempC;

    float _measure() {
        /* Trigger: 10µs HIGH pulse */
        digitalWrite(_trig, LOW);
        delayMicroseconds(2);
        digitalWrite(_trig, HIGH);
        delayMicroseconds(10);
        digitalWrite(_trig, LOW);

        /* Measure ECHO pulse duration */
        uint32_t t = micros();
        while (digitalRead(_echo) == LOW) {
            if (micros() - t > 5000) return -1.0f;  /* no pulse start */
        }
        uint32_t pulse_start = micros();
        while (digitalRead(_echo) == HIGH) {
            if (micros() - pulse_start > HCSR04_TIMEOUT_US) return -1.0f;
        }
        uint32_t duration_us = micros() - pulse_start;

        /* Speed of sound: 331.4 + (0.606 * tempC) m/s
         * Distance = duration * speed / 2 (there and back)
         * In mm: duration_us * (331400 + 606*T) / 2 / 1000000 */
        float speed_mps = 331.4f + (0.606f * _tempC);
        float dist_mm   = (duration_us * speed_mps) / 2000.0f;

        /* Sanity check — HC-SR04 spec: 20mm to 4000mm */
        if (dist_mm < 20.0f || dist_mm > 4500.0f) return -1.0f;
        return dist_mm;
    }
};

/* Alias for waterproof variant */
using WyJSNSR04T = WyHCSR04;
