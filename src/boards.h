/*
 * boards.h — Board registry for wyltek-embedded-builder
 * ======================================================
 * Each board define sets:
 *   WY_MCU_*        — chip family, cores, flash, PSRAM
 *   WY_DISPLAY_*    — driver, bus type, resolution, pins, backlight
 *   WY_TOUCH_*      — controller, bus type, pins, calibration
 *   WY_LED_*        — RGB LED pins (if present)
 *   WY_BOOT_BTN     — BOOT/user button GPIO
 *   WY_SCREEN_W/H   — logical screen dimensions (after rotation)
 *
 * Usage in platformio.ini:
 *   build_flags = -DWY_BOARD_CYD
 *
 * Compile-time gating — modules only pull in what a board needs:
 *   WY_HAS_DISPLAY, WY_HAS_TOUCH, WY_HAS_RGB_LED, WY_HAS_PSRAM
 */

#pragma once

/* ══════════════════════════════════════════════════════════════════
 * ESP32-2432S028R  — "CYD" Cheap Yellow Display (2.8" 320×240)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-D0WDQ6 dual-core 240MHz, 4MB flash, no PSRAM
 * Display: ILI9341, SPI (VSPI), 320×240, BL=PWM
 * Touch:   XPT2046, SPI (HSPI, separate bus), resistive
 * LED:     RGB on GPIO 4/16/17 (active LOW)
 * Boot btn: GPIO 0
 * USB:     Single micro-USB
 */
#if defined(WY_BOARD_CYD)
  #define WY_BOARD_NAME       "ESP32-2432S028R (CYD)"
  /* MCU */
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  /* Display */
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ILI9341
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        320
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      1       /* landscape */
  #define WY_DISPLAY_DC       2
  #define WY_DISPLAY_CS       15
  #define WY_DISPLAY_SCK      14
  #define WY_DISPLAY_MOSI     13
  #define WY_DISPLAY_MISO     12
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       21      /* PWM, active HIGH */
  #define WY_DISPLAY_BL_PWM   1
  /* Touch */
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_XPT2046
  #define WY_TOUCH_BUS_SPI    /* separate HSPI bus */
  #define WY_TOUCH_CS         33
  #define WY_TOUCH_IRQ        36
  #define WY_TOUCH_SCK        25
  #define WY_TOUCH_MOSI       32
  #define WY_TOUCH_MISO       39
  #define WY_TOUCH_X_MIN      200
  #define WY_TOUCH_X_MAX      3700
  #define WY_TOUCH_Y_MIN      240
  #define WY_TOUCH_Y_MAX      3800
  /* RGB LED (active LOW) */
  #define WY_HAS_RGB_LED      1
  #define WY_LED_R            4
  #define WY_LED_G            16
  #define WY_LED_B            17
  /* Boot button */
  #define WY_BOOT_BTN         0
  /* Screen logical size */
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * ESP32-2432S028R (CYD2USB) — dual USB variant
 * Same as CYD but inverted colours + different touch MISO
 * ══════════════════════════════════════════════════════════════════ */
#elif defined(WY_BOARD_CYD2USB)
  #define WY_BOARD_NAME       "ESP32-2432S028R (CYD2USB)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ILI9341
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        320
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      1
  #define WY_DISPLAY_DC       2
  #define WY_DISPLAY_CS       15
  #define WY_DISPLAY_SCK      14
  #define WY_DISPLAY_MOSI     13
  #define WY_DISPLAY_MISO     12
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       21
  #define WY_DISPLAY_BL_PWM   1
  #define WY_DISPLAY_INVERT   1       /* colours inverted on 2USB */
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_XPT2046
  #define WY_TOUCH_BUS_SPI
  #define WY_TOUCH_CS         33
  #define WY_TOUCH_IRQ        36
  #define WY_TOUCH_SCK        25
  #define WY_TOUCH_MOSI       32
  #define WY_TOUCH_MISO       39
  #define WY_TOUCH_X_MIN      200
  #define WY_TOUCH_X_MAX      3700
  #define WY_TOUCH_Y_MIN      240
  #define WY_TOUCH_Y_MAX      3800
  #define WY_HAS_RGB_LED      1
  #define WY_LED_R            4
  #define WY_LED_G            16
  #define WY_LED_B            17
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * ESP32-3248S035 — 3.5" 480×320 CYD-style board
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32, 4MB flash, no PSRAM
 * Display: ST7796, SPI (VSPI), 480×320, BL=PWM GPIO27
 * Touch:   GT911 capacitive (some variants) or XPT2046 resistive
 */
