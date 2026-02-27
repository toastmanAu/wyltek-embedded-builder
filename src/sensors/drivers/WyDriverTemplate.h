/*
 * drivers/WyDriverTemplate.h — Template for writing a new sensor driver
 * ======================================================================
 * Copy this file to WyYourSensorName.h and fill in the sections marked TODO.
 * The registry (WySensors.h) handles everything else.
 *
 * DATASHEET QUICK-REFERENCE
 * ─────────────────────────
 * Most sensor datasheets follow one of these patterns:
 *
 * I2C sensors:
 *   1. Write register address (1-2 bytes) to sensor I2C addr
 *   2. Read N bytes back
 *   Common pattern:
 *     Wire.beginTransmission(addr);
 *     Wire.write(REG);          // register address
 *     Wire.endTransmission(false); // false = repeated start (no STOP)
 *     Wire.requestFrom(addr, N);
 *     uint8_t val = Wire.read();
 *
 * SPI sensors:
 *   1. Pull CS LOW
 *   2. Write command byte + read response simultaneously (full-duplex)
 *     SPI.beginTransaction(SPISettings(freq, MSBFIRST, SPI_MODE0));
 *     digitalWrite(cs, LOW);
 *     uint8_t rx = SPI.transfer(txByte);
 *     digitalWrite(cs, HIGH);
 *     SPI.endTransaction();
 *   SPI modes: 0 (CPOL=0,CPHA=0), 1 (0,1), 2 (1,0), 3 (1,1)
 *   Byte order: MSBFIRST or LSBFIRST (check datasheet)
 *
 * GPIO / single-wire:
 *   Direct digitalRead/Write/pinMode operations
 *   Some use timing-sensitive bit-banging (see WyDHT22.h for example)
 *
 * UART sensors:
 *   Serial2.begin(baud, SERIAL_8N1, rx_pin, tx_pin);
 *   Serial2.write(cmd);
 *   while (Serial2.available()) { uint8_t b = Serial2.read(); }
 *
 * REGISTER PATTERNS (from datasheets)
 * ─────────────────────────────────────
 * Reading a 16-bit value MSB first:
 *   uint16_t val = ((uint16_t)buf[0] << 8) | buf[1];
 *
 * Reading a 16-bit value LSB first:
 *   uint16_t val = ((uint16_t)buf[1] << 8) | buf[0];
 *
 * Two's complement signed 16-bit (e.g. temperature):
 *   int16_t val = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
 *
 * 12-bit value in top bits of 2 bytes (e.g. some ADC chips):
 *   uint16_t raw = ((buf[0] << 8) | buf[1]) >> 4;
 *
 * Bit manipulation:
 *   Check bit N:     if (reg & (1 << N))
 *   Set bit N:       reg |= (1 << N)
 *   Clear bit N:     reg &= ~(1 << N)
 *   Get bits 3-0:    uint8_t nibble = reg & 0x0F
 *   Get bits 7-4:    uint8_t nibble = (reg >> 4) & 0x0F
 */

#pragma once
#include "../WySensors.h"

/* ── TODO: Define register addresses from datasheet ─────────────── */
#define MY_SENSOR_REG_WHO_AM_I  0x0F  /* identity register — most sensors have one */
#define MY_SENSOR_REG_CONFIG    0x20
#define MY_SENSOR_REG_DATA      0x28  /* usually burst-read from here */
#define MY_SENSOR_CHIP_ID       0x33  /* expected value of WHO_AM_I */

/* ── TODO: Change class name ────────────────────────────────────── */
class WyDriverTemplate : public WySensorBase {
public:

    /* ── TODO: Choose constructor based on bus type ──────────────── */
    /* I2C:  WyDriverTemplate(WyI2CPins pins)  : _pins(pins) {} */
    /* SPI:  WyDriverTemplate(WySPIPins pins)  : _pins(pins) {} */
    /* GPIO: WyDriverTemplate(WyGPIOPins pins) : _pin(pins.pin) {} */
    /* UART: WyDriverTemplate(WyUARTPins pins) : _uart(pins) {} */
    WyDriverTemplate(WyI2CPins pins) : _pins(pins) {}

    /* ── TODO: Change driver name ────────────────────────────────── */
    const char* driverName() override { return "MY_SENSOR"; }

    /* ── begin() — called once on startup ───────────────────────── */
    bool begin() override {
        /* TODO: Initialise your bus */
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq);

