/*
 * drivers/WyHSQR204.h — HS-QR204 / HSP04 thermal receipt printer (UART)
 * ========================================================================
 * Datasheet: Hoin/HS Printer Communication Protocol (ESC/POS compatible)
 * Bundled driver — no external library needed.
 * Protocol: ESC/POS (industry standard, Epson-origin)
 *
 * Registered via WySensors::addUART<WyHSQR204>("printer", tx, rx)
 * (Note: printers are output devices — read() returns status only)
 *
 * Wiring:
 *   HS-QR204 RX → ESP32 TX pin
 *   HS-QR204 TX → ESP32 RX pin (optional — for status queries)
 *   HS-QR204 GND → ESP32 GND
 *   HS-QR204 PWR → 5–9V (check your module — QR204 is typically 5V USB or 7.4V LiPo)
 *
 * Default baud: 9600 (some modules: 19200 or 115200 — check DIP/config)
 *
 * ESC/POS command reference used:
 *   ESC @      — initialize (reset)
 *   ESC a n    — justify: 0=left, 1=center, 2=right
 *   ESC E n    — bold: 1=on, 0=off
 *   ESC ! n    — character mode (bit 4=double height, bit 3=double width, bit 0=double strike)
 *   ESC { n    — upside down: 1=on, 0=off
 *   GS V n     — cut paper: 0=full, 1=partial
 *   GS k m ... — print barcode
 *   GS ( k ... — print QR code (2D)
 *   LF         — line feed (print and advance)
 *   ESC d n    — feed n lines
 *
 * Usage:
 *   auto* printer = sensors.addUART<WyHSQR204>("receipt", TX_PIN, RX_PIN);
 *   sensors.begin();
 *   printer->init();
 *   printer->println("Hello, world!");
 *   printer->setAlign(WY_PRINTER_CENTER);
 *   printer->setBold(true);
 *   printer->println("CKB Payment");
 *   printer->printQR("ckb1qzda0cr08m85hc8jlnfp3zer7xulejywt49kt2rr0vthywaa50xws...");
 *   printer->feed(3);
 *   printer->cut();
 */

#pragma once
#include "../WySensors.h"

/* Alignment constants */
#define WY_PRINTER_LEFT    0
#define WY_PRINTER_CENTER  1
#define WY_PRINTER_RIGHT   2

/* Text size constants (ESC ! bitmask) */
#define WY_PRINTER_NORMAL       0x00
#define WY_PRINTER_DOUBLE_H     0x10
#define WY_PRINTER_DOUBLE_W     0x20
#define WY_PRINTER_DOUBLE       0x30
#define WY_PRINTER_BOLD_STRIKE  0x01

/* Barcode type codes (GS k) */
#define WY_BARCODE_UPC_A    0x00
#define WY_BARCODE_EAN13    0x02
#define WY_BARCODE_CODE39   0x04
#define WY_BARCODE_CODE128  0x08

/* QR error correction levels */
#define WY_QR_EC_LOW     0x30  /* L: ~7% */
#define WY_QR_EC_MEDIUM  0x31  /* M: ~15% */
#define WY_QR_EC_QUARTILE 0x32 /* Q: ~25% */
#define WY_QR_EC_HIGH    0x33  /* H: ~30% */

class WyHSQR204 : public WySensorBase {
public:
    WyHSQR204(WyUARTPins pins) : _pins(pins) {}

    const char* driverName() override { return "HS-QR204"; }

    bool begin() override {
        Serial2.begin(_pins.baud, SERIAL_8N1, _pins.rx, _pins.tx);
        delay(500);
        init();
        return true;
    }

    /* read() — for registry compat. Returns ok=true if printer responded to status query.
     * Most uses will call print methods directly on the driver pointer. */
    WySensorData read() override {
        WySensorData d;
        /* Real-time status: ESC v → 1 byte response (0x12=online, bit patterns for errors) */
        /* Simplified: just check if serial is up */
        d.ok = (Serial2);
        return d;
    }

    /* ── Core control ───────────────────────────────────────────── */

    /* Initialize / reset printer (ESC @) */
    void init() {
        _write(0x1B); _write(0x40);
        delay(50);
    }

    /* Print a string */
    void print(const char* str) {
        while (*str) _write((uint8_t)*str++);
    }

    /* Print string + LF */
    void println(const char* str) {
        print(str);
        _write(0x0A);  /* LF */
    }

    /* Print a single line feed */
    void println() { _write(0x0A); }

    /* Feed N blank lines (ESC d n) */
    void feed(uint8_t lines = 1) {
        _write(0x1B); _write(0x64); _write(lines);
    }

    /* Cut paper — full cut (GS V 0) or partial (GS V 1) */
    void cut(bool partial = false) {
        _write(0x1D); _write(0x56); _write(partial ? 0x01 : 0x00);
    }

    /* ── Text formatting ─────────────────────────────────────────── */

    /* Alignment: WY_PRINTER_LEFT / CENTER / RIGHT (ESC a n) */
    void setAlign(uint8_t align) {
        _write(0x1B); _write(0x61); _write(align & 0x03);
    }

    /* Bold on/off (ESC E n) */
    void setBold(bool on) {
        _write(0x1B); _write(0x45); _write(on ? 0x01 : 0x00);
    }

    /* Underline: 0=off, 1=thin, 2=thick (ESC - n) */
    void setUnderline(uint8_t mode) {
        _write(0x1B); _write(0x2D); _write(mode & 0x03);
    }

