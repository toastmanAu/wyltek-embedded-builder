/*
 * drivers/WyDHT22.h — DHT22 / DHT11 temperature + humidity (GPIO)
 * =================================================================
 * Bundled single-wire driver. No external library needed.
 * Registered via WySensors::addGPIO<WyDHT22>("name", pin)
 *
 * Also works as WyDHT11 — same protocol, different bit interpretation.
 * Pass mode=11 for DHT11, mode=22 (default) for DHT22/AM2302.
 */

#pragma once
#include "../WySensors.h"

class WyDHT22 : public WySensorBase {
public:
    WyDHT22(WyGPIOPins pins, uint8_t model = 22)
        : _pin(pins.pin), _model(model) {}

    const char* driverName() override { return _model == 11 ? "DHT11" : "DHT22"; }

    bool begin() override {
        pinMode(_pin, INPUT_PULLUP);
        delay(2000);  /* DHT22 needs 2s after power-on */
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        uint8_t data[5] = {0};

        /* Send start signal */
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        delay(_model == 11 ? 18 : 1);
        digitalWrite(_pin, HIGH);
        delayMicroseconds(30);
        pinMode(_pin, INPUT_PULLUP);

        /* Wait for sensor response */
        if (!_waitLevel(LOW,  80)) { d.error = "no response (low)";  return d; }
        if (!_waitLevel(HIGH, 80)) { d.error = "no response (high)"; return d; }
        if (!_waitLevel(LOW,  80)) { d.error = "no response (sync)"; return d; }

        /* Read 40 bits */
        for (uint8_t i = 0; i < 40; i++) {
            if (!_waitLevel(HIGH, 50)) { d.error = "read timeout"; return d; }
            uint32_t t = micros();
            if (!_waitLevel(LOW, 70))  { d.error = "bit timeout";  return d; }
            if (micros() - t > 35) data[i/8] |= (1 << (7 - i%8));
        }

        /* Checksum */
        if (((data[0]+data[1]+data[2]+data[3]) & 0xFF) != data[4]) {
            d.error = "checksum fail"; return d;
        }

        if (_model == 11) {
            d.humidity    = data[0];
            d.temperature = data[2];
        } else {
            d.humidity    = ((data[0] << 8) | data[1]) * 0.1f;
            int16_t raw_t = ((data[2] & 0x7F) << 8) | data[3];
            d.temperature = raw_t * 0.1f * (data[2] & 0x80 ? -1 : 1);
        }
        d.ok = true;
        return d;
    }

private:
    int8_t  _pin;
    uint8_t _model;

    bool _waitLevel(uint8_t level, uint32_t timeoutUs) {
        uint32_t t = micros();
        while (digitalRead(_pin) != level)
            if (micros() - t > timeoutUs) return false;
        return true;
    }
};

/* Alias */
using WyDHT11 = WyDHT22;
