/*
 * drivers/WyMPU6050.h — MPU-6050 6-DoF IMU (I2C)
 * =================================================
 * Datasheet: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf
 * Register map: https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Register-Map1.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x68 (AD0 pin LOW) or 0x69 (AD0 pin HIGH)
 * Registered via WySensors::addI2C<WyMPU6050>("imu", sda, scl, 0x68)
 *
 * Measures:
 *   - Accelerometer: X, Y, Z (g)  ±2/4/8/16g selectable
 *   - Gyroscope:     X, Y, Z (°/s) ±250/500/1000/2000 °/s selectable
 *   - Temperature:   on-die temp (°C) — not ambient
 *
 * WySensorData mapping:
 *   temperature = die temp °C
 *   raw = accel magnitude (total g)
 *   rawInt = packed status byte
 *   Extended data via getAccel() / getGyro() direct methods
 */

#pragma once
#include "../WySensors.h"

/* MPU6050 register addresses */
#define MPU6050_REG_SELF_TEST_X  0x0D
#define MPU6050_REG_SMPLRT_DIV   0x19  /* sample rate = gyro_rate / (1 + div) */
#define MPU6050_REG_CONFIG       0x1A  /* DLPF config */
#define MPU6050_REG_GYRO_CONFIG  0x1B  /* FS_SEL bits 4:3 */
#define MPU6050_REG_ACCEL_CONFIG 0x1C  /* AFS_SEL bits 4:3 */
#define MPU6050_REG_FIFO_EN      0x23
#define MPU6050_REG_INT_ENABLE   0x38
#define MPU6050_REG_ACCEL_XOUT_H 0x3B  /* 6 bytes: AX_H AX_L AY_H AY_L AZ_H AZ_L */
#define MPU6050_REG_TEMP_OUT_H   0x41  /* 2 bytes: TEMP_H TEMP_L */
#define MPU6050_REG_GYRO_XOUT_H  0x43  /* 6 bytes: GX_H GX_L GY_H GY_L GZ_H GZ_L */
#define MPU6050_REG_PWR_MGMT_1   0x6B  /* bit 6 = SLEEP, bits 2:0 = CLKSEL */
#define MPU6050_REG_PWR_MGMT_2   0x6C
#define MPU6050_REG_WHO_AM_I     0x75  /* should read 0x68 */

/* Accelerometer full-scale range (AFS_SEL) */
#define MPU6050_ACCEL_2G   0x00  /* ±2g,  LSB = 16384 */
#define MPU6050_ACCEL_4G   0x08  /* ±4g,  LSB = 8192  */
#define MPU6050_ACCEL_8G   0x10  /* ±8g,  LSB = 4096  */
#define MPU6050_ACCEL_16G  0x18  /* ±16g, LSB = 2048  */

/* Gyroscope full-scale range (FS_SEL) */
#define MPU6050_GYRO_250   0x00  /* ±250 °/s,  LSB = 131.0 */
#define MPU6050_GYRO_500   0x08  /* ±500 °/s,  LSB = 65.5  */
#define MPU6050_GYRO_1000  0x10  /* ±1000 °/s, LSB = 32.8  */
#define MPU6050_GYRO_2000  0x18  /* ±2000 °/s, LSB = 16.4  */

struct WyIMUData {
    float ax, ay, az;   /* accelerometer (g) */
    float gx, gy, gz;   /* gyroscope (°/s) */
    float temp;          /* die temperature (°C) */
    float accelMag;      /* sqrt(ax²+ay²+az²) */
    float roll, pitch;   /* calculated from accel (°), no gyro fusion */
    bool ok = false;
};

class WyMPU6050 : public WySensorBase {
public:
    WyMPU6050(WyI2CPins pins,
              uint8_t accelRange = MPU6050_ACCEL_8G,
              uint8_t gyroRange  = MPU6050_GYRO_500)
        : _pins(pins), _accelCfg(accelRange), _gyroCfg(gyroRange) {}

