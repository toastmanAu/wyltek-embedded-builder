/*
 * drivers/WySFM.h — SFM V1.7 / SFM-V1.x Fingerprint Scanner (UART)
 * ==================================================================
 * Compatible with: SFM-V1.7, SFM-V1.0, SFM-V2.0, and most GROW/ZFM-series
 * fingerprint modules using the same packet protocol.
 * Also compatible with: R307, R503 (same protocol family)
 *
 * Bundled driver — no external library needed.
 * Uses UART — registered via WySensors::addUART<WySFM>("finger", TX, RX, 57600)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT DOES
 * ═══════════════════════════════════════════════════════════════════
 * - Capture fingerprint image from optical sensor
 * - Convert image to searchable feature template
 * - Store up to 200 templates in onboard flash (library)
 * - Search stored library for matching fingerprint (1:N search)
 * - Match two captured templates (1:1 verify)
 * - Delete individual or all stored templates
 *
 * ═══════════════════════════════════════════════════════════════════
 * PACKET PROTOCOL
 * ═══════════════════════════════════════════════════════════════════
 * All commands and responses use the same frame format:
 *
 * [Header 2B][Addr 4B][PID 1B][Len 2B][CMD 1B][Data NB][Checksum 2B]
 *
 * Header:   0xEF 0x01 (always)
 * Addr:     0xFF 0xFF 0xFF 0xFF (default broadcast, or configured addr)
 * PID:      0x01 = command packet, 0x07 = response packet, 0x02 = data packet
 * Len:      length of [CMD + Data + Checksum] in bytes
 * Checksum: sum of [PID + Len_hi + Len_lo + CMD + Data] bytes, 16-bit
 *
 * Response confirmation codes (first byte of response data):
 *   0x00 = OK / success
 *   0x01 = packet receive error
 *   0x02 = no finger on sensor
 *   0x03 = fail to enroll finger
 *   0x06 = fail to generate template
 *   0x07 = template upload success
 *   0x08 = no template found in position
 *   0x09 = fail to load template
 *   0x0A = fail to delete template
 *   0x0B = fail to clear library
 *   0x0C = wrong password
 *   0x0F = fail to capture image (finger not pressed)
 *   0x11 = template full — library at capacity
 *   0x13 = wrong command
 *   0x15 = fingerprint not found (search failed)
 *   0x18 = error reading/writing flash
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 *   Module VCC  → 3.3V or 5V (check your module — most are 3.3V)
 *   Module GND  → GND
 *   Module TX   → ESP32 RX pin
 *   Module RX   → ESP32 TX pin
 *   Module WAKE/IRQ → optional GPIO (finger-touch interrupt)
 *   Touch ring  → GND (capacitive touch ring on some models)
 *
 * ⚠️ VOLTAGE: SFM-V1.7 operates at 3.3V. Some modules have a 5V regulator
 *    on the board — check before applying 5V to the logic pins.
 *
 * ═══════════════════════════════════════════════════════════════════
 * ENROLMENT FLOW
 * ═══════════════════════════════════════════════════════════════════
 * Enrolling a new fingerprint requires TWO captures of the same finger:
 *   1. getImage()       — capture image into ImageBuffer
 *   2. image2Tz(1)      — convert to feature template, store in CharBuffer1
 *   3. Ask user to lift and re-place finger
 *   4. getImage()       — capture second image
 *   5. image2Tz(2)      — convert to feature template, store in CharBuffer2
 *   6. createModel()    — merge CharBuffer1 + CharBuffer2 into a model
 *   7. storeModel(id)   — save model to library slot [id]
 *
 * ═══════════════════════════════════════════════════════════════════
 * SEARCH FLOW (recognition)
 * ═══════════════════════════════════════════════════════════════════
 *   1. getImage()       — capture image
 *   2. image2Tz(1)      — convert to feature template
 *   3. search()         — compare against all library entries
 *   4. Read matchID and confidence score from response
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   auto* fp = sensors.addUART<WySFM>("finger", TX, RX, 57600);
 *   sensors.begin();
 *
 *   // Enroll:
 *   uint8_t id = 1;
 *   fp->enroll(id);  // blocking — prompts via Serial
 *
 *   // Recognise:
 *   WySensorData d = sensors.read("finger");
 *   if (d.ok) Serial.printf("Match: ID %d, score %d\n", d.rawInt, (int)d.raw);
 *
 * WySensorData:
 *   d.ok     = true when a matching fingerprint was found
 *   d.rawInt = matched template ID (1–200)
 *   d.raw    = match confidence score (0–100)
 *   d.error  = confirmation code string on failure
 */

