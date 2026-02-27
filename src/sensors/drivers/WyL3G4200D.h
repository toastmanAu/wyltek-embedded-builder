/*
 * drivers/WyL3G4200D.h — L3G4200D 3-axis gyroscope (I2C or SPI)
 * ===============================================================
 * Datasheet: https://www.st.com/resource/en/datasheet/l3g4200d.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x68 (SDO/SA0 pin LOW) or 0x69 (SDO/SA0 pin HIGH)
 * Registered via WySensors::addI2C<WyL3G4200D>("gyro", sda, scl, 0x68)
 *
 * Measures:
 *   Angular rate (gyroscope): X, Y, Z axes in degrees per second (°/s)
 *   Temperature: on-die thermal sensor (relative, not calibrated °C)
 *
 * Selectable full-scale:
 *   L3G4200D_FS_250DPS  — ±250 °/s,  LSB = 8.75 mdps
 *   L3G4200D_FS_500DPS  — ±500 °/s,  LSB = 17.50 mdps
 *   L3G4200D_FS_2000DPS — ±2000 °/s, LSB = 70.00 mdps
 *
 * Output data rate (ODR):
 *   100Hz, 200Hz, 400Hz, 800Hz — configurable via CTRL_REG1
 *   Default: 100Hz with 12.5Hz bandwidth cut-off
 *
 * vs MPU6050:
 *   L3G4200D = gyro ONLY — no accelerometer, no magnetometer
 *   Lower noise floor than MPU6050 gyro
 *   Better suited for precise rotation / stabilisation (gimbal, drone, robot)
 *   Higher ODR available (800Hz vs MPU6050's 8kHz gyro but 1kHz typical)
 *
 * GY-50 BREAKOUT BOARD:
 *   Has onboard 3.3V regulator — can accept 5V VCC
 *   SDO pin selects I2C address: open/GND = 0x68, +3.3V = 0x69
 *   Also exposes SPI pins (CS, SPC, SDI, SDO) — this driver uses I2C
 *
 * WIRING (I2C):
 *   VCC → 3.3V or 5V (GY-50 has regulator)
 *   GND → GND
 *   SDA → I2C SDA
 *   SCL → I2C SCL
 *   INT1 → optional interrupt GPIO (data ready / threshold)
 *   INT2 → optional interrupt GPIO (FIFO / drdy)
 *
 * ZERO-RATE CALIBRATION:
 *   All gyros have a zero-rate offset — they output a small non-zero
 *   value even when stationary. Call calibrate() with sensor at rest
 *   to measure and subtract this offset automatically.
 *
 * WySensorData mapping:
 *   temperature = on-die temp (relative — not accurate absolute °C)
 *   raw         = rotation magnitude √(gx²+gy²+gz²) in °/s
 *   Use getGyro() direct method for individual axis values
 *
 * FIFO:
 *   L3G4200D has 32-sample FIFO — useful for high-rate capture.
 *   Not implemented in this driver (streaming mode used).
 */

#pragma once
#include "../WySensors.h"

/* L3G4200D register addresses */
#define L3G4200D_REG_WHO_AM_I    0x0F  /* should read 0xD3 */
#define L3G4200D_REG_CTRL_REG1  0x20  /* ODR, bandwidth, power, axes enable */
#define L3G4200D_REG_CTRL_REG2  0x21  /* high-pass filter */
#define L3G4200D_REG_CTRL_REG3  0x22  /* interrupt config */
#define L3G4200D_REG_CTRL_REG4  0x23  /* full-scale, SPI mode, BDU */
#define L3G4200D_REG_CTRL_REG5  0x24  /* FIFO enable, HPF enable */
#define L3G4200D_REG_REFERENCE  0x25  /* reference for interrupt */
#define L3G4200D_REG_OUT_TEMP   0x26  /* temperature output (signed, 1°C/LSB relative) */
#define L3G4200D_REG_STATUS     0x27  /* data ready flags */
#define L3G4200D_REG_OUT_X_L    0x28  /* X-axis low byte  (burst read 28-2D = 6 bytes) */
#define L3G4200D_REG_OUT_X_H    0x29
#define L3G4200D_REG_OUT_Y_L    0x2A
#define L3G4200D_REG_OUT_Y_H    0x2B
#define L3G4200D_REG_OUT_Z_L    0x2C
#define L3G4200D_REG_OUT_Z_H    0x2D
#define L3G4200D_REG_FIFO_CTRL  0x2E
#define L3G4200D_REG_FIFO_SRC   0x2F
#define L3G4200D_REG_INT1_CFG   0x30
#define L3G4200D_REG_INT1_SRC   0x31

#define L3G4200D_CHIP_ID        0xD3

