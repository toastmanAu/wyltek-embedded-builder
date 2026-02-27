/*
 * drivers/WyGUVAS12SD.h — GUVA-S12SD UV Light Sensor (Analog)
 * =============================================================
 * Sensor: Genicom GUVA-S12SD — SiC photodiode sensitive to 240–370nm UV
 * Common modules: GY-UVME, GY-8511, bare GUVA-S12SD on breakout board
 *
 * Bundled driver — no external library needed.
 * Registered via WySensors::addGPIO<WyGUVAS12SD>("uv", AOUT_PIN)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT MEASURES
 * ═══════════════════════════════════════════════════════════════════
 * UV-A (315–400nm) and UV-B (280–315nm) radiation.
 * Peak sensitivity ~330nm (UV-A/B border).
 * NOT sensitive to visible light or IR — pure UV photodiode.
 *
 * Outputs:
 *   Analog voltage proportional to UV intensity
 *   UV Index (UVI) — 0–11+ international standard (WHO/WMO)
 *   Irradiance (mW/cm²) — raw radiometric power
 *
 * UV Index scale (WHO):
 *   0–2   Low        — minimal protection needed
 *   3–5   Moderate   — sunscreen recommended
 *   6–7   High       — sunscreen + hat
 *   8–10  Very High  — avoid midday sun
 *   11+   Extreme    — stay indoors if possible
 *
 * ═══════════════════════════════════════════════════════════════════
 * SENSOR CHARACTERISTICS
 * ═══════════════════════════════════════════════════════════════════
 * The GUVA-S12SD outputs a current proportional to UV intensity.
 * The breakout board has a transimpedance amplifier (op-amp) that
 * converts this current to a voltage.
 *
 * Voltage output (typical at 3.3V supply):
 *   Dark / indoors:    ~0.001–0.05V  (near zero, may have slight offset)
 *   Shade outdoors:    ~0.05–0.3V
 *   Overcast sky:      ~0.3–0.7V
 *   Partly cloudy:     ~0.5–1.0V
 *   Full sun (UVI 5):  ~0.9–1.2V
 *   Full sun (UVI 10): ~1.6–2.0V
 *   Extreme (UVI 13+): ~2.2–2.8V
 *
 * V → UV Index conversion (from Genicom datasheet + community calibration):
 *   UV_mW_cm2 = (Vout - Vdark) / sensitivity
 *   UV Index  = UV_mW_cm2 / 0.025
 *
 * Sensitivity: ~0.1 mA/mW·cm² (from datasheet, varies ±30% unit-to-unit)
 * The op-amp feedback resistor on most breakout boards: 1MΩ
 * → voltage sensitivity ≈ 1MΩ × 0.1 mA/mW·cm² = 0.1 V / (mW/cm²)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 *   Module VCC  → 3.3V (or 5V — check your board)
 *   Module GND  → GND
 *   Module OUT  → ESP32 ADC1 pin (GPIO32–39)
 *   (No voltage divider needed for 3.3V supply — output stays within 3.3V)
 *
 * ⚠️ If module is powered from 5V, the output CAN exceed 3.3V at high UVI.
 *    Add 100kΩ+68kΩ voltage divider (ratio 0.405) before ESP32 ADC pin.
 *    Or: power from 3.3V — output rails lower, simpler wiring.
 *
 * ⚠️ ADC1 only (GPIO32–39) — ADC2 corrupted by WiFi on ESP32.
 *
 * ⚠️ IMPORTANT: Point sensor at the sky, NOT at the sun directly.
 *    The sensor measures diffuse sky UV, which is what matters for
 *    UV Index (a measure of UV reaching a horizontal surface).
 *    Direct sun angle changes reading significantly.
 *
 * ═══════════════════════════════════════════════════════════════════
 * DARK CALIBRATION
 * ═══════════════════════════════════════════════════════════════════
 * The op-amp has a small voltage offset even in total darkness (~0–50mV).
 * For accurate low-UV readings, call calibrateDark() while covering the
 * sensor completely (lens cap, finger, dark cloth). This offset is
 * subtracted from all subsequent readings.
 *
 * Without dark calibration, indoor readings may show UVI 0.5–1.0 when
 * the actual value is near zero.
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   auto* uv = sensors.addGPIO<WyGUVAS12SD>("uv", GPIO34);
 *   uv->calibrateDark();    // cover sensor first
 *   sensors.begin();
 *
 *   WySensorData d = sensors.read("uv");
 *   if (d.ok) {
 *       Serial.printf("UV Index: %.1f  (%.3f mW/cm²)  %s\n",
 *           d.raw, d.voltage, uv->uviLabel());
 *   }
 *
 * WySensorData:
 *   d.raw      = UV Index (0.0–16.0+)
 *   d.voltage  = UV irradiance (mW/cm²)
 *   d.rawInt   = raw ADC value (0–4095)
 *   d.light    = analog output voltage (V, dark-corrected)
 *   d.humidity = UVI category: 0=low, 1=moderate, 2=high, 3=very high, 4=extreme
 *   d.ok       = true when valid
 */

