#pragma once
/**
 * WyMAX9814.h — MAX9814 Auto-Gain Microphone Amplifier
 * ======================================================
 * Electret mic with automatic gain control (AGC).
 * Analog output — read via ADC.
 * No digital control needed for basic use (gain set by AR pin).
 *
 * Wiring:
 *   OUT  → ADC pin (read analog)
 *   GND  → GND
 *   VDD  → 3.3V
 *   AR   → leave floating (40dB gain) or:
 *          GND = 40dB, VDD = 50dB, float = 60dB
 *   GAIN → not connected for auto mode
 *
 * Usage:
 *   WyMAX9814 mic(ADC_PIN);
 *   mic.begin();
 *   int level = mic.readLevel();     // 0–4095 (12-bit ADC)
 *   float db  = mic.readDB();        // rough dB estimate
 *   bool loud = mic.isLoud(2000);    // threshold detection
 */
class WyMAX9814 {
public:
    WyMAX9814(uint8_t pin, uint8_t samples = 32)
        : _pin(pin), _samples(samples) {}

    void begin() {
        pinMode(_pin, INPUT);
    }

    // Average of N samples (reduces noise)
    int readLevel() {
        long sum = 0;
        for (uint8_t i = 0; i < _samples; i++) sum += analogRead(_pin);
        return (int)(sum / _samples);
    }

    // Peak-to-peak amplitude over sample window (good for VU meter)
    int readPeakPeak(uint32_t windowMs = 50) {
        int hi = 0, lo = 4095;
        uint32_t end = millis() + windowMs;
        while (millis() < end) {
            int v = analogRead(_pin);
            if (v > hi) hi = v;
            if (v < lo) lo = v;
        }
        return hi - lo;
    }

    // Threshold detection (simple clap/knock detector)
    bool isLoud(int threshold = 2000) {
        return readLevel() > threshold;
    }

    // Rough dB estimate (not calibrated — relative only)
    float readDB() {
        int v = readLevel();
        if (v <= 0) return 0.0f;
        return 20.0f * log10f((float)v / 4095.0f) + 60.0f;
    }

private:
    uint8_t  _pin;
    uint8_t  _samples;
};
