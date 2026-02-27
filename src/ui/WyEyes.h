/*
 * ui/WyEyes.h — Animated robot eyes for dual GC9A01 round displays
 * =================================================================
 * Designed for: Double EYE module (WY_BOARD_DOUBLE_EYE)
 * Works with any two GC9A01 round displays sharing an SPI bus.
 *
 * ═══════════════════════════════════════════════════════════════════
 * HOW IT WORKS
 * ═══════════════════════════════════════════════════════════════════
 * Two GC9A01 displays share SCK, MOSI, DC, RST, BL — only CS differs.
 * Each frame: assert CS1, draw left eye, deassert; assert CS2, draw
 * right eye, deassert. Both eyes appear to update simultaneously
 * because the SPI bus is fast enough that the flicker is imperceptible.
 *
 * Eye anatomy drawn per frame:
 *   - Sclera (white of the eye) — filled circle, colour configurable
 *   - Iris   — filled circle, configurable colour + diameter
 *   - Pupil  — filled black circle inside iris
 *   - Highlight — small white dot offset from pupil centre
 *   - Eyelid — filled arc at top/bottom for blink/squint
 *
 * ═══════════════════════════════════════════════════════════════════
 * EXPRESSIONS
 * ═══════════════════════════════════════════════════════════════════
 *   EYES_IDLE       — gentle random drift, occasional blink
 *   EYES_BLINK      — single blink
 *   EYES_LOOK_LEFT  — pupils shift left
 *   EYES_LOOK_RIGHT — pupils shift right
 *   EYES_LOOK_UP    — pupils shift up
 *   EYES_LOOK_DOWN  — pupils shift down
 *   EYES_ANGRY      — lowered inner brows (red tint)
 *   EYES_HAPPY      — upward arc eyelids (squint)
 *   EYES_SAD        — drooped outer eyelids (blue tint)
 *   EYES_SURPRISED  — wide open, pupils small
 *   EYES_SLEEPY     — half-closed lids, slow blink
 *   EYES_DEAD       — X pupils (optional fun mode)
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   #include "ui/WyEyes.h"
 *
 *   WyEyes eyes;
 *   eyes.begin();                    // init both GC9A01s
 *   eyes.setIrisColor(0x07FF);       // cyan iris
 *   eyes.setExpression(EYES_HAPPY);
 *
 *   void loop() {
 *       eyes.update();               // call every loop tick
 *       delay(16);                   // ~60fps
 *   }
 *
 *   // Or use oneshot expressions:
 *   eyes.blink();
 *   eyes.lookAt(20, -10);            // look right and slightly up (pixels)
 *   eyes.setExpression(EYES_IDLE);   // return to idle drift
 */

#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "../boards.h"

/* ── Colour helpers (RGB565) ────────────────────────────────────── */
#define EYES_WHITE   0xFFFF
#define EYES_BLACK   0x0000
#define EYES_RED     0xF800
#define EYES_GREEN   0x07E0
#define EYES_BLUE    0x001F
#define EYES_CYAN    0x07FF
#define EYES_YELLOW  0xFFE0
#define EYES_ORANGE  0xFD20
#define EYES_PURPLE  0x780F
#define EYES_BROWN   0x8200

/* ── Expression enum ────────────────────────────────────────────── */
enum WyExpression {
    EYES_IDLE = 0,
    EYES_BLINK,
    EYES_LOOK_LEFT,
    EYES_LOOK_RIGHT,
    EYES_LOOK_UP,
    EYES_LOOK_DOWN,
    EYES_ANGRY,
    EYES_HAPPY,
    EYES_SAD,
    EYES_SURPRISED,
    EYES_SLEEPY,
    EYES_DEAD,
    EYES_EXPRESSION_COUNT
};

/* ── Eye state ──────────────────────────────────────────────────── */
struct EyeState {
    int16_t  pupilX     = 0;      /* pupil offset from centre, pixels */
    int16_t  pupilY     = 0;
    uint8_t  lidTop     = 0;      /* eyelid closure 0=open, 100=closed */
    uint8_t  lidBottom  = 0;
    uint16_t irisColor  = EYES_CYAN;
    uint16_t scleraColor = EYES_WHITE;
    bool     angry      = false;
    bool     happy      = false;
    bool     sad        = false;
};

