/*
 * drivers/WyINA219.h — INA219 current/voltage/power monitor (I2C)
 * =================================================================
 * Datasheet: https://www.ti.com/lit/ds/symlink/ina219.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x40–0x4F (configurable via A0/A1 pins)
 * Registered via WySensors::addI2C<WyINA219>("name", sda, scl, 0x40)
 *
 * Measures:
 *   - Bus voltage (V)   — load side, 0–26V, 4mV resolution
 *   - Shunt voltage (mV) — across shunt resistor (typically 0.1Ω)
 *   - Current (A)       — calculated from shunt voltage / shunt resistance
 *   - Power (W)         — bus voltage × current
 *
 * Shunt resistor:
 *   Most breakout boards use 0.1Ω — change WY_INA219_SHUNT_OHM if different
 *   Max current = max shunt voltage (320mV) / shunt resistance
 *   Default config (0.1Ω): max 3.2A
 */

#pragma once
#include "../WySensors.h"

/* INA219 register addresses */
#define INA219_REG_CONFIG        0x00
#define INA219_REG_SHUNT_VOLTAGE 0x01  /* shunt voltage in 10µV LSB */
#define INA219_REG_BUS_VOLTAGE   0x02  /* bus voltage in 4mV LSB (bits 15:3) */
#define INA219_REG_POWER         0x03  /* power (requires calibration reg set) */
#define INA219_REG_CURRENT       0x04  /* current (requires calibration reg set) */
#define INA219_REG_CALIBRATION   0x05

/* Configuration register bits (default 0x399F = 32V, gain /8, 12-bit, continuous) */
#define INA219_CONFIG_32V_2A     0x399F  /* 32V bus, ±320mV shunt, 12-bit, continuous */
#define INA219_CONFIG_16V_400mA  0x01FF  /* 16V bus, ±40mV shunt (low current, accurate) */

#ifndef WY_INA219_SHUNT_OHM
#define WY_INA219_SHUNT_OHM  0.1f   /* standard breakout board shunt resistance */
#endif

class WyINA219 : public WySensorBase {
public:
    WyINA219(WyI2CPins pins, float shuntOhm = WY_INA219_SHUNT_OHM)
        : _pins(pins), _shuntOhm(shuntOhm) {}

    const char* driverName() override { return "INA219"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);

        /* Reset */
        _writeReg16(INA219_REG_CONFIG, 0x8000);
        delay(5);

        /* Set config: 32V bus range, ±320mV shunt, 12-bit ADC, continuous */
        _writeReg16(INA219_REG_CONFIG, INA219_CONFIG_32V_2A);

        /* Calibration value = 0.04096 / (current_lsb * shunt_ohm)
         * current_lsb = max_current / 32768
         * For 3.2A max: current_lsb = 0.0001 A, cal = 4096 */
        _currentLSB = 3.2f / 32768.0f;
        uint16_t cal = (uint16_t)(0.04096f / (_currentLSB * _shuntOhm));
        _writeReg16(INA219_REG_CALIBRATION, cal);
        delay(10);

        /* Verify by reading config register back */
        uint16_t readback = _readReg16(INA219_REG_CONFIG);
        return (readback == INA219_CONFIG_32V_2A);
    }

    WySensorData read() override {
        WySensorData d;

        /* Bus voltage: bits 15:3, multiply by 4mV LSB */
        int16_t raw_bus = (int16_t)_readReg16(INA219_REG_BUS_VOLTAGE);
        if (raw_bus & 0x01) { d.error = "overflow"; return d; }
        d.voltage = (raw_bus >> 3) * 0.004f;  /* 4mV per LSB */

        /* Shunt voltage: signed 16-bit, 10µV per LSB */
        int16_t raw_shunt = (int16_t)_readReg16(INA219_REG_SHUNT_VOLTAGE);
        float shunt_mv = raw_shunt * 0.01f;  /* 10µV → mV */

        /* Current from shunt (Ohm's law, no calibration register needed) */
        d.current = (shunt_mv / 1000.0f) / _shuntOhm;  /* A */

        /* Power */
        d.raw = shunt_mv;  /* store shunt mV in raw field */
        d.weight = d.voltage * d.current;  /* W — using weight field as power */

        d.ok = true;
        return d;
    }

    /* Convenience: direct power in Watts */
    float readPowerW() {
        WySensorData d = read();
        return d.ok ? d.voltage * d.current : 0.0f;
    }

private:
    WyI2CPins _pins;
    float     _shuntOhm;
    float     _currentLSB = 0.0001f;

    void _writeReg16(uint8_t reg, uint16_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.write(val >> 8);
        Wire.write(val & 0xFF);
        Wire.endTransmission();
    }

    uint16_t _readReg16(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)2);
        if (Wire.available() < 2) return 0xFFFF;
        return ((uint16_t)Wire.read() << 8) | Wire.read();
    }
};
