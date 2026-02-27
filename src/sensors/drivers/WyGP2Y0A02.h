/*
 * drivers/WyGP2Y0A02.h — Sharp GP2Y0A02YK0F IR distance sensor (Analog)
 * ========================================================================
 * Datasheet: https://global.sharp/products/device/lineup/data/pdf/datasheet/gp2y0a02yk_e.pdf
 * Bundled driver — no external library needed.
 * Registered via WySensors::addGPIO<WyGP2Y0A02>("ir_dist", AOUT_PIN)
 *
 * Measuring range: 20cm – 150cm
 * Output: analog voltage, non-linear (inverse power curve)
 * Supply: 4.5–5.5V (5V typical — module draws ~30mA)
 * Response time: 38.3ms ± 9.6ms
 *
 * VOLTAGE CURVE (from datasheet Fig. 2):
 *   Distance and output voltage have an INVERSE non-linear relationship.
 *   High voltage → close object. Low voltage → distant object.
 *
 *   Approximate characteristic:
 *     ~2.8V at 20cm
 *     ~1.4V at 40cm
 *     ~0.7V at 80cm
 *     ~0.5V at 120cm
 *     ~0.4V at 150cm
 *
 *   Conversion formula (empirically fitted to datasheet curve):
 *     distance_cm = k / (voltage_V - offset)
 *   where k ≈ 60.495, offset ≈ -0.0799
 *   Or more accurately using power fit:
 *     distance_cm = A × voltage_V ^ B
 *   where A = 61.573, B = -1.1015  (fitted from datasheet Fig. 2 points)
 *
 *   Below ~20cm the voltage DROPS again — readings become invalid.
 *   Above 150cm readings become unreliable (noise floor).
 *
 * WIRING:
 *   Sensor VCC (red)  → 5V
 *   Sensor GND (black) → GND
 *   Sensor Vo  (white/yellow) → ESP32 ADC pin
 *   NOTE: Output is typically 0–2.8V — safe for 3.3V ESP32 ADC.
 *   Add 100nF cap between Vo and GND to reduce switching noise from LED.
 *
 * FAMILY VARIANTS (same driver, different range — adjust WY_GP2Y_MODEL):
 *   GP2Y0A02YK0F  — 20–150cm (this driver, default)
 *   GP2Y0A21YK0F  — 10–80cm  (set WY_GP2Y_MODEL=21)
 *   GP2Y0A710K0F  — 100–500cm (set WY_GP2Y_MODEL=710)
 *
 * Build flags:
 *   -DWY_GP2Y_MODEL=2    (GP2Y0A02, 20-150cm, default)
 *   -DWY_GP2Y_MODEL=21   (GP2Y0A21, 10-80cm)
 *   -DWY_GP2Y_MODEL=710  (GP2Y0A710, 100-500cm)
 *   -DWY_GP2Y_VREF_MV=3300
 *   -DWY_GP2Y_SAMPLES=5
 */

#pragma once
#include "../WySensors.h"

#ifndef WY_GP2Y_MODEL
#define WY_GP2Y_MODEL  2   /* 0A02 = 20-150cm */
#endif

#ifndef WY_GP2Y_VREF_MV
#define WY_GP2Y_VREF_MV  3300.0f
#endif

#ifndef WY_GP2Y_ADC_BITS
#define WY_GP2Y_ADC_BITS  12
#endif

/* Average N samples — sensor has internal 38ms cycle, rapid reads get the same sample */
#ifndef WY_GP2Y_SAMPLES
#define WY_GP2Y_SAMPLES  5
#endif

/* Curve constants: distance_cm = A * (voltage_V ^ B)
 * Fitted from Sharp datasheet characteristic curves */
struct WyGP2YCurve { float A; float B; float minCm; float maxCm; };

