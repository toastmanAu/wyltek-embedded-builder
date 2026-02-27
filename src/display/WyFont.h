/*
 * WyFont.h — Font management for wyltek-embedded-builder
 * ========================================================
 * Clean API for text rendering on Arduino_GFX displays.
 * Handles: font selection, sizing, alignment, wrapping, text bounds.
 *
 * Usage:
 *   #include <WyFont.h>
 *
 *   WyFont txt(display.gfx);
 *
 *   // Set font + colour
 *   txt.set(&FreeSans12pt7b, WY_WHITE);
 *
 *   // Draw at position
 *   txt.draw("Hello", 10, 40);
 *
 *   // Draw centred on screen
 *   txt.drawCentred("CKB WALLET", display.height / 2);
 *
 *   // Draw right-aligned
 *   txt.drawRight("100.00 CKB", display.width - 10, 40);
 *
 *   // Draw with background fill (no flicker on update)
 *   txt.drawFilled("12.5°C", 10, 60, WY_BLACK);
 *
 *   // Measure before drawing
 *   uint16_t w = txt.width("Hello");
 *   uint16_t h = txt.height("Hello");
 *
 *   // Reset to built-in font
 *   txt.reset();
 *
 * Built-in font shortcuts (GFX default font, no external header needed):
 *   txt.setSize(1)  — tiny  (6×8px)
 *   txt.setSize(2)  — small (12×16px)
 *   txt.setSize(3)  — medium
 *   txt.setSize(4)  — large
 *
 * GFX fonts (from Arduino_GFX or Adafruit_GFX font packs):
 *   #include <Fonts/FreeSans9pt7b.h>
 *   txt.set(&FreeSans9pt7b, WY_WHITE);
 *
 * platformio.ini: no extra lib needed — Arduino_GFX includes font support.
 * For extended font packs add: adafruit/Adafruit GFX Library
 */

#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

class WyFont {
public:
    Arduino_GFX    *gfx;
    uint16_t        colour  = 0xFFFF;   /* current text colour */
    uint8_t         size    = 1;        /* GFX text size multiplier */
    const GFXfont  *font    = nullptr;  /* nullptr = built-in 6×8 font */
    uint8_t         wrap    = 1;        /* text wrap enabled */

    WyFont(Arduino_GFX *g, uint16_t col = 0xFFFF)
        : gfx(g), colour(col) {}

    /* ── Font selection ──────────────────────────────────────────── */

    /* Set GFX font + colour */
    void set(const GFXfont *f, uint16_t col = 0xFFFF) {
        font   = f;
        colour = col;
        _apply();
    }

    /* Built-in font with size multiplier (no header needed) */
    void setSize(uint8_t s, uint16_t col = 0xFFFF) {
        font   = nullptr;
        size   = s;
        colour = col;
        _apply();
    }

    /* Colour only */
    void setColour(uint16_t col) {
        colour = col;
        gfx->setTextColor(col);
    }

    /* Reset to built-in font, size 1 */
    void reset(uint16_t col = 0xFFFF) {
        font   = nullptr;
        size   = 1;
        colour = col;
        _apply();
    }

    /* ── Measurement ─────────────────────────────────────────────── */

    uint16_t width(const char *str) {
        _apply();
        int16_t x1, y1; uint16_t w, h;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        return w;
    }

    uint16_t height(const char *str) {
        _apply();
        int16_t x1, y1; uint16_t w, h;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        return h;
    }

    /* Returns pixel width + height of string into w/h */
    void measure(const char *str, uint16_t &w, uint16_t &h) {
        _apply();
        int16_t x1, y1;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
    }

    /* ── Drawing — left-aligned (baseline at y) ──────────────────── */

    void draw(const char *str, int16_t x, int16_t y) {
        _apply();
        gfx->setCursor(x, y);
        gfx->print(str);
    }

    void draw(const String &str, int16_t x, int16_t y) {
        draw(str.c_str(), x, y);
    }

    /* Printf-style */
    void drawf(int16_t x, int16_t y, const char *fmt, ...) {
        char buf[128];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        draw(buf, x, y);
    }

    /* ── Drawing — centred horizontally at y ─────────────────────── */

    void drawCentred(const char *str, int16_t y) {
        _apply();
        int16_t x1, y1; uint16_t w, h;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        gfx->setCursor((gfx->width() - w) / 2 - x1, y);
        gfx->print(str);
    }

    /* Centred at explicit cx, cy */
    void drawCentredAt(const char *str, int16_t cx, int16_t cy) {
        _apply();
        int16_t x1, y1; uint16_t w, h;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        gfx->setCursor(cx - w/2 - x1, cy + h/2);
        gfx->print(str);
    }

