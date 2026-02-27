/*
 * drivers/WyLD2410.h — HLK-LD2410 / LD2410B / LD2410C mmWave Presence Sensor (UART)
 * ===================================================================================
 * Manufacturer: Hi-Link (Shenzhen HLK)
 * Variants: LD2410, LD2410B, LD2410C (same UART protocol, C adds Bluetooth)
 *
 * Bundled driver — no external library needed.
 * Registered via WySensors::addUART<WyLD2410>("presence", TX, RX, 256000)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT DOES
 * ═══════════════════════════════════════════════════════════════════
 * 24GHz FMCW (Frequency Modulated Continuous Wave) millimetre-wave radar.
 * Detects BOTH:
 *   - Moving targets (any motion — walking, waving, breathing fast)
 *   - Stationary targets (micro-motion — breathing, heartbeat, slight movement)
 *
 * This is the key difference from PIR:
 *   PIR:    detects gross movement, misses still people
 *   LD2410: detects that someone is SITTING IN A CHAIR breathing
 *
 * Output (from periodic data frames):
 *   - Target state: no target / moving only / stationary only / both
 *   - Moving target distance (cm) + energy (0–100)
 *   - Stationary target distance (cm) + energy (0–100)
 *   - Detection distance (cm) — whichever is closer
 *
 * 8 configurable detection gates (0.75m each, gate 0 = 0–0.75m ... gate 7 = 5.25–6m)
 * Per-gate sensitivity thresholds for both moving and stationary detection.
 *
 * ═══════════════════════════════════════════════════════════════════
 * UART PROTOCOL
 * ═══════════════════════════════════════════════════════════════════
 * Baud: 256000 (fixed — not configurable)
 *
 * Data frames (output continuously while in normal mode):
 *   [F4 F3 F2 F1][len 2B LE][data type 2B][target state 1B]
 *   [moving dist 2B LE][moving energy 1B]
 *   [stationary dist 2B LE][stationary energy 1B]
 *   [detection dist 2B LE]
 *   [F8 F7 F6 F5]
 *
 * Command frames (configuration mode):
 *   Header: [FD FC FB FA]
 *   Footer: [04 03 02 01]
 *   Command: [len 2B LE][cmd word 2B LE][data NB][ack word or empty]
 *
 * Target state byte:
 *   0x00 = no target
 *   0x01 = moving target only
 *   0x02 = stationary target only
 *   0x03 = both moving and stationary targets
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 *   LD2410 VCC  → 5V (sensor is 5V, but UART TX is 3.3V compatible)
 *   LD2410 GND  → GND
 *   LD2410 TX   → ESP32 RX pin
 *   LD2410 RX   → ESP32 TX pin (3.3V logic OK — internal level tolerant)
 *   LD2410 OUT  → optional GPIO (digital presence output — HIGH when presence detected)
 *
 * ⚠️ POWER: LD2410 needs 5V, draws up to 200mA during radar burst. Do NOT
 *    power from ESP32 3V3 pin. Use USB 5V or a dedicated 5V rail.
 *    Ground must be common with ESP32.
 *
 * ⚠️ BAUD RATE: 256000 is non-standard. Works fine with ESP32 hardware UART
 *    (Serial1/Serial2) but may fail with software serial. Always use hardware UART.
 *
 * ⚠️ PLACEMENT: The sensor sees through non-metallic materials (plastic, wood,
 *    drywall, glass). Mount behind a thin panel if aesthetics matter.
 *    Keep away from fans/HVAC — air movement can cause false positives.
 *    Minimum detection distance: ~0.75m (gate 0 sensitivity should be 0 in most cases).
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   auto* pres = sensors.addUART<WyLD2410>("presence", TX, RX, 256000);
 *   // Optional: add OUT pin for instant interrupt-driven detection
 *   pres->setOutPin(OUT_PIN);
 *   sensors.begin();
 *
 *   // In loop — parse incoming data stream:
 *   WySensorData d = sensors.read("presence");
 *   if (d.ok) {
 *       bool present = (d.rawInt > 0);           // anyone present?
 *       bool moving  = (d.rawInt & 0x01);        // moving target?
 *       bool still   = (d.rawInt & 0x02);        // stationary target?
 *       float dist_cm = d.raw;                    // detection distance
 *       float energy  = d.voltage;                // detection energy 0–100
 *   }
 *
 * Configuration (call before begin(), or in config mode):
 *   pres->setMaxGate(5);                          // detect up to gate 5 = 4.5m
 *   pres->setGateSensitivity(2, 40, 40);          // gate 2: move thresh 40, still thresh 40
 *   pres->setNoOneDuration(5);                    // 5s hold-off after target disappears
 *
 * WySensorData:
 *   d.ok      = true when a valid data frame was received
 *   d.rawInt  = target state: 0=none, 1=moving, 2=stationary, 3=both
 *   d.raw     = detection distance (cm) — closest detected target
 *   d.voltage = detection energy (0–100) — signal strength of detected target
 *   d.temperature = moving target energy (useful for debugging)
 *   d.humidity    = stationary target energy
 */

