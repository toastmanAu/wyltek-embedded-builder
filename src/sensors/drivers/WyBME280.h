/*
 * drivers/WyBME280.h — BME280 temperature/humidity/pressure (I2C)
 * =================================================================
 * Bundled driver — no external library needed.
 * Implements the minimal BME280 register protocol directly.
 * Inherits WySensorBase, registered via WySensors::addI2C<WyBME280>()
 */

#pragma once
#include "../WySensors.h"

/* BME280 registers */
#define BME280_REG_ID        0xD0
#define BME280_REG_RESET     0xE0
#define BME280_REG_CTRL_HUM  0xF2
#define BME280_REG_STATUS    0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG    0xF5
#define BME280_REG_DATA      0xF7  /* 8 bytes: press[3], temp[3], hum[2] */
#define BME280_REG_CALIB00   0x88  /* 26 bytes of calibration */
#define BME280_REG_CALIB26   0xE1  /* 7 more bytes */
#define BME280_CHIP_ID       0x60  /* also 0x58 for BMP280 (no humidity) */

class WyBME280 : public WySensorBase {
public:
    WyBME280(WyI2CPins pins) : _pins(pins) {}

    const char* driverName() override { return "BME280"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);

        uint8_t id = _readReg8(BME280_REG_ID);
        if (id != BME280_CHIP_ID && id != 0x58) {
            Serial.printf("[BME280] unexpected chip ID 0x%02X\n", id);
            return false;
        }
        _hasPressureHumidity = (id == BME280_CHIP_ID);

        /* Soft reset */
        _writeReg(BME280_REG_RESET, 0xB6);
        delay(10);

        /* Load calibration */
        _readCalib();

        /* Normal mode, oversampling x1 for all */
        _writeReg(BME280_REG_CTRL_HUM,  0x01);
        _writeReg(BME280_REG_CONFIG,    0xA0);  /* standby 1000ms, filter off */
        _writeReg(BME280_REG_CTRL_MEAS, 0x27);  /* temp x1, press x1, normal mode */
        delay(100);
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        uint8_t buf[8];
        _readRegBuf(BME280_REG_DATA, buf, 8);

        int32_t adc_P = ((int32_t)buf[0] << 12) | ((int32_t)buf[1] << 4) | (buf[2] >> 4);
        int32_t adc_T = ((int32_t)buf[3] << 12) | ((int32_t)buf[4] << 4) | (buf[5] >> 4);
        int32_t adc_H = ((int32_t)buf[6] << 8)  |  buf[7];

        int32_t t_fine;
        d.temperature = _compensateT(adc_T, t_fine);
        if (_hasPressureHumidity) {
            d.pressure  = _compensateP(adc_P, t_fine) / 100.0f;  /* Pa → hPa */
            d.humidity  = _compensateH(adc_H, t_fine);
            d.altitude  = 44330.0f * (1.0f - powf(d.pressure / 1013.25f, 0.1903f));
        }
        d.ok = true;
        return d;
    }

private:
    WyI2CPins _pins;
    bool _hasPressureHumidity = true;

    /* Calibration coefficients */
    uint16_t _T1; int16_t _T2, _T3;
    uint16_t _P1; int16_t _P2, _P3, _P4, _P5, _P6, _P7, _P8, _P9;
    uint8_t  _H1, _H3;
    int16_t  _H2, _H4, _H5;
    int8_t   _H6;

    void _readCalib() {
        uint8_t c[26];
        _readRegBuf(BME280_REG_CALIB00, c, 26);
        _T1 = (c[1]<<8)|c[0]; _T2 = (c[3]<<8)|c[2]; _T3 = (c[5]<<8)|c[4];
        _P1 = (c[7]<<8)|c[6]; _P2 = (c[9]<<8)|c[8]; _P3 = (c[11]<<8)|c[10];
        _P4 = (c[13]<<8)|c[12]; _P5 = (c[15]<<8)|c[14]; _P6 = (c[17]<<8)|c[16];
        _P7 = (c[19]<<8)|c[18]; _P8 = (c[21]<<8)|c[20]; _P9 = (c[23]<<8)|c[22];
        _H1 = c[25];
        uint8_t h[7]; _readRegBuf(BME280_REG_CALIB26, h, 7);
        _H2 = (h[1]<<8)|h[0]; _H3 = h[2];
        _H4 = ((int16_t)h[3]<<4) | (h[4] & 0x0F);
        _H5 = ((int16_t)h[5]<<4) | (h[4] >> 4);
        _H6 = (int8_t)h[6];
    }

    float _compensateT(int32_t adc_T, int32_t& t_fine) {
        int32_t v1 = ((((adc_T>>3) - ((int32_t)_T1<<1))) * _T2) >> 11;
        int32_t v2 = (((((adc_T>>4) - (int32_t)_T1) * ((adc_T>>4) - (int32_t)_T1)) >> 12) * _T3) >> 14;
        t_fine = v1 + v2;
        return ((t_fine * 5 + 128) >> 8) / 100.0f;
    }

    float _compensateP(int32_t adc_P, int32_t t_fine) {
        int64_t v1 = (int64_t)t_fine - 128000;
        int64_t v2 = v1*v1*(int64_t)_P6 + ((v1*(int64_t)_P5)<<17) + ((int64_t)_P4<<35);
        v1 = ((v1*v1*(int64_t)_P3)>>8) + ((v1*(int64_t)_P2)<<12);
        v1 = (((int64_t)1<<47) + v1) * _P1 >> 33;
        if (v1 == 0) return 0;
        int64_t p = 1048576 - adc_P;
        p = (((p<<31) - v2) * 3125) / v1;
        v1 = ((int64_t)_P9 * (p>>13) * (p>>13)) >> 25;
        v2 = ((int64_t)_P8 * p) >> 19;
        return ((p + v1 + v2) >> 8) + ((int64_t)_P7<<4) / 256.0f;
    }

    float _compensateH(int32_t adc_H, int32_t t_fine) {
        int32_t x = t_fine - 76800;
        x = (((adc_H<<14) - ((int32_t)_H4<<20) - ((int32_t)_H5*x) + 16384)>>15) *
            (((((((x*(int32_t)_H6)>>10) * (((x*(int32_t)_H3)>>11) + 32768))>>10) + 2097152) *
            _H2 + 8192) >> 14);
        x -= ((((x>>15)*(x>>15))>>7) * (int32_t)_H1) >> 4;
        x = constrain(x, 0, 419430400);
        return (x >> 12) / 1024.0f;
    }

    void _writeReg(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.write(val);
        Wire.endTransmission();
    }
    uint8_t _readReg8(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0;
    }
    void _readRegBuf(uint8_t reg, uint8_t* buf, uint8_t len) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, len);
        for (uint8_t i = 0; i < len && Wire.available(); i++) buf[i] = Wire.read();
    }
};