/* Full-scale selection (CTRL_REG4 bits 5:4) */
#define L3G4200D_FS_250DPS   0x00   /* ±250 °/s,  sensitivity = 8.75 mdps/digit */
#define L3G4200D_FS_500DPS   0x10   /* ±500 °/s,  sensitivity = 17.50 mdps/digit */
#define L3G4200D_FS_2000DPS  0x20   /* ±2000 °/s, sensitivity = 70.00 mdps/digit */

/* ODR + bandwidth (CTRL_REG1 bits 7:4) */
#define L3G4200D_ODR_100HZ_BW12  0x00  /* 100Hz, 12.5Hz BW (default) */
#define L3G4200D_ODR_100HZ_BW25  0x10  /* 100Hz, 25Hz BW */
#define L3G4200D_ODR_200HZ_BW12  0x40  /* 200Hz, 12.5Hz BW */
#define L3G4200D_ODR_200HZ_BW25  0x50  /* 200Hz, 25Hz BW */
#define L3G4200D_ODR_200HZ_BW50  0x60  /* 200Hz, 50Hz BW */
#define L3G4200D_ODR_200HZ_BW70  0x70  /* 200Hz, 70Hz BW */
#define L3G4200D_ODR_400HZ_BW20  0x80  /* 400Hz, 20Hz BW */
#define L3G4200D_ODR_400HZ_BW25  0x90  /* 400Hz, 25Hz BW */
#define L3G4200D_ODR_400HZ_BW50  0xA0  /* 400Hz, 50Hz BW */
#define L3G4200D_ODR_400HZ_BW110 0xB0  /* 400Hz, 110Hz BW */
#define L3G4200D_ODR_800HZ_BW30  0xC0  /* 800Hz, 30Hz BW */
#define L3G4200D_ODR_800HZ_BW35  0xD0  /* 800Hz, 35Hz BW */
#define L3G4200D_ODR_800HZ_BW50  0xE0  /* 800Hz, 50Hz BW */
#define L3G4200D_ODR_800HZ_BW110 0xF0  /* 800Hz, 110Hz BW */

/* I2C burst-read: set bit 7 of register address to auto-increment */
#define L3G4200D_AUTO_INC       0x80

struct WyGyroData {
    float gx, gy, gz;   /* angular rate °/s */
    float temp;          /* on-die temperature (relative °C) */
    float magnitude;     /* √(gx²+gy²+gz²) */
    bool  ok = false;
};

class WyL3G4200D : public WySensorBase {
public:
    WyL3G4200D(WyI2CPins pins,
               uint8_t fullScale = L3G4200D_FS_500DPS,
               uint8_t odr       = L3G4200D_ODR_100HZ_BW25)
        : _pins(pins), _fsCfg(fullScale), _odrCfg(odr) {}

    const char* driverName() override { return "L3G4200D"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);

        /* Verify chip ID */
        uint8_t id = _readReg8(L3G4200D_REG_WHO_AM_I);
        if (id != L3G4200D_CHIP_ID) {
            Serial.printf("[L3G4200D] wrong WHO_AM_I: 0x%02X (expected 0xD3)\n", id);
            return false;
        }

        /* CTRL_REG1: power on, enable all axes, set ODR
         * Bits: DR1|DR0|BW1|BW0|PD|Zen|Yen|Xen
         * PD=1 (normal mode), Zen=Yen=Xen=1 (all axes on) */
        _writeReg(L3G4200D_REG_CTRL_REG1, _odrCfg | 0x0F);

        /* CTRL_REG2: high-pass filter off */
        _writeReg(L3G4200D_REG_CTRL_REG2, 0x00);

        /* CTRL_REG3: no interrupts */
        _writeReg(L3G4200D_REG_CTRL_REG3, 0x00);

        /* CTRL_REG4: full-scale, BDU=1 (block data update — prevents read of mixed samples)
         * BDU: output registers not updated until both MSB and LSB have been read */
        _writeReg(L3G4200D_REG_CTRL_REG4, _fsCfg | 0x80);

        /* CTRL_REG5: no FIFO, no HPF */
        _writeReg(L3G4200D_REG_CTRL_REG5, 0x00);

        /* Compute sensitivity (mdps per digit → °/s per digit) */
        switch (_fsCfg) {
            case L3G4200D_FS_250DPS:  _sensitivity =  8.75f / 1000.0f; break;
            case L3G4200D_FS_500DPS:  _sensitivity = 17.50f / 1000.0f; break;
            case L3G4200D_FS_2000DPS: _sensitivity = 70.00f / 1000.0f; break;
            default:                  _sensitivity = 17.50f / 1000.0f;
        }

