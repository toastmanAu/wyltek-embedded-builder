/*
 * drivers/WyDS18B20.h — DS18B20 1-Wire temperature sensor (GPIO)
 * ===============================================================
 * Datasheet: https://www.analog.com/media/en/technical-documentation/data-sheets/ds18b20.pdf
 * Bundled driver — zero external dependencies, bit-banged 1-Wire.
 * Supports multiple sensors on same bus, parasitic power mode.
 * Registered via WySensors::addGPIO<WyDS18B20>("name", pin)
 *
 * Multiple sensors on one pin:
 *   addGPIO<WyDS18B20>("water",   4)    // first sensor found (index 0)
 *   addGPIO<WyDS18B20>("ambient", 4, 1) // pin2 = index 1 (second found)
 *   addGPIO<WyDS18B20>("inlet",   4, 2) // pin2 = index 2 (third found)
 *
 * Non-blocking (start conversion, read later):
 *   ds->startConversion();
 *   delay(750);                          // or do other work
 *   float t = ds->readTemperature();
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 * Standard 3-wire (recommended):
 *   GND (black) → GND
 *   VDD (red)   → 3.3V
 *   DQ  (yellow/white) → ESP32 GPIO + 4.7kΩ pullup to 3.3V
 *
 * ⚠️ The 4.7kΩ pull-up resistor is mandatory. Without it the bus
 *    stays LOW and no communication is possible.
 *
 * Long runs (>10m): use 2.2kΩ pullup.
 * Many sensors (>10): add second 4.7kΩ in parallel (effective 2.35kΩ).
 * Parasitic 2-wire: connect GND+DQ, leave VDD unconnected. Works but
 *    limits cable length and sensor count. Avoid on new designs.
 */

#pragma once
#include "../WySensors.h"

/* DS18B20 1-Wire ROM commands */
#define OW_CMD_SEARCH_ROM    0xF0
#define OW_CMD_READ_ROM      0x33
#define OW_CMD_MATCH_ROM     0x55
#define OW_CMD_SKIP_ROM      0xCC  /* broadcast to all devices */
#define OW_CMD_ALARM_SEARCH  0xEC

/* DS18B20 function commands */
#define DS18B20_CMD_CONVERT_T    0x44
#define DS18B20_CMD_WRITE_SCPAD  0x4E
#define DS18B20_CMD_READ_SCPAD   0xBE
#define DS18B20_CMD_COPY_SCPAD   0x48
#define DS18B20_CMD_RECALL_E2    0xB8
#define DS18B20_CMD_READ_POWER   0xB4

/* Resolution config register values */
#define DS18B20_RES_9BIT   0x1F  /* 93.75ms,  ±0.5°C  */
#define DS18B20_RES_10BIT  0x3F  /* 187.5ms,  ±0.25°C */
#define DS18B20_RES_11BIT  0x5F  /* 375ms,    ±0.125°C */
#define DS18B20_RES_12BIT  0x7F  /* 750ms,    ±0.0625°C — default */

/* Conversion time for each resolution (ms) */
static const uint16_t DS18B20_CONV_MS[] = { 94, 188, 375, 750 };

class WyDS18B20 : public WySensorBase {
public:
    /* pin2 = sensor index on multi-sensor bus (0=first, 1=second, etc.) */
    WyDS18B20(WyGPIOPins pins, uint8_t res = DS18B20_RES_12BIT)
        : _pin(pins.pin), _index((uint8_t)(pins.pin2 < 0 ? 0 : pins.pin2)), _res(res) {}

    const char* driverName() override { return "DS18B20"; }

    /* Set resolution: 9, 10, 11, or 12 bits */
    void setResolution(uint8_t bits) {
        switch (bits) {
            case 9:  _res = DS18B20_RES_9BIT;  break;
            case 10: _res = DS18B20_RES_10BIT; break;
            case 11: _res = DS18B20_RES_11BIT; break;
            default: _res = DS18B20_RES_12BIT; break;
        }
    }

    /* Set alarm thresholds (stored in sensor EEPROM, persist power-off).
     * Alarm search (0xEC ROM command) finds sensors outside TH/TL range.
     * th = high alarm °C (int8_t, -55 to +125)
     * tl = low alarm °C */
    bool setAlarm(int8_t thHigh, int8_t tlLow) {
        if (!_reset()) return false;
        _selectDevice();
        _writeByte(DS18B20_CMD_WRITE_SCPAD);
        _writeByte((uint8_t)thHigh);
        _writeByte((uint8_t)tlLow);
        _writeByte(_res);
        /* Copy to EEPROM */
        if (!_reset()) return false;
        _selectDevice();
        _writeByte(DS18B20_CMD_COPY_SCPAD);
        delay(10);  /* EEPROM write time */
        return true;
    }