#pragma once
#include "../WySensors.h"

/* ADC samples to average */
#ifndef WY_UV_SAMPLES
#define WY_UV_SAMPLES  16
#endif

/* Sensitivity: V per (mW/cm²). Most boards: 1MΩ feedback × ~0.1A/W·cm² ≈ 0.1
 * If your readings seem off, adjust this value. Range: 0.05–0.20 typical. */
#ifndef WY_UV_SENSITIVITY_V_PER_MW
#define WY_UV_SENSITIVITY_V_PER_MW  0.1f
#endif

/* UV Index per mW/cm² (per WHO definition, solar spectrum weighted) */
#define WY_UVI_PER_MW_CM2  (1.0f / 0.025f)   /* 1 UVI = 0.025 mW/cm² */

class WyGUVAS12SD : public WySensorBase {
public:
    WyGUVAS12SD(WyGPIOPins pins) : _aoPin(pins.pin) {}

    const char* driverName() override { return "GUVA-S12SD"; }

    /* Voltage divider ratio if sensor powered from 5V (default 1.0 = no divider) */
    void setDividerRatio(float ratio)      { _divRatio = (ratio > 0) ? ratio : 1.0f; }

    /* Override op-amp sensitivity (V per mW/cm²). Default 0.1V/(mW/cm²). */
    void setSensitivity(float vPerMW)      { _sensitivity = vPerMW; }

    /* Override dark voltage offset (measured in darkness). Default 0.0V. */
    void setDarkVoltage(float v)           { _darkV = v; }

    /* ADC averaging samples */
    void setSamples(uint8_t n)             { _samples = n ? n : 1; }

    /* Measure dark offset — cover sensor completely before calling.
     * Takes 32 samples, stores offset. Call before begin() or after. */
    void calibrateDark() {
        uint32_t sum = 0;
        for (uint8_t i = 0; i < 32; i++) {
            sum += analogRead(_aoPin);
            delay(5);
        }
        float rawV = (sum / 32.0f / 4095.0f) * 3.3f / _divRatio;
        _darkV = rawV;
        Serial.printf("[GUVA-S12SD] dark offset: %.4fV\n", _darkV);
    }

    bool begin() override {
        if (_aoPin < 0) {
            Serial.println("[GUVA-S12SD] analog pin required");
            return false;
        }
        delay(20);  /* sensor warm-up */
        WySensorData d = read();
        Serial.printf("[GUVA-S12SD] online — UVI:%.1f (%.3f mW/cm²)\n",
            d.raw, d.voltage);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        /* Average ADC readings */
        uint32_t sum = 0;
        for (uint8_t i = 0; i < _samples; i++) {
            sum += analogRead(_aoPin);
            delay(2);
        }
        uint16_t raw = (uint16_t)(sum / _samples);

        /* ADC raw → voltage at ADC pin → actual sensor voltage */
        float adcV    = (raw / 4095.0f) * 3.3f;
        float sensorV = adcV / _divRatio;

        /* Subtract dark offset */
        float uvV = sensorV - _darkV;
        if (uvV < 0) uvV = 0;

        /* Convert to irradiance and UV Index */
        float irradiance = uvV / _sensitivity;           /* mW/cm² */
        float uvi        = irradiance * WY_UVI_PER_MW_CM2;
        uvi = max(uvi, 0.0f);

        d.rawInt   = raw;
        d.light    = sensorV;     /* actual sensor voltage */
        d.voltage  = irradiance;  /* mW/cm² */
        d.raw      = uvi;         /* UV Index */
        d.humidity = _uviCategory(uvi);
        d.ok       = true;
        return d;
    }

    /* UVI category label */
    const char* uviLabel() {
        static const char* labels[] = {"Low", "Moderate", "High", "Very High", "Extreme"};
        WySensorData d = read();
        uint8_t cat = (uint8_t)d.humidity;
        return labels[cat < 5 ? cat : 4];
    }

    /* UVI only — lightweight read */
    float readUVI() { return read().raw; }

private:
    int8_t  _aoPin       = -1;
    float   _divRatio    = 1.0f;
    float   _sensitivity = WY_UV_SENSITIVITY_V_PER_MW;
    float   _darkV       = 0.0f;
    uint8_t _samples     = WY_UV_SAMPLES;

    uint8_t _uviCategory(float uvi) {
        if (uvi < 3.0f)  return 0;  /* Low */
        if (uvi < 6.0f)  return 1;  /* Moderate */
        if (uvi < 8.0f)  return 2;  /* High */
        if (uvi < 11.0f) return 3;  /* Very High */
        return 4;                    /* Extreme */
    }
};