        delay(10);
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        WyGyroData g = readGyro();
        if (!g.ok) { d.error = "data not ready"; return d; }
        d.temperature = g.temp;
        d.raw         = g.magnitude;
        d.ok          = true;
        _last = g;
        return d;
    }

    /* Full gyro read — use this for individual axis data */
    WyGyroData readGyro() {
        WyGyroData g;

        /* Check data-ready bit */
        if (!(_readReg8(L3G4200D_REG_STATUS) & 0x08)) {
            /* ZYXDA bit — new set of XYZ data available */
            /* Not ready yet — return last valid */
            if (_last.ok) return _last;
            g.ok = false;
            return g;
        }

        /* Burst read 6 bytes: X_L, X_H, Y_L, Y_H, Z_L, Z_H
         * Auto-increment by setting bit 7 of start address */
        uint8_t buf[6];
        _readRegBuf(L3G4200D_REG_OUT_X_L | L3G4200D_AUTO_INC, buf, 6);

        int16_t raw_x = (int16_t)((buf[1] << 8) | buf[0]);
        int16_t raw_y = (int16_t)((buf[3] << 8) | buf[2]);
        int16_t raw_z = (int16_t)((buf[5] << 8) | buf[4]);

        /* Apply sensitivity + zero-rate offset */
        g.gx = (raw_x - _offsetX) * _sensitivity;
        g.gy = (raw_y - _offsetY) * _sensitivity;
        g.gz = (raw_z - _offsetZ) * _sensitivity;

        /* Temperature: signed 8-bit, relative (not calibrated absolute) */
        g.temp = (int8_t)_readReg8(L3G4200D_REG_OUT_TEMP);

        g.magnitude = sqrtf(g.gx*g.gx + g.gy*g.gy + g.gz*g.gz);
        g.ok = true;
        return g;
    }

    /* Calibrate zero-rate offset — keep sensor STATIONARY during calibration */
    void calibrate(uint16_t samples = 200) {
        Serial.println("[L3G4200D] calibrating — keep sensor completely still...");
        int64_t sx = 0, sy = 0, sz = 0;
        uint16_t count = 0;
        uint32_t deadline = millis() + (samples * 15);  /* 100Hz → ~10ms per sample */

        while (millis() < deadline) {
            if (_readReg8(L3G4200D_REG_STATUS) & 0x08) {
                uint8_t buf[6];
                _readRegBuf(L3G4200D_REG_OUT_X_L | L3G4200D_AUTO_INC, buf, 6);
                sx += (int16_t)((buf[1]<<8)|buf[0]);
                sy += (int16_t)((buf[3]<<8)|buf[2]);
                sz += (int16_t)((buf[5]<<8)|buf[4]);
                count++;
            }
            delay(1);
        }

        if (count > 0) {
            _offsetX = sx / count;
            _offsetY = sy / count;
            _offsetZ = sz / count;
        }
        Serial.printf("[L3G4200D] zero-rate offset: X=%d Y=%d Z=%d (%d samples)\n",
            _offsetX, _offsetY, _offsetZ, count);
    }

    void setOffset(int16_t x, int16_t y, int16_t z) {
        _offsetX = x; _offsetY = y; _offsetZ = z;
    }

    WyGyroData last() { return _last; }

    /* Set full-scale range at runtime (re-writes CTRL_REG4) */
    void setFullScale(uint8_t fs) {
        _fsCfg = fs;
        _writeReg(L3G4200D_REG_CTRL_REG4, _fsCfg | 0x80);
        switch (_fsCfg) {
            case L3G4200D_FS_250DPS:  _sensitivity =  8.75f / 1000.0f; break;
            case L3G4200D_FS_500DPS:  _sensitivity = 17.50f / 1000.0f; break;
            case L3G4200D_FS_2000DPS: _sensitivity = 70.00f / 1000.0f; break;
        }
    }

    /* Set ODR at runtime */
    void setODR(uint8_t odr) {
        _odrCfg = odr;
        _writeReg(L3G4200D_REG_CTRL_REG1, _odrCfg | 0x0F);
    }

    /* Check if new data is ready */
    bool dataReady() { return (_readReg8(L3G4200D_REG_STATUS) & 0x08) != 0; }

private:
    WyI2CPins _pins;
    uint8_t   _fsCfg, _odrCfg;
    float     _sensitivity = 17.50f / 1000.0f;
    int16_t   _offsetX = 0, _offsetY = 0, _offsetZ = 0;
    WyGyroData _last;

    void _writeReg(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.write(val);
        Wire.endTransmission();
    }

    uint8_t _readReg8(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0xFF;
    }

    void _readRegBuf(uint8_t reg, uint8_t* buf, uint8_t len) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg); Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, len);
        for (uint8_t i = 0; i < len && Wire.available(); i++) buf[i] = Wire.read();
    }
};