#pragma once
#include "../WySensors.h"

/* SFM protocol constants */
#define SFM_HEADER_HI   0xEF
#define SFM_HEADER_LO   0x01
#define SFM_ADDR_3      0xFF
#define SFM_ADDR_2      0xFF
#define SFM_ADDR_1      0xFF
#define SFM_ADDR_0      0xFF
#define SFM_PID_CMD     0x01   /* command packet */
#define SFM_PID_DATA    0x02   /* data packet */
#define SFM_PID_RSP     0x07   /* response packet */

/* Command codes */
#define SFM_CMD_GET_IMAGE    0x01   /* capture fingerprint image */
#define SFM_CMD_IMG_TO_TZ   0x02   /* convert image → feature template (CharBuffer 1 or 2) */
#define SFM_CMD_MATCH        0x03   /* compare CharBuffer1 vs CharBuffer2 */
#define SFM_CMD_SEARCH       0x04   /* search library for CharBuffer1 match */
#define SFM_CMD_REG_MODEL    0x05   /* merge CharBuffer1+2 into model */
#define SFM_CMD_STORE        0x06   /* store model to library */
#define SFM_CMD_LOAD_CHAR    0x07   /* load template from library to CharBuffer */
#define SFM_CMD_UPLOAD_CHAR  0x08   /* upload template from module */
#define SFM_CMD_DOWNLOAD_CHAR 0x09  /* download template to module */
#define SFM_CMD_UPLOAD_IMG   0x0A   /* upload raw image */
#define SFM_CMD_DELETE_CHAR  0x0C   /* delete template(s) from library */
#define SFM_CMD_EMPTY        0x0D   /* clear entire library */
#define SFM_CMD_SET_SYS_PARA 0x0E   /* set system parameter */
#define SFM_CMD_READ_SYS_PARA 0x0F  /* read system parameters */
#define SFM_CMD_VERIFY_PW    0x13   /* verify password */
#define SFM_CMD_WRITE_NOTEPAD 0x18  /* write to notepad */
#define SFM_CMD_READ_NOTEPAD  0x19  /* read from notepad */
#define SFM_CMD_HAND_ID_IMG  0x29   /* high-speed image capture */
#define SFM_CMD_GEN_BIN_IMG  0x2A   /* generate binary image */

/* Confirmation codes */
#define SFM_OK              0x00
#define SFM_ERR_RECV        0x01
#define SFM_ERR_NO_FINGER   0x02
#define SFM_ERR_ENROLL      0x03
#define SFM_ERR_TEMPLATE    0x06
#define SFM_ERR_NOT_FOUND   0x09
#define SFM_ERR_DELETE      0x0A
#define SFM_ERR_CLEAR       0x0B
#define SFM_ERR_NO_MATCH    0x08
#define SFM_ERR_NO_FINGER2  0x0F
#define SFM_ERR_FULL        0x11
#define SFM_ERR_SEARCH_FAIL 0x09
#define SFM_NOMATCH         0x09

/* Max templates in library */
#define SFM_LIBRARY_SIZE    200

/* Timeout for response */
#ifndef WY_SFM_TIMEOUT_MS
#define WY_SFM_TIMEOUT_MS  2000
#endif

class WySFM : public WySensorBase {
public:
    WySFM(WyUARTPins pins, uint32_t password = 0x00000000)
        : _pins(pins), _password(password) {}

    const char* driverName() override { return "SFM-V1.7"; }

