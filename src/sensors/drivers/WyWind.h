/*
 * drivers/WyWind.h — Generic Wind Speed + Direction Sensors
 * ==========================================================
 * Compatible with:
 *   Speed: cup anemometers with reed switch or hall-effect pulse output
 *          (common 0–5V pulse types, Davis, Misol, generic weather station)
 *   Direction: resistor-ladder wind vanes (8 or 16 position, 0–5V analog)
 *
 * Bundled driver — no external library needed.
 * Two separate driver classes:
 *   WyWindSpeed     — pulse counting, km/h or m/s output
 *   WyWindDirection — analog resistor ladder, compass bearing output
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIND SPEED SENSOR
 * ═══════════════════════════════════════════════════════════════════
 * Most cup anemometers output one or two pulses per revolution via a
 * reed switch or hall-effect sensor.
 *
 * Speed = (pulse_count / pulses_per_rev) × circumference / time
 * OR (simpler): speed = pulse_count × calibration_factor / time_seconds
 *
 * Common calibration factors (km/h per pulse per second):
 *   Davis anemometers: 2.4 km/h per Hz (1 pulse/rev, 0.5m arm radius)
 *   Generic cheap anemometers: 2.4 km/h per Hz (same standard)
 *   Some 2-pulse/rev types: 1.2 km/h per Hz
 *   Check your datasheet — the factor varies by cup arm radius
 *
 * ⚠️ VOLTAGE: Reed switch sensors output 5V pulses (open collector pulled
 *    to 5V via resistor). ESP32 GPIO is 3.3V tolerant INPUT only — do NOT
 *    connect 5V signal directly. Use a voltage divider or level shifter.
 *    10kΩ + 10kΩ divider (ratio 0.5): 5V → 2.5V ✓
 *    Or: use a 3.3V pull-up to ESP32 GPIO, let reed switch pull to GND.
 *    Most reed switch anemometers are open-collector — just pull up to 3.3V.
 *
 * ⚠️ DEBOUNCE: Reed switches bounce. Without debounce, one physical close
 *    can register 5–20 pulses. The driver implements 5ms debounce via
 *    interrupt timing. If your anemometer spins fast (>1000 RPM unlikely
 *    but possible in storms), reduce debounce with setDebounceMs().
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIND DIRECTION SENSOR
 * ═══════════════════════════════════════════════════════════════════
 * Wind vanes use a resistor ladder — different resistors for each of
 * 8 or 16 positions. Combined with a fixed pull-up resistor, each
 * position produces a distinct voltage.
 *
 * Output: analog voltage 0–5V (or 0–3.3V if 3.3V supplied)
 * Positions: 8 cardinal (N/NE/E/SE/S/SW/W/NW) or 16 intercardinal
 *
 * ⚠️ VOLTAGE: 5V analog output from a 5V-supplied vane → same problem.
 *    Use a voltage divider or power the vane from 3.3V.
 *    Most vanes work fine at 3.3V — the resistor ratios are unchanged,
 *    just scaled down. Recommended: power vane from 3.3V.
 *
 * ⚠️ PULL-UP RESISTOR: The vane has internal resistors — you need an
 *    external pull-up (typically 4.7kΩ–10kΩ) from the signal wire to VCC.
 *    Without it, output is floating. Most modules include this.
 *    The pull-up value changes the voltage for each direction position —
 *    the LUT in the driver assumes 10kΩ pull-up at 3.3V supply.
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING — SPEED SENSOR (reed switch, open collector)
 * ═══════════════════════════════════════════════════════════════════
 *   Sensor wire 1 (pulse) → ESP32 GPIO (interrupt capable)
 *   Sensor wire 2 (GND)   → GND
 *   Add 10kΩ pull-up from GPIO to 3.3V (most reed switches are open-drain)
 *
 *   If sensor has internal 5V pull-up (active 5V output):
 *     10kΩ from sensor output → GPIO
 *     10kΩ from GPIO → GND   (voltage divider → 2.5V max)
 *
 * WIRING — DIRECTION SENSOR (analog resistor ladder)
 * ═══════════════════════════════════════════════════════════════════
 *   Sensor VCC  → 3.3V
 *   Sensor GND  → GND
 *   Sensor SIG  → ADC1 pin (GPIO32–39) + 10kΩ pull-up to 3.3V if not built-in
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   // Speed sensor:
 *   auto* spd = sensors.addGPIO<WyWindSpeed>("windspeed", PULSE_PIN);
 *   spd->setCalibration(2.4f);     // km/h per Hz (Davis standard)
 *   spd->setAverageSeconds(3);     // 3-second average (WMO standard: 10s)
 *   sensors.begin();
 *
 *   // Direction sensor:
 *   auto* dir = sensors.addGPIO<WyWindDirection>("winddir", ADC_PIN);
 *   dir->setSupplyVoltage(3.3f);   // 3.3V supply
 *   sensors.begin();
 *
 *   WySensorData spd_d = sensors.read("windspeed");
 *   WySensorData dir_d = sensors.read("winddir");
 *   // spd_d.raw = km/h, dir_d.raw = degrees (0=N, 90=E, 180=S, 270=W)
 *   // dir_d.rawInt = compass point index (0–15)
 *   // dir_d.voltage = analog voltage read
 *
 * ═══════════════════════════════════════════════════════════════════
 * GUST DETECTION
 * ═══════════════════════════════════════════════════════════════════
 * WMO standard: wind speed = 10-minute average.
 * Gust = highest 3-second average within 10 minutes.
 * The driver tracks peak speed since last resetGust() call.
 */

