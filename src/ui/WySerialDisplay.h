/*
 * WySerialDisplay.h — Scrolling terminal overlay for any WY_HAS_DISPLAY board
 * ============================================================================
 * Mirrors Serial output to the display as a scrolling terminal window.
 * Drop into any test sketch — zero changes to Serial.print() calls needed.
 *
 * Usage (minimal):
 *   #include <WyDisplay.h>
 *   #include <WySerialDisplay.h>
 *
 *   WyDisplay display;
 *   WySerialDisplay term;
 *
 *   void setup() {
 *     Serial.begin(115200);
 *     display.begin();
 *     term.begin(display.gfx);   // attach to display
 *     Serial.println("Hello!");  // appears on screen AND serial
 *   }
 *
 *   void loop() {
 *     term.update();   // call once per loop — flushes pending chars
 *   }
 *
 * Configuration (optional, set before #include or in platformio.ini):
 *   WY_TERM_LINES      - visible lines (default: auto from display height)
 *   WY_TERM_COLS       - chars per line (default: auto from display width)
 *   WY_TERM_FONT_SIZE  - 1 or 2 (default: 1 = 6×8px chars)
 *   WY_TERM_BG         - background colour (default: WY_BLACK)
 *   WY_TERM_FG         - text colour (default: WY_GREEN)
 *   WY_TERM_HEADER     - 1 to show board name + uptime header bar (default: 1)
 *   WY_TERM_INTERCEPT  - 1 to auto-intercept Serial (default: 1)
 *
 * Intercepting Serial:
 *   When WY_TERM_INTERCEPT=1 (default), WySerialDisplay installs itself as an
 *   additional Print sink. Serial.print() goes to both USB serial AND the screen.
 *   No code changes needed in your sketch.
 *
 * Memory:
 *   Line buffer: WY_TERM_LINES * WY_TERM_COLS bytes (stack-allocated)
 *   Default 320×240 board: 40 lines × 53 cols = ~2.1 KB
 *
 * Requires: WY_HAS_DISPLAY=1 (set by boards.h for display-capable boards)
 */

#pragma once
#include "boards.h"

#if WY_HAS_DISPLAY

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

/* ── Defaults ────────────────────────────────────────────────────── */
#ifndef WY_TERM_FONT_SIZE
  #define WY_TERM_FONT_SIZE 1
#endif
#ifndef WY_TERM_BG
  #define WY_TERM_BG  0x0000  /* black */
#endif
#ifndef WY_TERM_FG
  #define WY_TERM_FG  0x07E0  /* green */
#endif
#ifndef WY_TERM_HEADER
  #define WY_TERM_HEADER 1
#endif
#ifndef WY_TERM_INTERCEPT
  #define WY_TERM_INTERCEPT 1
#endif

/* Character cell size for built-in GFX font */
#define WY_TERM_CHAR_W  (6 * WY_TERM_FONT_SIZE)
#define WY_TERM_CHAR_H  (8 * WY_TERM_FONT_SIZE)

/* Header bar height (0 if disabled) */
#define WY_TERM_HEADER_H  (WY_TERM_HEADER ? (WY_TERM_CHAR_H + 4) : 0)

/* Auto-calculate cols/lines from display dimensions if not set */
#ifndef WY_TERM_COLS
  #define WY_TERM_COLS  (WY_DISPLAY_W / WY_TERM_CHAR_W)
#endif
#ifndef WY_TERM_LINES
  #define WY_TERM_LINES ((WY_DISPLAY_H - WY_TERM_HEADER_H) / WY_TERM_CHAR_H)
#endif

/* ── WySerialDisplay ─────────────────────────────────────────────── */
class WySerialDisplay : public Print {
public:
    WySerialDisplay() : _gfx(nullptr), _row(0), _col(0), _dirty(false) {
        memset(_buf, ' ', sizeof(_buf));
    }

    /* Attach to a display. Call after display.begin(). */
    void begin(Arduino_GFX* gfx, bool clearScreen = true) {
        _gfx = gfx;
        if (clearScreen) {
            _gfx->fillScreen(WY_TERM_BG);
        }
        _drawHeader();
#if WY_TERM_INTERCEPT
        /* Hook into Serial's additional Print sink if available */
        /* (HardwareSerial on ESP32 supports addPrintHandler) */
        _hookSerial();
#endif
    }

    /* Call once per loop() — processes any buffered chars */
    void update() {
        if (_dirty) {
            _dirty = false;
            /* Redraw is done inline in write(), nothing to flush here.
               This hook exists for future double-buffered mode. */
        }
        /* Refresh header uptime every ~1s */
#if WY_TERM_HEADER
        uint32_t now = millis();
        if (now - _lastHeaderMs > 1000) {
            _lastHeaderMs = now;
            _drawHeader();
        }
#endif
    }

    /* Print interface — write a single character */
    size_t write(uint8_t c) override {
        if (!_gfx) return 1;

        if (c == '\r') return 1;  /* ignore CR */

        if (c == '\n') {
            _newline();
            return 1;
        }

        if (c == '\t') {
            /* Tab → advance to next 8-char boundary */
            uint8_t spaces = 8 - (_col % 8);
            for (uint8_t i = 0; i < spaces && _col < WY_TERM_COLS; i++) {
                _putChar(' ');
            }
            return 1;
        }

        _putChar(c);
        return 1;
    }

    /* Convenience: print directly to terminal (bypasses Serial) */
    void printDirect(const char* s) {
        while (*s) write((uint8_t)*s++);
    }