/* ── Configurable parameters ────────────────────────────────────── */
#ifndef EYES_W
  #define EYES_W          128    /* display width */
#endif
#ifndef EYES_H
  #define EYES_H          128    /* display height */
#endif
#ifndef EYES_IRIS_R
  #define EYES_IRIS_R     38     /* iris radius (px) */
#endif
#ifndef EYES_PUPIL_R
  #define EYES_PUPIL_R    18     /* pupil radius (px) */
#endif
#ifndef EYES_SCLERA_R
  #define EYES_SCLERA_R   62     /* sclera (white) radius — fills round display */
#endif
#ifndef EYES_PUPIL_RANGE
  #define EYES_PUPIL_RANGE 12   /* max pupil travel from centre (px) */
#endif

static const int16_t EYES_CX = EYES_W / 2;  /* centre X */
static const int16_t EYES_CY = EYES_H / 2;  /* centre Y */

class WyEyes {
public:
    Arduino_GFX* gfxL = nullptr;   /* left eye GFX */
    Arduino_GFX* gfxR = nullptr;   /* right eye GFX */

    WyEyes() {}

    /* Set iris colour (both eyes) */
    void setIrisColor(uint16_t c)    { _stL.irisColor = _stR.irisColor = c; }

    /* Set sclera colour (default white) */
    void setScleraColor(uint16_t c)  { _stL.scleraColor = _stR.scleraColor = c; }

    /* Set expression — persists until changed */
    void setExpression(WyExpression e) {
        _expr = e;
        _exprStartMs = millis();
        _applyExpression(e);
    }

    /* Oneshot blink — returns to current expression after */
    void blink() { _blinkQueued = true; }

    /* Direct pupil position override (pixels from centre, clamped) */
    void lookAt(int16_t dx, int16_t dy) {
        _stL.pupilX = _stR.pupilX = constrain(dx, -EYES_PUPIL_RANGE, EYES_PUPIL_RANGE);
        _stL.pupilY = _stR.pupilY = constrain(dy, -EYES_PUPIL_RANGE, EYES_PUPIL_RANGE);
    }

    /* Independent eye control (for cross-eye effect etc.) */
    void lookAtLeft(int16_t dx, int16_t dy) {
        _stL.pupilX = constrain(dx, -EYES_PUPIL_RANGE, EYES_PUPIL_RANGE);
        _stL.pupilY = constrain(dy, -EYES_PUPIL_RANGE, EYES_PUPIL_RANGE);
    }
    void lookAtRight(int16_t dx, int16_t dy) {
        _stR.pupilX = constrain(dx, -EYES_PUPIL_RANGE, EYES_PUPIL_RANGE);
        _stR.pupilY = constrain(dy, -EYES_PUPIL_RANGE, EYES_PUPIL_RANGE);
    }

    bool begin() {
#if defined(WY_BOARD_DOUBLE_EYE) || defined(WY_HAS_DUAL_DISPLAY)
        auto* bus = new Arduino_ESP32SPI(
            WY_DISPLAY_DC, WY_DISPLAY_CS,
            WY_DISPLAY_SCK, WY_DISPLAY_MOSI, GFX_NOT_DEFINED);
        gfxL = new Arduino_GC9A01(bus, WY_DISPLAY_RST, 0, true);

        auto* busR = new Arduino_ESP32SPI(
            WY_DISPLAY_DC, WY_EYE_CS2,
            WY_DISPLAY_SCK, WY_DISPLAY_MOSI, GFX_NOT_DEFINED);
        gfxR = new Arduino_GC9A01(busR, GFX_NOT_DEFINED, 0, true);

        if (WY_DISPLAY_BL >= 0) {
            ledcSetup(1, 5000, 8);
            ledcAttachPin(WY_DISPLAY_BL, 1);
            ledcWrite(1, 255);
        }
#else
        Serial.println("[WyEyes] define WY_BOARD_DOUBLE_EYE or set CS pins manually");
        return false;
#endif
        if (!gfxL->begin() || !gfxR->begin()) {
            Serial.println("[WyEyes] GC9A01 init failed");
            return false;
        }

        /* Initial clear */
        gfxL->fillScreen(EYES_BLACK);
        gfxR->fillScreen(EYES_BLACK);

        setExpression(EYES_IDLE);
        Serial.println("[WyEyes] ready — dual GC9A01 128×128");
        return true;
    }