#pragma once
#include "../WySensors.h"

/* Frame headers/footers */
static const uint8_t LD2410_DATA_HEADER[4] = {0xF4, 0xF3, 0xF2, 0xF1};
static const uint8_t LD2410_DATA_FOOTER[4] = {0xF8, 0xF7, 0xF6, 0xF5};
static const uint8_t LD2410_CMD_HEADER[4]  = {0xFD, 0xFC, 0xFB, 0xFA};
static const uint8_t LD2410_CMD_FOOTER[4]  = {0x04, 0x03, 0x02, 0x01};

/* Command words */
#define LD2410_CMD_ENTER_CONFIG   0x00FF
#define LD2410_CMD_EXIT_CONFIG    0x00FE
#define LD2410_CMD_SET_MAX_GATE   0x0060
#define LD2410_CMD_SET_GATE_SENS  0x0064
#define LD2410_CMD_GET_PARAMS     0x0061
#define LD2410_CMD_SET_NO_ONE_DUR 0x0060
#define LD2410_CMD_RESTART        0x00A3
#define LD2410_CMD_FACTORY_RESET  0x00A2
#define LD2410_CMD_READ_FIRMWARE  0x00A0

/* Target state */
#define LD2410_NO_TARGET    0x00
#define LD2410_MOVING       0x01
#define LD2410_STATIONARY   0x02
#define LD2410_BOTH         0x03

/* Data type byte in data frames */
#define LD2410_BASIC_DATA   0x0200  /* normal report */
#define LD2410_ENG_DATA     0x0100  /* engineering mode report (per-gate energy) */

/* Max number of detection gates */
#define LD2410_MAX_GATES    8

/* Timeout for command ACK */
#ifndef WY_LD2410_ACK_TIMEOUT_MS
#define WY_LD2410_ACK_TIMEOUT_MS  500
#endif

/* Parsed frame result */
struct LD2410Frame {
    uint8_t  targetState;    /* 0=none 1=moving 2=still 3=both */
    uint16_t movingDist;     /* cm */
    uint8_t  movingEnergy;   /* 0–100 */
    uint16_t stillDist;      /* cm */
    uint8_t  stillEnergy;    /* 0–100 */
    uint16_t detectionDist;  /* cm — nearest detected target */
    bool     valid;
};

class WyLD2410 : public WySensorBase {
public:
    WyLD2410(WyUARTPins pins) : _pins(pins) {}

    const char* driverName() override { return "LD2410C"; }

    /* Optional OUT pin — digital HIGH when presence detected */
    void setOutPin(int8_t pin) { _outPin = pin; }

    /* Configuration — call before sensors.begin() or after entering config mode */
    void setMaxGate(uint8_t movingGate, uint8_t stillGate = 0xFF) {
        _cfgMaxMoving = movingGate;
        _cfgMaxStill  = (stillGate == 0xFF) ? movingGate : stillGate;
    }

    /* Set sensitivity for one gate (0–7)
     * movingSens/stillSens: 0–100. Default: 50/20 for most gates, 0/0 for gate 0 */
    void setGateSensitivity(uint8_t gate, uint8_t movingSens, uint8_t stillSens) {
        if (gate >= LD2410_MAX_GATES) return;
        _cfgGateMoving[gate] = movingSens;
        _cfgGateStill[gate]  = stillSens;
        _cfgDirty = true;
    }

    /* Time (seconds) to hold detection after target disappears (1–32767s) */
    void setNoOneDuration(uint16_t seconds) { _cfgNoOneDur = seconds; }

    /* ── WySensorBase interface ───────────────────────────────────── */

