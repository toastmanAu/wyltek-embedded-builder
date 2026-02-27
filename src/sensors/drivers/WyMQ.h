/*
 * drivers/WyMQ.h — MQ-series metal oxide gas sensors (Analog ADC)
 * =================================================================
 * Covers: MQ-2, MQ-3, MQ-4, MQ-5, MQ-6, MQ-7, MQ-8, MQ-9, MQ-135, MQ-136, MQ-137
 *
 * MQ sensors are MOX (metal oxide) resistive gas sensors.
 * They output an analog voltage proportional to gas concentration.
 * Each sensor type has a different sensitivity curve and target gas.
 *
 * Physical operation:
 *   - Heater (H) pins: require 5V heater supply (some 3.3V tolerant — check module)
 *   - Analog out (AOUT): 0–5V (or 0–3.3V on 3.3V modules) — read with ADC
 *   - Digital out (DOUT): threshold comparator output — HIGH/LOW only
 *   - Rs = sensor resistance = varies with gas concentration
 *   - R0 = sensor resistance in clean air (must be calibrated)
 *   - ppm = a * (Rs/R0)^b  where a,b are curve constants from datasheet
 *
 * ESP32 ADC note:
 *   - ESP32 ADC is 12-bit (0–4095) at 3.3V reference
 *   - Accuracy is poor at extremes (<100 and >3900) — use 10-bit mode if needed
 *   - Use analogReadMilliVolts() (Arduino ESP32 SDK) for better accuracy
 *   - 5V sensor AOUT → voltage divider needed if ESP32 is 3.3V-only GPIO
 *
 * Registered via WySensors::addGPIO<WyMQ2>("name", AOUT_PIN)
 * or: auto* mq = sensors.addGPIO<WyMQ135>("air_quality", A0_PIN);
 *
 * Calibration (required for ppm accuracy):
 *   1. In clean air, read raw ADC for 1 minute (sensor must be warm — 20+ min)
 *   2. Call sensor->calibrateR0() — stores average Rs as R0 baseline
 *   3. Or manually set: sensor->setR0(known_R0_value)
 *
 * Output:
 *   d.co2     = ppm of primary target gas (CO2, CH4, CO, etc.)
 *   d.raw     = raw ADC value (0–4095)
 *   d.voltage = analog voltage in mV
 */

#pragma once
#include "../WySensors.h"

/* Load resistance on breakout board (kΩ) — typically 1kΩ or 10kΩ */
#ifndef WY_MQ_RLOAD_KOHM
#define WY_MQ_RLOAD_KOHM  10.0f
#endif

/* ADC reference voltage (mV) */
#ifndef WY_MQ_VREF_MV
#define WY_MQ_VREF_MV  3300.0f
#endif

/* ADC resolution */
#ifndef WY_MQ_ADC_BITS
#define WY_MQ_ADC_BITS  12
#endif

/* ── Base class ─────────────────────────────────────────────────── */
class WyMQBase : public WySensorBase {
public:
    WyMQBase(WyGPIOPins pins, float a, float b, float r0Default = 10.0f)
        : _pin(pins.pin), _a(a), _b(b), _r0(r0Default) {}

    bool begin() override {
        pinMode(_pin, INPUT);
        analogReadResolution(WY_MQ_ADC_BITS);
        delay(20000);  /* MOX sensors need 20 seconds preheat minimum */
        /* NOTE: For best accuracy, preheat 24 hours before first calibration */
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        int32_t raw = analogRead(_pin);
        float vMV   = (raw / (float)((1 << WY_MQ_ADC_BITS) - 1)) * WY_MQ_VREF_MV;

        /* Sensor resistance: Rs = (Vref - Vout) / Vout * Rload */
        float rs = (WY_MQ_VREF_MV - vMV) / vMV * WY_MQ_RLOAD_KOHM;
        if (rs <= 0) { d.error = "check wiring (heater on?)"; return d; }

        float ratio = rs / _r0;
        float ppm   = _a * powf(ratio, _b);
        if (ppm < 0) ppm = 0;

        d.co2    = ppm;           /* primary gas ppm (field reused — see subclass) */
        d.raw    = (float)raw;
        d.voltage = vMV;
        d.ok     = true;
        _lastRs  = rs;
        return d;
    }

    /* Calibrate R0 in clean air — run for ~1 min with warm sensor */
    void calibrateR0(uint16_t samples = 50) {
        Serial.printf("[%s] calibrating R0 in clean air...\n", driverName());
        float sum = 0;
        for (uint16_t i = 0; i < samples; i++) {
            int32_t raw = analogRead(_pin);
            float vMV   = (raw / (float)((1 << WY_MQ_ADC_BITS) - 1)) * WY_MQ_VREF_MV;
            float rs    = (WY_MQ_VREF_MV - vMV) / vMV * WY_MQ_RLOAD_KOHM;
            sum += rs;
            delay(500);
        }
        _r0 = sum / samples;
        Serial.printf("[%s] R0 = %.2f kΩ\n", driverName(), _r0);
    }

    void setR0(float r0) { _r0 = r0; }
    float getR0()  { return _r0; }
    float getLastRs() { return _lastRs; }
    float getRsR0()   { return _r0 > 0 ? _lastRs / _r0 : 0; }

    /* Skip the 20s preheat in begin() — use when sensor is already warm */
    void skipPreheat() { _skipPreheat = true; }