    bool begin() override {
        Serial2.begin(_pins.baud, SERIAL_8N1, _pins.rx, _pins.tx);
        delay(200);  /* module boot time */

        /* Flush any garbage */
        while (Serial2.available()) Serial2.read();

        /* Verify password */
        uint8_t data[4] = {
            (uint8_t)(_password >> 24),
            (uint8_t)(_password >> 16),
            (uint8_t)(_password >> 8),
            (uint8_t)(_password)
        };
        uint8_t conf = _sendCmd(SFM_CMD_VERIFY_PW, data, 4);
        if (conf != SFM_OK) {
            Serial.printf("[SFM] password verify failed: 0x%02X\n", conf);
            return false;
        }

        /* Read system params */
        uint8_t rsp[16];
        uint8_t rlen;
        conf = _sendCmdGetResponse(SFM_CMD_READ_SYS_PARA, nullptr, 0, rsp, sizeof(rsp), rlen);
        if (conf == SFM_OK && rlen >= 14) {
            _librarySize = ((uint16_t)rsp[4] << 8) | rsp[5];
            _templateCount = ((uint16_t)rsp[10] << 8) | rsp[11];
            Serial.printf("[SFM] ready — library: %u slots, %u enrolled\n",
                _librarySize, _templateCount);
        } else {
            Serial.println("[SFM] online (couldn't read params)");
        }
        return true;
    }

    /* read() — non-blocking scan: captures image, converts, searches.
     * Returns immediately with ok=false if no finger present.
     * Returns ok=true + rawInt=matchID + raw=score if match found. */
    WySensorData read() override {
        WySensorData d;

        /* 1. Get image */
        uint8_t conf = _sendCmd(SFM_CMD_GET_IMAGE, nullptr, 0);
        if (conf == SFM_ERR_NO_FINGER || conf == SFM_ERR_NO_FINGER2) {
            d.ok = false;  /* no finger — normal, not an error */
            return d;
        }
        if (conf != SFM_OK) {
            d.error = _confStr(conf);
            return d;
        }

        /* 2. Image to CharBuffer1 */
        uint8_t buf = 0x01;
        conf = _sendCmd(SFM_CMD_IMG_TO_TZ, &buf, 1);
        if (conf != SFM_OK) { d.error = _confStr(conf); return d; }

        /* 3. Search library */
        uint16_t matchID = 0, score = 0;
        conf = _search(matchID, score);
        if (conf == SFM_OK) {
            d.rawInt = matchID;
            d.raw    = (float)score;
            d.ok     = true;
        } else {
            d.error = _confStr(conf);
        }
        return d;
    }

    /* ── High-level operations ────────────────────────────────────── */

    /* Enroll a new fingerprint. id = 1–200 (library slot).
     * Prints instructions to Serial. Returns true on success.
     * Blocking — takes ~5–10 seconds for two captures. */
    bool enroll(uint8_t id) {
        Serial.printf("[SFM] enrolling fingerprint to slot %u\n", id);

        /* First capture */
        Serial.println("[SFM] Place finger on sensor...");
        if (!_captureAndConvert(1)) return false;

        Serial.println("[SFM] Lift finger...");
        delay(1500);
        while (_sendCmd(SFM_CMD_GET_IMAGE, nullptr, 0) != SFM_ERR_NO_FINGER) delay(100);

        /* Second capture */
        Serial.println("[SFM] Place same finger again...");
        if (!_captureAndConvert(2)) return false;

        /* Create and store model */
        uint8_t conf = _sendCmd(SFM_CMD_REG_MODEL, nullptr, 0);
        if (conf != SFM_OK) {
            Serial.printf("[SFM] model creation failed: %s\n", _confStr(conf));
            return false;
        }

        uint8_t storeData[3] = {0x01, (uint8_t)(id >> 8), (uint8_t)(id & 0xFF)};
        conf = _sendCmd(SFM_CMD_STORE, storeData, 3);
        if (conf != SFM_OK) {
            Serial.printf("[SFM] store failed: %s\n", _confStr(conf));
            return false;
        }

        _templateCount++;
        Serial.printf("[SFM] ✓ fingerprint enrolled to slot %u\n", id);
        return true;
    }

