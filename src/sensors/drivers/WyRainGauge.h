/*
 * drivers/WyRainGauge.h — Optical Rain Gauge (RS485/Modbus + Pulse)
 * ==================================================================
 * Target: AliExpress "Dual Channel Output RS485 Pulse" optical rain sensor
 *         0.1mm resolution, infrared optical tipping bucket equivalent
 *
 * Bundled driver — no external library needed.
 *
 * ═══════════════════════════════════════════════════════════════════
 * ABOUT THIS SENSOR
 * ═══════════════════════════════════════════════════════════════════
 * This sensor uses an infrared optical beam that detects individual
 * raindrops falling through it, rather than a physical tipping bucket.
 * Each detected event increments an internal counter. The resolution
 * is 0.1mm of rainfall per count.
 *
 * It has TWO output interfaces — use whichever fits your wiring:
 *   1. RS485 Modbus RTU  — digital, accurate, long cable runs, recommended
 *   2. Pulse output      — one LOW pulse per 0.1mm, simpler but needs care
 *
 * ═══════════════════════════════════════════════════════════════════
 * INTERFACE 1: RS485 / MODBUS RTU
 * ═══════════════════════════════════════════════════════════════════
 * RS485 is a differential serial bus — needs a MAX485 or SP3485 transceiver
 * between the sensor and ESP32.
 *
 * Wiring (with MAX485 module):
 *   Sensor A (RS485+) → MAX485 A
 *   Sensor B (RS485-) → MAX485 B
 *   MAX485 RO         → ESP32 RX pin
 *   MAX485 DI         → ESP32 TX pin
 *   MAX485 DE + RE    → ESP32 DE pin (tied together, HIGH=transmit, LOW=receive)
 *   MAX485 VCC        → 3.3V or 5V
 *   MAX485 GND        → GND
 *   Sensor VCC        → 12V (check your module — some are 5V or 9–24V)
 *   Sensor GND        → GND
 *
 * Modbus RTU frame format:
 *   [device_addr][function][reg_hi][reg_lo][count_hi][count_lo][crc_lo][crc_hi]
 *   CRC: Modbus CRC-16 (poly 0xA001, init 0xFFFF, little-endian)
 *
 * ASSUMED MODBUS REGISTER MAP (reverse-engineered — verify with your unit):
 *   Register 0x0000 — accumulated rainfall (0.1mm per count, uint16)
 *   Register 0x0001 — rainfall intensity (mm/min × 10, uint16) — if supported
 *   Register 0x0002 — sensor status flags
 *   Register 0x0003 — firmware version
 *
 * NOTE: These modules are poorly documented. The register map above is based
 * on common patterns for Chinese RS485 weather sensors. Your unit may differ.
 * Use the probe routine (probeRegisters()) to dump registers 0x00–0x0F and
 * identify what each one contains by correlating with known rainfall.
 *
 * Default Modbus settings (typical, may vary):
 *   Baud: 9600
 *   Data: 8N1
 *   Device address: 0x01
 *
 * ═══════════════════════════════════════════════════════════════════
 * INTERFACE 2: PULSE OUTPUT
 * ═══════════════════════════════════════════════════════════════════
 * One falling-edge pulse per 0.1mm rainfall event.
 * Pulse: typically 100–250ms LOW, then HIGH (open collector or push-pull)
 * MUST use INPUT_PULLUP and count falling edges via interrupt.
 *
 * Wiring (pulse mode):
 *   Sensor PULSE/OUT → ESP32 GPIO (interrupt-capable)
 *   Sensor VCC       → 12V (or per module spec)
 *   Sensor GND       → GND (common with ESP32 GND)
 *
 * ⚠️ Do NOT use polling for pulse — you will miss pulses during heavy rain.
 *    Use attachInterrupt() on FALLING edge.
 *
 * ═══════════════════════════════════════════════════════════════════
 * REGISTERED USE
 * ═══════════════════════════════════════════════════════════════════
 * RS485 mode:
 *   auto* rain = sensors.addUART<WyRainGauge>("rain", TX, RX, 9600);
 *   rain->setDEPin(DE_PIN);
 *   rain->setModbusAddr(0x01);
 *   sensors.begin();
 *
 * Pulse mode:
 *   auto* rain = sensors.addGPIO<WyRainGaugePulse>("rain", PULSE_PIN);
 *   sensors.begin();
 *
 * WySensorData:
 *   d.raw      = accumulated rainfall in mm
 *   d.rawInt   = raw count (counts × 0.1 = mm)
 *   d.voltage  = rainfall rate mm/hr (if available from Modbus)
 *   d.ok       = true when reading succeeded
 */