    /* Print 64-bit ROM address (useful for identifying sensors on a bus) */
    void printROM(Stream& s = Serial) {
        if (!_hasROM) { s.println("[DS18B20] no ROM stored (index 0)"); return; }
        s.printf("[DS18B20] ROM: %02X:%02X:%02X:%02X:%02X:%02X:%02X:%02X\n",
            _rom[0],_rom[1],_rom[2],_rom[3],_rom[4],_rom[5],_rom[6],_rom[7]);
    }

    /* Read ROM directly (single sensor on bus — faster than search) */
    bool readROM() {
        if (!_reset()) return false;
        _writeByte(OW_CMD_READ_ROM);
        for (int i = 0; i < 8; i++) _rom[i] = _readByte();
        if (_crc8(_rom, 7) != _rom[7]) return false;
        _hasROM = true;
        return true;
    }

    bool begin() override {
        pinMode(_pin, INPUT);
        if (!_reset()) {
            Serial.printf("[DS18B20] no presence pulse on pin %d\n", _pin);
            return false;
        }
        /* For index 0 on a single-sensor bus, use SKIP ROM for speed.
         * For index > 0, must find and store the specific ROM address. */
        if (_index == 0) {
            /* Check if single sensor — use READ ROM (faster than search) */
            bool single = readROM();
            if (!single) {
                /* Multiple devices or CRC fail — fall back to SKIP ROM */
                _hasROM = false;
            }
        } else {
            if (!_findDevice(_index)) {
                Serial.printf("[DS18B20] sensor index %u not found on pin %d\n", _index, _pin);
                return false;
            }
            _hasROM = true;
        }
        _writeConfig();
        Serial.printf("[DS18B20] ready on pin %d, index %u, res=%dbit%s\n",
            _pin, _index, _resBits(), _hasROM ? "" : " (SKIP_ROM)");
        return true;
    }

    /* ── Blocking read (starts conversion + waits + returns temp) ── */
    WySensorData read() override {
        WySensorData d;
        startConversion();
        uint32_t deadline = millis() + _convMs() + 50;
        /* Busy-wait on conversion complete bit */
        while (!_readBit() && millis() < deadline) yield();
        if (millis() >= deadline) { d.error = "conversion timeout"; return d; }
        float t = readTemperature();
        if (isnan(t)) { d.error = "CRC fail"; return d; }
        d.temperature = t;
        d.ok = true;
        return d;
    }

    /* ── Non-blocking API ─────────────────────────────────────────── */

    /* Start temperature conversion. Returns immediately.
     * Wait conversionMs() before calling readTemperature(). */
    bool startConversion() {
        if (!_reset()) return false;
        _selectDevice();
        _writeByte(DS18B20_CMD_CONVERT_T);
        return true;
    }

    /* Conversion time in ms for current resolution */
    uint16_t conversionMs() { return _convMs(); }

    /* Read scratchpad after conversion completes. Returns NAN on error. */
    float readTemperature() {
        if (!_reset()) return NAN;
        _selectDevice();
        _writeByte(DS18B20_CMD_READ_SCPAD);
        uint8_t sp[9];
        for (uint8_t i = 0; i < 9; i++) sp[i] = _readByte();
        if (_crc8(sp, 8) != sp[8]) return NAN;
        int16_t raw = (int16_t)((sp[1] << 8) | sp[0]);
        /* Mask unused bits for lower resolutions */
        switch (_res) {
            case DS18B20_RES_9BIT:  raw &= ~0x07; break;
            case DS18B20_RES_10BIT: raw &= ~0x03; break;
            case DS18B20_RES_11BIT: raw &= ~0x01; break;
            default: break;
        }
        return raw / 16.0f;
    }

    /* Check if conversion is done (poll instead of fixed delay) */
    bool conversionDone() { return _readBit(); }

    /* Convenience: start all sensors on bus simultaneously (SKIP ROM broadcast)
     * then read each by ROM address. Call on any instance. */
    bool startConversionAll() {
        if (!_reset()) return false;
        _writeByte(OW_CMD_SKIP_ROM);
        _writeByte(DS18B20_CMD_CONVERT_T);
        return true;
    }

private:
    int8_t  _pin;
    uint8_t _index;
    uint8_t _res;
    uint8_t _rom[8];
    bool    _hasROM = false;

    void _selectDevice() {
        if (_hasROM) {
            _writeByte(OW_CMD_MATCH_ROM);
            for (int i = 0; i < 8; i++) _writeByte(_rom[i]);
        } else {
            _writeByte(OW_CMD_SKIP_ROM);
        }
    }

