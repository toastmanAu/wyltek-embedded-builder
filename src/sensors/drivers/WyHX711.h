/*
 * drivers/WyHX711.h — HX711 24-bit ADC for load cells / weight scales (GPIO)
 * ============================================================================
 * Datasheet: https://cdn.sparkfun.com/datasheets/Sensors/ForceFlex/hx711_english.pdf
 * Bundled driver — no external library needed.
 * Uses 2 GPIO pins: CLK (output) + DOUT (input).
 * Registered via WySensors::addGPIO<WyHX711>("scale", DOUT, CLK)
 * (pin = DOUT, pin2 = CLK)
 *
 * Load cell wiring (standard colour code):
 *   Red   → E+ (excitation +)
 *   Black → E- (excitation -)
 *   White → A- (signal -)
 *   Green → A+ (signal +)
 *
 * HX711 module typically has 5V power, 3.3V-tolerant DOUT on ESP32 GPIO.
 *
 * Gain channels:
 *   Channel A gain 128 (default): most common, 40mV full-scale
 *   Channel A gain 64:            20mV full-scale
 *   Channel B gain 32:            80mV full-scale (second load cell)
 *
 * Usage:
 *   auto* scale = sensors.addGPIO<WyHX711>("weight", DOUT, CLK);
 *   sensors.begin();
 *   scale->tare();          // zero the scale
 *   scale->setCalib(420.3); // grams per raw unit (from known weight)
 *   WySensorData d = sensors.read("weight");
 *   // d.weight = grams, d.raw = raw 24-bit reading
 */

#pragma once
#include "../WySensors.h"

#define HX711_GAIN_A128  1  /* 25 pulses */
#define HX711_GAIN_B32   2  /* 26 pulses */
#define HX711_GAIN_A64   3  /* 27 pulses */

class WyHX711 : public WySensorBase {
public:
    /* pin = DOUT, pin2 = CLK */
    WyHX711(WyGPIOPins pins, uint8_t gain = HX711_GAIN_A128)
        : _dout(pins.pin), _clk(pins.pin2), _gain(gain) {}

    const char* driverName() override { return "HX711"; }

    bool begin() override {
        if (_clk < 0) {
            Serial.println("[HX711] CLK pin required as pin2");
            return false;
        }
        pinMode(_clk,  OUTPUT);
        pinMode(_dout, INPUT);
        digitalWrite(_clk, LOW);

        /* Power on and verify DOUT goes LOW (data ready) */
        powerOn();
        uint32_t t = millis();
        while (digitalRead(_dout) == HIGH) {
            if (millis() - t > 1000) {
                Serial.println("[HX711] DOUT never went LOW — check wiring");
                return false;
            }
        }

        /* Initial read to set gain channel */
        _readRaw();
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        if (!isReady()) {
            d.error = "not ready";
            return d;
        }
        int32_t raw = _readRaw();
        _lastRaw = raw;
        d.raw    = (float)raw;
        d.weight = (raw - _tare) / _calibFactor;
        d.ok     = true;
        return d;
    }

    /* ── Scale control ─────────────────────────────────────────── */

    /* Tare: record current reading as zero (average N samples) */
    void tare(uint8_t samples = 10) {
        int64_t sum = 0;
        for (uint8_t i = 0; i < samples; i++) {
            while (!isReady()) delay(1);
            sum += _readRaw();
        }
        _tare = sum / samples;
        Serial.printf("[HX711] tare = %ld\n", (long)_tare);
    }

    /* Set calibration factor: grams per raw unit
     * How to calibrate:
     *   1. tare()
     *   2. Place known weight (e.g. 500g)
     *   3. Read raw: raw = scale->rawValue()
     *   4. setCalib(500.0 / raw)
     */
    void setCalib(float gramsPerUnit) { _calibFactor = gramsPerUnit; }

    /* Get raw ADC value (after tare subtracted) */
    int32_t rawValue() { return _lastRaw - _tare; }

    /* Check if data is ready (DOUT goes LOW) */
    bool isReady() { return digitalRead(_dout) == LOW; }

    /* Power down (CLK HIGH > 60µs = power down) */
    void powerDown() {
        digitalWrite(_clk, LOW);
        digitalWrite(_clk, HIGH);
        delayMicroseconds(65);
    }

    /* Power on */
    void powerOn() {
        digitalWrite(_clk, LOW);
        delay(400);  /* HX711 needs ~400ms to stabilise after power-on */
    }

    /* Set gain channel for next reading */
    void setGain(uint8_t gain) { _gain = constrain(gain, 1, 3); }

private:
    int8_t   _dout, _clk;
    uint8_t  _gain;
    int32_t  _tare       = 0;
    int32_t  _lastRaw    = 0;
    float    _calibFactor = 1.0f;

    int32_t _readRaw() {
        uint32_t data = 0;
        /* Read 24 bits MSB first */
        noInterrupts();
        for (uint8_t i = 0; i < 24; i++) {
            digitalWrite(_clk, HIGH);
            delayMicroseconds(1);
            data = (data << 1) | digitalRead(_dout);
            digitalWrite(_clk, LOW);
            delayMicroseconds(1);
        }
        /* Extra pulses set gain for next reading */
        for (uint8_t i = 0; i < _gain; i++) {
            digitalWrite(_clk, HIGH);
            delayMicroseconds(1);
            digitalWrite(_clk, LOW);
            delayMicroseconds(1);
        }
        interrupts();

        /* Sign-extend 24-bit two's complement to int32_t */
        if (data & 0x800000) data |= 0xFF000000;
        return (int32_t)data;
    }
};