/* ══════════════════════════════════════════════════════════════════
 * Adafruit ILI9341 2.8" breakout (SPI, 240×320, with XPT2046 touch)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     Any ESP32 (pins mapped to typical VSPI defaults)
 * Display: ILI9341, SPI, 240×320 portrait
 * Touch:   XPT2046 resistive, same SPI bus, separate CS
 *
 * Wiring (typical VSPI):
 *   CLK  → GPIO18   MOSI → GPIO23   MISO → GPIO19
 *   CS   → GPIO5    DC   → GPIO21   RST  → GPIO22
 *   T_CS → GPIO4    T_IRQ → GPIO15
 *   LED  → 3.3V (always on) or GPIO with PWM
 *
 * ⚠️ Adafruit breakout has a 10-pin 0.1" header — verify your wiring
 *    with the Adafruit pinout diagram, it differs from Chinese clones.
 */
/* ══════════════════════════════════════════════════════════════════
 * Double EYE — Dual GC9A01 0.71" Round Display Module (128×128 × 2)
 * ══════════════════════════════════════════════════════════════════
 * Two GC9A01 round LCDs (128×128, 0.71") on one PCB.
 * Shared SPI bus, independent CS pins — drive alternately.
 * Designed for robot eyes / animatronic face projects.
 * Compatible with any ESP32 / ESP32-S3.
 *
 * Typical module pinout (may vary slightly by seller):
 *   VCC  → 3.3V
 *   GND  → GND
 *   SCL  → SPI CLK
 *   SDA  → SPI MOSI
 *   RES  → shared reset (both displays)
 *   DC   → shared data/command
 *   CS1  → left eye chip select
 *   CS2  → right eye chip select
 *   BLK  → shared backlight (active HIGH)
 *
 * ⚠️ Both displays share DC, RST, BL — only CS is separate.
 *    Drive one eye at a time: assert CS1 LOW, send frame, deassert,
 *    then assert CS2 LOW, send frame, deassert. See WyEyes.h.
 * ⚠️ 128×128 (not 240×240) — these are 0.71", smaller than the
 *    common 1.28" GC9A01 round displays.
 */