    bool begin() override {
        Serial2.begin(_pins.baud, SERIAL_8N1, _pins.rx, _pins.tx);
        delay(100);

        if (_outPin >= 0) {
            pinMode(_outPin, INPUT);
        }

        /* Flush startup noise */
        uint32_t flush = millis() + 500;
        while (millis() < flush) { if (Serial2.available()) Serial2.read(); }

        /* Push configuration if any was set */
        if (_cfgDirty || _cfgMaxMoving != 0xFF || _cfgNoOneDur != 0xFFFF) {
            if (!_applyConfig()) {
                Serial.println("[LD2410] config failed — using defaults");
            }
        }

        /* Verify we get data frames */
        uint32_t deadline = millis() + 2000;
        while (millis() < deadline) {
            LD2410Frame f = _readFrame(200);
            if (f.valid) {
                Serial.printf("[LD2410] online — state:%u dist:%ucm\n",
                    f.targetState, f.detectionDist);
                return true;
            }
        }
        Serial.println("[LD2410] no data frames received — check baud (256000) and wiring");
        return false;
    }

    WySensorData read() override {
        WySensorData d;

        /* Fast path: OUT pin available */
        if (_outPin >= 0 && !digitalRead(_outPin)) {
            /* OUT is LOW = no presence — skip UART parse */
            d.rawInt = LD2410_NO_TARGET;
            d.raw    = 0;
            d.ok     = true;
            return d;
        }

        LD2410Frame f = _readFrame(100);  /* 100ms = ~1 frame at normal rate */
        if (!f.valid) {
            d.error = "no frame";
            return d;
        }

        d.rawInt      = f.targetState;
        d.raw         = f.detectionDist;
        d.voltage     = (f.targetState & LD2410_MOVING)     ? f.movingEnergy : f.stillEnergy;
        d.temperature = f.movingEnergy;   /* debug: per-target energies */
        d.humidity    = f.stillEnergy;
        d.ok          = true;
        return d;
    }

    /* ── Direct accessors ────────────────────────────────────────── */

    bool    presence()        { return _last.targetState != LD2410_NO_TARGET; }
    bool    movingPresence()  { return _last.targetState & LD2410_MOVING; }
    bool    stillPresence()   { return _last.targetState & LD2410_STATIONARY; }
    uint16_t movingDist()     { return _last.movingDist; }
    uint16_t stillDist()      { return _last.stillDist; }
    uint8_t  movingEnergy()   { return _last.movingEnergy; }
    uint8_t  stillEnergy()    { return _last.stillEnergy; }
    uint16_t detectionDist()  { return _last.detectionDist; }

    /* ── Configuration commands ──────────────────────────────────── */

    bool restart() {
        if (!_enterConfig()) return false;
        _sendCmd(LD2410_CMD_RESTART, nullptr, 0);
        delay(1000);  /* sensor reboots */
        return true;
    }

    bool factoryReset() {
        if (!_enterConfig()) return false;
        bool ok = _sendCmdAck(LD2410_CMD_FACTORY_RESET, nullptr, 0);
        _exitConfig();
        return ok;
    }

    /* Read firmware version string into buf (at least 16 chars) */
    bool getFirmwareVersion(char* buf, uint8_t bufLen) {
        if (!_enterConfig()) return false;
        _sendCmd(LD2410_CMD_READ_FIRMWARE, nullptr, 0);
        uint8_t rsp[32] = {};
        uint8_t rlen = _readAck(rsp, sizeof(rsp));
        _exitConfig();
        if (rlen >= 12) {
            /* Bytes 8–11: major.minor.patch.build */
            snprintf(buf, bufLen, "V%u.%u.%u.%u", rsp[8], rsp[9], rsp[10], rsp[11]);
            return true;
        }
        return false;
    }

    /* Dump current configuration to Serial — useful for debugging */
    void dumpConfig() {
        if (!_enterConfig()) { Serial.println("[LD2410] config enter failed"); return; }
        _sendCmd(LD2410_CMD_GET_PARAMS, nullptr, 0);
        uint8_t rsp[64] = {};
        uint8_t rlen = _readAck(rsp, sizeof(rsp));
        _exitConfig();
        if (rlen >= 20) {
            Serial.printf("[LD2410] max moving gate: %u  max still gate: %u\n", rsp[8], rsp[9]);
            Serial.printf("[LD2410] no-one duration: %us\n", rsp[20] | (rsp[21] << 8));
            for (int g = 0; g < LD2410_MAX_GATES; g++) {
                if (10 + g < rlen && 18 + g < rlen)
                    Serial.printf("[LD2410]   gate %u: move=%u still=%u\n",
                        g, rsp[10 + g], rsp[18 + g]);
            }
        } else {
            Serial.printf("[LD2410] get params: only %u bytes received\n", rlen);
        }
    }

private:
    WyUARTPins _pins;
    int8_t     _outPin       = -1;
    LD2410Frame _last        = {};

