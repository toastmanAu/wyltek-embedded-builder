/*
 * drivers/WyINA219.h — INA219 current/voltage/power monitor (I2C)
 * =================================================================
 * Datasheet: https://www.ti.com/lit/ds/symlink/ina219.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x40–0x4F (A0/A1 pin selectable — see address table below)
 * Registered via WySensors::addI2C<WyINA219>("name", sda, scl, 0x40)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT MEASURES
 * ═══════════════════════════════════════════════════════════════════
 *   Bus voltage   — voltage on the load side, 0–26V (or 0–16V in low range)
 *   Shunt voltage — tiny voltage drop across shunt resistor (±40mV or ±320mV)
 *   Current       — derived from shunt voltage (Ohm's law)
 *   Power         — bus voltage × current (computed on-chip or in driver)
 *
 * ═══════════════════════════════════════════════════════════════════
 * SHUNT RESISTOR
 * ═══════════════════════════════════════════════════════════════════
 * The INA219 measures current indirectly via the voltage drop across
 * a small shunt resistor placed in series with the load.
 *
 * Most breakout boards use 0.1Ω — max 3.2A at ±320mV gain.
 *
 * CHOOSING A SHUNT:
 *   Larger resistor → more voltage drop → better resolution at low current
 *                  → more power loss, lower max current
 *   Smaller resistor → less voltage drop → higher max current
 *                   → less resolution at low current
 *
 * Common configurations:
 *   0.1Ω  (board default) — 3.2A max, 0.1mA resolution
 *   0.01Ω                 — 32A max, 1mA resolution (industrial)
 *   1.0Ω                  — 0.32A max, 0.01mA resolution (micro-power)
 *
 * For solar panel monitoring, battery charging, motor current:
 *   Use 0.01Ω–0.1Ω and set setRange(INA219_RANGE_32A) or similar.
 *
 * ═══════════════════════════════════════════════════════════════════
 * I2C ADDRESS TABLE
 * ═══════════════════════════════════════════════════════════════════
 *   A1=GND, A0=GND → 0x40 (default on most boards)
 *   A1=GND, A0=VS  → 0x41
 *   A1=GND, A0=SDA → 0x42
 *   A1=GND, A0=SCL → 0x43
 *   A1=VS,  A0=GND → 0x44
 *   A1=VS,  A0=VS  → 0x45
 *   Up to 16 INA219s on one I2C bus (0x40–0x4F)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 * The shunt resistor goes IN SERIES with the load (not across it).
 *
 *   Power supply (+) → INA219 V+ → shunt → INA219 V- → load → GND
 *
 *   INA219 SDA → ESP32 SDA
 *   INA219 SCL → ESP32 SCL
 *   INA219 VCC → 3.3V (I2C side power — separate from measured circuit)
 *   INA219 GND → GND (must be common with measured circuit GND)
 *
 * ⚠️ COMMON GROUND is mandatory. The INA219's GND must be at the same
 *    potential as the return side of the circuit being measured.
 *    For high-voltage (>3.3V) systems, only GND and the shunt connect
 *    to the ESP32 circuit — VCC of the monitored device is isolated.
 *
 * ⚠️ Bus voltage limit: 26V max (32V bus range setting) or 16V (16V range).
 *    Shunt differential input: ±320mV (gain /8) or ±40mV (gain /1).
 *    Exceeding either will damage the IC.
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   // Default: 32V bus, ±320mV shunt (0.1Ω board default → 3.2A max)
 *   auto* pwr = sensors.addI2C<WyINA219>("power", SDA, SCL, 0x40);
 *   sensors.begin();
 *
 *   WySensorData d = sensors.read("power");
 *   if (d.ok) {
 *       Serial.printf("V=%.3fV  I=%.3fA  P=%.3fW\n",
 *           d.voltage, d.current, d.weight);
 *   }
 *
 * High-precision low-current (e.g. 100mA max, 1Ω shunt):
 *   auto* pwr = sensors.addI2C<WyINA219>("power", SDA, SCL, 0x40, 1.0f);
 *   pwr->setGain(INA219_GAIN_1);    // ±40mV range — max 40mA with 1Ω
 *   pwr->setMaxCurrent(0.04f);      // 40mA — sets calibration LSB
 *
 * Multiple monitors (battery + solar + load):
 *   auto* bat  = sensors.addI2C<WyINA219>("battery", SDA, SCL, 0x40);
 *   auto* solar= sensors.addI2C<WyINA219>("solar",   SDA, SCL, 0x41);
 *   auto* load = sensors.addI2C<WyINA219>("load",    SDA, SCL, 0x44);
 *
 * WySensorData:
 *   d.voltage  = bus voltage (V)
 *   d.current  = current (A) — negative = reverse flow
 *   d.weight   = power (W)
 *   d.raw      = shunt voltage (mV)
 *   d.ok       = true when reading valid
 *   d.error    = "overflow" if current exceeds shunt range
 */

