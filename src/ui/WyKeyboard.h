/*
 * WyKeyboard.h — On-screen keyboard for wyltek-embedded-builder
 * ==============================================================
 * Adaptive touch keyboard that scales to any display size.
 * Works with any Arduino_GFX display + WyTouch touch input.
 *
 * Features:
 *   - QWERTY, Numeric, and Symbol layouts
 *   - Auto-scales key size to display width/height
 *   - Shift, Caps Lock, backspace, enter, space
 *   - Optional: password mode (shows ● instead of chars)
 *   - Optional: input length limit
 *   - Callback-based or polling API
 *   - Zero heap allocation — all buffers static/stack
 *
 * Usage:
 *   #include <ui/WyKeyboard.h>
 *
 *   WyKeyboard kb;
 *   kb.begin(display.gfx, display.width, display.height);
 *
 *   // Show keyboard at bottom half of screen, 64 char limit
 *   kb.show("Enter WiFi password:", 64, true);  // true = password mode
 *
 *   // In loop():
 *   int tx, ty;
 *   if (touch.getXY(&tx, &ty)) {
 *       WyKbResult res = kb.press(tx, ty);
 *       if (res == WY_KB_DONE)   String val = kb.value();
 *       if (res == WY_KB_CANCEL) Serial.println("cancelled");
 *   }
 *
 *   // Or polling (non-blocking):
 *   if (kb.active()) kb.press(tx, ty);
 *
 * Requires: WY_HAS_DISPLAY=1, WY_HAS_TOUCH=1
 */

#pragma once
#include "boards.h"

#if WY_HAS_DISPLAY

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

/* ── Result codes ────────────────────────────────────────────────── */
enum WyKbResult {
    WY_KB_NONE   = 0,   /* no action */
    WY_KB_TYPING = 1,   /* character added/removed */
    WY_KB_DONE   = 2,   /* Enter pressed */
    WY_KB_CANCEL = 3,   /* Cancel/ESC pressed */
};

/* ── Keyboard layout ─────────────────────────────────────────────── */
enum WyKbLayout {
    WY_KB_QWERTY  = 0,
    WY_KB_NUMERIC = 1,
    WY_KB_SYMBOLS = 2,
};

/* ── Theme ───────────────────────────────────────────────────────── */
struct WyKbTheme {
    uint16_t bg;          /* keyboard background */
    uint16_t key_bg;      /* normal key background */
    uint16_t key_fg;      /* normal key text */
    uint16_t key_special; /* shift/del/enter background */
    uint16_t key_accent;  /* enter/done key */
    uint16_t key_press;   /* key highlight on press */
    uint16_t field_bg;    /* input field background */
    uint16_t field_fg;    /* input field text */
    uint16_t label_fg;    /* prompt label */
    uint16_t border;      /* field border */
};

/* Default dark theme */
static const WyKbTheme WY_KB_THEME_DARK = {
    .bg          = 0x1082,   /* #101820 */
    .key_bg      = 0x2945,   /* #212830 */
    .key_fg      = 0xEF7D,   /* #E8ECF0 */
    .key_special = 0x39C7,   /* #374050 */
    .key_accent  = 0x0696,   /* #00D4AA teal */
    .key_press   = 0x0475,   /* #009080 */
    .field_bg    = 0x10A2,   /* #101820 slightly lighter */
    .field_fg    = 0xEF7D,
    .label_fg    = 0x8C71,   /* #808080 */
    .border      = 0x0696,
};

/* Light theme */
static const WyKbTheme WY_KB_THEME_LIGHT = {
    .bg          = 0xD69A,   /* #D0D8E0 */
    .key_bg      = 0xFFFF,
    .key_fg      = 0x0000,
    .key_special = 0xC618,   /* #C0C8D0 */
    .key_accent  = 0x0696,
    .key_press   = 0xB5B6,
    .field_bg    = 0xFFFF,
    .field_fg    = 0x0000,
    .label_fg    = 0x4A49,
    .border      = 0x0696,
};