    /* Delete a single template by ID */
    bool deleteTemplate(uint16_t id) {
        uint8_t data[4] = {(uint8_t)(id>>8), (uint8_t)(id&0xFF), 0x00, 0x01};
        return _sendCmd(SFM_CMD_DELETE_CHAR, data, 4) == SFM_OK;
    }

    /* Delete all templates — clear entire library */
    bool clearLibrary() {
        if (_sendCmd(SFM_CMD_EMPTY, nullptr, 0) == SFM_OK) {
            _templateCount = 0;
            return true;
        }
        return false;
    }

    /* Check if a specific template slot is occupied */
    bool templateExists(uint16_t id) {
        uint8_t data[3] = {0x01, (uint8_t)(id>>8), (uint8_t)(id&0xFF)};
        return _sendCmd(SFM_CMD_LOAD_CHAR, data, 3) == SFM_OK;
    }

    /* Capture + match to a specific template ID (1:1 verify) */
    bool verify(uint16_t id, uint16_t& score) {
        if (!_captureAndConvert(1)) return false;

        /* Load target template into CharBuffer2 */
        uint8_t loadData[3] = {0x02, (uint8_t)(id>>8), (uint8_t)(id&0xFF)};
        if (_sendCmd(SFM_CMD_LOAD_CHAR, loadData, 3) != SFM_OK) return false;

        /* Match CharBuffer1 vs CharBuffer2 */
        uint8_t rsp[4];
        uint8_t rlen;
        uint8_t conf = _sendCmdGetResponse(SFM_CMD_MATCH, nullptr, 0, rsp, sizeof(rsp), rlen);
        if (conf == SFM_OK && rlen >= 2) {
            score = ((uint16_t)rsp[0] << 8) | rsp[1];
            return true;
        }
        return false;
    }

    uint16_t templateCount() { return _templateCount; }
    uint16_t librarySize()   { return _librarySize; }

    /* LED control (some modules have onboard LED ring) */
    void ledOn()  { /* module-specific — override if needed */ }
    void ledOff() { /* module-specific — override if needed */ }

private:
    WyUARTPins _pins;
    uint32_t   _password;
    uint16_t   _templateCount = 0;
    uint16_t   _librarySize   = SFM_LIBRARY_SIZE;

    bool _captureAndConvert(uint8_t bufNum) {
        uint32_t deadline = millis() + 8000;
        while (millis() < deadline) {
            uint8_t conf = _sendCmd(SFM_CMD_GET_IMAGE, nullptr, 0);
            if (conf == SFM_OK) {
                conf = _sendCmd(SFM_CMD_IMG_TO_TZ, &bufNum, 1);
                if (conf == SFM_OK) return true;
                Serial.printf("[SFM] feature extract failed: %s\n", _confStr(conf));
                return false;
            }
            delay(100);
        }
        Serial.println("[SFM] capture timeout");
        return false;
    }

    uint8_t _search(uint16_t& matchID, uint16_t& score) {
        /* Search all library slots starting from 0 */
        uint8_t data[5] = {0x01, 0x00, 0x00,
            (uint8_t)(_librarySize >> 8), (uint8_t)(_librarySize & 0xFF)};
        uint8_t rsp[8];
        uint8_t rlen;
        uint8_t conf = _sendCmdGetResponse(SFM_CMD_SEARCH, data, 5, rsp, sizeof(rsp), rlen);
        if (conf == SFM_OK && rlen >= 4) {
            matchID = ((uint16_t)rsp[0] << 8) | rsp[1];
            score   = ((uint16_t)rsp[2] << 8) | rsp[3];
        }
        return conf;
    }

    /* ── Packet layer ─────────────────────────────────────────────── */

