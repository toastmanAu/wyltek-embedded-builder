// test_boards.cpp — Compile-time and runtime verification of boards.h
// Each board is tested in its own translation unit via a subprocess compile.
// This file tests ONE board per compilation — invoked by run_tests.sh per board.
//
// Build: g++ -std=c++17 -DWY_BOARD_CYD -DBOARD_UNDER_TEST=\"CYD\" -Isrc \
//           test/test_boards.cpp -o test/test_board_CYD
//
// The test verifies:
//   1. WY_BOARD_NAME is defined and non-empty
//   2. WY_MCU_* consistency (cores >= 1, freq in {80,160,240,400})
//   3. If WY_HAS_DISPLAY=1 then WY_DISPLAY_W/H/ROT are defined and sane
//   4. If WY_HAS_TOUCH=1 then touch type is defined
//   5. WY_SCREEN_W/H match DISPLAY_W/H (or are independently defined)
//   6. No conflicting MCU type defines
//   7. WY_HAS_PSRAM is 0 or 1
//   8. If WY_HAS_RGB_LED=1 then WY_LED_R/G/B are defined (>= 0)
//   9. GPIO pins are in valid ESP32 range (-1 for not-present, or 0..48)

#include <stdio.h>
#include <string.h>

// Stub out Arduino.h — boards.h is pure #define, no Arduino calls needed
#pragma once

// Include the board definitions
#include "boards.h"

// ── Test infrastructure ────────────────────────────────────────────────────────
static int _pass = 0, _fail = 0;
#define PASS(n)       do { printf("  PASS: %s\n", n); _pass++; } while(0)
#define FAIL(n, m)    do { printf("  FAIL: %s  (%s)\n", n, m); _fail++; } while(0)
#define CHECK(c,n,m)  do { if(c) PASS(n); else FAIL(n,m); } while(0)
#define SECTION(s)    printf("\n  [%s]\n", s)
#define XSTR(x) #x
#define STR(x) XSTR(x)

static bool pinValid(int p) { return p == -1 || (p >= 0 && p <= 48); }