/* ══════════════════════════════════════════════════════════════════
 * WyKeyboard
 * ══════════════════════════════════════════════════════════════════ */
class WyKeyboard {
public:
    static const int MAX_LEN = 128;

    /* ── Init ────────────────────────────────────────────────────── */
    void begin(Arduino_GFX *gfx, uint16_t screen_w, uint16_t screen_h,
               const WyKbTheme *theme = &WY_KB_THEME_DARK) {
        _gfx    = gfx;
        _sw     = screen_w;
        _sh     = screen_h;
        _theme  = theme;
        _active = false;
        _recalc_layout();
    }

    /* ── Show keyboard ───────────────────────────────────────────── */
    /*
     * prompt   — label above input field (nullptr = none)
     * maxLen   — maximum input length (0 = MAX_LEN)
     * password — mask input with ●
     * layout   — initial layout (QWERTY/NUMERIC/SYMBOLS)
     * yOffset  — top of keyboard area in pixels (default = auto: bottom half)
     */
    void show(const char *prompt = nullptr,
              uint8_t     maxLen   = 0,
              bool        password = false,
              WyKbLayout  layout   = WY_KB_QWERTY,
              int         yOffset  = -1) {
        _active   = true;
        _password = password;
        _maxLen   = maxLen ? maxLen : MAX_LEN;
        _layout   = layout;
        _shift    = false;
        _caps     = false;
        _bufLen   = 0;
        _buf[0]   = '\0';

        if (prompt) {
            strncpy(_prompt, prompt, sizeof(_prompt) - 1);
            _prompt[sizeof(_prompt) - 1] = '\0';
        } else {
            _prompt[0] = '\0';
        }

        /* Keyboard top = yOffset or auto (bottom 55% of screen) */
        _kb_y = (yOffset >= 0) ? yOffset : (int)(_sh * 0.45f);
        _recalc_layout();
        _draw_all();
    }

    /* ── Hide keyboard ───────────────────────────────────────────── */
    void hide() {
        _active = false;
        /* Caller is responsible for redrawing the screen area */
    }

    /* ── Process a touch event ───────────────────────────────────── */
    WyKbResult press(int tx, int ty) {
        if (!_active) return WY_KB_NONE;

        /* Only handle touches in keyboard area */
        if (ty < _kb_y) return WY_KB_NONE;

        /* Hit test all keys */
        for (int i = 0; i < _key_count; i++) {
            KeyDef &k = _keys[i];
            if (tx >= k.x && tx <= k.x + k.w &&
                ty >= k.y && ty <= k.y + k.h) {
                return _handle_key(i);
            }
        }
        return WY_KB_NONE;
    }

    /* ── Getters ─────────────────────────────────────────────────── */
    bool        active()  const { return _active; }
    const char *value()   const { return _buf; }
    int         length()  const { return _bufLen; }
    int         kb_top()  const { return _kb_y; }  /* y where keyboard starts */

    /* ── Set initial value (e.g. for edit mode) ──────────────────── */
    void setValue(const char *s) {
        strncpy(_buf, s, _maxLen);
        _buf[_maxLen] = '\0';
        _bufLen = strlen(_buf);
        _draw_field();
    }

    /* ── Clear input ─────────────────────────────────────────────── */
    void clear() {
        _bufLen = 0;
        _buf[0] = '\0';
        _draw_field();
    }

private:
    /* ── Types ───────────────────────────────────────────────────── */
    enum KeyType {
        KT_CHAR,    /* printable character */
        KT_SHIFT,
        KT_CAPS,
        KT_BACKSPACE,
        KT_ENTER,
        KT_CANCEL,
        KT_SPACE,
        KT_LAYOUT,  /* switch layout */
        KT_CLEAR,
    };

    struct KeyDef {
        int16_t x, y, w, h;
        KeyType type;
        char    ch;         /* for KT_CHAR */
        char    label[6];   /* display label */
        WyKbLayout switchTo; /* for KT_LAYOUT */
    };