#pragma once
#include "../WySensors.h"

/* ══════════════════════════════════════════════════════════════════
 * WyWindSpeed — pulse-counting anemometer
 * ══════════════════════════════════════════════════════════════════ */

/* Global ISR state — one instance only (ESP32 interrupt constraint) */
static volatile uint32_t _wyWindPulseCount  = 0;
static volatile uint32_t _wyWindLastPulseMs = 0;
static uint16_t          _wyWindDebounceMs  = 5;

static void IRAM_ATTR _wyWindISR() {
    uint32_t now = millis();
    if ((now - _wyWindLastPulseMs) >= _wyWindDebounceMs) {
        _wyWindPulseCount++;
        _wyWindLastPulseMs = now;
    }
}

class WyWindSpeed : public WySensorBase {
public:
    WyWindSpeed(WyGPIOPins pins) : _pin(pins.pin) {}

    const char* driverName() override { return "WindSpeed"; }

    /* km/h per Hz (pulses per second). Davis/generic standard = 2.4
     * To use m/s: call setCalibration(2.4f / 3.6f) then output is m/s */
    void setCalibration(float kmhPerHz) { _kmhPerHz = kmhPerHz; }

    /* Averaging window in seconds. WMO standard: 600s (10 min).
     * For display: 3s. For gust detection: 3s. */
    void setAverageSeconds(uint16_t s) { _avgSec = s ? s : 1; }

    /* Debounce interval for reed switch bounce suppression (ms, default 5) */
    void setDebounceMs(uint16_t ms)    { _wyWindDebounceMs = ms; }

    bool begin() override {
        if (_pin < 0) { Serial.println("[WindSpeed] pin required"); return false; }
        pinMode(_pin, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(_pin), _wyWindISR, FALLING);
        _wyWindPulseCount  = 0;
        _lastCount         = 0;
        _lastMs            = millis();
        _windowStart       = millis();
        Serial.printf("[WindSpeed] online — pin:%d cal:%.2f km/h/Hz avg:%ds\n",
            _pin, _kmhPerHz, _avgSec);
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        uint32_t now     = millis();
        uint32_t elapsed = now - _lastMs;
        if (elapsed == 0) { d.ok = true; d.raw = _lastKmh; return d; }

        /* Snapshot pulse counter atomically */
        noInterrupts();
        uint32_t count = _wyWindPulseCount;
        interrupts();

        uint32_t delta  = count - _lastCount;
        float    hz     = (float)delta / (elapsed / 1000.0f);
        float    kmh    = hz * _kmhPerHz;

        /* Accumulate for averaging window */
        _accumKmh   += kmh;
        _accumCount++;

        /* Update gust */
        if (kmh > _gustKmh) _gustKmh = kmh;

        _lastCount = count;
        _lastMs    = now;
        _lastKmh   = kmh;

        /* Return windowed average if window elapsed, else instant */
        float outKmh = kmh;
        if ((now - _windowStart) >= (uint32_t)_avgSec * 1000UL) {
            outKmh      = (_accumCount > 0) ? _accumKmh / _accumCount : 0;
            _accumKmh   = 0;
            _accumCount = 0;
            _windowStart = now;
        }

        d.raw     = outKmh;           /* km/h averaged */
        d.rawInt  = (uint32_t)delta;  /* pulse count since last read */
        d.voltage = _gustKmh;         /* gust speed since last resetGust() */
        d.ok      = true;
        return d;
    }