    /* Clear screen */
    void clear() {
        memset(_buf, ' ', sizeof(_buf));
        _row = 0; _col = 0;
        if (_gfx) {
            _gfx->fillRect(0, WY_TERM_HEADER_H,
                           WY_DISPLAY_W, WY_DISPLAY_H - WY_TERM_HEADER_H,
                           WY_TERM_BG);
        }
    }

    /* Change colours at runtime */
    void setColors(uint16_t fg, uint16_t bg) {
        _fg = fg; _bg = bg;
    }

private:
    Arduino_GFX* _gfx;
    char  _buf[WY_TERM_LINES][WY_TERM_COLS + 1];  /* +1 for safety */
    uint8_t _row, _col;
    bool _dirty;
    uint16_t _fg = WY_TERM_FG;
    uint16_t _bg = WY_TERM_BG;
#if WY_TERM_HEADER
    uint32_t _lastHeaderMs = 0;
#endif

    void _putChar(char c) {
        if (_col >= WY_TERM_COLS) _newline();
        _buf[_row][_col] = c;
        /* Draw char in-place */
        int16_t px = _col * WY_TERM_CHAR_W;
        int16_t py = WY_TERM_HEADER_H + _row * WY_TERM_CHAR_H;
        _gfx->fillRect(px, py, WY_TERM_CHAR_W, WY_TERM_CHAR_H, _bg);
        _gfx->setTextSize(WY_TERM_FONT_SIZE);
        _gfx->setTextColor(_fg);
        _gfx->setCursor(px, py);
        _gfx->print(c);
        _col++;
    }

    void _newline() {
        /* Pad rest of current line */
        while (_col < WY_TERM_COLS) _buf[_row][_col++] = ' ';
        _col = 0;

        if (_row < WY_TERM_LINES - 1) {
            _row++;
        } else {
            /* Scroll: shift buffer up one row */
            for (uint8_t r = 0; r < WY_TERM_LINES - 1; r++) {
                memcpy(_buf[r], _buf[r + 1], WY_TERM_COLS);
            }
            memset(_buf[WY_TERM_LINES - 1], ' ', WY_TERM_COLS);
            _redrawAll();
        }
    }

    void _redrawAll() {
        /* Redraw full terminal area — called on scroll */
        _gfx->fillRect(0, WY_TERM_HEADER_H,
                       WY_DISPLAY_W, WY_DISPLAY_H - WY_TERM_HEADER_H,
                       _bg);
        _gfx->setTextSize(WY_TERM_FONT_SIZE);
        _gfx->setTextColor(_fg);
        for (uint8_t r = 0; r < WY_TERM_LINES; r++) {
            _gfx->setCursor(0, WY_TERM_HEADER_H + r * WY_TERM_CHAR_H);
            for (uint8_t c = 0; c < WY_TERM_COLS; c++) {
                _gfx->print(_buf[r][c]);
            }
        }
    }

    void _drawHeader() {
#if WY_TERM_HEADER
        if (!_gfx) return;
        /* Dark bar across top */
        _gfx->fillRect(0, 0, WY_DISPLAY_W, WY_TERM_HEADER_H, 0x1082 /* dark grey */);
        /* Board name left, uptime right */
        _gfx->setTextSize(1);
        _gfx->setTextColor(0xAD75 /* light grey */);
        _gfx->setCursor(3, 2);
        _gfx->print(F(WY_BOARD_NAME));
        /* Uptime */
        char upbuf[16];
        uint32_t s = millis() / 1000;
        if (s < 60)        snprintf(upbuf, sizeof(upbuf), "%lus", s);
        else if (s < 3600) snprintf(upbuf, sizeof(upbuf), "%lum%lus", s/60, s%60);
        else               snprintf(upbuf, sizeof(upbuf), "%luh%lum", s/3600, (s%3600)/60);
        int16_t uw = strlen(upbuf) * 6;
        _gfx->setCursor(WY_DISPLAY_W - uw - 3, 2);
        _gfx->print(upbuf);
        /* Separator line */
        _gfx->drawFastHLine(0, WY_TERM_HEADER_H - 1, WY_DISPLAY_W, 0x2945 /* dim teal */);
#endif
    }

    void _hookSerial() {
        /* ESP32 HardwareSerial supports addPrintHandler() since arduino-esp32 2.x */
#if defined(ESP32) || defined(ESP8266)
        Serial.addPrintHandler(this);
#endif
        /* On other platforms, user must call term.print() manually or use
           a macro to redirect Serial — see WY_SERIAL_MIRROR below */
    }
};

/* ── Optional macro for platforms without addPrintHandler ────────── */
/* Add #define WY_SERIAL_MIRROR(term)  before including this header,
   then use the SERIAL_PRINT / SERIAL_PRINTLN macros below instead.
   On ESP32 with addPrintHandler you don't need these. */
#ifdef WY_SERIAL_MIRROR
  #define SERIAL_PRINT(t, x)    do { Serial.print(x);   (t).print(x);   } while(0)
  #define SERIAL_PRINTLN(t, x)  do { Serial.println(x); (t).println(x); } while(0)
  #define SERIAL_PRINTF(t, ...) do { Serial.printf(__VA_ARGS__); (t).printf(__VA_ARGS__); } while(0)
#endif

#else  /* !WY_HAS_DISPLAY */

/* Stub for non-display builds — compiles to nothing */
class WySerialDisplay : public Print {
public:
    void begin(void* gfx = nullptr, bool clear = true) {}
    void update() {}
    void clear() {}
    size_t write(uint8_t c) override { return 1; }
    void printDirect(const char*) {}
    void setColors(uint16_t, uint16_t) {}
};

#endif /* WY_HAS_DISPLAY */