        /* TODO: Verify chip identity (almost every sensor has this) */
        uint8_t id = _readReg8(MY_SENSOR_REG_WHO_AM_I);
        if (id != MY_SENSOR_CHIP_ID) {
            Serial.printf("[MY_SENSOR] wrong ID: 0x%02X (expected 0x%02X)\n",
                id, MY_SENSOR_CHIP_ID);
            return false;
        }

        /* TODO: Configure the sensor (power on, set ODR, resolution, etc.)
         * Check datasheet section "Operating modes" or "Configuration register"
         * Common pattern: write config register to wake up from sleep/power-down
         */
        _writeReg(MY_SENSOR_REG_CONFIG, 0x80);  /* example: power on, continuous mode */
        delay(10);  /* allow sensor to stabilise */

        return true;
    }

    /* ── read() — called to get a measurement ───────────────────── */
    WySensorData read() override {
        WySensorData d;

        /* TODO: Check if data is ready (some sensors have a status/DRDY bit)
         * uint8_t status = _readReg8(REG_STATUS);
         * if (!(status & 0x01)) { d.error = "not ready"; return d; }
         */

        /* TODO: Read raw data registers
         * Most sensors let you burst-read multiple registers starting at base addr
         */
        uint8_t buf[6];
        _readRegBuf(MY_SENSOR_REG_DATA, buf, 6);

        /* TODO: Parse raw bytes into physical values
         * Check datasheet for conversion formula
         * Example for a temperature sensor with 1/16 °C resolution:
         */
        int16_t raw_t = (int16_t)((buf[1] << 8) | buf[0]);
        d.temperature = raw_t / 16.0f;

        /* TODO: Set the fields your sensor provides
         * Available: temperature, humidity, pressure, altitude, light,
         *            co2, distance, voltage, current, weight, raw, rawInt
         */

        d.ok = true;
        return d;
    }

private:
    WyI2CPins _pins;  /* TODO: match your bus type */

    /* ── I2C helpers ─────────────────────────────────────────────── */
    void _writeReg(uint8_t reg, uint8_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.write(val);
        Wire.endTransmission();
    }

    uint8_t _readReg8(uint8_t reg) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);  /* repeated start */
        Wire.requestFrom(_pins.addr, (uint8_t)1);
        return Wire.available() ? Wire.read() : 0xFF;
    }

    uint16_t _readReg16LE(uint8_t reg) {
        uint8_t buf[2] = {0};
        _readRegBuf(reg, buf, 2);
        return (uint16_t)(buf[1] << 8) | buf[0];
    }

    uint16_t _readReg16BE(uint8_t reg) {
        uint8_t buf[2] = {0};
        _readRegBuf(reg, buf, 2);
        return (uint16_t)(buf[0] << 8) | buf[1];
    }

    void _readRegBuf(uint8_t reg, uint8_t* buf, uint8_t len) {
        Wire.beginTransmission(_pins.addr);
        Wire.write(reg);
        Wire.endTransmission(false);
        Wire.requestFrom(_pins.addr, len);
        for (uint8_t i = 0; i < len && Wire.available(); i++)
            buf[i] = Wire.read();
    }

    /* ── SPI helpers (uncomment if using SPI) ───────────────────── */
    /*
    WySPIPins _spi_pins;
    SPIClass  _spi{VSPI};

    void _spiBegin() {
        _spi.begin(_spi_pins.sck, _spi_pins.miso, _spi_pins.mosi);
        pinMode(_spi_pins.cs, OUTPUT);
        digitalWrite(_spi_pins.cs, HIGH);
    }

    uint8_t _spiTransfer(uint8_t cmd) {
        _spi.beginTransaction(SPISettings(_spi_pins.freq, MSBFIRST, SPI_MODE0));
        digitalWrite(_spi_pins.cs, LOW);
        uint8_t rx = _spi.transfer(cmd);
        digitalWrite(_spi_pins.cs, HIGH);
        _spi.endTransaction();
        return rx;
    }

    void _spiReadBuf(uint8_t reg, uint8_t* buf, uint8_t len) {
        _spi.beginTransaction(SPISettings(_spi_pins.freq, MSBFIRST, SPI_MODE0));
        digitalWrite(_spi_pins.cs, LOW);
        _spi.transfer(reg | 0x80); // set read bit (check datasheet — varies by chip)
        for (uint8_t i = 0; i < len; i++) buf[i] = _spi.transfer(0x00);
        digitalWrite(_spi_pins.cs, HIGH);
        _spi.endTransaction();
    }
    */
};