    bool begin(bool skipPreheat) {
        _skipPreheat = skipPreheat;
        return begin();
    }

protected:
    int8_t _pin;
    float  _a, _b;       /* sensitivity curve constants: ppm = a * (Rs/R0)^b */
    float  _r0;          /* calibrated clean-air resistance (kΩ) */
    float  _lastRs = 0;
    bool   _skipPreheat = false;
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-2 — LPG, propane, hydrogen, smoke, methane
 * Typical R0: 9.83 kΩ (in clean air)
 * Target: LPG (a=574.25, b=-2.222)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ2 : public WyMQBase {
public:
    WyMQ2(WyGPIOPins pins) : WyMQBase(pins, 574.25f, -2.222f, 9.83f) {}
    const char* driverName() override { return "MQ-2"; }
    /* d.co2 field = LPG ppm */
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-3 — Alcohol / ethanol vapour
 * Typical R0: 60 kΩ (in clean air, high sensitivity to humidity)
 * Target: Alcohol (a=0.3934, b=-1.504)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ3 : public WyMQBase {
public:
    WyMQ3(WyGPIOPins pins) : WyMQBase(pins, 0.3934f, -1.504f, 60.0f) {}
    const char* driverName() override { return "MQ-3"; }
    /* d.co2 field = ethanol mg/L */
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-4 — Methane (natural gas / CNG)
 * Typical R0: 4.4 kΩ
 * Target: CH4 (a=1012.7, b=-2.786)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ4 : public WyMQBase {
public:
    WyMQ4(WyGPIOPins pins) : WyMQBase(pins, 1012.7f, -2.786f, 4.4f) {}
    const char* driverName() override { return "MQ-4"; }
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-5 — LPG, natural gas, coal gas, H2
 * Typical R0: 6.5 kΩ
 * Target: LPG (a=503.35, b=-3.495)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ5 : public WyMQBase {
public:
    WyMQ5(WyGPIOPins pins) : WyMQBase(pins, 503.35f, -3.495f, 6.5f) {}
    const char* driverName() override { return "MQ-5"; }
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-6 — LPG, butane
 * Typical R0: 10 kΩ
 * Target: LPG (a=1009.2, b=-2.35)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ6 : public WyMQBase {
public:
    WyMQ6(WyGPIOPins pins) : WyMQBase(pins, 1009.2f, -2.35f, 10.0f) {}
    const char* driverName() override { return "MQ-6"; }
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-7 — Carbon monoxide (CO)
 * Typical R0: 27.5 kΩ
 * Target: CO (a=99.042, b=-1.518)
 * Note: MQ-7 needs a specific heating cycle:
 *   60s at 5V heater, 90s at 1.4V heater — for best accuracy
 *   Simplified: run at fixed 5V (less accurate but simpler)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ7 : public WyMQBase {
public:
    WyMQ7(WyGPIOPins pins) : WyMQBase(pins, 99.042f, -1.518f, 27.5f) {}
    const char* driverName() override { return "MQ-7"; }
    /* d.co2 field = CO ppm */
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-8 — Hydrogen (H2)
 * Typical R0: 1000 kΩ (very high in clean air)
 * Target: H2 (a=976.97, b=-0.688)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ8 : public WyMQBase {
public:
    WyMQ8(WyGPIOPins pins) : WyMQBase(pins, 976.97f, -0.688f, 1000.0f) {}
    const char* driverName() override { return "MQ-8"; }
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-9 — Carbon monoxide (CO) + flammable gases (LPG, CH4)
 * Typical R0: 9.6 kΩ
 * Target: CO (a=1000.5, b=-2.186)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ9 : public WyMQBase {
public:
    WyMQ9(WyGPIOPins pins) : WyMQBase(pins, 1000.5f, -2.186f, 9.6f) {}
    const char* driverName() override { return "MQ-9"; }
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-135 — Air quality: CO2, NH3, NOx, alcohol, benzene, smoke
 * Most popular for general air quality monitoring
 * Typical R0: 76.63 kΩ (varies a LOT — calibration critical)
 * Target: CO2 (a=110.47, b=-2.862) — approximate
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ135 : public WyMQBase {
public:
    WyMQ135(WyGPIOPins pins) : WyMQBase(pins, 110.47f, -2.862f, 76.63f) {}
    const char* driverName() override { return "MQ-135"; }
    /* d.co2 = approximate CO2 ppm (combine with actual CO2 sensor for calibration) */
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-136 — Hydrogen sulphide (H2S)
 * Typical R0: 3.5 kΩ
 * Target: H2S (a=36.737, b=-3.536)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ136 : public WyMQBase {
public:
    WyMQ136(WyGPIOPins pins) : WyMQBase(pins, 36.737f, -3.536f, 3.5f) {}
    const char* driverName() override { return "MQ-136"; }
};

/* ══════════════════════════════════════════════════════════════════
 * MQ-137 — Ammonia (NH3)
 * Typical R0: 35 kΩ
 * Target: NH3 (a=102.63, b=-2.773)
 * ══════════════════════════════════════════════════════════════════ */
class WyMQ137 : public WyMQBase {
public:
    WyMQ137(WyGPIOPins pins) : WyMQBase(pins, 102.63f, -2.773f, 35.0f) {}
    const char* driverName() override { return "MQ-137"; }
};