    /* Send command, return confirmation code */
    uint8_t _sendCmd(uint8_t cmd, uint8_t* data, uint8_t dataLen) {
        uint8_t rsp[4];
        uint8_t rlen;
        return _sendCmdGetResponse(cmd, data, dataLen, rsp, sizeof(rsp), rlen);
    }

    /* Send command, return conf code + extra response data */
    uint8_t _sendCmdGetResponse(uint8_t cmd, uint8_t* data, uint8_t dataLen,
                                 uint8_t* rspData, uint8_t maxRsp, uint8_t& rspLen) {
        _sendPacket(SFM_PID_CMD, cmd, data, dataLen);
        return _recvPacket(rspData, maxRsp, rspLen);
    }

    void _sendPacket(uint8_t pid, uint8_t cmd, uint8_t* data, uint8_t dataLen) {
        uint16_t len = dataLen + 1 + 2;  /* cmd + data + checksum */

        /* Header */
        Serial2.write(SFM_HEADER_HI);
        Serial2.write(SFM_HEADER_LO);
        /* Address */
        Serial2.write(SFM_ADDR_3); Serial2.write(SFM_ADDR_2);
        Serial2.write(SFM_ADDR_1); Serial2.write(SFM_ADDR_0);
        /* PID */
        Serial2.write(pid);
        /* Length */
        Serial2.write((uint8_t)(len >> 8));
        Serial2.write((uint8_t)(len & 0xFF));
        /* Command */
        Serial2.write(cmd);
        /* Data */
        for (uint8_t i = 0; i < dataLen; i++) Serial2.write(data[i]);

        /* Checksum: sum of PID + len_hi + len_lo + cmd + data */
        uint16_t sum = pid + (len >> 8) + (len & 0xFF) + cmd;
        for (uint8_t i = 0; i < dataLen; i++) sum += data[i];
        Serial2.write((uint8_t)(sum >> 8));
        Serial2.write((uint8_t)(sum & 0xFF));
    }

    uint8_t _recvPacket(uint8_t* rspData, uint8_t maxRsp, uint8_t& rspLen) {
        /* Read header + addr + pid + len */
        uint8_t hdr[9];
        if (!_readBytes(hdr, 9)) return 0xFF;  /* timeout */

        if (hdr[0] != SFM_HEADER_HI || hdr[1] != SFM_HEADER_LO) return 0xFE;  /* bad sync */

        uint8_t  pid = hdr[6];
        uint16_t len = ((uint16_t)hdr[7] << 8) | hdr[8];
        if (len < 3 || len > 200) return 0xFD;  /* implausible length */

        /* Read [conf_code][data...][checksum 2B] */
        uint8_t body[200];
        if (!_readBytes(body, len)) return 0xFF;

        uint8_t conf = body[0];
        rspLen = 0;
        uint8_t dataCnt = len - 3;  /* subtract conf + 2 checksum bytes */
        for (uint8_t i = 0; i < dataCnt && i < maxRsp; i++) {
            rspData[i] = body[1 + i];
            rspLen++;
        }
        /* Checksum not verified here — add if you need paranoid mode */
        return conf;
    }

    bool _readBytes(uint8_t* buf, uint16_t count) {
        uint32_t deadline = millis() + WY_SFM_TIMEOUT_MS;
        uint16_t i = 0;
        while (i < count && millis() < deadline) {
            if (Serial2.available()) buf[i++] = Serial2.read();
            else delayMicroseconds(500);
        }
        return (i == count);
    }

    const char* _confStr(uint8_t conf) {
        switch (conf) {
            case 0x00: return "OK";
            case 0x01: return "packet error";
            case 0x02: return "no finger";
            case 0x03: return "enroll fail";
            case 0x06: return "feature fail";
            case 0x08: return "no match";
            case 0x09: return "not found";
            case 0x0A: return "delete fail";
            case 0x0B: return "clear fail";
            case 0x0C: return "wrong password";
            case 0x0F: return "no finger (2)";
            case 0x11: return "library full";
            case 0x15: return "not found";
            case 0x18: return "flash error";
            default:   return "unknown error";
        }
    }
};