#elif defined(WY_BOARD_DOUBLE_EYE)
  #define WY_BOARD_NAME       "Double EYE Dual GC9A01 0.71\" (128x128 x2)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1       /* left eye — primary */
  #define WY_DISPLAY_GC9A01
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        128
  #define WY_DISPLAY_H        128
  #define WY_DISPLAY_ROT      0
  #ifndef WY_DISPLAY_DC
    #define WY_DISPLAY_DC     2       /* shared between both eyes */
  #endif
  #ifndef WY_DISPLAY_CS
    #define WY_DISPLAY_CS     5       /* CS1 — left eye */
  #endif
  #ifndef WY_EYE_CS2
    #define WY_EYE_CS2        15      /* CS2 — right eye */
  #endif
  #ifndef WY_DISPLAY_SCK
    #define WY_DISPLAY_SCK    18
  #endif
  #ifndef WY_DISPLAY_MOSI
    #define WY_DISPLAY_MOSI   23
  #endif
  #ifndef WY_DISPLAY_RST
    #define WY_DISPLAY_RST    4       /* shared reset */
  #endif
  #ifndef WY_DISPLAY_BL
    #define WY_DISPLAY_BL     21      /* shared backlight */
  #endif
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_DUAL_DISPLAY 1       /* signals WyEyes.h to init both */
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * Generic GC9A01 round display (1.28" 240×240)
 * ══════════════════════════════════════════════════════════════════
 * The common 1.28" round LCD (240×240) — larger than the Double EYE.
 * No touch. All pins overridable.
 */
#elif defined(WY_BOARD_GC9A01_GENERIC)
  #define WY_BOARD_NAME       "Generic GC9A01 Round 1.28\" (240x240)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_GC9A01
  #define WY_DISPLAY_BUS_SPI
  #ifndef WY_DISPLAY_W
    #define WY_DISPLAY_W      240
  #endif
  #ifndef WY_DISPLAY_H
    #define WY_DISPLAY_H      240
  #endif
  #ifndef WY_DISPLAY_ROT
    #define WY_DISPLAY_ROT    0
  #endif
  #ifndef WY_DISPLAY_DC
    #define WY_DISPLAY_DC     2
  #endif
  #ifndef WY_DISPLAY_CS
    #define WY_DISPLAY_CS     15
  #endif
  #ifndef WY_DISPLAY_SCK
    #define WY_DISPLAY_SCK    14
  #endif
  #ifndef WY_DISPLAY_MOSI
    #define WY_DISPLAY_MOSI   13
  #endif
  #ifndef WY_DISPLAY_RST
    #define WY_DISPLAY_RST    4
  #endif
  #ifndef WY_DISPLAY_BL
    #define WY_DISPLAY_BL     21
  #endif
  #ifndef WY_DISPLAY_BL_PWM
    #define WY_DISPLAY_BL_PWM 1
  #endif
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

#elif defined(WY_BOARD_ILI9341_ADAFRUIT)
  #define WY_BOARD_NAME       "Adafruit ILI9341 2.8\" SPI (240x320)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ILI9341
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        240
  #define WY_DISPLAY_H        320
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       21
  #define WY_DISPLAY_CS       5
  #define WY_DISPLAY_SCK      18
  #define WY_DISPLAY_MOSI     23
  #define WY_DISPLAY_MISO     19
  #define WY_DISPLAY_RST      22
  #define WY_DISPLAY_BL       -1      /* tie LED to 3.3V or add GPIO */
  #define WY_DISPLAY_BL_PWM   0
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_XPT2046
  #define WY_TOUCH_BUS_SPI
  #define WY_TOUCH_CS         4
  #define WY_TOUCH_IRQ        15
  #define WY_TOUCH_SCK        18      /* shared SPI bus */
  #define WY_TOUCH_MOSI       23
  #define WY_TOUCH_MISO       19
  #define WY_TOUCH_X_MIN      200
  #define WY_TOUCH_X_MAX      3700
  #define WY_TOUCH_Y_MIN      200
  #define WY_TOUCH_Y_MAX      3800
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * Generic ILI9341 2.4" / 2.8" SPI breakout (Chinese clone)
 * ══════════════════════════════════════════════════════════════════
 * The most common bare ILI9341 module on AliExpress/eBay.
 * Red PCB, 14-pin header. 240×320. Often includes XPT2046 touch.
 *
 * Typical pin labelling on module: LED, SCK, SDI(MOSI), DC, RST,
 * CS, GND, VCC, SDO(MISO), T_CLK, T_CS, T_DIN, T_DO, T_IRQ
 *
 * Default mapping to ESP32 VSPI:
 *   SCK  → GPIO14   SDI  → GPIO13   SDO  → GPIO12
 *   CS   → GPIO15   DC   → GPIO2    RST  → GPIO4
 *   T_CLK→ GPIO14   T_DIN→ GPIO13   T_DO → GPIO12  (shared bus)
 *   T_CS → GPIO33   T_IRQ→ GPIO36
 *   LED  → GPIO21 (or 3.3V directly for always-on)
 */
#elif defined(WY_BOARD_ILI9341_GENERIC)
  #define WY_BOARD_NAME       "Generic ILI9341 SPI 2.4\"/2.8\" (240x320)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ILI9341
  #define WY_DISPLAY_BUS_SPI
  #ifndef WY_DISPLAY_W
    #define WY_DISPLAY_W      240
  #endif
  #ifndef WY_DISPLAY_H
    #define WY_DISPLAY_H      320
  #endif
  #ifndef WY_DISPLAY_ROT
    #define WY_DISPLAY_ROT    0
  #endif
  #ifndef WY_DISPLAY_DC
    #define WY_DISPLAY_DC     2
  #endif
  #ifndef WY_DISPLAY_CS
    #define WY_DISPLAY_CS     15
  #endif
  #ifndef WY_DISPLAY_SCK
    #define WY_DISPLAY_SCK    14
  #endif
  #ifndef WY_DISPLAY_MOSI
    #define WY_DISPLAY_MOSI   13
  #endif
  #ifndef WY_DISPLAY_MISO
    #define WY_DISPLAY_MISO   12
  #endif
  #ifndef WY_DISPLAY_RST
    #define WY_DISPLAY_RST    4
  #endif
  #ifndef WY_DISPLAY_BL
    #define WY_DISPLAY_BL     21
  #endif
  #ifndef WY_DISPLAY_BL_PWM
    #define WY_DISPLAY_BL_PWM 1
  #endif
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_XPT2046
  #define WY_TOUCH_BUS_SPI
  #ifndef WY_TOUCH_CS
    #define WY_TOUCH_CS       33
  #endif
  #ifndef WY_TOUCH_IRQ
    #define WY_TOUCH_IRQ      36
  #endif
  #define WY_TOUCH_SCK        WY_DISPLAY_SCK    /* shared bus */
  #define WY_TOUCH_MOSI       WY_DISPLAY_MOSI
  #define WY_TOUCH_MISO       WY_DISPLAY_MISO
  #define WY_TOUCH_X_MIN      200
  #define WY_TOUCH_X_MAX      3700
  #define WY_TOUCH_Y_MIN      200
  #define WY_TOUCH_Y_MAX      3800
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * M5Stack Core / Core2 (ILI9342C — ILI9341 variant, 320×240)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-D0WDQ6, 4MB flash, 8MB PSRAM (Core2: 16MB flash)
 * Display: ILI9342C (ILI9341 in landscape native — same driver),
 *          SPI, 320×240
 * Touch:   Core: resistive (NS2009). Core2: FT6336U capacitive.
 * Speaker: DAC on GPIO25
 * SD:      SPI on GPIO4
 *
 * ⚠️ ILI9342C = ILI9341 in landscape-native mode. The controller
 *    MADCTL is set differently but Arduino_ILI9341 handles it via
 *    rotation=0 → landscape (swap W/H vs ILI9341 convention).
 * ⚠️ Use M5Stack's own library for full hardware support (speaker,
 *    power management, IMU). This board target is for WyDisplay
 *    display-only use without the M5 framework.
 */