    /* Pending config */
    uint8_t  _cfgMaxMoving   = 0xFF;  /* 0xFF = don't set */
    uint8_t  _cfgMaxStill    = 0xFF;
    uint16_t _cfgNoOneDur    = 0xFFFF;
    uint8_t  _cfgGateMoving[LD2410_MAX_GATES] = {0, 50, 50, 40, 40, 30, 30, 20};
    uint8_t  _cfgGateStill[LD2410_MAX_GATES]  = {0, 20, 20, 20, 20, 20, 20, 20};
    bool     _cfgDirty       = false;

    /* ── Frame parsing ────────────────────────────────────────────── */

    LD2410Frame _readFrame(uint16_t timeoutMs) {
        LD2410Frame f = {};
        uint32_t deadline = millis() + timeoutMs;

        /* Look for header bytes F4 F3 F2 F1 */
        uint8_t  hdrMatch = 0;
        while (millis() < deadline && hdrMatch < 4) {
            if (!Serial2.available()) { delayMicroseconds(100); continue; }
            uint8_t b = Serial2.read();
            if (b == LD2410_DATA_HEADER[hdrMatch]) hdrMatch++;
            else hdrMatch = (b == LD2410_DATA_HEADER[0]) ? 1 : 0;
        }
        if (hdrMatch < 4) return f;

        /* Read data length (2 bytes LE) */
        uint8_t lenBuf[2];
        if (!_readBytes(lenBuf, 2, deadline)) return f;
        uint16_t dataLen = lenBuf[0] | ((uint16_t)lenBuf[1] << 8);
        if (dataLen < 13 || dataLen > 64) return f;  /* sanity */

        /* Read payload */
        uint8_t payload[64] = {};
        if (!_readBytes(payload, dataLen, deadline)) return f;

        /* Read footer */
        uint8_t footer[4];
        if (!_readBytes(footer, 4, deadline)) return f;
        if (memcmp(footer, LD2410_DATA_FOOTER, 4) != 0) return f;

        /* Parse data type — expect basic data (0x02 0x00) */
        /* payload[0]=data type lo, payload[1]=data type hi */
        if (payload[0] != 0x02) return f;  /* not basic data frame */
        if (payload[2] != 0xAA) return f;  /* target data marker */

        /* Target state */
        f.targetState   = payload[3];
        f.movingDist    = payload[4]  | ((uint16_t)payload[5]  << 8);
        f.movingEnergy  = payload[6];
        f.stillDist     = payload[7]  | ((uint16_t)payload[8]  << 8);
        f.stillEnergy   = payload[9];
        f.detectionDist = payload[10] | ((uint16_t)payload[11] << 8);
        /* payload[12] = 0x55 (end marker) */

        _last  = f;
        f.valid = true;
        return f;
    }

    /* ── Configuration commands ──────────────────────────────────── */

    bool _applyConfig() {
        if (!_enterConfig()) return false;

        /* Set max detection gate + no-one duration */
        if (_cfgMaxMoving != 0xFF) {
            uint8_t data[18] = {};
            /* Word 0: max moving gate word, word 1: value, word 2: max still gate word,
             * word 3: value, word 4: no-one duration word, word 5: value */
            data[0] = 0x00; data[1] = 0x00;  /* word index: max moving gate = 0 */
            data[2] = _cfgMaxMoving; data[3] = 0;
            data[4] = 0x01; data[5] = 0x00;  /* word index: max still gate = 1 */
            data[6] = _cfgMaxStill; data[7] = 0;
            data[8] = 0x02; data[9] = 0x00;  /* word index: no-one duration = 2 */
            uint16_t dur = (_cfgNoOneDur != 0xFFFF) ? _cfgNoOneDur : 5;
            data[10] = dur & 0xFF; data[11] = (dur >> 8) & 0xFF;
            _sendCmdAck(LD2410_CMD_SET_MAX_GATE, data, 12);
        }

        /* Set per-gate sensitivity */
        if (_cfgDirty) {
            for (uint8_t g = 0; g < LD2410_MAX_GATES; g++) {
                uint8_t data[6] = {};
                data[0] = 0x00; data[1] = 0x00;  /* word index: gate num */
                data[2] = g;    data[3] = 0x00;
                data[4] = 0x01; data[5] = 0x00;  /* word index: moving sens */
                uint8_t gdata[10] = {
                    0x00, 0x00, g, 0x00,
                    0x01, 0x00, _cfgGateMoving[g], 0x00,
                    0x02, 0x00
                };
                /* append still sens */
                uint8_t fulldata[12];
                memcpy(fulldata, gdata, 10);
                fulldata[10] = _cfgGateStill[g];
                fulldata[11] = 0x00;
                _sendCmdAck(LD2410_CMD_SET_GATE_SENS, fulldata, 12);
            }
        }

        return _exitConfig();
    }

