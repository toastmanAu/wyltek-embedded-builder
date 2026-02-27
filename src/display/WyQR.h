/*
 * WyQR.h — QR code renderer for wyltek-embedded-builder
 * =======================================================
 * Renders QR codes onto any Arduino_GFX display.
 * Uses the lightweight qrcode library (Rick Osgood / nayuki port).
 *
 * Usage:
 *   #include <WyQR.h>
 *
 *   WyQR qr(display.gfx);
 *
 *   // Centred on screen, auto-sized
 *   qr.draw("ckb1qz...");
 *
 *   // Explicit position + module size
 *   qr.draw("https://wyltekindustries.com", 60, 60, 4);
 *
 *   // With background fill + quiet zone
 *   qr.drawBox("0x1234abcd...", 40, 40, 3, WY_WHITE, WY_BLACK);
 *
 * platformio.ini lib_deps:
 *   ricmoo/QRCode@^0.0.1
 *
 * QR version auto-selected (1–10) based on data length.
 * ECC level: MEDIUM (M) by default — good balance of density vs error recovery.
 */

#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <qrcode.h>       /* ricmoo/QRCode */

/* ── Defaults ────────────────────────────────────────────────────── */
#define WY_QR_ECC        ECC_MEDIUM   /* M = ~15% recovery */
#define WY_QR_MODULE_PX  4            /* pixels per QR module (default) */
#define WY_QR_QUIET      2            /* quiet zone modules around QR */
#define WY_QR_FG         0x0000       /* foreground: black */
#define WY_QR_BG         0xFFFF       /* background: white */

class WyQR {
public:
    Arduino_GFX *gfx;

    WyQR(Arduino_GFX *g) : gfx(g) {}

    /* ── draw() — centred on screen, auto module size ────────────── */
    bool draw(const char *data,
              uint16_t fg = WY_QR_FG,
              uint16_t bg = WY_QR_BG) {
        uint8_t ver = _autoVersion(strlen(data));
        uint8_t sz  = qrcode_getBufferSize(ver);
        QRCode  qrc;
        uint8_t *buf = (uint8_t *)malloc(sz);
        if (!buf) return false;

        qrcode_initText(&qrc, buf, ver, WY_QR_ECC, data);

        // Auto-fit: largest module size that fits on screen with quiet zone
        uint16_t available = min(gfx->width(), gfx->height()) - 16;
        uint8_t  mod_px    = available / (qrc.size + WY_QR_QUIET * 2);
        if (mod_px < 1) mod_px = 1;

        uint16_t total = (qrc.size + WY_QR_QUIET * 2) * mod_px;
        int16_t  ox    = (gfx->width()  - total) / 2;
        int16_t  oy    = (gfx->height() - total) / 2;

        _render(&qrc, ox, oy, mod_px, fg, bg, true);
        free(buf);
        return true;
    }

    /* ── draw() — explicit position + module size ────────────────── */
    bool draw(const char *data,
              int16_t x, int16_t y,
              uint8_t mod_px = WY_QR_MODULE_PX,
              uint16_t fg    = WY_QR_FG,
              uint16_t bg    = WY_QR_BG) {
        uint8_t ver = _autoVersion(strlen(data));
        uint8_t sz  = qrcode_getBufferSize(ver);
        QRCode  qrc;
        uint8_t *buf = (uint8_t *)malloc(sz);
        if (!buf) return false;

        qrcode_initText(&qrc, buf, ver, WY_QR_ECC, data);
        _render(&qrc, x, y, mod_px, fg, bg, false);
        free(buf);
        return true;
    }