    /* Reset gust tracker */
    void resetGust() { _gustKmh = 0.0f; }

    /* Current gust speed */
    float gustKmh() { return _gustKmh; }

    /* Total pulse count (lifetime) */
    uint32_t totalPulses() { return _wyWindPulseCount; }

    /* Instant speed without averaging */
    float instantKmh() { return _lastKmh; }

private:
    int8_t   _pin         = -1;
    float    _kmhPerHz    = 2.4f;
    uint16_t _avgSec      = 3;
    uint32_t _lastCount   = 0;
    uint32_t _lastMs      = 0;
    uint32_t _windowStart = 0;
    float    _lastKmh     = 0.0f;
    float    _gustKmh     = 0.0f;
    float    _accumKmh    = 0.0f;
    uint32_t _accumCount  = 0;
};


/* ══════════════════════════════════════════════════════════════════
 * WyWindDirection — resistor-ladder wind vane (analog)
 * ══════════════════════════════════════════════════════════════════ */

/* Resistor values for each direction in a typical 8-position vane.
 * Internal resistors (kΩ), combined with external 10kΩ pull-up to VCC.
 * Voltage ratio = R_internal / (R_internal + R_pullup)
 * These are for the common Misol/generic vane — your unit may differ slightly.
 *
 * Direction → internal R (kΩ) → V ratio (10kΩ pull-up)
 * N   (0°)   33.0 kΩ → 0.767
 * NE  (45°)  6.57 kΩ → 0.396
 * E   (90°)  8.2  kΩ → 0.450
 * SE  (135°) 0.891kΩ → 0.082
 * S   (180°) 1.0  kΩ → 0.091
 * SW  (225°) 0.688kΩ → 0.064
 * W   (270°) 120  kΩ → 0.923
 * NW  (315°) 42.12kΩ → 0.808
 *
 * Voltage ratios sorted ascending for lookup:
 */
struct WyVaneEntry {
    float    ratio;   /* V/VCC */
    uint16_t degrees; /* compass bearing */
    const char* label;
};

/* 8-point vane LUT — sorted ascending by voltage ratio */
static const WyVaneEntry WY_VANE_8PT[] = {
    {0.064f,   225, "SW"},
    {0.082f,   135, "SE"},
    {0.091f,   180, "S" },
    {0.396f,    45, "NE"},
    {0.450f,    90, "E" },
    {0.767f,     0, "N" },
    {0.808f,   315, "NW"},
    {0.923f,   270, "W" },
};

/* 16-point vane LUT (common on better weather stations) — sorted ascending */
static const WyVaneEntry WY_VANE_16PT[] = {
    {0.064f,  225, "SW" },
    {0.074f,  247, "WSW"},
    {0.082f,  135, "SE" },
    {0.091f,  180, "S"  },
    {0.127f,  157, "SSE"},
    {0.184f,  202, "SSW"},
    {0.266f,   22, "NNE"},
    {0.315f,   67, "ENE"},
    {0.396f,   45, "NE" },
    {0.450f,   90, "E"  },
    {0.512f,  112, "ESE"},
    {0.617f,  337, "NNW"},
    {0.767f,    0, "N"  },
    {0.808f,  315, "NW" },
    {0.857f,  292, "WNW"},
    {0.923f,  270, "W"  },
};