    const char* driverName() override { return "MPU6050"; }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);

        uint8_t id = _readReg8(MPU6050_REG_WHO_AM_I);
        if (id != 0x68 && id != 0x69 && id != 0x70) {
            Serial.printf("[MPU6050] wrong WHO_AM_I: 0x%02X\n", id);
            return false;
        }

        /* Wake up — default is SLEEP mode */
        _writeReg(MPU6050_REG_PWR_MGMT_1, 0x00);  /* clear SLEEP, use internal 8MHz */
        delay(100);

        /* Use PLL with X-axis gyro reference (better stability) */
        _writeReg(MPU6050_REG_PWR_MGMT_1, 0x01);
        delay(10);

        /* DLPF: 44Hz accel / 42Hz gyro bandwidth (smooth, low noise) */
        _writeReg(MPU6050_REG_CONFIG, 0x03);

        /* Sample rate = 1kHz / (1 + 9) = 100Hz */
        _writeReg(MPU6050_REG_SMPLRT_DIV, 0x09);

        /* Set ranges */
        _writeReg(MPU6050_REG_GYRO_CONFIG,  _gyroCfg);
        _writeReg(MPU6050_REG_ACCEL_CONFIG, _accelCfg);

        /* Compute LSB divisors */
        const float accelLSB[] = {16384.0f, 8192.0f, 4096.0f, 2048.0f};
        const float gyroLSB[]  = {131.0f, 65.5f, 32.8f, 16.4f};
        _accelDiv = accelLSB[(_accelCfg >> 3) & 0x03];
        _gyroDiv  = gyroLSB [(_gyroCfg  >> 3) & 0x03];

        return true;
    }

    WySensorData read() override {
        WySensorData d;
        WyIMUData imu = readIMU();
        if (!imu.ok) { d.error = "read fail"; return d; }
        d.temperature = imu.temp;
        d.raw         = imu.accelMag;
        d.ok          = true;
        _last = imu;
        return d;
    }

    /* Full IMU read — use this directly for accel + gyro data */
    WyIMUData readIMU() {
        WyIMUData d;
        uint8_t buf[14];
        _readRegBuf(MPU6050_REG_ACCEL_XOUT_H, buf, 14);

        /* Accel: bytes 0-5 */
        d.ax = (int16_t)((buf[0]<<8)|buf[1])  / _accelDiv;
        d.ay = (int16_t)((buf[2]<<8)|buf[3])  / _accelDiv;
        d.az = (int16_t)((buf[4]<<8)|buf[5])  / _accelDiv;

        /* Temperature: bytes 6-7. Formula from datasheet: T = raw/340 + 36.53 */
        d.temp = (int16_t)((buf[6]<<8)|buf[7]) / 340.0f + 36.53f;

        /* Gyro: bytes 8-13 */
        d.gx = (int16_t)((buf[8]<<8) |buf[9])  / _gyroDiv;
        d.gy = (int16_t)((buf[10]<<8)|buf[11]) / _gyroDiv;
        d.gz = (int16_t)((buf[12]<<8)|buf[13]) / _gyroDiv;

        d.accelMag = sqrtf(d.ax*d.ax + d.ay*d.ay + d.az*d.az);
        /* Roll/pitch from accelerometer (no gyro fusion — drifts under motion) */
        d.roll  = atan2f(d.ay, d.az) * 57.2958f;
        d.pitch = atan2f(-d.ax, sqrtf(d.ay*d.ay + d.az*d.az)) * 57.2958f;
        d.ok = true;
        return d;
    }

    WyIMUData last() { return _last; }

    /* Calibrate offsets — place sensor flat, call this once */
    void calibrate(uint16_t samples = 200) {
        Serial.println("[MPU6050] calibrating — keep flat and still...");
        float ax=0,ay=0,az=0,gx=0,gy=0,gz=0;
        for (uint16_t i = 0; i < samples; i++) {
            WyIMUData d = readIMU();
            ax+=d.ax; ay+=d.ay; az+=d.az;
            gx+=d.gx; gy+=d.gy; gz+=d.gz;
            delay(5);
        }
        Serial.printf("[MPU6050] accel bias: %.3f %.3f %.3f\n", ax/samples, ay/samples, az/samples);
        Serial.printf("[MPU6050] gyro bias:  %.3f %.3f %.3f\n", gx/samples, gy/samples, gz/samples);
    }

private:
    WyI2CPins _pins;
    uint8_t   _accelCfg, _gyroCfg;
    float     _accelDiv = 8192.0f;
    float     _gyroDiv  = 65.5f;
    WyIMUData _last;

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