    void _writeConfig() {
        if (!_reset()) return;
        _selectDevice();
        _writeByte(DS18B20_CMD_WRITE_SCPAD);
        _writeByte(0x00);  /* TH — alarm high (unused, set 0) */
        _writeByte(0x00);  /* TL — alarm low */
        _writeByte(_res);  /* configuration register */
    }

    uint8_t _resBits() {
        switch (_res) {
            case DS18B20_RES_9BIT:  return 9;
            case DS18B20_RES_10BIT: return 10;
            case DS18B20_RES_11BIT: return 11;
            default:                return 12;
        }
    }

    uint16_t _convMs() {
        switch (_res) {
            case DS18B20_RES_9BIT:  return DS18B20_CONV_MS[0];
            case DS18B20_RES_10BIT: return DS18B20_CONV_MS[1];
            case DS18B20_RES_11BIT: return DS18B20_CONV_MS[2];
            default:                return DS18B20_CONV_MS[3];
        }
    }

    /* 1-Wire search algorithm (Dallas/Maxim AN 27) — finds Nth device */
    bool _findDevice(uint8_t targetIdx) {
        uint8_t rom[8]   = {};
        uint8_t lastDisc = 0;
        uint8_t found    = 0;
        bool    lastDev  = false;

        while (!lastDev) {
            if (!_reset()) return false;
            _writeByte(OW_CMD_SEARCH_ROM);

            uint8_t lastZero  = 0;
            uint8_t idBitNum  = 1;

            for (uint8_t byteIdx = 0; byteIdx < 8; byteIdx++) {
                for (uint8_t bit = 0; bit < 8; bit++, idBitNum++) {
                    bool idBit    = _readBit();
                    bool cmpIdBit = _readBit();

                    if (idBit && cmpIdBit) return false;  /* no devices */

                    bool dir;
                    if (!idBit && !cmpIdBit) {
                        /* Discrepancy */
                        if (idBitNum == lastDisc)       dir = true;
                        else if (idBitNum > lastDisc)   { dir = false; lastZero = idBitNum; }
                        else                             dir = (rom[byteIdx] >> bit) & 1;
                        if (!dir) lastZero = idBitNum;
                    } else {
                        dir = idBit;
                    }

                    if (dir) rom[byteIdx] |=  (1 << bit);
                    else     rom[byteIdx] &= ~(1 << bit);
                    _writeBit(dir);
                }
            }

            lastDisc = lastZero;
            lastDev  = (lastDisc == 0);

            /* CRC check on found ROM */
            if (_crc8(rom, 7) != rom[7]) return false;

            if (found == targetIdx) {
                memcpy(_rom, rom, 8);
                return true;
            }
            found++;
        }
        return false;  /* index out of range */
    }

    /* ── 1-Wire low-level bit-bang ──────────────────────────────── */

    bool _reset() {
        noInterrupts();
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        interrupts();
        delayMicroseconds(480);
        noInterrupts();
        pinMode(_pin, INPUT);
        delayMicroseconds(70);
        bool presence = !digitalRead(_pin);
        interrupts();
        delayMicroseconds(410);
        return presence;
    }

    void _writeBit(bool b) {
        noInterrupts();
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        delayMicroseconds(b ? 10 : 65);
        digitalWrite(_pin, HIGH);
        pinMode(_pin, INPUT);
        interrupts();
        delayMicroseconds(b ? 55 : 5);
    }

    bool _readBit() {
        noInterrupts();
        pinMode(_pin, OUTPUT);
        digitalWrite(_pin, LOW);
        delayMicroseconds(3);
        pinMode(_pin, INPUT);
        delayMicroseconds(10);
        bool b = digitalRead(_pin);
        interrupts();
        delayMicroseconds(50);
        return b;
    }

    void _writeByte(uint8_t b) {
        for (uint8_t i = 0; i < 8; i++) { _writeBit(b & 1); b >>= 1; }
    }

    uint8_t _readByte() {
        uint8_t b = 0;
        for (uint8_t i = 0; i < 8; i++) if (_readBit()) b |= (1 << i);
        return b;
    }

    /* Dallas/Maxim 1-Wire CRC-8 (polynomial 0x31 = x^8+x^5+x^4+1) */
    uint8_t _crc8(const uint8_t* data, uint8_t len) {
        uint8_t crc = 0;
        for (uint8_t i = 0; i < len; i++) {
            uint8_t b = data[i];
            for (uint8_t j = 0; j < 8; j++) {
                if ((crc ^ b) & 1) crc = (crc >> 1) ^ 0x8C;
                else               crc >>= 1;
                b >>= 1;
            }
        }
        return crc;
    }
};