#elif defined(WY_BOARD_M5STACK_CORE)
  #define WY_BOARD_NAME       "M5Stack Core (ILI9342C 320x240)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ILI9341  /* ILI9342C = ILI9341 variant, same driver */
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        320
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      0       /* ILI9342C native landscape */
  #define WY_DISPLAY_DC       27
  #define WY_DISPLAY_CS       14
  #define WY_DISPLAY_SCK      18
  #define WY_DISPLAY_MOSI     23
  #define WY_DISPLAY_MISO     19
  #define WY_DISPLAY_RST      33
  #define WY_DISPLAY_BL       32
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        0       /* use M5.Touch or dedicated driver */
  #define WY_HAS_RGB_LED      0
  #define WY_BOOT_BTN         39      /* btn A */
  #define WY_SD_CS            4
  #define WY_SPEAKER_DAC      25
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

#elif defined(WY_BOARD_ESP32_3248S035)
  #define WY_BOARD_NAME       "ESP32-3248S035 (3.5\" 480x320)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7796
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        480
  #define WY_DISPLAY_H        320
  #define WY_DISPLAY_ROT      1
  #define WY_DISPLAY_DC       2
  #define WY_DISPLAY_CS       15
  #define WY_DISPLAY_SCK      14
  #define WY_DISPLAY_MOSI     13
  #define WY_DISPLAY_MISO     12
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       27
  #define WY_DISPLAY_BL_PWM   1
  /* Touch: XPT2046 resistive variant */
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_XPT2046
  #define WY_TOUCH_BUS_SPI
  #define WY_TOUCH_CS         33
  #define WY_TOUCH_IRQ        36
  #define WY_TOUCH_SCK        25
  #define WY_TOUCH_MOSI       32
  #define WY_TOUCH_MISO       39
  #define WY_TOUCH_X_MIN      200
  #define WY_TOUCH_X_MAX      3800
  #define WY_TOUCH_Y_MIN      200
  #define WY_TOUCH_Y_MAX      3800
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * Guition ESP32-S3-4848S040 — 4" 480×480 square panel
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, dual-core 240MHz, 16MB flash, 8MB PSRAM OPI
 * Display: ST7701S, 16-bit RGB parallel, 480×480, BL=GPIO38
 * Touch:   GT911, I2C, capacitive, 480×480
 */