    static const int MAX_KEYS = 48;

    /* ── State ───────────────────────────────────────────────────── */
    Arduino_GFX      *_gfx     = nullptr;
    const WyKbTheme  *_theme   = &WY_KB_THEME_DARK;
    uint16_t          _sw, _sh;
    bool              _active  = false;
    bool              _password= false;
    bool              _shift   = false;
    bool              _caps    = false;
    uint8_t           _maxLen  = MAX_LEN;
    WyKbLayout        _layout  = WY_KB_QWERTY;

    char     _buf[MAX_LEN + 1];
    int      _bufLen = 0;
    char     _prompt[64];

    KeyDef   _keys[MAX_KEYS];
    int      _key_count = 0;
    int      _kb_y      = 0;   /* top of keyboard panel in screen coords */

    /* Computed layout dimensions */
    int _key_h;     /* key height */
    int _key_gap;   /* gap between keys */
    int _row_gap;   /* gap between rows */
    int _field_h;   /* input field height */
    int _label_h;   /* prompt label height */

    /* ── Layout recalc ───────────────────────────────────────────── */
    void _recalc_layout() {
        int kb_height = _sh - _kb_y;

        /* Rows: label + field + 4 key rows + bottom row = 7 slots */
        _label_h = max(14, kb_height / 14);
        _field_h = max(24, kb_height / 8);
        _key_gap = max(2,  kb_height / 60);
        _row_gap = max(2,  kb_height / 50);
        int rows_area = kb_height - _label_h - _field_h - _key_gap * 2 - _row_gap * 2;
        _key_h = max(20, rows_area / 5 - _row_gap);
    }

    /* ── Build QWERTY key grid ───────────────────────────────────── */
    void _build_keys_qwerty() {
        static const char *ROW0 = "qwertyuiop";
        static const char *ROW1 = "asdfghjkl";
        static const char *ROW2 = "zxcvbnm";

        _key_count = 0;

        int y = _kb_y + _label_h + _field_h + _key_gap;

        /* Row 0: q-p (10 keys) */
        _add_char_row(ROW0, 10, y, 0, false);
        y += _key_h + _row_gap;

        /* Row 1: a-l (9 keys) — centred */
        _add_char_row(ROW1, 9, y, _sw / 20, false);
        y += _key_h + _row_gap;

        /* Row 2: SHIFT + z-m + BACKSPACE */
        int side_w = max(28, _sw / 9);
        _add_char_row(ROW2, 7, y, side_w + _key_gap, false);
        /* SHIFT left */
        _add_key(0, y, side_w, _key_h, KT_SHIFT, 0, _shift ? "⬆!" : "⬆");
        /* BACKSPACE right */
        _add_key(_sw - side_w, y, side_w, _key_h, KT_BACKSPACE, 0, "⌫");
        y += _key_h + _row_gap;

        /* Row 3: ?123 | SPACE | CANCEL | ENTER */
        int sym_w  = max(40, _sw / 8);
        int ent_w  = max(52, _sw / 6);
        int cxl_w  = max(40, _sw / 9);
        int spc_w  = _sw - sym_w - ent_w - cxl_w - 4 * _key_gap;

        int rx = _key_gap;
        _add_key(rx, y, sym_w, _key_h, KT_LAYOUT, 0, "?123", WY_KB_NUMERIC); rx += sym_w + _key_gap;
        _add_key(rx, y, spc_w, _key_h, KT_SPACE,  0, "SPACE");               rx += spc_w + _key_gap;
        _add_key(rx, y, cxl_w, _key_h, KT_CANCEL, 0, "ESC");                 rx += cxl_w + _key_gap;
        _add_key(rx, y, ent_w, _key_h, KT_ENTER,  0, "OK");
    }

