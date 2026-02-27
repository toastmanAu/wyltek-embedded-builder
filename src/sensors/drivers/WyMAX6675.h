/*
 * drivers/WyMAX6675.h — MAX6675 thermocouple-to-digital (SPI)
 * =============================================================
 * Bundled driver — no external library needed.
 * Reads K-type thermocouple temperature up to 1023.75°C.
 * Registered via WySensors::addSPI<WyMAX6675>("name", mosi, miso, sck, cs)
 *
 * Note: MAX6675 is read-only SPI (MISO only), MOSI not needed but
 * registered for consistency. CS is active LOW.
 */

#pragma once
#include "../WySensors.h"

class WyMAX6675 : public WySensorBase {
public:
    WyMAX6675(WySPIPins pins) : _pins(pins) {}

    const char* driverName() override { return "MAX6675"; }

    bool begin() override {
        _spi.begin(_pins.sck, _pins.miso, _pins.mosi, _pins.cs);
        pinMode(_pins.cs, OUTPUT);
        digitalWrite(_pins.cs, HIGH);
        delay(500);  /* MAX6675 needs 500ms after power-on */
        /* Quick read to verify - if all 0xFFFF probably not connected */
        uint16_t raw = _read16();
        if (raw == 0xFFFF) { return false; }
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        uint16_t raw = _read16();
        if (raw & 0x04) { d.error = "no thermocouple"; return d; }
        d.temperature = (raw >> 3) * 0.25f;
        d.ok = true;
        return d;
    }

private:
    WySPIPins _pins;
    SPIClass  _spi{VSPI};

    uint16_t _read16() {
        digitalWrite(_pins.cs, LOW);
        delayMicroseconds(2);
        uint16_t raw = _spi.transfer16(0x0000);
        digitalWrite(_pins.cs, HIGH);
        delay(1);
        return raw;
    }
};