#pragma once
#include "../WySensors.h"

/* Modbus register addresses — VERIFY THESE WITH YOUR UNIT */
/* Use probeRegisters() to dump and identify */
#define RAIN_REG_ACCUMULATED  0x0000  /* total rainfall × 10 (0.1mm per count) */
#define RAIN_REG_INTENSITY    0x0001  /* mm/min × 10 (may not exist on all units) */
#define RAIN_REG_STATUS       0x0002  /* status flags */
#define RAIN_REG_VERSION      0x0003  /* firmware version */

/* Modbus function codes */
#define MODBUS_FC_READ_HOLDING  0x03
#define MODBUS_FC_WRITE_SINGLE  0x06

/* Typical response timeout for RS485 sensors */
#ifndef WY_RAIN_TIMEOUT_MS
#define WY_RAIN_TIMEOUT_MS  500
#endif

/* ══════════════════════════════════════════════════════════════════
 * RS485 / Modbus RTU variant
 * ══════════════════════════════════════════════════════════════════ */
class WyRainGauge : public WySensorBase {
public:
    WyRainGauge(WyUARTPins pins) : _pins(pins) {}

    void setDEPin(int8_t pin)      { _dePin = pin; }
    void setModbusAddr(uint8_t addr) { _addr = addr; }

    /* Override assumed register map if your unit differs */
    void setAccumReg(uint16_t reg)    { _accumReg = reg; }
    void setIntensityReg(uint16_t reg){ _intensityReg = reg; }

    const char* driverName() override { return "RainGauge-RS485"; }

    bool begin() override {
        Serial2.begin(_pins.baud, SERIAL_8N1, _pins.rx, _pins.tx);
        delay(100);

        if (_dePin >= 0) {
            pinMode(_dePin, OUTPUT);
            digitalWrite(_dePin, LOW);  /* receive mode */
        }

        /* Try reading accumulated register to verify comms */
        uint16_t val;
        if (!_readRegister(_addr, _accumReg, val)) {
            Serial.printf("[RainGauge] no response from addr 0x%02X baud %d\n",
                _addr, _pins.baud);
            Serial.println("[RainGauge] try probeRegisters() to investigate");
            return false;
        }
        Serial.printf("[RainGauge] online — accumulated count: %u (%.1f mm)\n",
            val, val * 0.1f);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        uint16_t accum = 0;
        if (!_readRegister(_addr, _accumReg, accum)) {
            d.error = "Modbus timeout";
            return d;
        }

        d.rawInt = accum;
        d.raw    = accum * 0.1f;  /* mm */

        /* Try intensity register (not all units have this) */
        uint16_t intensity = 0;
        if (_intensityReg != 0xFFFF) {
            if (_readRegister(_addr, _intensityReg, intensity)) {
                d.voltage = intensity * 0.1f;  /* mm/min — reusing voltage field */
            }
        }

        d.ok = true;
        return d;
    }

    /* Reset accumulated rainfall counter
     * NOTE: register write address may differ — probe to find it */
    bool resetAccumulated() {
        return _writeRegister(_addr, _accumReg, 0x0000);
    }

    /* ── Probe / debug ────────────────────────────────────────────── */