#elif defined(WY_BOARD_GUITION4848S040)
  #define WY_BOARD_NAME       "Guition ESP32-S3-4848S040 (4\" 480x480)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_PSRAM_MODE       "opi"
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7701S
  #define WY_DISPLAY_BUS_RGB16
  #define WY_DISPLAY_W        480
  #define WY_DISPLAY_H        480
  #define WY_DISPLAY_ROT      0
  /* RGB panel data pins */
  #define WY_RGB_DE           39
  #define WY_RGB_VSYNC        48
  #define WY_RGB_HSYNC        47
  #define WY_RGB_PCLK         18
  #define WY_RGB_R0           17
  #define WY_RGB_R1           16
  #define WY_RGB_R2           21
  #define WY_RGB_R3           11
  #define WY_RGB_R4           10
  #define WY_RGB_G0           12
  #define WY_RGB_G1           13
  #define WY_RGB_G2           14
  #define WY_RGB_G3           0
  #define WY_RGB_G4           9
  #define WY_RGB_G5           46
  #define WY_RGB_B0           4
  #define WY_RGB_B1           5
  #define WY_RGB_B2           6
  #define WY_RGB_B3           7
  #define WY_RGB_B4           15
  /* ST7701S SPI init bus */
  #define WY_RGB_SPI_CS       39   /* note: shared DE */
  #define WY_RGB_SPI_SCK      48
  #define WY_RGB_SPI_MOSI     47
  /* Backlight */
  #define WY_DISPLAY_BL       38
  #define WY_DISPLAY_BL_PWM   0    /* simple GPIO HIGH/LOW */
  /* Touch */
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_GT911
  #define WY_TOUCH_BUS_I2C
  #define WY_TOUCH_SDA        19
  #define WY_TOUCH_SCL        45
  #define WY_TOUCH_INT        40
  #define WY_TOUCH_RST        41
  #define WY_TOUCH_ADDR       0x5D
  #define WY_TOUCH_X_MAX      480
  #define WY_TOUCH_Y_MAX      480
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * Sunton ESP32-S3 8048S043 — 4.3" 800×480 RGB panel
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, 16MB flash, 8MB PSRAM
 * Display: ST7262 / EK9716 RGB, 800×480
 * Touch:   GT911, I2C
 */
#elif defined(WY_BOARD_SUNTON_8048S043)
  #define WY_BOARD_NAME       "Sunton ESP32-S3-8048S043 (4.3\" 800x480)"
  #define WY_MCU_ESP32S3
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_BUS_RGB16
  #define WY_DISPLAY_W        800
  #define WY_DISPLAY_H        480
  #define WY_DISPLAY_ROT      0
  #define WY_RGB_DE           40
  #define WY_RGB_VSYNC        41
  #define WY_RGB_HSYNC        39
  #define WY_RGB_PCLK         42
  #define WY_RGB_R0           45
  #define WY_RGB_R1           48
  #define WY_RGB_R2           47
  #define WY_RGB_R3           21
  #define WY_RGB_R4           14
  #define WY_RGB_G0           5
  #define WY_RGB_G1           6
  #define WY_RGB_G2           7
  #define WY_RGB_G3           15
  #define WY_RGB_G4           16
  #define WY_RGB_G5           4
  #define WY_RGB_B0           8
  #define WY_RGB_B1           3
  #define WY_RGB_B2           46
  #define WY_RGB_B3           9
  #define WY_RGB_B4           1
  #define WY_DISPLAY_BL       2
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_GT911
  #define WY_TOUCH_BUS_I2C
  #define WY_TOUCH_SDA        19
  #define WY_TOUCH_SCL        20
  #define WY_TOUCH_INT        18  /* some variants -1 */
  #define WY_TOUCH_RST        -1
  #define WY_TOUCH_ADDR       0x5D
  #define WY_TOUCH_X_MAX      800
  #define WY_TOUCH_Y_MAX      480
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * WT32-SC01 Plus — 3.5" 480×320 RGB
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, 8MB flash, no PSRAM
 * Display: ST7796, 8-bit parallel, 480×320
 * Touch:   FT5x06, I2C
 */
#elif defined(WY_BOARD_WT32_SC01_PLUS)
  #define WY_BOARD_NAME       "WT32-SC01 Plus (3.5\" 480x320)"
  #define WY_MCU_ESP32S3
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7796
  #define WY_DISPLAY_BUS_PAR8
  #define WY_DISPLAY_W        480
  #define WY_DISPLAY_H        320
  #define WY_DISPLAY_ROT      1
  #define WY_DISPLAY_DC       0
  #define WY_DISPLAY_WR       47
  #define WY_DISPLAY_D0       9
  #define WY_DISPLAY_D1       46
  #define WY_DISPLAY_D2       3
  #define WY_DISPLAY_D3       8
  #define WY_DISPLAY_D4       18
  #define WY_DISPLAY_D5       17
  #define WY_DISPLAY_D6       16
  #define WY_DISPLAY_D7       15
  #define WY_DISPLAY_BL       45
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_FT5X06
  #define WY_TOUCH_BUS_I2C
  #define WY_TOUCH_SDA        6
  #define WY_TOUCH_SCL        5
  #define WY_TOUCH_INT        4
  #define WY_TOUCH_RST        -1
  #define WY_TOUCH_ADDR       0x38
  #define WY_TOUCH_X_MAX      480
  #define WY_TOUCH_Y_MAX      320
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Display S3 — 1.9" 320×170 AMOLED
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3R8, dual-core, 16MB flash, 8MB PSRAM
 * Display: ST7789, SPI, 320×170
 * Touch:   none
 * Buttons: 2x user buttons
 */