#pragma once
#include "../WySensors.h"

/* INA219 register addresses */
#define INA219_REG_CONFIG        0x00
#define INA219_REG_SHUNT         0x01  /* shunt voltage, signed, 10µV/LSB */
#define INA219_REG_BUS           0x02  /* bus voltage, 4mV/LSB (bits 15:3) */
#define INA219_REG_POWER         0x03  /* power (uses calibration LSB × 20) */
#define INA219_REG_CURRENT       0x04  /* current (uses calibration LSB) */
#define INA219_REG_CALIBRATION   0x05

/* Config register field values */
/* Bus voltage range [13] */
#define INA219_BUS_16V           0x0000
#define INA219_BUS_32V           0x2000  /* default */

/* Gain (shunt voltage range) [12:11] */
#define INA219_GAIN_1            0x0000  /* ±40mV  — max 40mA/0.1Ω, highest precision */
#define INA219_GAIN_2            0x0800  /* ±80mV  */
#define INA219_GAIN_4            0x1000  /* ±160mV */
#define INA219_GAIN_8            0x1800  /* ±320mV — default, 3.2A/0.1Ω */

/* ADC resolution/averaging [10:7] and [6:3] */
#define INA219_ADC_12BIT         0x0018  /* 12-bit, no averaging, ~532µs */
#define INA219_ADC_12BIT_4AVG    0x0048  /* 12-bit × 4 average, ~2.1ms */
#define INA219_ADC_12BIT_8AVG    0x0058  /* 12-bit × 8 average, ~4.3ms */
#define INA219_ADC_12BIT_16AVG   0x0068  /* 12-bit × 16 average, ~8.5ms */
#define INA219_ADC_12BIT_32AVG   0x0078  /* 12-bit × 32 average, ~17ms */
#define INA219_ADC_12BIT_128AVG  0x00F8  /* 12-bit × 128 average, ~68ms — smoothest */

/* Operating mode [2:0] */
#define INA219_MODE_POWER_DOWN   0x0000
#define INA219_MODE_SHUNT_TRIG   0x0001  /* triggered shunt only */
#define INA219_MODE_BUS_TRIG     0x0002  /* triggered bus only */
#define INA219_MODE_BOTH_TRIG    0x0003  /* triggered shunt + bus */
#define INA219_MODE_ADC_OFF      0x0004
#define INA219_MODE_SHUNT_CONT   0x0005  /* continuous shunt only */
#define INA219_MODE_BUS_CONT     0x0006  /* continuous bus only */
#define INA219_MODE_BOTH_CONT    0x0007  /* continuous shunt + bus (default) */

#ifndef WY_INA219_SHUNT_OHM
#define WY_INA219_SHUNT_OHM  0.1f
#endif

class WyINA219 : public WySensorBase {
public:
    WyINA219(WyI2CPins pins, float shuntOhm = WY_INA219_SHUNT_OHM)
        : _pins(pins), _shuntOhm(shuntOhm) {}

    const char* driverName() override { return "INA219"; }

    /* Shunt voltage gain — sets max current range
     * GAIN_1 = ±40mV:  max 400mA with 0.1Ω, highest resolution
     * GAIN_8 = ±320mV: max 3.2A with 0.1Ω (default) */
    void setGain(uint16_t gain) { _gain = gain; }

    /* Bus voltage range (default 32V for safety) */
    void setBusRange(uint16_t range) { _busRange = range; }

    /* ADC averaging (default 12-bit × 1 sample — fast) */
    void setADCMode(uint16_t adcMode) { _adcMode = adcMode; }

    /* Set maximum expected current — tunes calibration LSB for best resolution
     * Call this if you know your max current (e.g. 0.5f for 500mA systems) */
    void setMaxCurrent(float maxAmps) { _maxCurrent = maxAmps; }

    /* Triggered (one-shot) vs continuous mode */
    void setMode(uint16_t mode) { _mode = mode; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq ? _pins.freq : 400000);

        /* Reset to power-on defaults */
        _writeReg(INA219_REG_CONFIG, 0x8000);
        delay(5);

        /* Build config word */
        uint16_t config = _busRange | _gain |
                          (_adcMode << 3) | _adcMode |  /* same for bus + shunt ADC */
                          _mode;
        _writeReg(INA219_REG_CONFIG, config);