    /* Dump registers 0x00–0x0F — use to reverse-engineer your unit.
     * Place sensor in known state (dry, then wet) and watch which
     * registers change. */
    void probeRegisters(uint8_t startReg = 0x00, uint8_t count = 16) {
        Serial.printf("[RainGauge] probing addr 0x%02X registers:\n", _addr);
        for (uint8_t i = 0; i < count; i++) {
            uint16_t val;
            uint8_t reg = startReg + i;
            if (_readRegister(_addr, reg, val)) {
                Serial.printf("  0x%02X = 0x%04X (%u / %.2f)\n",
                    reg, val, val, val * 0.1f);
            } else {
                Serial.printf("  0x%02X = [no response]\n", reg);
            }
            delay(50);
        }
    }

    /* Try common baud rates to find the sensor */
    void probeBaud() {
        const uint32_t bauds[] = {1200, 2400, 4800, 9600, 19200, 38400, 115200};
        Serial.println("[RainGauge] probing baud rates...");
        for (uint8_t i = 0; i < 7; i++) {
            Serial2.end();
            Serial2.begin(bauds[i], SERIAL_8N1, _pins.rx, _pins.tx);
            delay(100);
            uint16_t val;
            if (_readRegister(_addr, 0x00, val)) {
                Serial.printf("[RainGauge] FOUND at %u baud! reg[0]=%u\n",
                    bauds[i], val);
                _pins.baud = bauds[i];
                return;
            }
            Serial.printf("  %u baud — no response\n", bauds[i]);
        }
        Serial.println("[RainGauge] probe failed — check wiring and RS485 transceiver");
    }

    /* Try addresses 0x01–0x10 at current baud */
    void probeAddress() {
        Serial.printf("[RainGauge] scanning addresses at %u baud...\n", _pins.baud);
        for (uint8_t addr = 0x01; addr <= 0x10; addr++) {
            uint16_t val;
            if (_readRegister(addr, 0x00, val)) {
                Serial.printf("[RainGauge] FOUND at address 0x%02X! reg[0]=%u\n", addr, val);
                _addr = addr;
                return;
            }
            delay(20);
        }
        Serial.println("[RainGauge] no device found at addresses 0x01-0x10");
    }

private:
    WyUARTPins _pins;
    int8_t     _dePin         = -1;
    uint8_t    _addr          = 0x01;
    uint16_t   _accumReg      = RAIN_REG_ACCUMULATED;
    uint16_t   _intensityReg  = RAIN_REG_INTENSITY;

    /* ── Modbus RTU low level ─────────────────────────────────────── */

    /* Read single holding register (FC03) */
    bool _readRegister(uint8_t addr, uint16_t reg, uint16_t& val) {
        uint8_t req[8];
        req[0] = addr;
        req[1] = MODBUS_FC_READ_HOLDING;
        req[2] = reg >> 8;
        req[3] = reg & 0xFF;
        req[4] = 0x00;
        req[5] = 0x01;  /* read 1 register */
        uint16_t crc = _crc16(req, 6);
        req[6] = crc & 0xFF;
        req[7] = crc >> 8;

        _transmit(req, 8);

        /* Expect 7-byte response: addr + fc + bytecount + hi + lo + crc×2 */
        uint8_t rsp[8] = {};
        uint8_t len = _receive(rsp, 7);
        if (len < 7) return false;

        /* Validate response */
        if (rsp[0] != addr || rsp[1] != MODBUS_FC_READ_HOLDING) return false;
        uint16_t rcrc = _crc16(rsp, 5);
        if (rsp[5] != (rcrc & 0xFF) || rsp[6] != (rcrc >> 8)) return false;

        val = ((uint16_t)rsp[3] << 8) | rsp[4];
        return true;
    }

    /* Write single holding register (FC06) */
    bool _writeRegister(uint8_t addr, uint16_t reg, uint16_t val) {
        uint8_t req[8];
        req[0] = addr;
        req[1] = MODBUS_FC_WRITE_SINGLE;
        req[2] = reg >> 8;
        req[3] = reg & 0xFF;
        req[4] = val >> 8;
        req[5] = val & 0xFF;
        uint16_t crc = _crc16(req, 6);
        req[6] = crc & 0xFF;
        req[7] = crc >> 8;
        _transmit(req, 8);
        /* Response mirrors the request on success — just check it arrives */
        uint8_t rsp[8] = {};
        return _receive(rsp, 8) == 8;
    }