    void _build_keys_numeric() {
        _key_count = 0;
        /* 3×3 numpad + 0 row + extras */
        static const char *rows[] = {"789", "456", "123"};
        int cell_w = _sw / 4;
        int y = _kb_y + _label_h + _field_h + _key_gap;

        for (int r = 0; r < 3; r++) {
            for (int c = 0; c < 3; c++) {
                char ch = rows[r][c];
                char label[2] = {ch, 0};
                _add_key(c * cell_w + _key_gap, y,
                         cell_w - _key_gap, _key_h,
                         KT_CHAR, ch, label);
            }
            /* Extra symbols column */
            static const char extras[] = "+-*/";
            char elabel[2] = {extras[r], 0};
            _add_key(3 * cell_w + _key_gap, y, cell_w - _key_gap * 2, _key_h,
                     KT_CHAR, extras[r], elabel);
            y += _key_h + _row_gap;
        }
        /* Bottom: ABC | 0 | . | DEL */
        int abc_w = cell_w;
        _add_key(_key_gap,                y, abc_w - _key_gap,    _key_h, KT_LAYOUT,    0, "ABC",  WY_KB_QWERTY);
        _add_key(abc_w + _key_gap,        y, cell_w - _key_gap,   _key_h, KT_CHAR,      '0', "0");
        _add_key(2*cell_w + _key_gap,     y, cell_w - _key_gap,   _key_h, KT_CHAR,      '.', ".");
        _add_key(3*cell_w + _key_gap,     y, cell_w - _key_gap*2, _key_h, KT_BACKSPACE, 0,   "⌫");
        y += _key_h + _row_gap;
        /* CANCEL | SPACE | ENTER */
        int cxl_w = _sw / 4, ent_w = _sw / 4;
        int spc_w = _sw - cxl_w - ent_w - 3 * _key_gap;
        int rx = _key_gap;
        _add_key(rx, y, cxl_w - _key_gap, _key_h, KT_CANCEL, 0, "ESC");   rx += cxl_w;
        _add_key(rx, y, spc_w,             _key_h, KT_SPACE,  0, "SPACE"); rx += spc_w + _key_gap;
        _add_key(rx, y, ent_w - _key_gap,  _key_h, KT_ENTER,  0, "OK");
    }

    void _build_keys_symbols() {
        _key_count = 0;
        static const char *ROW0 = "!@#$%^&*()";
        static const char *ROW1 = "-_=+[]{}|;";
        static const char *ROW2 = ":',.<>?/`~";

        int y = _kb_y + _label_h + _field_h + _key_gap;
        _add_char_row(ROW0, 10, y, 0, false); y += _key_h + _row_gap;
        _add_char_row(ROW1, 10, y, 0, false); y += _key_h + _row_gap;
        _add_char_row(ROW2, 10, y, 0, false); y += _key_h + _row_gap;

        int abc_w = _sw / 4, del_w = _sw / 5;
        int spc_w = _sw - abc_w - del_w - 3 * _key_gap;
        int rx = _key_gap;
        _add_key(rx, y, abc_w - _key_gap, _key_h, KT_LAYOUT,    0, "ABC",  WY_KB_QWERTY); rx += abc_w;
        _add_key(rx, y, spc_w,            _key_h, KT_SPACE,     0, "SPACE");               rx += spc_w + _key_gap;
        _add_key(rx, y, del_w - _key_gap, _key_h, KT_BACKSPACE, 0, "⌫");
        y += _key_h + _row_gap;

        int cx = _key_gap, ex = _sw / 2;
        _add_key(cx, y, _sw/2 - _key_gap*2, _key_h, KT_CANCEL, 0, "ESC");
        _add_key(ex, y, _sw/2 - _key_gap,   _key_h, KT_ENTER,  0, "OK");
    }

