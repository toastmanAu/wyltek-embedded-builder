/*
 * drivers/WyGM861S.h — Grow GM861S barcode/QR scanner (UART)
 * =============================================================
 * Datasheet: Grow GM861S Series Communication Protocol
 * Bundled driver — no external library needed.
 * Uses UART RX/TX — the scanner sends decoded barcode data as ASCII.
 *
 * Registered via WySensors::addUART<WyGM861S>("scanner", tx, rx)
 *
 * Protocol:
 *   Default: 9600 baud, 8N1
 *   Output mode: "Continuous" or "Trigger" (via hardware TRIG pin or command)
 *   Scan result: raw ASCII barcode string terminated by \r\n (default suffix)
 *   Command format: 0x7E 0xFF 0x06 <cmd> 0x01 <param_hi> <param_lo> <crc_hi> <crc_lo>
 *   Response:      0x02 0x00 0x00 0x01 0x00 <cmd> <status>
 *
 * Wiring:
 *   GM861S TX → ESP32 RX pin
 *   GM861S RX → ESP32 TX pin (optional — only needed for commands/config)
 *   GM861S TRIG → GPIO (optional — pull LOW to trigger scan, HIGH to stop)
 *   GM861S PWR — 3.3V or 5V
 *
 * Trigger modes:
 *   WY_GM861S_MODE_CONTINUOUS — scans constantly, outputs on new barcode
 *   WY_GM861S_MODE_TRIGGER    — only scans while TRIG pin held LOW
 *   WY_GM861S_MODE_COMMAND    — triggered via sendTrigger() UART command
 *
 * Usage:
 *   auto* scanner = sensors.addUART<WyGM861S>("barcode", TX_PIN, RX_PIN);
 *   sensors.begin();
 *   // In loop:
 *   WySensorData d = sensors.read("barcode");  // non-blocking
 *   if (d.ok) Serial.println(d.error);  // barcode string in .error field (reused as string buffer)
 *   // Or direct:
 *   if (scanner->available()) Serial.println(scanner->lastBarcode());
 */

#pragma once
#include "../WySensors.h"

/* Trigger mode constants */
#define WY_GM861S_MODE_CONTINUOUS  0
#define WY_GM861S_MODE_TRIGGER     1
#define WY_GM861S_MODE_COMMAND     2

/* GM861S command bytes (sent in 9-byte command frame) */
#define GM861S_CMD_TRIGGER_START   0x72
#define GM861S_CMD_TRIGGER_STOP    0x73
#define GM861S_CMD_SET_BAUD        0x08
#define GM861S_CMD_BEEP            0x76
#define GM861S_CMD_LED             0x77
#define GM861S_CMD_SLEEP           0x55
#define GM861S_CMD_WAKE            0xFF

/* Max barcode length — GM861S supports up to 7089 chars (QR data),
 * but practical ESP32 limit is set here */
#ifndef WY_GM861S_BUF_SIZE
#define WY_GM861S_BUF_SIZE  256
#endif

class WyGM861S : public WySensorBase {
public:
    WyGM861S(WyUARTPins pins, uint8_t trigPin = -1, uint8_t mode = WY_GM861S_MODE_CONTINUOUS)
        : _pins(pins), _trigPin(trigPin), _mode(mode) {}

    const char* driverName() override { return "GM861S"; }

    bool begin() override {
        /* UART init — GM861S default: 9600 8N1 */
        _serial = &Serial2;  /* use Serial2 by default */
        Serial2.begin(_pins.baud, SERIAL_8N1, _pins.rx, _pins.tx);
        delay(500);  /* scanner needs time to boot */

        /* Configure TRIG pin if provided */
        if (_trigPin >= 0) {
            pinMode(_trigPin, OUTPUT);
            digitalWrite(_trigPin, HIGH);  /* HIGH = idle (not scanning) */
        }

        /* Flush any buffered garbage */
        while (Serial2.available()) Serial2.read();

        _bufLen = 0;
        _newData = false;

        /* Optional: set to trigger mode if requested */
        if (_mode == WY_GM861S_MODE_TRIGGER && _trigPin < 0) {
            /* No trig pin — switch to command mode */
            _mode = WY_GM861S_MODE_COMMAND;
        }

        return true;
    }