    /* Upside-down text (ESC { n) */
    void setUpsideDown(bool on) {
        _write(0x1B); _write(0x7B); _write(on ? 0x01 : 0x00);
    }

    /* Text size: use WY_PRINTER_NORMAL / DOUBLE_H / DOUBLE_W / DOUBLE (ESC ! n) */
    void setSize(uint8_t mode) {
        _write(0x1B); _write(0x21); _write(mode);
    }

    /* Combined size via GS ! for some printers (height bits 6:4, width bits 2:0) */
    void setCharSize(uint8_t width = 0, uint8_t height = 0) {
        _write(0x1D); _write(0x21); _write(((height & 0x07) << 4) | (width & 0x07));
    }

    /* Reset all formatting */
    void resetFormat() {
        setBold(false);
        setUnderline(0);
        setAlign(WY_PRINTER_LEFT);
        setSize(WY_PRINTER_NORMAL);
        setCharSize(0, 0);
    }

    /* ── Barcode printing ────────────────────────────────────────── */

    /* Print 1D barcode (GS k)
     * type: WY_BARCODE_UPC_A, EAN13, CODE39, CODE128
     * data: barcode content string
     * height: bar height in dots (default 80)
     * hriPos: 0=none, 1=above, 2=below, 3=both (Human Readable Interpretation)
     */
    void printBarcode(const char* data, uint8_t type = WY_BARCODE_CODE128,
                      uint8_t height = 80, uint8_t hriPos = 2) {
        uint8_t len = strlen(data);
        /* Set bar height (GS h n) */
        _write(0x1D); _write(0x68); _write(height);
        /* Set HRI position (GS H n) */
        _write(0x1D); _write(0x48); _write(hriPos);
        /* Print barcode (GS k type len data) */
        _write(0x1D); _write(0x6B); _write(type); _write(len);
        for (uint8_t i = 0; i < len; i++) _write((uint8_t)data[i]);
    }

    /* Print QR code (GS ( k sequence — ESC/POS 2D barcode)
     * content: the URL, text, or CKB address to encode
     * size:    module size 1–8 (dots per QR module, default 4)
     * ec:      error correction WY_QR_EC_LOW / MEDIUM / QUARTILE / HIGH
     */
    void printQR(const char* content, uint8_t size = 4, uint8_t ec = WY_QR_EC_MEDIUM) {
        uint16_t len = strlen(content);

        /* fn 65: set module size */
        _qrCmd(0x41, nullptr, 0, size);

        /* fn 69: set error correction */
        _qrCmd(0x45, nullptr, 0, ec);

        /* fn 80: store data in symbol storage area
         * pL pH = data_len + 3, cn=49, fn=80, m=48, data */
        uint16_t pLen = len + 3;
        uint8_t hdr[4] = {0x31, 0x50, 0x30};  /* cn=49, fn=80, m=48 */
        _write(0x1D); _write(0x28); _write(0x6B);
        _write(pLen & 0xFF); _write(pLen >> 8);
        _write(hdr[0]); _write(hdr[1]); _write(hdr[2]);
        for (uint16_t i = 0; i < len; i++) _write((uint8_t)content[i]);

        /* fn 81: print symbol */
        _qrCmd(0x51, nullptr, 0, 0);

        delay(len / 4 + 200);  /* allow time to print */
    }

    /* ── Convenience helpers ─────────────────────────────────────── */

    /* Print a divider line (dashes or custom char, printer-width) */
    void divider(char c = '-', uint8_t cols = 32) {
        for (uint8_t i = 0; i < cols; i++) _write((uint8_t)c);
        _write(0x0A);
    }

    /* Print centered text with divider above and below */
    void header(const char* title) {
        divider();
        setAlign(WY_PRINTER_CENTER);
        setBold(true);
        println(title);
        setBold(false);
        setAlign(WY_PRINTER_LEFT);
        divider();
    }

    /* Print a key:value row — right-aligns value for receipts */
    void receiptRow(const char* label, const char* value, uint8_t cols = 32) {
        uint8_t labelLen = strlen(label);
        uint8_t valueLen = strlen(value);
        print(label);
        uint8_t spaces = cols > labelLen + valueLen ? cols - labelLen - valueLen : 1;
        for (uint8_t i = 0; i < spaces; i++) _write(' ');
        println(value);
    }

    /* Print number as value */
    void receiptRow(const char* label, float value, const char* unit = "", uint8_t cols = 32) {
        char buf[24];
        snprintf(buf, sizeof(buf), "%.2f%s", value, unit);
        receiptRow(label, buf, cols);
    }

private:
    WyUARTPins _pins;

    void _write(uint8_t b) { Serial2.write(b); }

    /* QR code sub-command helper (GS ( k pL pH cn fn ...) */
    void _qrCmd(uint8_t fn, const uint8_t* data, uint16_t dataLen, uint8_t param) {
        uint16_t pLen = dataLen + (data ? 2 : 3);  /* cn + fn + [param] + data */
        _write(0x1D); _write(0x28); _write(0x6B);
        _write(pLen & 0xFF); _write(pLen >> 8);
        _write(0x31);   /* cn = 49 */
        _write(fn);
        if (!data) { _write(param); }
        else { for (uint16_t i = 0; i < dataLen; i++) _write(data[i]); }
    }
};