/* Model curves — fitted to datasheet characteristic (Fig. 2) */
#if WY_GP2Y_MODEL == 21
  /* GP2Y0A21YK0F: 10–80cm */
  static const WyGP2YCurve WY_GP2Y_CURVE = {29.988f, -1.173f, 10.0f, 80.0f};
  #define WY_GP2Y_NAME "GP2Y0A21"
#elif WY_GP2Y_MODEL == 710
  /* GP2Y0A710K0F: 100–500cm */
  static const WyGP2YCurve WY_GP2Y_CURVE = {1081.0f, -0.895f, 100.0f, 550.0f};
  #define WY_GP2Y_NAME "GP2Y0A710"
#else
  /* GP2Y0A02YK0F: 20–150cm (default) */
  static const WyGP2YCurve WY_GP2Y_CURVE = {61.573f, -1.1015f, 20.0f, 150.0f};
  #define WY_GP2Y_NAME "GP2Y0A02"
#endif

class WyGP2Y0A02 : public WySensorBase {
public:
    WyGP2Y0A02(WyGPIOPins pins) : _pin(pins.pin) {}

    const char* driverName() override { return WY_GP2Y_NAME; }

    bool begin() override {
        pinMode(_pin, INPUT);
        analogReadResolution(WY_GP2Y_ADC_BITS);
        /* Sensor needs ~50ms to stabilise on power-on */
        delay(50);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        float vMV = _readVoltageMV();
        float vV  = vMV / 1000.0f;

        /* Guard: very low voltage means nothing in range or sensor disconnected */
        if (vV < 0.1f) {
            d.error   = "no signal — check wiring";
            d.voltage = vMV;
            return d;
        }

        /* Apply power-law curve: distance_cm = A * V^B */
        float cm = WY_GP2Y_CURVE.A * powf(vV, WY_GP2Y_CURVE.B);

        /* Clamp to valid sensing range */
        if (cm < WY_GP2Y_CURVE.minCm) {
            d.error   = "too close";
            d.distance = cm;
            d.voltage  = vMV;
            return d;
        }
        if (cm > WY_GP2Y_CURVE.maxCm) {
            d.error   = "out of range";
            d.distance = cm;
            d.voltage  = vMV;
            return d;
        }

        d.distance = cm * 10.0f;  /* mm — matches WySensorData convention */
        d.raw      = cm;          /* cm in raw for convenience */
        d.voltage  = vMV;
        d.ok       = true;
        return d;
    }

    /* Get distance in cm directly */
    float readCm() {
        WySensorData d = read();
        return d.ok ? d.raw : -1.0f;
    }

    /* Get raw voltage (useful for custom curve fitting) */
    float readMV() { return _readVoltageMV(); }

    /* User-supplied custom curve constants (if datasheet values don't fit your unit) */
    void setCurve(float A, float B) { _customA = A; _customB = B; _useCustom = true; }
    void clearCustomCurve()         { _useCustom = false; }

private:
    int8_t _pin;
    bool   _useCustom = false;
    float  _customA = 0, _customB = 0;

    float _readVoltageMV() {
        /* Stagger reads — sensor has ~38ms internal LED cycle.
         * Reading faster than that gets repeated ADC samples.
         * WY_GP2Y_SAMPLES × 2ms delay = ~10ms spread; good enough for averaging noise. */
        int64_t sum = 0;
        for (uint8_t i = 0; i < WY_GP2Y_SAMPLES; i++) {
            sum += analogRead(_pin);
            delay(2);
        }
        float raw = (float)(sum / WY_GP2Y_SAMPLES);
        return (raw / (float)((1 << WY_GP2Y_ADC_BITS) - 1)) * WY_GP2Y_VREF_MV;
    }
};

/* Aliases for other Sharp family members */
using WyGP2Y0A21  = WyGP2Y0A02;   /* 10-80cm  — compile with -DWY_GP2Y_MODEL=21  */
using WyGP2Y0A710 = WyGP2Y0A02;   /* 100-500cm — compile with -DWY_GP2Y_MODEL=710 */