    /* Call every loop tick (~60fps ideal, 30fps acceptable) */
    void update() {
        uint32_t now = millis();

        /* Handle blink */
        if (_blinkQueued && !_blinking) {
            _blinking     = true;
            _blinkPhase   = 0;
            _blinkStartMs = now;
            _blinkQueued  = false;
        }

        if (_blinking) {
            uint32_t elapsed = now - _blinkStartMs;
            if (elapsed < 80) {
                /* Close */
                _stL.lidTop = _stR.lidTop = map(elapsed, 0, 80, 0, 100);
            } else if (elapsed < 160) {
                /* Open */
                _stL.lidTop = _stR.lidTop = map(elapsed, 80, 160, 100, 0);
            } else {
                _stL.lidTop = _stR.lidTop = 0;
                _blinking = false;
            }
        }

        /* Idle drift — small random pupil wander */
        if (_expr == EYES_IDLE && !_blinking) {
            if (now - _lastDriftMs > _driftInterval) {
                _driftTargetX = random(-EYES_PUPIL_RANGE, EYES_PUPIL_RANGE + 1);
                _driftTargetY = random(-EYES_PUPIL_RANGE, EYES_PUPIL_RANGE + 1);
                _driftInterval = random(1500, 4000);
                _lastDriftMs   = now;
            }
            /* Smoothly interpolate toward target */
            _stL.pupilX += (_driftTargetX - _stL.pupilX) / 8;
            _stL.pupilY += (_driftTargetY - _stL.pupilY) / 8;
            _stR.pupilX = _stL.pupilX;
            _stR.pupilY = _stL.pupilY;

            /* Random blink every 3–6 seconds */
            if (now - _lastBlinkMs > _nextBlinkMs) {
                blink();
                _lastBlinkMs = now;
                _nextBlinkMs = random(3000, 6000);
            }
        }

        _drawEye(gfxL, _stL);
        _drawEye(gfxR, _stR);
    }

    /* Force immediate redraw */
    void redraw() {
        _drawEye(gfxL, _stL);
        _drawEye(gfxR, _stR);
    }

private:
    WyExpression _expr       = EYES_IDLE;
    EyeState     _stL, _stR;
    uint32_t     _exprStartMs = 0;

    /* Blink state */
    bool         _blinkQueued  = false;
    bool         _blinking     = false;
    uint32_t     _blinkStartMs = 0;
    uint8_t      _blinkPhase   = 0;

    /* Idle drift state */
    uint32_t     _lastDriftMs    = 0;
    uint32_t     _driftInterval  = 2000;
    int16_t      _driftTargetX   = 0;
    int16_t      _driftTargetY   = 0;
    uint32_t     _lastBlinkMs    = 0;
    uint32_t     _nextBlinkMs    = 4000;

    void _applyExpression(WyExpression e) {
        /* Reset to neutral first */
        _stL.lidTop     = _stR.lidTop    = 0;
        _stL.lidBottom  = _stR.lidBottom = 0;
        _stL.angry      = _stR.angry     = false;
        _stL.happy      = _stR.happy     = false;
        _stL.sad        = _stR.sad       = false;
        uint16_t irisC  = _stL.irisColor;

        switch (e) {
            case EYES_IDLE:
                break;
            case EYES_BLINK:
                _blinkQueued = true;
                break;
            case EYES_LOOK_LEFT:
                lookAt(-EYES_PUPIL_RANGE, 0); break;
            case EYES_LOOK_RIGHT:
                lookAt(EYES_PUPIL_RANGE, 0);  break;
            case EYES_LOOK_UP:
                lookAt(0, -EYES_PUPIL_RANGE); break;
            case EYES_LOOK_DOWN:
                lookAt(0, EYES_PUPIL_RANGE);  break;
            case EYES_ANGRY:
                _stL.lidTop = _stR.lidTop = 35;
                _stL.angry  = _stR.angry  = true;
                _stL.irisColor = _stR.irisColor = EYES_RED;
                break;
            case EYES_HAPPY:
                _stL.lidBottom = _stR.lidBottom = 30;
                _stL.happy     = _stR.happy     = true;
                _stL.irisColor = _stR.irisColor = EYES_GREEN;
                break;
            case EYES_SAD:
                _stL.lidTop = _stR.lidTop = 20;
                _stL.sad    = _stR.sad    = true;
                _stL.irisColor = _stR.irisColor = EYES_BLUE;
                break;
            case EYES_SURPRISED:
                _stL.pupilX = _stR.pupilX = 0;
                _stL.pupilY = _stR.pupilY = 0;
                /* Pupils shrink to suggest surprise — draw smaller */
                break;
            case EYES_SLEEPY:
                _stL.lidTop = _stR.lidTop = 55;
                break;
            case EYES_DEAD:
                /* X pupils drawn in _drawEye when _expr == EYES_DEAD */
                break;
            default: break;
        }
        (void)irisC;
    }