int main() {
    printf("\n========================================\n");
#ifdef WY_BOARD_NAME
    printf("  Board: %s\n", WY_BOARD_NAME);
#else
    printf("  Board: (WY_BOARD_NAME not defined)\n");
#endif
    printf("========================================\n");

    // ── 1. Identity ────────────────────────────────────────────────────────────
    SECTION("Identity");
    {
#ifdef WY_BOARD_NAME
        CHECK(strlen(WY_BOARD_NAME) > 4, "WY_BOARD_NAME non-empty", "empty");
        CHECK(strstr(WY_BOARD_NAME, "ESP32") || strstr(WY_BOARD_NAME, "LilyGo") ||
              strstr(WY_BOARD_NAME, "TTGO")  || strstr(WY_BOARD_NAME, "M5")     ||
              strstr(WY_BOARD_NAME, "Wemos") || strstr(WY_BOARD_NAME, "Adafruit") ||
              strstr(WY_BOARD_NAME, "Waveshare") || strstr(WY_BOARD_NAME, "Sunton") ||
              strstr(WY_BOARD_NAME, "Guition") || strstr(WY_BOARD_NAME, "Heltec") ||
              strstr(WY_BOARD_NAME, "XIAO")   || strstr(WY_BOARD_NAME, "WT32") ||
              strstr(WY_BOARD_NAME, "Double") || strstr(WY_BOARD_NAME, "Generic") ||
              strstr(WY_BOARD_NAME, "Freenove") || strstr(WY_BOARD_NAME, "T-") ||
              strstr(WY_BOARD_NAME, "LOLIN") || strstr(WY_BOARD_NAME, "Wemos"),
              "WY_BOARD_NAME contains known brand/type", WY_BOARD_NAME);
#else
        FAIL("WY_BOARD_NAME defined", "not defined — board not matched in boards.h");
#endif
    }

    // ── 2. MCU ─────────────────────────────────────────────────────────────────
    SECTION("MCU");
    {
#ifdef WY_MCU_CORES
        CHECK(WY_MCU_CORES >= 1 && WY_MCU_CORES <= 2, "WY_MCU_CORES in [1,2]",
              "cores=" STR(WY_MCU_CORES));
#else
        FAIL("WY_MCU_CORES defined", "not defined");
#endif
#ifdef WY_MCU_FREQ
        CHECK(WY_MCU_FREQ == 80 || WY_MCU_FREQ == 160 ||
              WY_MCU_FREQ == 240 || WY_MCU_FREQ == 400,
              "WY_MCU_FREQ in {80,160,240,400}", "freq=" STR(WY_MCU_FREQ));
#else
        FAIL("WY_MCU_FREQ defined", "not defined");
#endif
        // Exactly one MCU type
        int mcuCount = 0;
#ifdef WY_MCU_ESP32
        mcuCount++;
#endif
#ifdef WY_MCU_ESP32S2
        mcuCount++;
#endif
#ifdef WY_MCU_ESP32S3
        mcuCount++;
#endif
#ifdef WY_MCU_ESP32C3
        mcuCount++;
#endif
#ifdef WY_MCU_ESP32C6
        mcuCount++;
#endif
#ifdef WY_MCU_ESP32P4
        mcuCount++;
#endif
        char mcuMsg[32]; snprintf(mcuMsg, sizeof(mcuMsg), "count=%d", mcuCount);
        CHECK(mcuCount == 1, "exactly one WY_MCU_* type defined", mcuMsg);

#ifdef WY_HAS_PSRAM
        CHECK(WY_HAS_PSRAM == 0 || WY_HAS_PSRAM == 1,
              "WY_HAS_PSRAM is 0 or 1", "invalid value");
#else
        FAIL("WY_HAS_PSRAM defined", "not defined");
#endif
    }

    // ── 3. Display ─────────────────────────────────────────────────────────────
    SECTION("Display");
    {
#if WY_HAS_DISPLAY
        CHECK(WY_DISPLAY_W >= 64 && WY_DISPLAY_W <= 1920,
              "WY_DISPLAY_W in [64,1920]", "bad value");
        CHECK(WY_DISPLAY_H >= 64 && WY_DISPLAY_H <= 1080,
              "WY_DISPLAY_H in [64,1080]", "bad value");
        CHECK(WY_DISPLAY_ROT >= 0 && WY_DISPLAY_ROT <= 3,
              "WY_DISPLAY_ROT in [0,3]", "bad value");
        // Screen logical size
        CHECK(WY_SCREEN_W > 0, "WY_SCREEN_W > 0", "zero or undefined");
        CHECK(WY_SCREEN_H > 0, "WY_SCREEN_H > 0", "zero or undefined");
        // Landscape: W > H; Portrait: H > W; Square: W == H — all valid
        CHECK((WY_SCREEN_W * WY_SCREEN_H) >= (64*64),
              "WY_SCREEN_W*H >= 64x64", "implausibly small");
        // At least one display driver defined
        int dispDrv = 0;
#ifdef WY_DISPLAY_ILI9341
        dispDrv++;
#endif
#ifdef WY_DISPLAY_ILI9342
        dispDrv++;
#endif
#ifdef WY_DISPLAY_ST7789
        dispDrv++;
#endif
#ifdef WY_DISPLAY_ST7701S
        dispDrv++;
#endif
#ifdef WY_DISPLAY_ST7735
        dispDrv++;
#endif
#ifdef WY_DISPLAY_GC9A01
        dispDrv++;
#endif
#ifdef WY_DISPLAY_GC9107
        dispDrv++;
#endif
#ifdef WY_DISPLAY_RM67162
        dispDrv++;
#endif
#ifdef WY_DISPLAY_ST7262
        dispDrv++;
#endif
#ifdef WY_DISPLAY_OV2640
        dispDrv++;
#endif
#ifdef WY_DISPLAY_ILI9488
        dispDrv++;
#endif
#ifdef WY_DISPLAY_ST7796
        dispDrv++;
#endif
#ifdef WY_DISPLAY_RGB_PANEL
        dispDrv++;
#endif
#ifdef WY_DISPLAY_SH8601
        dispDrv++;
#endif
#ifdef WY_DISPLAY_SSD1306
        dispDrv++;
#endif
#ifdef WY_DISPLAY_SH1106
        dispDrv++;
#endif
#ifdef WY_DISPLAY_SSD1309
        dispDrv++;
#endif
        char drvMsg[32]; snprintf(drvMsg, sizeof(drvMsg), "count=%d", dispDrv);
        CHECK(dispDrv >= 1, "at least one WY_DISPLAY_* driver defined", drvMsg);
#else
        PASS("no display — skipping display checks");
#endif
    }

    // ── 4. Touch ───────────────────────────────────────────────────────────────
    SECTION("Touch");
    {
#if WY_HAS_TOUCH
        int touchDrv = 0;
#ifdef WY_TOUCH_XPT2046
        touchDrv++;
#endif
#ifdef WY_TOUCH_GT911
        touchDrv++;
#endif
#ifdef WY_TOUCH_FT5X06
        touchDrv++;
#endif
#ifdef WY_TOUCH_FT6336
        touchDrv++;
#endif
#ifdef WY_TOUCH_CST816S
        touchDrv++;
#endif
#ifdef WY_TOUCH_CHSC5816
        touchDrv++;
#endif
#ifdef WY_TOUCH_AXS15231
        touchDrv++;
#endif
#ifdef WY_TOUCH_FT3267
        touchDrv++;
#endif
#ifdef WY_TOUCH_FT6236
        touchDrv++;
#endif
#ifdef WY_TOUCH_LILYGO_AMOLED
        touchDrv++;
#endif
        char tdrvMsg[32]; snprintf(tdrvMsg, sizeof(tdrvMsg), "count=%d", touchDrv);
        CHECK(touchDrv >= 1, "at least one WY_TOUCH_* driver defined", tdrvMsg);
        // Touch calibration range sanity (if present)
#if defined(WY_TOUCH_X_MAX) && defined(WY_TOUCH_Y_MAX)
        CHECK(WY_TOUCH_X_MAX > 0, "WY_TOUCH_X_MAX > 0", "zero");
        CHECK(WY_TOUCH_Y_MAX > 0, "WY_TOUCH_Y_MAX > 0", "zero");
#endif
#else
        PASS("no touch — skipping touch checks");
#endif
    }

    // ── 5. RGB LED ─────────────────────────────────────────────────────────────
    SECTION("RGB LED");
    {
#if WY_HAS_RGB_LED
#if defined(WY_LED_R) && defined(WY_LED_G) && defined(WY_LED_B)
        CHECK(pinValid(WY_LED_R), "WY_LED_R pin valid", "out of range");
        CHECK(pinValid(WY_LED_G), "WY_LED_G pin valid", "out of range");
        CHECK(pinValid(WY_LED_B), "WY_LED_B pin valid", "out of range");
        // All -1 means WS2812 only — distinct check doesn't apply
        bool allMinus1 = (WY_LED_R == -1 && WY_LED_G == -1 && WY_LED_B == -1);
        if (!allMinus1) {
            CHECK(WY_LED_R != WY_LED_G && WY_LED_G != WY_LED_B && WY_LED_R != WY_LED_B,
                  "LED pins are distinct", "conflict");
        } else {
            PASS("LED pins all -1 (WS2812 only — distinct check skipped)");
        }
#else
        FAIL("WY_LED_R/G/B defined when WY_HAS_RGB_LED=1", "missing");
#endif
#else
        PASS("no RGB LED — skipping");
#endif
    }

    // ── 6. Boot button ─────────────────────────────────────────────────────────
    SECTION("Boot button");
    {
#ifdef WY_BOOT_BTN
        CHECK(pinValid(WY_BOOT_BTN), "WY_BOOT_BTN pin valid", "out of range");
#else
        // Optional — some boards don't expose it
        PASS("WY_BOOT_BTN not defined (optional)");
#endif
    }

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", _pass, _fail);
    printf("========================================\n");
    return _fail ? 1 : 0;
}
