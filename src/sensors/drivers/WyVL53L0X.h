/*
 * drivers/WyVL53L0X.h — VL53L0X Time-of-Flight laser distance sensor (I2C)
 * ===========================================================================
 * Datasheet: https://www.st.com/resource/en/datasheet/vl53l0x.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x29 (default), can be changed in firmware
 * Registered via WySensors::addI2C<WyVL53L0X>("name", sda, scl, 0x29)
 *
 * Range: 30mm – 2000mm (typical), 8m in ideal conditions
 * Resolution: 1mm
 *
 * Note: The VL53L0X has a complex init sequence with SPAD calibration.
 * This driver uses a simplified init (direct register writes) that works
 * for most use cases. For production with full ST accuracy, use their API.
 */

#pragma once
#include "../WySensors.h"

/* VL53L0X key registers */
#define VL53L0X_REG_ID_HIGH         0xC0  /* should read 0xEE */
#define VL53L0X_REG_ID_LOW          0xC1  /* should read 0xAA */
#define VL53L0X_REG_REV             0xC2  /* should read 0x10 */
#define VL53L0X_REG_SYS_INTERRUPT   0x0A
#define VL53L0X_REG_SEQUENCE_CONFIG 0x01
#define VL53L0X_REG_SYSRANGE_START  0x00
#define VL53L0X_REG_RESULT_STATUS   0x13
#define VL53L0X_REG_RESULT_RANGE    0x14  /* 2 bytes: range in mm */
#define VL53L0X_REG_OSC_CALIBRATE   0xF8
#define VL53L0X_REG_GPIO_HV_MUX     0x84
#define VL53L0X_REG_GPIO_CONFIG      0x89
#define VL53L0X_REG_POWER_MGMT      0x80

/* Measurement modes */
#define VL53L0X_MODE_SINGLE    0  /* one shot, lower power */
#define VL53L0X_MODE_CONTINUOUS 1  /* continuous, faster */

class WyVL53L0X : public WySensorBase {
public:
    WyVL53L0X(WyI2CPins pins, uint8_t mode = VL53L0X_MODE_SINGLE)
        : _pins(pins), _mode(mode) {}

    const char* driverName() override { return "VL53L0X"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);
        delay(2);

        /* Verify device identity */
        uint8_t id_hi = _readReg8(VL53L0X_REG_ID_HIGH);
        uint8_t id_lo = _readReg8(VL53L0X_REG_ID_LOW);
        if (id_hi != 0xEE || id_lo != 0xAA) {
            Serial.printf("[VL53L0X] wrong ID: 0x%02X 0x%02X\n", id_hi, id_lo);
            return false;
        }

        /* Simplified init sequence (based on ST API essential registers) */
        _writeReg(0x88, 0x00);  /* set I2C standard mode */
        _writeReg(0x80, 0x01);
        _writeReg(0xFF, 0x01);
        _writeReg(0x00, 0x00);
        _stopVar = _readReg8(0x91);
        _writeReg(0x00, 0x01);
        _writeReg(0xFF, 0x00);
        _writeReg(0x80, 0x00);

        /* Disable SIGNAL_RATE_MSRC and SIGNAL_RATE_PRE_RANGE limit checks */
        _writeReg(0x60, _readReg8(0x60) | 0x12);

        /* Set signal rate limit to 0.25 MCPS (unit: MCPS × 65536) */
        _writeReg16(0x44, 0x0020);

        /* Enable sequence steps: DSS, PRE-RANGE, FINAL-RANGE */
        _writeReg(VL53L0X_REG_SEQUENCE_CONFIG, 0xE8);

        /* Perform SPAD management calibration */
        _performSPADCalibration();

        /* Perform reference calibration */
        _writeReg(VL53L0X_REG_SEQUENCE_CONFIG, 0x01);
        _performRefCalibration(0x40);  /* VHV */
        _writeReg(VL53L0X_REG_SEQUENCE_CONFIG, 0x02);
        _performRefCalibration(0x00);  /* phase */
        _writeReg(VL53L0X_REG_SEQUENCE_CONFIG, 0xE8);

        /* Start continuous if needed */
        if (_mode == VL53L0X_MODE_CONTINUOUS) _startContinuous();