    /* Centred in a rect (x, y, w, h) */
    void drawInRect(const char *str, int16_t rx, int16_t ry, int16_t rw, int16_t rh) {
        _apply();
        int16_t x1, y1; uint16_t tw, th;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &tw, &th);
        gfx->setCursor(rx + (rw - tw)/2 - x1, ry + (rh + th)/2);
        gfx->print(str);
    }

    /* ── Drawing — right-aligned at x ───────────────────────────── */

    void drawRight(const char *str, int16_t x, int16_t y) {
        _apply();
        int16_t x1, y1; uint16_t w, h;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        gfx->setCursor(x - w - x1, y);
        gfx->print(str);
    }

    /* ── Drawing — with background fill (update without flicker) ─── */

    void drawFilled(const char *str, int16_t x, int16_t y, uint16_t bg) {
        _apply();
        int16_t x1, y1; uint16_t w, h;
        gfx->getTextBounds(str, x, y, &x1, &y1, &w, &h);
        gfx->fillRect(x1 - 1, y1 - 1, w + 2, h + 2, bg);
        gfx->setCursor(x, y);
        gfx->print(str);
    }

    void drawFilledCentred(const char *str, int16_t y, uint16_t bg) {
        _apply();
        int16_t x1, y1; uint16_t w, h;
        gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
        int16_t cx = (gfx->width() - w) / 2 - x1;
        gfx->getTextBounds(str, cx, y, &x1, &y1, &w, &h);
        gfx->fillRect(x1 - 2, y1 - 2, w + 4, h + 4, bg);
        gfx->setCursor(cx, y);
        gfx->print(str);
    }

    /* ── Multi-line / word wrap ──────────────────────────────────── */
    /*
     * Draws text with manual word-wrap at max_width pixels.
     * Newlines in str are respected.
     * Returns final y position (useful for stacking multiple blocks).
     */
    int16_t drawWrapped(const char *str, int16_t x, int16_t y,
                        uint16_t max_width, uint16_t line_spacing = 4) {
        _apply();
        int16_t cx = x, cy = y;
        uint16_t lh = height("A") + line_spacing;

        char word[64];
        const char *p = str;
        while (*p) {
            // Collect next word
            int wi = 0;
            while (*p && *p != ' ' && *p != '\n' && wi < 63)
                word[wi++] = *p++;
            word[wi] = 0;

            if (*p == '\n') { cx = x; cy += lh; p++; continue; }
            if (*p == ' ')  p++;

            uint16_t ww = width(word);
            if (cx + ww > x + max_width && cx > x) {
                cx  = x;
                cy += lh;
            }
            draw(word, cx, cy);
            cx += ww + width(" ");
        }
        return cy + lh;
    }

    /* ── Label helper — draws label:value pair ───────────────────── */
    /*
     * txt.label("Balance", "100.00 CKB", 12, 80, dim_col, value_col);
     * Draws "Balance" in label_col then "100.00 CKB" in value_col.
     * value is right-aligned to right_edge if right_edge > 0.
     */
    void label(const char *lbl, const char *val,
               int16_t x, int16_t y,
               uint16_t lbl_col, uint16_t val_col,
               int16_t right_edge = -1) {
        _apply();
        setColour(lbl_col);
        draw(lbl, x, y);
        setColour(val_col);
        if (right_edge > 0)
            drawRight(val, right_edge, y);
        else
            draw(val, x + width(lbl) + width(" "), y);
        setColour(colour); // restore
    }

private:
    void _apply() {
        gfx->setFont(font);
        gfx->setTextSize(font ? 1 : size);
        gfx->setTextColor(colour);
        gfx->setTextWrap(wrap);
    }
};

/* ── Quick free functions ────────────────────────────────────────── */
static inline void wyText(Arduino_GFX *gfx, const char *str,
                           int16_t x, int16_t y,
                           uint16_t col = 0xFFFF,
                           const GFXfont *font = nullptr,
                           uint8_t size = 1) {
    gfx->setFont(font);
    gfx->setTextSize(size);
    gfx->setTextColor(col);
    gfx->setCursor(x, y);
    gfx->print(str);
    gfx->setFont(nullptr);
}

static inline void wyTextCentred(Arduino_GFX *gfx, const char *str,
                                  int16_t y, uint16_t col = 0xFFFF,
                                  const GFXfont *font = nullptr) {
    gfx->setFont(font);
    gfx->setTextSize(1);
    gfx->setTextColor(col);
    int16_t x1, y1; uint16_t w, h;
    gfx->getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
    gfx->setCursor((gfx->width() - w)/2 - x1, y);
    gfx->print(str);
    gfx->setFont(nullptr);
}