    void _drawEye(Arduino_GFX* gfx, const EyeState& st) {
        if (!gfx) return;
        int16_t cx = EYES_CX;
        int16_t cy = EYES_CY;

        /* Background — clear to black (outside sclera) */
        gfx->fillScreen(EYES_BLACK);

        /* Sclera */
        gfx->fillCircle(cx, cy, EYES_SCLERA_R, st.scleraColor);

        /* Iris */
        int16_t px = cx + st.pupilX;
        int16_t py = cy + st.pupilY;
        gfx->fillCircle(px, py, EYES_IRIS_R, st.irisColor);

        /* Pupil */
        uint8_t pupilR = EYES_PUPIL_R;
        if (_expr == EYES_SURPRISED) pupilR = EYES_PUPIL_R / 2;

        if (_expr == EYES_DEAD) {
            /* X eyes */
            int16_t d = pupilR;
            gfx->drawLine(px-d, py-d, px+d, py+d, EYES_BLACK);
            gfx->drawLine(px-d, py+d, px+d, py-d, EYES_BLACK);
            gfx->drawLine(px-d+1, py-d, px+d+1, py+d, EYES_BLACK);
            gfx->drawLine(px-d-1, py-d, px+d-1, py+d, EYES_BLACK);
        } else {
            gfx->fillCircle(px, py, pupilR, EYES_BLACK);
        }

        /* Highlight — small white dot, offset upper-right of pupil */
        int16_t hlX = px + pupilR / 3;
        int16_t hlY = py - pupilR / 3;
        gfx->fillCircle(hlX, hlY, max(2, (int)(pupilR / 5)), EYES_WHITE);

        /* Eyelids — fill rectangles from top/bottom edge */
        if (st.lidTop > 0) {
            int16_t lidH = map(st.lidTop, 0, 100, 0, EYES_H / 2 + 10);
            /* Angry: angled inner corner (simplified as trapezoid) */
            if (st.angry) {
                for (int16_t y = 0; y < lidH; y++) {
                    /* Inner corner higher — rough slant */
                    int16_t xOff = (y * 8) / lidH;
                    gfx->drawFastHLine(cx - EYES_SCLERA_R + xOff, y, EYES_SCLERA_R * 2 - xOff, EYES_BLACK);
                }
            } else if (st.sad) {
                for (int16_t y = 0; y < lidH; y++) {
                    int16_t xOff = ((lidH - y) * 8) / lidH;
                    gfx->drawFastHLine(cx - EYES_SCLERA_R, y, EYES_SCLERA_R * 2 - xOff, EYES_BLACK);
                }
            } else {
                gfx->fillRect(cx - EYES_SCLERA_R, 0, EYES_SCLERA_R * 2, lidH, EYES_BLACK);
            }
        }

        if (st.lidBottom > 0) {
            int16_t lidH = map(st.lidBottom, 0, 100, 0, EYES_H / 2 + 10);
            if (st.happy) {
                /* Happy: curved bottom lid (arc from bottom) */
                for (int16_t y = EYES_H - lidH; y < EYES_H; y++) {
                    int16_t row = y - (EYES_H - lidH);
                    int16_t xOff = (row * 10) / lidH;
                    gfx->drawFastHLine(cx - EYES_SCLERA_R + xOff, y,
                        EYES_SCLERA_R * 2 - xOff * 2, EYES_BLACK);
                }
            } else {
                gfx->fillRect(cx - EYES_SCLERA_R, EYES_H - lidH, EYES_SCLERA_R * 2, lidH, EYES_BLACK);
            }
        }
    }
};