    /* ── Add a row of character keys ─────────────────────────────── */
    void _add_char_row(const char *chars, int count,
                        int y, int x_offset, bool upper) {
        /* Distribute keys evenly across width */
        int total_gap  = _key_gap * (count + 1);
        int key_w      = (_sw - total_gap - x_offset * 2) / count;
        int x          = x_offset + _key_gap;
        for (int i = 0; i < count && _key_count < MAX_KEYS; i++) {
            char c   = chars[i];
            char uc  = (_shift || _caps || upper) ? toupper(c) : c;
            char label[2] = {uc, 0};
            _add_key(x, y, key_w, _key_h, KT_CHAR, uc, label);
            x += key_w + _key_gap;
        }
    }

    /* ── Add a single key ────────────────────────────────────────── */
    void _add_key(int x, int y, int w, int h,
                  KeyType type, char ch, const char *label,
                  WyKbLayout switchTo = WY_KB_QWERTY) {
        if (_key_count >= MAX_KEYS) return;
        KeyDef &k = _keys[_key_count++];
        k.x = x; k.y = y; k.w = w; k.h = h;
        k.type = type; k.ch = ch;
        k.switchTo = switchTo;
        strncpy(k.label, label, sizeof(k.label) - 1);
        k.label[sizeof(k.label) - 1] = '\0';
    }

    /* ── Rebuild key positions ───────────────────────────────────── */
    void _rebuild_keys() {
        switch (_layout) {
            case WY_KB_QWERTY:  _build_keys_qwerty();  break;
            case WY_KB_NUMERIC: _build_keys_numeric();  break;
            case WY_KB_SYMBOLS: _build_keys_symbols();  break;
        }
    }

    /* ── Handle key press ────────────────────────────────────────── */
    WyKbResult _handle_key(int idx) {
        KeyDef &k = _keys[idx];

        /* Flash key */
        _draw_key(idx, true);
        delay(60);
        _draw_key(idx, false);

        switch (k.type) {
            case KT_CHAR:
                if (_bufLen < _maxLen) {
                    _buf[_bufLen++] = k.ch;
                    _buf[_bufLen]   = '\0';
                    if (_shift && !_caps) {
                        _shift = false;
                        _rebuild_keys();
                        _draw_keys();
                    }
                    _draw_field();
                    return WY_KB_TYPING;
                }
                return WY_KB_NONE;

            case KT_BACKSPACE:
                if (_bufLen > 0) {
                    _buf[--_bufLen] = '\0';
                    _draw_field();
                    return WY_KB_TYPING;
                }
                return WY_KB_NONE;

            case KT_SPACE:
                if (_bufLen < _maxLen) {
                    _buf[_bufLen++] = ' ';
                    _buf[_bufLen]   = '\0';
                    _draw_field();
                    return WY_KB_TYPING;
                }
                return WY_KB_NONE;

            case KT_SHIFT:
                _shift = !_shift;
                _caps  = false;
                _rebuild_keys();
                _draw_keys();
                return WY_KB_TYPING;

            case KT_CAPS:
                _caps  = !_caps;
                _shift = false;
                _rebuild_keys();
                _draw_keys();
                return WY_KB_TYPING;

            case KT_CLEAR:
                _bufLen = 0;
                _buf[0] = '\0';
                _draw_field();
                return WY_KB_TYPING;

            case KT_LAYOUT:
                _layout = k.switchTo;
                _rebuild_keys();
                _draw_keys();
                return WY_KB_NONE;

            case KT_ENTER:
                _active = false;
                return WY_KB_DONE;

            case KT_CANCEL:
                _active = false;
                _bufLen = 0;
                _buf[0] = '\0';
                return WY_KB_CANCEL;

            default:
                return WY_KB_NONE;
        }
    }

    /* ── Draw: all ───────────────────────────────────────────────── */
    void _draw_all() {
        /* Background */
        _gfx->fillRect(0, _kb_y, _sw, _sh - _kb_y, _theme->bg);

        _draw_label();
        _draw_field();
        _rebuild_keys();
        _draw_keys();
    }