    void _transmit(uint8_t* buf, uint8_t len) {
        if (_dePin >= 0) digitalWrite(_dePin, HIGH);  /* drive mode */
        delayMicroseconds(100);
        Serial2.write(buf, len);
        Serial2.flush();  /* wait for TX to complete */
        delayMicroseconds(100);
        if (_dePin >= 0) digitalWrite(_dePin, LOW);   /* receive mode */
    }

    uint8_t _receive(uint8_t* buf, uint8_t maxLen) {
        uint32_t deadline = millis() + WY_RAIN_TIMEOUT_MS;
        uint8_t  i = 0;
        while (millis() < deadline && i < maxLen) {
            if (Serial2.available()) buf[i++] = Serial2.read();
            else delayMicroseconds(100);
        }
        return i;
    }

    /* Modbus CRC-16: poly 0xA001, init 0xFFFF, result little-endian */
    uint16_t _crc16(uint8_t* buf, uint8_t len) {
        uint16_t crc = 0xFFFF;
        for (uint8_t i = 0; i < len; i++) {
            crc ^= buf[i];
            for (uint8_t b = 0; b < 8; b++) {
                if (crc & 0x0001) crc = (crc >> 1) ^ 0xA001;
                else              crc >>= 1;
            }
        }
        return crc;
    }
};


/* ══════════════════════════════════════════════════════════════════
 * Pulse output variant
 * One falling-edge pulse per 0.1mm — counted via interrupt
 * ══════════════════════════════════════════════════════════════════ */

/* Global pulse counter — ISR must be a free function */
static volatile uint32_t _wyRainPulseCount = 0;
static void IRAM_ATTR _wyRainISR() { _wyRainPulseCount++; }

class WyRainGaugePulse : public WySensorBase {
public:
    WyRainGaugePulse(WyGPIOPins pins) : _pin(pins.pin) {}

    const char* driverName() override { return "RainGauge-Pulse"; }

    bool begin() override {
        if (_pin < 0) { Serial.println("[RainPulse] pin required"); return false; }
        pinMode(_pin, INPUT_PULLUP);
        /* Attach interrupt on falling edge */
        attachInterrupt(digitalPinToInterrupt(_pin), _wyRainISR, FALLING);
        _wyRainPulseCount = 0;
        _lastCount = 0;
        _startTime = millis();
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        uint32_t count = _wyRainPulseCount;   /* atomic read (32-bit aligned) */
        uint32_t elapsed_ms = millis() - _lastRateTime;

        d.rawInt  = count;
        d.raw     = count * 0.1f;             /* mm accumulated */

        /* Rate: mm/hr from pulses since last read */
        if (elapsed_ms > 0) {
            uint32_t delta = count - _lastCount;
            d.voltage = (delta * 0.1f) / (elapsed_ms / 3600000.0f);  /* mm/hr */
        }

        _lastCount    = count;
        _lastRateTime = millis();
        d.ok = true;
        return d;
    }

    /* Reset accumulated count */
    void reset() {
        noInterrupts();
        _wyRainPulseCount = 0;
        interrupts();
        _lastCount    = 0;
        _lastRateTime = millis();
        _startTime    = millis();
    }

    uint32_t pulseCount()  { return _wyRainPulseCount; }
    float    accumulated() { return _wyRainPulseCount * 0.1f; }  /* mm */
    float    ratePerHour() {
        uint32_t elapsed = millis() - _startTime;
        if (elapsed == 0) return 0;
        return (_wyRainPulseCount * 0.1f) / (elapsed / 3600000.0f);
    }

private:
    int8_t   _pin;
    uint32_t _lastCount    = 0;
    uint32_t _lastRateTime = 0;
    uint32_t _startTime    = 0;
};