#define WY_VANE_TOLERANCE  0.05f   /* ±5% of VCC voltage matching window */

class WyWindDirection : public WySensorBase {
public:
    WyWindDirection(WyGPIOPins pins) : _aoPin(pins.pin) {}

    const char* driverName() override { return "WindDirection"; }

    /* Supply voltage to sensor (default 3.3V) — must match actual wiring */
    void setSupplyVoltage(float vcc)    { _vcc = vcc; }

    /* Voltage divider ratio if sensor powered from 5V (default 1.0 = none) */
    void setDividerRatio(float ratio)   { _divRatio = (ratio > 0) ? ratio : 1.0f; }

    /* Use 16-point LUT (default: 8-point) */
    void use16Point()                   { _lut = WY_VANE_16PT; _lutSize = 16; }

    /* Custom LUT — sorted ascending by ratio */
    void setLUT(const WyVaneEntry* lut, uint8_t size) { _lut = lut; _lutSize = size; }

    /* ADC averaging samples */
    void setSamples(uint8_t n)          { _samples = n ? n : 1; }

    /* North offset — add degrees if vane N arrow isn't pointing true north */
    void setNorthOffset(int16_t deg)    { _northOffset = deg; }

    bool begin() override {
        if (_aoPin < 0) { Serial.println("[WindDir] analog pin required"); return false; }
        WySensorData d = read();
        Serial.printf("[WindDir] online — %.0f° (%s)  %.3fV\n",
            d.raw, _lut[d.rawInt].label, d.voltage);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        /* Average ADC */
        uint32_t sum = 0;
        for (uint8_t i = 0; i < _samples; i++) {
            sum += analogRead(_aoPin);
            delay(2);
        }
        uint16_t raw = (uint16_t)(sum / _samples);

        /* Convert to voltage → ratio */
        float adcV  = (raw / 4095.0f) * 3.3f;
        float sensV = adcV / _divRatio;
        float ratio = sensV / _vcc;

        /* Find closest LUT entry */
        uint8_t best     = 0;
        float   bestDist = fabsf(ratio - _lut[0].ratio);
        for (uint8_t i = 1; i < _lutSize; i++) {
            float dist = fabsf(ratio - _lut[i].ratio);
            if (dist < bestDist) { bestDist = dist; best = i; }
        }

        /* Apply north offset + wrap */
        int16_t deg = (int16_t)_lut[best].degrees + _northOffset;
        while (deg < 0)   deg += 360;
        while (deg >= 360) deg -= 360;

        d.rawInt  = best;          /* LUT index */
        d.voltage = sensV;         /* actual sensor voltage */
        d.raw     = (float)deg;    /* compass bearing 0–359° */
        d.ok      = (bestDist < WY_VANE_TOLERANCE);  /* false if no match */
        if (!d.ok) d.error = "no match — check pull-up and supply voltage";
        return d;
    }

    /* Compass label for last reading */
    const char* compassLabel() {
        WySensorData d = read();
        return d.ok ? _lut[d.rawInt].label : "---";
    }

    /* Bearing in degrees */
    float bearingDeg() { return read().raw; }

    /* Circular average of N readings — smooths rapid vane flutter */
    float averagedBearing(uint8_t n = 5) {
        float sinSum = 0, cosSum = 0;
        for (uint8_t i = 0; i < n; i++) {
            float rad = read().raw * DEG_TO_RAD;
            sinSum += sinf(rad);
            cosSum += cosf(rad);
            delay(50);
        }
        float avg = atan2f(sinSum / n, cosSum / n) * RAD_TO_DEG;
        if (avg < 0) avg += 360.0f;
        return avg;
    }

private:
    int8_t   _aoPin      = -1;
    float    _vcc        = 3.3f;
    float    _divRatio   = 1.0f;
    uint8_t  _samples    = 8;
    int16_t  _northOffset = 0;
    const WyVaneEntry* _lut    = WY_VANE_8PT;
    uint8_t            _lutSize = 8;
};