#elif defined(WY_BOARD_LILYGO_TDISPLAY_S3)
  #define WY_BOARD_NAME       "LilyGo T-Display S3 (1.9\" 320x170)"
  #define WY_MCU_ESP32S3
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        320
  #define WY_DISPLAY_H        170
  #define WY_DISPLAY_ROT      1
  #define WY_DISPLAY_DC       38
  #define WY_DISPLAY_CS       6
  #define WY_DISPLAY_SCK      17
  #define WY_DISPLAY_MOSI     18
  #define WY_DISPLAY_RST      5
  #define WY_DISPLAY_BL       15
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        0
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * XIAO ESP32-S3 + Round Display (1.28" GC9A01 round)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, 8MB flash, 8MB PSRAM
 * Display: GC9A01, SPI, 240×240 round
 * Touch:   CST816S, I2C capacitive
 */
#elif defined(WY_BOARD_XIAO_S3_ROUND)
  #define WY_BOARD_NAME       "XIAO ESP32-S3 Round Display (1.28\" 240x240)"
  #define WY_MCU_ESP32S3
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_GC9A01
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        240
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       3
  #define WY_DISPLAY_CS       1
  #define WY_DISPLAY_SCK      7
  #define WY_DISPLAY_MOSI     9
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       -1   /* always on via ext enable */
  #define WY_DISPLAY_BL_PWM   0
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_CST816S
  #define WY_TOUCH_BUS_I2C
  #define WY_TOUCH_SDA        5
  #define WY_TOUCH_SCL        6
  #define WY_TOUCH_INT        -1
  #define WY_TOUCH_RST        -1
  #define WY_TOUCH_ADDR       0x15
  #define WY_TOUCH_X_MAX      240
  #define WY_TOUCH_Y_MAX      240
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * TTGO T-Display (original ESP32, 1.14" ST7789 135×240)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-D0WDQ6, dual-core 240MHz, 4MB flash, no PSRAM
 * Display: ST7789, SPI, 135×240, portrait native
 * Buttons: GPIO 0 (BOOT/left), GPIO 35 (right — INPUT only, no pullup)
 * Battery: BAT_ADC on GPIO 34 (voltage divider ÷2 — read × 2 × 3.3/4096)
 * USB:     CP2104 USB-to-serial
 *
 * ⚠️ GPIO 35 is input-only — no internal pullup. Add external 10kΩ to 3.3V.
 * ⚠️ Display is 135×240 (portrait). Rotation 1 gives 240×135 landscape.
 * ⚠️ No PSRAM — keep heap allocations small.
 */
#elif defined(WY_BOARD_TTGO_TDISPLAY)
  #define WY_BOARD_NAME       "TTGO T-Display (1.14\" 135x240)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        135
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      0       /* portrait — use ROT 1 for landscape (240×135) */
  #define WY_DISPLAY_DC       16
  #define WY_DISPLAY_CS       5
  #define WY_DISPLAY_SCK      18
  #define WY_DISPLAY_MOSI     19
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      23
  #define WY_DISPLAY_BL       4
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  #define WY_BOOT_BTN         0
  #define WY_BTN_RIGHT        35      /* input-only GPIO, no internal pullup */
  #define WY_BAT_ADC          34      /* battery voltage ÷2 divider */
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * Waveshare ESP32-S3 1.47" (ST7789, 172×320)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3FH4R2, dual-core 240MHz, 4MB flash, 2MB PSRAM
 * Display: ST7789, SPI, 172×320, rounded corners, portrait native
 * LED:     RGB LED (WS2812 on GPIO 38)
 * USB:     USB-C native (GPIO 19/20), no UART chip
 *
 * ⚠️ Rounded corners — UI elements near corners may be clipped by hardware.
 * ⚠️ WS2812 RGB LED — use NeoPixel or FastLED, not analogWrite.
 */
#elif defined(WY_BOARD_WAVESHARE_147_S3)
  #define WY_BOARD_NAME       "Waveshare ESP32-S3 1.47\" (172x320)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        172
  #define WY_DISPLAY_H        320
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       8
  #define WY_DISPLAY_CS       9
  #define WY_DISPLAY_SCK      10
  #define WY_DISPLAY_MOSI     11
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      12
  #define WY_DISPLAY_BL       -1      /* always on via PWR_EN */
  #define WY_DISPLAY_BL_PWM   0
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0       /* WS2812 on GPIO 38 — use NeoPixel */
  #define WY_WS2812_PIN       38
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * Waveshare ESP32-S3 2.0" (ST7789, 240×320)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3FH4R2, dual-core 240MHz, 4MB flash, 2MB PSRAM
 * Display: ST7789, SPI, 240×320, standard aspect portrait
 * LED:     WS2812 RGB on GPIO 38
 * USB:     USB-C native
 */
#elif defined(WY_BOARD_WAVESHARE_200_S3)
  #define WY_BOARD_NAME       "Waveshare ESP32-S3 2.0\" (240x320)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        240
  #define WY_DISPLAY_H        320
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       8
  #define WY_DISPLAY_CS       9
  #define WY_DISPLAY_SCK      10
  #define WY_DISPLAY_MOSI     11
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      12
  #define WY_DISPLAY_BL       -1
  #define WY_DISPLAY_BL_PWM   0
  #define WY_HAS_TOUCH        0
  #define WY_WS2812_PIN       38
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * Generic ST7789 SPI breakout (user-defined pins)
 * ══════════════════════════════════════════════════════════════════
 * For bare ST7789 modules wired to any ESP32/ESP32-S3.
 * Override any WY_DISPLAY_* define in your build_flags.
 *
 * platformio.ini example:
 *   build_flags =
 *     -DWY_BOARD_ST7789_GENERIC
 *     -DWY_DISPLAY_W=240
 *     -DWY_DISPLAY_H=240
 *     -DWY_DISPLAY_DC=2
 *     -DWY_DISPLAY_CS=15
 *     -DWY_DISPLAY_SCK=14
 *     -DWY_DISPLAY_MOSI=13
 *     -DWY_DISPLAY_RST=4
 *     -DWY_DISPLAY_BL=21
 *
 * Common ST7789 resolutions:
 *   240×240  — 1.3" / 1.54" square modules
 *   240×320  — 2.0" / 2.4" portrait
 *   172×320  — 1.47" (Waveshare, rounded corners)
 *   135×240  — 1.14" (TTGO style)
 *   280×240  — 1.69" (some Adafruit modules)
 */
#elif defined(WY_BOARD_ST7789_GENERIC)
  #define WY_BOARD_NAME       "Generic ST7789 SPI"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  /* Defaults — override all in build_flags */
  #ifndef WY_DISPLAY_W
    #define WY_DISPLAY_W      240
  #endif
  #ifndef WY_DISPLAY_H
    #define WY_DISPLAY_H      240
  #endif
  #ifndef WY_DISPLAY_ROT
    #define WY_DISPLAY_ROT    0
  #endif
  #ifndef WY_DISPLAY_DC
    #define WY_DISPLAY_DC     2
  #endif
  #ifndef WY_DISPLAY_CS
    #define WY_DISPLAY_CS     15
  #endif
  #ifndef WY_DISPLAY_SCK
    #define WY_DISPLAY_SCK    14
  #endif
  #ifndef WY_DISPLAY_MOSI
    #define WY_DISPLAY_MOSI   13
  #endif
  #ifndef WY_DISPLAY_MISO
    #define WY_DISPLAY_MISO   -1
  #endif
  #ifndef WY_DISPLAY_RST
    #define WY_DISPLAY_RST    -1
  #endif
  #ifndef WY_DISPLAY_BL
    #define WY_DISPLAY_BL     21
  #endif
  #ifndef WY_DISPLAY_BL_PWM
    #define WY_DISPLAY_BL_PWM 1
  #endif
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * ESP32-CAM (AI-Thinker module)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S, single-core 240MHz, 4MB flash, 4MB PSRAM (QSPI)
 * Camera:  OV2640 (DVP parallel interface), up to 2MP (1600×1200)
 *          Typically used at UXGA(1600×1200), SXGA(1280×1024),
 *          XGA(1024×768), SVGA(800×600), VGA(640×480), CIF(400×296),
 *          QVGA(320×240), HQVGA(240×176), QQVGA(160×120)
 * SD card: SPI on shared GPIO (conflicts with camera at runtime)
 * Flash:   White LED flash on GPIO 4 (also SD card CS — shared!)
 * USB:     No native USB — program via UART (GPIO 1/3) with FTDI
 *          IO0 must be LOW during flash, then HIGH/open for run
 *
 * ⚠️ GPIO 4 (flash LED) is shared with SD card CS — pick one or multiplex
 * ⚠️ No display onboard — output via WiFi stream or UART
 * ⚠️ Single-core ESP32-S: WiFi + camera takes most of the CPU
 * ⚠️ Must use AI-Thinker board target in PlatformIO: board = esp32cam
 */
#elif defined(WY_BOARD_ESP32CAM)
  #define WY_BOARD_NAME       "AI-Thinker ESP32-CAM"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        1       /* single-core ESP32-S variant */
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1       /* 4MB QSPI PSRAM — required for camera */
  /* No display */
  #define WY_HAS_DISPLAY      0
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  /* Camera (OV2640, DVP interface) */
  #define WY_HAS_CAMERA       1
  #define WY_CAM_PWDN         32
  #define WY_CAM_RESET        -1
  #define WY_CAM_XCLK         0
  #define WY_CAM_SIOD         26      /* I2C SDA for OV2640 config */
  #define WY_CAM_SIOC         27      /* I2C SCL for OV2640 config */
  #define WY_CAM_D7           35
  #define WY_CAM_D6           34
  #define WY_CAM_D5           39
  #define WY_CAM_D4           36
  #define WY_CAM_D3           21
  #define WY_CAM_D2           19
  #define WY_CAM_D1           18
  #define WY_CAM_D0           5
  #define WY_CAM_VSYNC        25
  #define WY_CAM_HREF         23
  #define WY_CAM_PCLK         22
  /* Flash LED (also SD card CS — shared) */
  #define WY_FLASH_LED        4
  /* SD card (SPI, shares GPIO 4 with flash) */
  #define WY_SD_CS            4       /* shared with flash LED */
  #define WY_SD_MOSI          12
  #define WY_SD_MISO          13
  #define WY_SD_SCK           14
  /* Free GPIO for external use */
  #define WY_GPIO_FREE_1      2       /* general purpose */
  #define WY_GPIO_FREE_2      15      /* general purpose */
  /* Screen (logical — camera frame size used instead of display) */
  #define WY_SCREEN_W         640
  #define WY_SCREEN_H         480

/* ══════════════════════════════════════════════════════════════════
 * ESP32-S3-EYE (Espressif dev board with OV2640 + LCD)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, dual-core 240MHz, 8MB flash, 8MB PSRAM
 * Camera:  OV2640 (DVP parallel interface)
 * Display: ST7789, SPI, 240×240 (on bottom of board)
 * Mic:     PDM microphone
 * Button:  Boot + Menu + Up + Down buttons
 */
#elif defined(WY_BOARD_ESP32S3EYE)
  #define WY_BOARD_NAME       "ESP32-S3-EYE"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  /* Display */
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        240
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       3
  #define WY_DISPLAY_CS       46
  #define WY_DISPLAY_SCK      40
  #define WY_DISPLAY_MOSI     47
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       -1
  #define WY_HAS_TOUCH        0
  /* Camera (OV2640) */
  #define WY_HAS_CAMERA       1
  #define WY_CAM_PWDN         -1
  #define WY_CAM_RESET        -1
  #define WY_CAM_XCLK         15
  #define WY_CAM_SIOD         4
  #define WY_CAM_SIOC         5
  #define WY_CAM_D7           16
  #define WY_CAM_D6           17
  #define WY_CAM_D5           18
  #define WY_CAM_D4           12
  #define WY_CAM_D3           10
  #define WY_CAM_D2           8
  #define WY_CAM_D1           9
  #define WY_CAM_D0           11
  #define WY_CAM_VSYNC        6
  #define WY_CAM_HREF         7
  #define WY_CAM_PCLK         13
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

#else
  #warning "wyltek-embedded-builder: no board defined. Add -DWY_BOARD_xxx to build_flags."
  #define WY_HAS_DISPLAY  0
  #define WY_HAS_TOUCH    0
  #define WY_HAS_RGB_LED  0
  #define WY_HAS_PSRAM    0
  #define WY_HAS_CAMERA   0
  #define WY_SCREEN_W     0
  #define WY_SCREEN_H     0
#endif