    /* read() — non-blocking. Returns ok=true when a new barcode is available.
     * Barcode string stored in _lastBarcode[], accessible via lastBarcode().
     * WySensorData.error field reused as pointer to the barcode string. */
    WySensorData read() override {
        WySensorData d;
        _poll();  /* drain UART buffer */
        if (_newData) {
            _newData = false;
            d.ok    = true;
            d.error = _lastBarcode;  /* point to our buffer */
            d.rawInt = _scanCount;
        }
        return d;
    }

    /* ── Direct API ───────────────────────────────────────────── */

    /* Check if a new barcode is waiting */
    bool available() { _poll(); return _newData; }

    /* Get last decoded barcode string */
    const char* lastBarcode() { return _lastBarcode; }

    /* How many barcodes decoded total */
    uint32_t scanCount() { return _scanCount; }

    /* Trigger a single scan (command mode) */
    void sendTrigger() { _sendCmd(GM861S_CMD_TRIGGER_START, 0x00, 0x00); }

    /* Stop scanning (command mode) */
    void sendStop() { _sendCmd(GM861S_CMD_TRIGGER_STOP, 0x00, 0x00); }

    /* TRIG pin control (trigger mode) */
    void triggerOn()  { if (_trigPin >= 0) digitalWrite(_trigPin, LOW);  }
    void triggerOff() { if (_trigPin >= 0) digitalWrite(_trigPin, HIGH); }

    /* Beep the scanner buzzer */
    void beep() { _sendCmd(GM861S_CMD_BEEP, 0x00, 0x01); }

    /* Sleep/wake */
    void sleep() { _sendCmd(GM861S_CMD_SLEEP, 0x00, 0x00); }
    void wake()  { _sendCmd(GM861S_CMD_WAKE,  0x00, 0x00); }

    /* Clear last barcode */
    void clear() { _lastBarcode[0] = 0; _newData = false; }

private:
    WyUARTPins  _pins;
    int8_t      _trigPin;
    uint8_t     _mode;
    HardwareSerial* _serial = nullptr;

    char     _lastBarcode[WY_GM861S_BUF_SIZE];
    char     _buf[WY_GM861S_BUF_SIZE];
    uint16_t _bufLen   = 0;
    bool     _newData  = false;
    uint32_t _scanCount = 0;

    /* Poll UART — accumulate bytes, detect end of barcode on \r or \n */
    void _poll() {
        while (Serial2.available()) {
            char c = (char)Serial2.read();
            if (c == '\r' || c == '\n') {
                if (_bufLen > 0) {
                    _buf[_bufLen] = 0;
                    strncpy(_lastBarcode, _buf, WY_GM861S_BUF_SIZE - 1);
                    _lastBarcode[WY_GM861S_BUF_SIZE - 1] = 0;
                    _bufLen = 0;
                    _scanCount++;
                    _newData = true;
                }
            } else if (_bufLen < WY_GM861S_BUF_SIZE - 1) {
                _buf[_bufLen++] = c;
            } else {
                /* Buffer overflow — discard accumulated, start fresh */
                _bufLen = 0;
            }
        }
    }

    /* Send 9-byte command frame to scanner
     * Format: 0x7E 0xFF 0x06 <cmd> 0x01 <p1> <p2> <crc_hi> <crc_lo>
     * CRC: sum of bytes 1-6 */
    void _sendCmd(uint8_t cmd, uint8_t p1, uint8_t p2) {
        uint8_t frame[9] = {0x7E, 0xFF, 0x06, cmd, 0x01, p1, p2, 0x00, 0x00};
        uint16_t crc = 0;
        for (uint8_t i = 1; i <= 6; i++) crc += frame[i];
        frame[7] = (crc >> 8) & 0xFF;
        frame[8] =  crc       & 0xFF;
        Serial2.write(frame, 9);
    }
};
