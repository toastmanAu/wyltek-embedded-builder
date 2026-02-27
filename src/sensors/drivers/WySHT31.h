/*
 * drivers/WySHT31.h — SHT31 temperature + humidity (I2C)
 * ========================================================
 * Bundled driver — no external library needed.
 * Registered via WySensors::addI2C<WySHT31>("name", sda, scl, addr)
 * Default I2C address: 0x44 (ADDR pin LOW), 0x45 (ADDR pin HIGH)
 */

#pragma once
#include "../WySensors.h"

#define SHT31_CMD_MEAS_HIGHREP  0x2400
#define SHT31_CMD_SOFT_RESET    0x30A2
#define SHT31_CMD_STATUS        0xF32D

class WySHT31 : public WySensorBase {
public:
    WySHT31(WyI2CPins pins) : _pins(pins) {}

    const char* driverName() override { return "SHT31"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);
        _sendCmd(SHT31_CMD_SOFT_RESET);
        delay(10);
        /* Read status to verify comms */
        _sendCmd(SHT31_CMD_STATUS);
        Wire.requestFrom(_pins.addr, (uint8_t)3);
        if (!Wire.available()) return false;
        Wire.read(); Wire.read(); Wire.read();  /* discard status + CRC */
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        _sendCmd(SHT31_CMD_MEAS_HIGHREP);
        delay(20);
        Wire.requestFrom(_pins.addr, (uint8_t)6);
        if (Wire.available() < 6) { d.error = "no data"; return d; }
        uint8_t buf[6];
        for (uint8_t i = 0; i < 6; i++) buf[i] = Wire.read();
        /* CRC check (optional — skip for speed) */
        uint16_t raw_t = (buf[0] << 8) | buf[1];
        uint16_t raw_h = (buf[3] << 8) | buf[4];
        d.temperature = -45.0f + 175.0f * raw_t / 65535.0f;
        d.humidity    = 100.0f * raw_h / 65535.0f;
        d.ok = true;
        return d;
    }

private:
    WyI2CPins _pins;

    void _sendCmd(uint16_t cmd) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(cmd >> 8);
        Wire.write(cmd & 0xFF);
        Wire.endTransmission();
    }
};