    bool _enterConfig() {
        /* Flush pending data frames */
        uint32_t flush = millis() + 100;
        while (millis() < flush) { if (Serial2.available()) Serial2.read(); }

        uint8_t data[2] = {0x01, 0x00};  /* protocol version = 1 */
        _sendCmd(LD2410_CMD_ENTER_CONFIG, data, 2);
        uint8_t rsp[16] = {};
        uint8_t rlen = _readAck(rsp, sizeof(rsp));
        if (rlen < 4) return false;
        /* ACK: bytes 4–5 = 0xFF 0x00 = success */
        return (rsp[4] == 0xFF && rsp[5] == 0x00) || (rsp[6] == 0x00);
    }

    bool _exitConfig() {
        _sendCmd(LD2410_CMD_EXIT_CONFIG, nullptr, 0);
        uint8_t rsp[8] = {};
        _readAck(rsp, sizeof(rsp));
        delay(50);
        return true;
    }

    void _sendCmd(uint16_t cmd, uint8_t* data, uint8_t dataLen) {
        /* [FD FC FB FA][len_lo len_hi][cmd_lo cmd_hi][data...][04 03 02 01] */
        uint16_t len = 2 + dataLen;  /* cmd word + data */
        Serial2.write(LD2410_CMD_HEADER, 4);
        Serial2.write((uint8_t)(len & 0xFF));
        Serial2.write((uint8_t)(len >> 8));
        Serial2.write((uint8_t)(cmd & 0xFF));
        Serial2.write((uint8_t)(cmd >> 8));
        if (data && dataLen) Serial2.write(data, dataLen);
        Serial2.write(LD2410_CMD_FOOTER, 4);
        Serial2.flush();
    }

    bool _sendCmdAck(uint16_t cmd, uint8_t* data, uint8_t dataLen) {
        _sendCmd(cmd, data, dataLen);
        uint8_t rsp[32] = {};
        return _readAck(rsp, sizeof(rsp)) > 0;
    }

    /* Read ACK command frame — returns payload bytes read */
    uint8_t _readAck(uint8_t* buf, uint8_t maxLen) {
        uint32_t deadline = millis() + WY_LD2410_ACK_TIMEOUT_MS;
        /* Find ACK header FD FC FB FA */
        uint8_t hdrMatch = 0;
        while (millis() < deadline && hdrMatch < 4) {
            if (!Serial2.available()) { delayMicroseconds(200); continue; }
            uint8_t b = Serial2.read();
            if (b == LD2410_CMD_HEADER[hdrMatch]) hdrMatch++;
            else hdrMatch = (b == LD2410_CMD_HEADER[0]) ? 1 : 0;
        }
        if (hdrMatch < 4) return 0;

        /* Length */
        uint8_t lenBuf[2];
        if (!_readBytes(lenBuf, 2, deadline)) return 0;
        uint16_t payLen = lenBuf[0] | ((uint16_t)lenBuf[1] << 8);
        if (payLen > maxLen) payLen = maxLen;

        if (!_readBytes(buf, payLen, deadline)) return 0;

        /* Consume footer */
        uint8_t footer[4];
        _readBytes(footer, 4, deadline);

        return (uint8_t)payLen;
    }

    bool _readBytes(uint8_t* buf, uint16_t count, uint32_t deadline) {
        uint16_t i = 0;
        while (i < count && millis() < deadline) {
            if (Serial2.available()) buf[i++] = Serial2.read();
            else delayMicroseconds(200);
        }
        return (i == count);
    }
};