        /* Calibration:
         * current_lsb = maxCurrent / 32768
         * cal = trunc(0.04096 / (current_lsb × shuntOhm))
         * Larger cal = finer current resolution but lower max current */
        _currentLSB = _maxCurrent / 32768.0f;
        uint16_t cal = (uint16_t)(0.04096f / (_currentLSB * _shuntOhm));
        _writeReg(INA219_REG_CALIBRATION, cal);

        delay(10);

        /* Verify comms — read back config */
        uint16_t rb = _readReg(INA219_REG_CONFIG);
        if (rb == 0xFFFF) {
            Serial.println("[INA219] not found — check wiring and I2C address");
            return false;
        }
        Serial.printf("[INA219] ready — shunt:%.3fΩ maxI:%.3fA LSB:%.6fA cal:%u\n",
            _shuntOhm, _maxCurrent, _currentLSB, cal);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        /* Trigger one-shot conversion if in triggered mode */
        if (_mode <= INA219_MODE_BOTH_TRIG && _mode > 0) {
            uint16_t config = _busRange | _gain |
                              (_adcMode << 3) | _adcMode | _mode;
            _writeReg(INA219_REG_CONFIG, config);
            /* Wait for conversion: 12-bit = 532µs × 2 channels + margin */
            delay(2);
        }

        /* Bus voltage — bits 15:3, 4mV/LSB, bit 1 = OVF, bit 0 = CNVR */
        uint16_t busRaw = _readReg(INA219_REG_BUS);
        if (busRaw & 0x0001) {
            d.error = "overflow";
            return d;
        }
        d.voltage = (float)(busRaw >> 3) * 0.004f;   /* V */

        /* Shunt voltage — signed 16-bit, 10µV/LSB */
        int16_t shuntRaw = (int16_t)_readReg(INA219_REG_SHUNT);
        float shuntMV = shuntRaw * 0.01f;             /* mV */
        d.raw = shuntMV;

        /* Current — two methods:
         * 1. Calibration register method (uses INA219 hardware multiplier)
         * 2. Direct Ohm's law (more reliable when cal register is tricky) */
        int16_t currentRaw = (int16_t)_readReg(INA219_REG_CURRENT);
        if (currentRaw != 0) {
            d.current = currentRaw * _currentLSB;    /* A (from cal register) */
        } else {
            /* Fallback: Ohm's law — always valid */
            d.current = (shuntMV / 1000.0f) / _shuntOhm;
        }

        /* Power — read hardware register (power_lsb = current_lsb × 20) */
        uint16_t powerRaw = _readReg(INA219_REG_POWER);
        if (powerRaw != 0) {
            d.weight = powerRaw * (_currentLSB * 20.0f);   /* W */
        } else {
            d.weight = d.voltage * d.current;              /* W fallback */
        }

        d.ok = true;
        return d;
    }

    /* ── Convenience methods ─────────────────────────────────────── */

    float busVoltage()   { return read().voltage; }
    float current()      { return read().current; }
    float power()        { return read().weight; }
    float shuntMV()      { return read().raw; }

    /* Energy accumulation — call regularly, returns Wh since last reset */
    float energyWh() {
        uint32_t now = millis();
        WySensorData d = read();
        if (d.ok) {
            float hrs = (now - _lastMs) / 3600000.0f;
            _energyWh += d.weight * hrs;
        }
        _lastMs = now;
        return _energyWh;
    }

    void resetEnergy() { _energyWh = 0.0f; _lastMs = millis(); }

private:
    WyI2CPins _pins;
    float     _shuntOhm   = WY_INA219_SHUNT_OHM;
    float     _maxCurrent = 3.2f;       /* A — default matches GAIN_8 + 0.1Ω */
    float     _currentLSB = 0.0001f;
    uint16_t  _gain       = INA219_GAIN_8;
    uint16_t  _busRange   = INA219_BUS_32V;
    uint16_t  _adcMode    = INA219_ADC_12BIT;
    uint16_t  _mode       = INA219_MODE_BOTH_CONT;
    float     _energyWh   = 0.0f;
    uint32_t  _lastMs     = 0;

    void _writeReg(uint8_t reg, uint16_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.write((uint8_t)(val >> 8));
        Wire.write((uint8_t)(val & 0xFF));
        Wire.endTransmission();
    }

    uint16_t _readReg(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)2);
        if (Wire.available() < 2) return 0xFFFF;
        return ((uint16_t)Wire.read() << 8) | Wire.read();
    }
};