    /* ── Draw: prompt label ──────────────────────────────────────── */
    void _draw_label() {
        if (!_prompt[0]) return;
        int y = _kb_y + 2;
        _gfx->setTextSize(1);
        _gfx->setTextColor(_theme->label_fg);
        _gfx->setCursor(6, y + 4);
        _gfx->print(_prompt);
    }

    /* ── Draw: input field ───────────────────────────────────────── */
    void _draw_field() {
        int fy = _kb_y + _label_h;
        int fw = _sw - 4;
        _gfx->fillRect(2, fy, fw, _field_h, _theme->field_bg);
        _gfx->drawRect(2, fy, fw, _field_h, _theme->border);

        /* Display text or password mask */
        _gfx->setTextSize(1);
        _gfx->setTextColor(_theme->field_fg);
        _gfx->setCursor(8, fy + (_field_h - 8) / 2);

        if (_password && _bufLen > 0) {
            /* Show ●●●● */
            for (int i = 0; i < _bufLen; i++) _gfx->print('\xDB'); /* solid block */
        } else {
            /* Scroll to show last N chars if too long */
            int max_chars = (fw - 16) / 6;  /* approx for default font */
            if (_bufLen > max_chars) {
                _gfx->print(_buf + _bufLen - max_chars);
            } else {
                _gfx->print(_buf);
            }
        }

        /* Cursor */
        int cx = 8 + min(_bufLen, (fw - 16) / 6) * 6;
        if (cx < fw - 4) {
            _gfx->fillRect(cx, fy + 4, 2, _field_h - 8, _theme->border);
        }
    }

    /* ── Draw: all keys ──────────────────────────────────────────── */
    void _draw_keys() {
        for (int i = 0; i < _key_count; i++) _draw_key(i, false);
    }

    /* ── Draw: single key ────────────────────────────────────────── */
    void _draw_key(int idx, bool pressed) {
        KeyDef &k = _keys[idx];

        /* Background colour */
        uint16_t bg, fg;
        if (pressed) {
            bg = _theme->key_press;
            fg = _theme->key_fg;
        } else {
            switch (k.type) {
                case KT_ENTER:
                    bg = _theme->key_accent;
                    fg = 0x0000;
                    break;
                case KT_SHIFT:
                    bg = (_shift || _caps) ? _theme->key_accent : _theme->key_special;
                    fg = (_shift || _caps) ? 0x0000 : _theme->key_fg;
                    break;
                case KT_BACKSPACE:
                case KT_CANCEL:
                case KT_LAYOUT:
                    bg = _theme->key_special;
                    fg = _theme->key_fg;
                    break;
                case KT_SPACE:
                    bg = _theme->key_special;
                    fg = _theme->label_fg;
                    break;
                default:
                    bg = _theme->key_bg;
                    fg = _theme->key_fg;
                    break;
            }
        }

        /* Draw rounded rectangle */
        int r = min(4, _key_h / 5);
        _gfx->fillRoundRect(k.x, k.y, k.w, k.h, r, bg);

        /* Label */
        _gfx->setTextSize(1);
        _gfx->setTextColor(fg);

        /* Centre label in key */
        int label_w = strlen(k.label) * 6;
        int label_h = 8;
        int lx = k.x + (k.w - label_w) / 2;
        int ly = k.y + (k.h - label_h) / 2;
        if (lx < k.x) lx = k.x + 2;
        _gfx->setCursor(lx, ly);
        _gfx->print(k.label);
    }
};

/* ── Convenience macro ───────────────────────────────────────────── */
/*
 * WY_KB_POLL(kb, touch, result) — call in loop() when keyboard is active
 *
 * touch must provide getXY(int *x, int *y) returning bool.
 * result will be set to WyKbResult.
 */
#define WY_KB_POLL(kb, touch, result) \
    do { \
        int _kbx, _kby; \
        if ((kb).active() && (touch).getXY(&_kbx, &_kby)) { \
            (result) = (kb).press(_kbx, _kby); \
        } \
    } while(0)

#endif /* WY_HAS_DISPLAY */