    /* ── drawBox() — fill background rect + draw QR with quiet zone ─ */
    bool drawBox(const char *data,
                 int16_t x, int16_t y,
                 uint8_t mod_px  = WY_QR_MODULE_PX,
                 uint16_t bg     = WY_QR_BG,
                 uint16_t fg     = WY_QR_FG,
                 uint8_t  quiet  = WY_QR_QUIET) {
        uint8_t ver = _autoVersion(strlen(data));
        uint8_t sz  = qrcode_getBufferSize(ver);
        QRCode  qrc;
        uint8_t *buf = (uint8_t *)malloc(sz);
        if (!buf) return false;

        qrcode_initText(&qrc, buf, ver, WY_QR_ECC, data);

        uint16_t total = (qrc.size + quiet * 2) * mod_px;
        gfx->fillRect(x, y, total, total, bg);
        _render(&qrc, x + quiet * mod_px, y + quiet * mod_px, mod_px, fg, bg, false);

        free(buf);
        return true;
    }

    /* ── drawCentred() — centred at cx, cy ───────────────────────── */
    bool drawCentred(const char *data,
                     int16_t cx, int16_t cy,
                     uint8_t mod_px = WY_QR_MODULE_PX,
                     uint16_t fg    = WY_QR_FG,
                     uint16_t bg    = WY_QR_BG) {
        uint8_t ver  = _autoVersion(strlen(data));
        uint8_t sz   = qrcode_getBufferSize(ver);
        QRCode  qrc;
        uint8_t *buf = (uint8_t *)malloc(sz);
        if (!buf) return false;

        qrcode_initText(&qrc, buf, ver, WY_QR_ECC, data);
        uint16_t total = (qrc.size + WY_QR_QUIET * 2) * mod_px;
        _render(&qrc, cx - total/2, cy - total/2, mod_px, fg, bg, true);
        free(buf);
        return true;
    }

    /* ── pixelSize() — how many pixels wide will the QR be? ─────── */
    uint16_t pixelSize(const char *data, uint8_t mod_px = WY_QR_MODULE_PX) {
        uint8_t ver = _autoVersion(strlen(data));
        QRCode  qrc;
        uint8_t *buf = (uint8_t *)malloc(qrcode_getBufferSize(ver));
        if (!buf) return 0;
        qrcode_initText(&qrc, buf, ver, WY_QR_ECC, data);
        uint16_t sz = (qrc.size + WY_QR_QUIET * 2) * mod_px;
        free(buf);
        return sz;
    }

private:
    /* Auto-select minimum QR version for data length */
    uint8_t _autoVersion(size_t len) {
        // Approximate capacity (alphanumeric/byte, ECC M):
        if (len <=  20) return 2;
        if (len <=  32) return 3;
        if (len <=  46) return 4;
        if (len <=  60) return 5;
        if (len <=  74) return 6;
        if (len <=  86) return 7;
        if (len <= 108) return 8;
        if (len <= 130) return 9;
        return 10;  // max we support — ~154 bytes ECC-M
    }

    void _render(QRCode *qrc, int16_t ox, int16_t oy, uint8_t mod_px,
                 uint16_t fg, uint16_t bg, bool fill_bg) {
        if (fill_bg) {
            uint16_t total = qrc->size * mod_px;
            gfx->fillRect(ox, oy, total, total, bg);
        }
        for (uint8_t row = 0; row < qrc->size; row++) {
            for (uint8_t col = 0; col < qrc->size; col++) {
                uint16_t col16 = qrcode_getModule(qrc, col, row) ? fg : bg;
                if (mod_px == 1) {
                    gfx->drawPixel(ox + col, oy + row, col16);
                } else {
                    gfx->fillRect(ox + col * mod_px, oy + row * mod_px,
                                  mod_px, mod_px, col16);
                }
            }
        }
    }
};

/* ── One-liner free function ─────────────────────────────────────── */
static inline bool wyDrawQR(Arduino_GFX *gfx, const char *data,
                             int16_t x = -1, int16_t y = -1,
                             uint8_t mod_px = WY_QR_MODULE_PX) {
    WyQR qr(gfx);
    if (x < 0 || y < 0) return qr.draw(data);
    return qr.draw(data, x, y, mod_px);
}