        return true;
    }

    WySensorData read() override {
        WySensorData d;
        uint16_t mm;

        if (_mode == VL53L0X_MODE_CONTINUOUS) {
            mm = _readContinuous();
        } else {
            mm = _readSingle();
        }

        if (mm == 0 || mm >= 8190) { d.error = "out of range"; return d; }
        d.distance  = mm;
        d.ok        = true;
        return d;
    }

    /* Change I2C address (useful for multiple sensors on same bus) */
    bool setAddress(uint8_t newAddr) {
        _writeReg(0x8A, newAddr & 0x7F);
        _pins.addr = newAddr;
        return true;
    }

private:
    WyI2CPins _pins;
    uint8_t   _mode;
    uint8_t   _stopVar = 0;

    uint16_t _readSingle() {
        _writeReg(0x80, 0x01);
        _writeReg(0xFF, 0x01);
        _writeReg(0x00, 0x00);
        _writeReg(0x91, _stopVar);
        _writeReg(0x00, 0x01);
        _writeReg(0xFF, 0x00);
        _writeReg(0x80, 0x00);
        _writeReg(VL53L0X_REG_SYSRANGE_START, 0x01);

        uint32_t t = millis();
        while (_readReg8(VL53L0X_REG_SYSRANGE_START) & 0x01) {
            if (millis() - t > 500) return 0;
        }
        return _readResult();
    }

    void _startContinuous() {
        _writeReg(0x80, 0x01);
        _writeReg(0xFF, 0x01);
        _writeReg(0x00, 0x00);
        _writeReg(0x91, _stopVar);
        _writeReg(0x00, 0x01);
        _writeReg(0xFF, 0x00);
        _writeReg(0x80, 0x00);
        _writeReg(VL53L0X_REG_SYSRANGE_START, 0x02);
    }

    uint16_t _readContinuous() {
        uint32_t t = millis();
        while (!(_readReg8(VL53L0X_REG_RESULT_STATUS) & 0x07)) {
            if (millis() - t > 500) return 0;
        }
        return _readResult();
    }

    uint16_t _readResult() {
        uint16_t mm = _readReg16(VL53L0X_REG_RESULT_RANGE);
        _writeReg(VL53L0X_REG_SYS_INTERRUPT, 0x01);  /* clear interrupt */
        return mm;
    }

    void _performSPADCalibration() {
        uint8_t spads[6] = {0};
        _writeReg(0x80, 0x01);
        _writeReg(0xFF, 0x01);
        _writeReg(0x00, 0x00);
        _writeReg(0xFF, 0x06);
        _writeReg(0x83, _readReg8(0x83) | 0x04);
        _writeReg(0xFF, 0x07);
        _writeReg(0x81, 0x01);
        _writeReg(0x80, 0x01);
        _writeReg(0x94, 0x6B);
        _writeReg(0x83, 0x00);
        uint32_t t = millis();
        while (!_readReg8(0x83)) { if (millis()-t > 500) return; }
        _writeReg(0x83, 0x01);
        for (uint8_t i=0;i<6;i++) spads[i] = _readReg8(0x90+i); (void)spads;
        _writeReg(0x81, 0x00);
        _writeReg(0xFF, 0x06);
        _writeReg(0x83, _readReg8(0x83) & ~0x04);
        _writeReg(0xFF, 0x01);
        _writeReg(0x00, 0x01);
        _writeReg(0xFF, 0x00);
        _writeReg(0x80, 0x00);
    }

    void _performRefCalibration(uint8_t vhv_init_byte) {
        _writeReg(VL53L0X_REG_SYSRANGE_START, 0x01 | vhv_init_byte);
        uint32_t t = millis();
        while (!(_readReg8(VL53L0X_REG_RESULT_STATUS) & 0x07)) {
            if (millis()-t > 500) return;
        }
        _writeReg(VL53L0X_REG_SYS_INTERRUPT, 0x01);
        _writeReg(VL53L0X_REG_SYSRANGE_START, 0x00);
    }

    void _writeReg(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.write(val);
        Wire.endTransmission();
    }

    void _writeReg16(uint8_t reg, uint16_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.write(val >> 8); Wire.write(val & 0xFF);
        Wire.endTransmission();
    }

    uint8_t _readReg8(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0xFF;
    }

    uint16_t _readReg16(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)2);
        if (Wire.available() < 2) return 0;
        return ((uint16_t)Wire.read() << 8) | Wire.read();
    }
};
