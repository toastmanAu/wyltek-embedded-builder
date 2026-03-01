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
/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-A7670SA — LTE Cat-1 + GPS development board
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, dual-core 240MHz, 16MB flash, 8MB PSRAM
 * Modem:   SIM7670G (A7670SA variant) — LTE Cat-1, SMS, voice
 *          Also sold as A7670E/A7670G — different regional bands
 * Display: None onboard
 * GPS:     AXP2101 PMU + optional L76K GPS module header
 * Solar:   Solar charging via AXP2101 (JST PH2.0 connector)
 * Battery: 18650 holder onboard
 * USB:     USB-C native (ESP32-S3)
 *
 * ⚠️ Modem uses UART1 (GPIO17=TX, GPIO18=RX). PWR_KEY=GPIO41.
 * ⚠️ Modem requires 3.7V LiPo — do NOT run modem from USB alone
 *    (peak current during registration can be 2A+)
 * ⚠️ Use TinyGSM library for modem AT commands / data connection
 */
#elif defined(WY_BOARD_LILYGO_A7670SA)
  #define WY_BOARD_NAME       "LilyGo T-A7670SA (LTE Cat-1 + GPS)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      0
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  /* Modem */
  #define WY_MODEM_TX         17
  #define WY_MODEM_RX         18
  #define WY_MODEM_PWR        41    /* active HIGH to power on */
  #define WY_MODEM_RST        -1
  #define WY_MODEM_DTR        42
  /* PMU */
  #define WY_PMU_SDA          1
  #define WY_PMU_SCL          2
  #define WY_PMU_IRQ          3
  /* SD card */
  #define WY_SD_MOSI          11
  #define WY_SD_MISO          13
  #define WY_SD_SCK           12
  #define WY_SD_CS            10
  /* User GPIO */
  #define WY_BOOT_BTN         0
  #define WY_LED_PIN          -1
  #define WY_SCREEN_W         0
  #define WY_SCREEN_H         0

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-QT C6 — ESP32-C6 tiny dev board with 0.85" display
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-C6, single RISC-V core 160MHz, 4MB flash, no PSRAM
 * Display: GC9107, SPI, 128×128, 0.85"
 * WiFi:    WiFi 6 (802.11ax) + Bluetooth 5 LE + Zigbee + Thread
 * USB:     USB-C native (JTAG/serial)
 * Size:    Very small — QT = "quiet tiny"
 *
 * ⚠️ ESP32-C6 is RISC-V (single core, 160MHz) not Xtensa.
 *    Some Arduino libs assume Xtensa — test before deploying.
 * ⚠️ No PSRAM. 128×128 display fits in DRAM fine.
 * ⚠️ GC9107 = stripped-down GC9A01 variant. Same driver works.
 */
#elif defined(WY_BOARD_LILYGO_TQTC6)
  #define WY_BOARD_NAME       "LilyGo T-QT C6 (0.85\" 128x128)"
  #define WY_MCU_ESP32C6
  #define WY_MCU_CORES        1
  #define WY_MCU_FREQ         160
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_GC9A01   /* GC9107 — same driver */
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        128
  #define WY_DISPLAY_H        128
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       2
  #define WY_DISPLAY_CS       1
  #define WY_DISPLAY_SCK      3
  #define WY_DISPLAY_MOSI     7
  #define WY_DISPLAY_RST      8
  #define WY_DISPLAY_BL       -1    /* always on */
  #define WY_DISPLAY_BL_PWM   0
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      1
  #define WY_LED_R            -1
  #define WY_LED_G            -1
  #define WY_LED_B            -1
  #define WY_WS2812_PIN       11    /* WS2812 RGB LED */
  #define WY_BOOT_BTN         9
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-SIM7080G-S3 — NB-IoT / Cat-M1 + GPS
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, dual-core 240MHz, 16MB flash, 8MB PSRAM
 * Modem:   SIM7080G — NB-IoT + LTE-M (Cat-M1/NB1/NB2)
 *          Ultra-low power IoT modem (not for voice/data like A7670)
 * Display: None onboard
 * GPS:     Built into SIM7080G (GNSS capable)
 * Solar:   Solar charging header
 * Battery: 18650 holder
 *
 * ⚠️ NB-IoT/LTE-M only — no regular 4G data. For IoT sensors only.
 * ⚠️ Use TinyGSM with TINY_GSM_MODEM_SIM7080 define
 * ⚠️ Modem UART on GPIO17(TX)/GPIO18(RX), PWR_KEY=GPIO41
 */
#elif defined(WY_BOARD_LILYGO_TSIM7080G_S3)
  #define WY_BOARD_NAME       "LilyGo T-SIM7080G-S3 (NB-IoT + GPS)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      0
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  #define WY_MODEM_TX         17
  #define WY_MODEM_RX         18
  #define WY_MODEM_PWR        41
  #define WY_MODEM_RST        42
  #define WY_MODEM_DTR        -1
  #define WY_PMU_SDA          1
  #define WY_PMU_SCL          2
  #define WY_PMU_IRQ          3
  #define WY_SD_MOSI          11
  #define WY_SD_MISO          13
  #define WY_SD_SCK           12
  #define WY_SD_CS            10
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         0
  #define WY_SCREEN_H         0

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Display S3 Long — ESP32-S3 with 2.04" bar display
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3R8, dual-core 240MHz, 16MB flash, 8MB PSRAM
 * Display: ST7796, SPI, 170×320, 2.04" long bar form factor
 *          Landscape native — narrow and tall in portrait
 * Buttons: 3 side buttons (BOOT + 2 user)
 * USB:     USB-C native
 * Battery: JST connector, charging via onboard circuit
 *
 * ⚠️ ST7796 at 170×320 — unusual resolution. Width is only 170px.
 *    Good for long horizontal bars, menus, instrument panels.
 */
#elif defined(WY_BOARD_LILYGO_TDISPLAY_S3_LONG)
  #define WY_BOARD_NAME       "LilyGo T-Display S3 Long (2.04\" 170x320)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7796
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        170
  #define WY_DISPLAY_H        320
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       8
  #define WY_DISPLAY_CS       6
  #define WY_DISPLAY_SCK      17
  #define WY_DISPLAY_MOSI     18
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      5
  #define WY_DISPLAY_BL       38
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  #define WY_BOOT_BTN         0
  #define WY_BTN_A            21
  #define WY_BTN_B            -1
  #define WY_BAT_ADC          4
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Keyboard S3 — 4 mechanical keys with per-key displays
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3R8 (WROOM-1), dual-core 240MHz, 16MB flash, 8MB PSRAM
 * Display: 4x GC9107, SPI, 128x128, 0.85" — one per mechanical keycap
 *          Model: N085-1212TBWIG06-C08
 *          Shared SPI bus (SCK=47, MOSI=48, DC=45, RST=38)
 *          CS is GPIO-controlled (not SPI HW): CS1=12 CS2=13 CS3=14 CS4=21
 *          col_offset=2, row_offset=1 (required for correct display origin)
 * Keys:    4 hot-swap mechanical switches (Kailh compatible, 6.35mm pitch)
 *          KEY1=10, KEY2=9, KEY3=46, KEY4=3 (KEY4 also BOOT0 — avoid at boot)
 * LEDs:    14x WS2812B addressable RGB (DATA=11)
 * Wireless: 2.4GHz WiFi + BLE5
 * USB:     USB-C native (ESP32-S3 native USB)
 *
 * Use WyKeyDisplay.h for the multi-display API (select/draw/deselect).
 * Use cases: CKB node status macro pad, SBC control surface,
 *   programmable shortcut panel with live per-key visual feedback
 *
 * IMPORTANT: GC9107 needs col_offset=2, row_offset=1 — handled in WyKeyDisplay.h
 * Ref: github.com/Xinyuan-LilyGO/T-Keyboard-S3 (GPL 3.0 — pin mapping only)
 */
#elif defined(WY_BOARD_LILYGO_TKEYBOARD_S3)
  #define WY_BOARD_NAME       "LilyGo T-Keyboard S3 (4x GC9107 128x128)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  /* 4x GC9107 128x128 displays — one per mechanical keycap.
   * WY_DISPLAY_* describes a single key display; app must drive all 4.
   * Each display has its own CS pin — CS pins: KEY0=5 KEY1=6 KEY2=7 KEY3=8
   * (verify against your unit — may vary by hardware revision) */
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_GC9107
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        128
  #define WY_DISPLAY_H        128
  #define WY_DISPLAY_ROT      0
  /* Shared SPI bus — CS managed manually by WyKeyDisplay::select() */
  #define WY_KDISP_SCK        47
  #define WY_KDISP_MOSI       48
  #define WY_KDISP_DC         45
  #define WY_KDISP_RST        38
  #define WY_KDISP_BL         39
  #define WY_KDISP_BL_CHAN    1   /* ledc channel */
  /* CS per keycap display (active LOW, GPIO-controlled not SPI) */
  #define WY_KDISP_CS0        12  /* KEY1 display */
  #define WY_KDISP_CS1        13  /* KEY2 display */
  #define WY_KDISP_CS2        14  /* KEY3 display */
  #define WY_KDISP_CS3        21  /* KEY4 display */
  /* Mechanical key sense pins (active LOW, use INPUT_PULLUP) */
  #define WY_KEY1             10
  #define WY_KEY2             9
  #define WY_KEY3             46
  #define WY_KEY4             3   /* Also BOOT0 — avoid during startup */
  /* WS2812B RGB LEDs */
  #define WY_WS2812_DATA      11
  #define WY_WS2812_COUNT     14  /* 14 addressable LEDs total */
  /* WyDisplay compat aliases */
  #define WY_DISPLAY_DC       WY_KDISP_DC
  #define WY_DISPLAY_CS       GFX_NOT_DEFINED
  #define WY_DISPLAY_SCK      WY_KDISP_SCK
  #define WY_DISPLAY_MOSI     WY_KDISP_MOSI
  #define WY_DISPLAY_RST      WY_KDISP_RST
  #define WY_DISPLAY_BL       WY_KDISP_BL
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        0
  #define WY_HAS_TOUCH        0
  /* STM32 co-processor I2C (key scanning + LED) */
  #define WY_KB_SDA           8
  #define WY_KB_SCL           9
  #define WY_KB_INT           7
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Display S3 AMOLED — ESP32-S3 with 1.91" AMOLED
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3R8, dual-core 240MHz, 16MB flash, 8MB PSRAM
 * Display: RM67162 AMOLED, QSPI, 170×320, 1.91"
 *          True black pixels, HDR contrast — stunning for dark UIs
 * Touch:   FT3267 capacitive (some variants CST820)
 * Buttons: 2 side buttons
 * USB:     USB-C native
 * Battery: JST 1.25mm, AXP2101 PMU
 *
 * ⚠️ AMOLED uses QSPI not standard SPI — needs Arduino_GFX
 *    Arduino_RM67162 driver (part of Arduino_GFX_Library)
 * ⚠️ Do not display static white screens for extended periods
 *    (OLED burn-in on white areas — use dark themes)
 */
#elif defined(WY_BOARD_LILYGO_TDISPLAY_S3_AMOLED)
  #define WY_BOARD_NAME       "LilyGo T-Display S3 AMOLED (1.91\" 170x320)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_RM67162  /* AMOLED QSPI */
  #define WY_DISPLAY_BUS_QSPI
  #define WY_DISPLAY_W        170
  #define WY_DISPLAY_H        320
  #define WY_DISPLAY_ROT      0
  /* QSPI pins */
  #define WY_DISPLAY_CS       6
  #define WY_DISPLAY_SCK      47
  #define WY_DISPLAY_D0       18  /* QSPI data 0 */
  #define WY_DISPLAY_D1       7
  #define WY_DISPLAY_D2       48
  #define WY_DISPLAY_D3       5
  #define WY_DISPLAY_RST      17
  #define WY_DISPLAY_BL       -1  /* AMOLED — no backlight */
  #define WY_DISPLAY_BL_PWM   0
  /* Touch */
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_FT3267
  #define WY_TOUCH_BUS_I2C
  #define WY_TOUCH_SDA        3
  #define WY_TOUCH_SCL        2
  #define WY_TOUCH_INT        21
  #define WY_TOUCH_RST        -1
  #define WY_TOUCH_ADDR       0x38
  #define WY_TOUCH_X_MAX      170
  #define WY_TOUCH_Y_MAX      320
  /* PMU */
  #define WY_PMU_SDA          3
  #define WY_PMU_SCL          2
  #define WY_PMU_IRQ          4
  #define WY_BOOT_BTN         0
  #define WY_BTN_A            21
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Impulse — ESP32-S3 + SX1262 LoRa wristband/watch
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3, dual-core 240MHz, 4MB flash, 2MB PSRAM
 * Display: ST7789, SPI, 240×280 round-corner, 1.69"
 * Radio:   SX1262 LoRa (868/915MHz)
 * IMU:     QMC5883L compass + BMA423 accelerometer
 * Battery: Built-in LiPo, charging via USB-C
 * USB:     USB-C native
 * Touch:   Capacitive button (no touchscreen)
 *
 * ⚠️ Wristband form factor — compact PCB, limited GPIO breakout
 * ⚠️ SX1262 SPI on shared bus with display
 */
#elif defined(WY_BOARD_LILYGO_TIMPULSE)
  #define WY_BOARD_NAME       "LilyGo T-Impulse LoRa Wristband (1.69\" 240x280)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        240
  #define WY_DISPLAY_H        280
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       2
  #define WY_DISPLAY_CS       5
  #define WY_DISPLAY_SCK      3
  #define WY_DISPLAY_MOSI     7
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      8
  #define WY_DISPLAY_BL       6
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        0
  /* LoRa SX1262 */
  #define WY_LORA_CS          9
  #define WY_LORA_RST         14
  #define WY_LORA_IRQ         13
  #define WY_LORA_BUSY        12
  #define WY_LORA_SCK         3
  #define WY_LORA_MOSI        7
  #define WY_LORA_MISO        10
  /* IMU */
  #define WY_IMU_SDA          39
  #define WY_IMU_SCL          40
  #define WY_IMU_IRQ          38
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Deck SX1262 — ESP32-S3 LoRa messenger with keyboard
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3R8, dual-core 240MHz, 16MB flash, 8MB PSRAM
 * Display: ST7789, SPI, 320×240, 2.8"
 * Touch:   GT911 capacitive
 * Radio:   SX1262 LoRa (868/915MHz)
 * Keyboard: Small QWERTY keyboard (I2C via trackball PCB)
 * Trackball: Optical trackball (I2C)
 * Mic:     PDM microphone
 * Speaker: I2S amplifier
 * GPS:     Optional GNSS header
 *
 * ⚠️ T-Deck is the full BlackBerry-style handheld form factor
 * ⚠️ Great for Meshtastic — keyboard + LoRa + display all onboard
 * ⚠️ Keyboard/trackball on same I2C bus (0x55 trackball, 0x55 KB)
 */
#elif defined(WY_BOARD_LILYGO_TDECK)
  #define WY_BOARD_NAME       "LilyGo T-Deck SX1262 (2.8\" 320x240)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        320
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      1
  #define WY_DISPLAY_DC       11
  #define WY_DISPLAY_CS       12
  #define WY_DISPLAY_SCK      40
  #define WY_DISPLAY_MOSI     41
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       42
  #define WY_DISPLAY_BL_PWM   1
  /* Touch GT911 */
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_GT911
  #define WY_TOUCH_BUS_I2C
  #define WY_TOUCH_SDA        18
  #define WY_TOUCH_SCL        8
  #define WY_TOUCH_INT        16
  #define WY_TOUCH_RST        -1
  #define WY_TOUCH_ADDR       0x5D
  #define WY_TOUCH_X_MAX      320
  #define WY_TOUCH_Y_MAX      240
  /* LoRa SX1262 */
  #define WY_LORA_CS          9
  #define WY_LORA_RST         17
  #define WY_LORA_IRQ         45
  #define WY_LORA_BUSY        13
  #define WY_LORA_SCK         40
  #define WY_LORA_MOSI        41
  #define WY_LORA_MISO        38
  /* Keyboard / Trackball I2C */
  #define WY_KB_SDA           18
  #define WY_KB_SCL           8
  #define WY_KB_IRQ           46
  /* Audio */
  #define WY_MIC_WS           5
  #define WY_MIC_SCK          7
  #define WY_MIC_DATA         6
  #define WY_SPK_BCLK         47
  #define WY_SPK_LRC          48
  #define WY_SPK_DIN          2
  /* Power */
  #define WY_PERIPH_PWR       10   /* HIGH to enable periph power */
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Pico S3 — RP2040 + ESP32-S3 dual MCU board
 * ══════════════════════════════════════════════════════════════════
 * MCU:     RP2040 (primary) + ESP32-S3 (WiFi/BT co-processor)
 * Display: ST7789, SPI, 240×135, 1.14"
 * Communication: UART between RP2040 and ESP32-S3
 * USB:     USB-C (RP2040 native USB)
 *
 * ⚠️ Primary MCU is RP2040, NOT ESP32 — this target is for the
 *    ESP32-S3 co-processor side only (WiFi/BT tasks)
 * ⚠️ WyDisplay targets the ESP32-S3 side display
 * ⚠️ Use board = lilygo-t-pico-s3 in PlatformIO
 */
#elif defined(WY_BOARD_LILYGO_TPICO_S3)
  #define WY_BOARD_NAME       "LilyGo T-Pico S3 (ESP32-S3 side, 1.14\" 240x135)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        240
  #define WY_DISPLAY_H        135
  #define WY_DISPLAY_ROT      1
  #define WY_DISPLAY_DC       2
  #define WY_DISPLAY_CS       3
  #define WY_DISPLAY_SCK      0
  #define WY_DISPLAY_MOSI     1
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       4
  #define WY_DISPLAY_BL_PWM   1
  #define WY_HAS_TOUCH        0
  /* UART to RP2040 */
  #define WY_CO_TX            21
  #define WY_CO_RX            20
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * TTGO T-Beam v1.x — ESP32 + SX1276/SX1262 LoRa + GPS + 18650
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-D0WDQ6, dual-core 240MHz, 4MB flash
 * Radio:   SX1276 (v1.0/v1.1) or SX1262 (v1.2+) LoRa
 * GPS:     NEO-6M / NEO-8M (UART2 GPIO34/12)
 * PMU:     AXP192 (v1.0/v1.1) or AXP2101 (v1.2)
 * Display: None onboard (OLED via I2C header common add-on)
 * Battery: 18650 holder
 *
 * ⚠️ Most used board for Meshtastic and TTN tracking nodes
 * ⚠️ AXP192 MUST be initialised before LoRa or GPS will work
 *    (powers the peripherals via LDO outputs)
 * ⚠️ SX1276 on v1.0/v1.1 vs SX1262 on v1.2 — check silkscreen
 * ⚠️ GPS UART: TX=GPIO34 (input only!), RX=GPIO12
 */
#elif defined(WY_BOARD_TTGO_TBEAM)
  #define WY_BOARD_NAME       "TTGO T-Beam (LoRa + GPS + 18650)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      0   /* no onboard display */
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  /* LoRa (SX1276 / SX1262 depending on version) */
  #define WY_LORA_CS          18
  #define WY_LORA_RST         23
  #define WY_LORA_IRQ         26
  #define WY_LORA_SCK         5
  #define WY_LORA_MOSI        27
  #define WY_LORA_MISO        19
  /* GPS UART */
  #define WY_GPS_TX           12   /* ESP32 TX → GPS RX */
  #define WY_GPS_RX           34   /* GPS TX → ESP32 RX (input only) */
  /* PMU I2C */
  #define WY_PMU_SDA          21
  #define WY_PMU_SCL          22
  #define WY_PMU_IRQ          35
  /* User */
  #define WY_BOOT_BTN         38
  #define WY_LED_PIN          4
  #define WY_SCREEN_W         0
  #define WY_SCREEN_H         0

/* ══════════════════════════════════════════════════════════════════
 * TTGO T-Go (ESP32 basic — no display, WiFi dev board)
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-D0WDQ6, dual-core 240MHz, 4MB flash, no PSRAM
 * Display: None
 * LED:     Blue LED GPIO2
 * USB:     CP2104
 */
#elif defined(WY_BOARD_TTGO_TGO)
  #define WY_BOARD_NAME       "TTGO T-Go (ESP32 WiFi dev board)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      0
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  #define WY_LED_PIN          2
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         0
  #define WY_SCREEN_H         0

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

/* ══════════════════════════════════════════════════════════════════
 * TTGO T-Beam Meshtastic (v1.1/v1.2 with AXP192/AXP2101 PMU)
 * ══════════════════════════════════════════════════════════════════
 * Same hardware as WY_BOARD_TTGO_TBEAM but defines for Meshtastic
 * firmware builds — adds I2C OLED header defines and speaker pin.
 *
 * Standard add-on: 0.96" I2C OLED (SSD1306) on GPIO21/22
 */
#elif defined(WY_BOARD_TTGO_TBEAM_MESHTASTIC)
  #define WY_BOARD_NAME       "TTGO T-Beam Meshtastic (LoRa+GPS+OLED)"
  #define WY_MCU_ESP32
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  #define WY_HAS_DISPLAY      1       /* SSD1306 OLED via I2C */
  #define WY_DISPLAY_SSD1306
  #define WY_DISPLAY_BUS_I2C
  #define WY_DISPLAY_W        128
  #define WY_DISPLAY_H        64
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_SDA      21
  #define WY_DISPLAY_SCL      22
  #define WY_DISPLAY_ADDR     0x3C
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  /* LoRa */
  #define WY_LORA_CS          18
  #define WY_LORA_RST         23
  #define WY_LORA_IRQ         26
  #define WY_LORA_SCK         5
  #define WY_LORA_MOSI        27
  #define WY_LORA_MISO        19
  /* GPS */
  #define WY_GPS_TX           12
  #define WY_GPS_RX           34
  /* PMU */
  #define WY_PMU_SDA          21
  #define WY_PMU_SCL          22
  #define WY_PMU_IRQ          35
  #define WY_BOOT_BTN         38
  #define WY_LED_PIN          4
  #define WY_SCREEN_W         128
  #define WY_SCREEN_H         64

/* ══════════════════════════════════════════════════════════════════
 * T-Watch 2020 V3 — ESP32-S3 smartwatch
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3R8, dual-core 240MHz, 16MB flash, 8MB PSRAM
 * Display: ST7789, SPI, 240×240, 1.54" square
 * Touch:   FT6336U capacitive
 * IMU:     BMA423 accelerometer + step counter, wrist-tilt wakeup
 * RTC:     PCF8563 real-time clock
 * PMU:     AXP2101 — battery management, vibration motor
 * Radio:   Optional LoRa module (ESP32-S3 variant)
 * Audio:   MAX98357A I2S amplifier
 * IR:      IR transmitter (universal remote)
 *
 * ⚠️ AXP2101 PMU must be init'd before display/touch/IMU will power
 * ⚠️ FT6336U I2C address 0x38
 * ⚠️ Great for custom watch faces — use dark themes to save battery
 */
#elif defined(WY_BOARD_TWATCH_2020_V3)
  #define WY_BOARD_NAME       "T-Watch 2020 V3 (1.54\" 240x240)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_ST7789
  #define WY_DISPLAY_BUS_SPI
  #define WY_DISPLAY_W        240
  #define WY_DISPLAY_H        240
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_DC       38
  #define WY_DISPLAY_CS       12
  #define WY_DISPLAY_SCK      18
  #define WY_DISPLAY_MOSI     13
  #define WY_DISPLAY_MISO     -1
  #define WY_DISPLAY_RST      -1
  #define WY_DISPLAY_BL       45
  #define WY_DISPLAY_BL_PWM   1
  /* Touch FT6336U */
  #define WY_HAS_TOUCH        1
  #define WY_TOUCH_FT6336
  #define WY_TOUCH_BUS_I2C
  #define WY_TOUCH_SDA        10
  #define WY_TOUCH_SCL        11
  #define WY_TOUCH_INT        16
  #define WY_TOUCH_RST        -1
  #define WY_TOUCH_ADDR       0x38
  #define WY_TOUCH_X_MAX      240
  #define WY_TOUCH_Y_MAX      240
  /* IMU BMA423 */
  #define WY_IMU_SDA          10
  #define WY_IMU_SCL          11
  #define WY_IMU_IRQ1         14
  #define WY_IMU_IRQ2         -1
  /* PMU AXP2101 */
  #define WY_PMU_SDA          10
  #define WY_PMU_SCL          11
  #define WY_PMU_IRQ          21
  /* RTC */
  #define WY_RTC_SDA          10
  #define WY_RTC_SCL          11
  /* Audio */
  #define WY_SPK_BCLK         48
  #define WY_SPK_LRC          53
  #define WY_SPK_DIN          46
  /* IR */
  #define WY_IR_TX            2
  /* Vibration motor */
  #define WY_VIBRO_PIN        17
  #define WY_BOOT_BTN         0
  #define WY_CROWN_BTN        35   /* side crown button */
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H


/* ══════════════════════════════════════════════════════════════════
 * Heltec WiFi LoRa 32 V3 — ESP32-S3 + SX1262 + OLED
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3FN8, dual-core 240MHz, 8MB flash, no PSRAM
 * Radio:   SX1262, SPI, 868/915MHz, +22dBm, -148dBm sensitivity
 * Display: SSD1306 OLED 0.96", I2C, 128×64
 * USB:     USB-C (CDC)
 * Buy:     https://heltec.org/project/wifi-lora-32-v3/
 * ⚠️ Vext (GPIO36) must be HIGH to power OLED + peripherals
 */
#elif defined(WY_BOARD_HELTEC_LORA32_V3)
  #define WY_BOARD_NAME       "Heltec WiFi LoRa 32 V3 (SX1262 + SSD1306)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        0
  /* Display SSD1306 OLED */
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_SSD1306
  #define WY_DISPLAY_BUS_I2C
  #define WY_DISPLAY_W        128
  #define WY_DISPLAY_H        64
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_SDA      17
  #define WY_DISPLAY_SCL      18
  #define WY_DISPLAY_RST      21
  #define WY_DISPLAY_ADDR     0x3C
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  /* LoRa SX1262 */
  #define WY_HAS_LORA         1
  #define WY_LORA_SX1262
  #define WY_LORA_CS          8
  #define WY_LORA_RST         12
  #define WY_LORA_IRQ         14    /* DIO1 */
  #define WY_LORA_BUSY        13
  #define WY_LORA_SCK         9
  #define WY_LORA_MOSI        10
  #define WY_LORA_MISO        11
  /* Power enable for OLED + peripherals */
  #define WY_VEXT_PIN         36    /* HIGH = power on */
  #define WY_BOOT_BTN         0
  #define WY_SCREEN_W         WY_DISPLAY_W
  #define WY_SCREEN_H         WY_DISPLAY_H

/* ══════════════════════════════════════════════════════════════════
 * LilyGo T-Beam Supreme S3 — ESP32-S3 + SX1262 + SH1106 + GPS + IMU
 * ══════════════════════════════════════════════════════════════════
 * MCU:     ESP32-S3FN8, dual-core 240MHz, 8MB flash, 8MB QSPI PSRAM
 * Radio:   SX1262, SPI, 868/915MHz
 * Display: SH1106 OLED 1.3", I2C (shared bus), 128×64
 * GPS:     L76K or Ublox MAX-M10S (UART, GPIO8/9)
 * IMU:     QMI8658 6-axis (SPI shared, CS=34)
 * RTC:     PCF8563 (I2C shared)
 * PMU:     AXP2101 (I2C shared) — must init before display/GPS
 * SD:      SPI shared, CS=47
 * Buy:     https://www.lilygo.cc/products/softrf-t-beamsupreme
 * ⚠️ AXP2101 PMU must be init first — powers LoRa, GPS, OLED rails
 * ⚠️ SPI shared: LoRa CS=39, IMU CS=34, SD CS=47
 */
#elif defined(WY_BOARD_LILYGO_TBEAM_SUPREME)
  #define WY_BOARD_NAME       "LilyGo T-Beam Supreme S3 (SX1262 + SH1106 + GPS)"
  #define WY_MCU_ESP32S3
  #define WY_MCU_CORES        2
  #define WY_MCU_FREQ         240
  #define WY_HAS_PSRAM        1
  /* Display SH1106 OLED */
  #define WY_HAS_DISPLAY      1
  #define WY_DISPLAY_SH1106
  #define WY_DISPLAY_BUS_I2C
  #define WY_DISPLAY_W        128
  #define WY_DISPLAY_H        64
  #define WY_DISPLAY_ROT      0
  #define WY_DISPLAY_SDA      17
  #define WY_DISPLAY_SCL      18
  #define WY_DISPLAY_ADDR     0x3C
  #define WY_HAS_TOUCH        0
  #define WY_HAS_RGB_LED      0
  /* LoRa SX1262 — shared SPI bus */
  #define WY_HAS_LORA         1
  #define WY_LORA_SX1262
  #define WY_LORA_CS          39
  #define WY_LORA_RST         -1    /* reset via PMU */
  #define WY_LORA_IRQ         1     /* DIO1 */
  #define WY_LORA_BUSY        4
  #define WY_LORA_SCK         36
  #define WY_LORA_MOSI        35
  #define WY_LORA_MISO        37
  /* GPS UART */
  #define WY_GPS_TX           8
  #define WY_GPS_RX           9
  #define WY_GPS_PPS          6
  #define WY_GPS_WAKEUP       7
  /* IMU QMI8658 */
  #define WY_IMU_CS           34
  #define WY_IMU_IRQ          33
  /* SD card */
  #define WY_SD_CS            47
  /* PMU AXP2101 + RTC PCF8563 — shared I2C */
  #define WY_PMU_ADDR         0x34
  #define WY_RTC_ADDR         0x51
  #define WY_PMU_IRQ          40
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

// ═══════════════════════════════════════════════════════════════════
// LOLIN S3 Pro v1.0.0
// ESP32-S3-WROOM-1, 16MB Flash, 8MB PSRAM (Octal), 25 IO
// LOLIN I2C port, LOLIN Display port, MicroSD, LiPo charging 500mA
// 65.3×25.4mm · https://www.wemos.cc/en/latest/s3/s3_pro.html
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_LOLIN_S3_PRO)
  #define WY_BOARD_NAME          "LOLIN S3 Pro"
  #define WY_MCU                 "ESP32-S3"
  #define WY_ARCH                "xtensa-lx7"
  #define WY_FLASH_MB            16
  #define WY_PSRAM_MB            8
  #define WY_CPU_MHZ             240
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  #define WY_HAS_SD              1
  #define WY_HAS_BATTERY         1
  // LOLIN I2C Port (Qwiic compatible)
  #define WY_I2C_SDA             8
  #define WY_I2C_SCL             9
  // LOLIN Display Port (SPI)
  #define WY_SPI_MOSI            11
  #define WY_SPI_MISO            13
  #define WY_SPI_SCK             12
  #define WY_SPI_SS              10
  // SD card (SPI shared)
  #define WY_SD_CS               34
  // Battery / charging
  #define WY_BAT_ADC             1
  // USB OTG
  #define WY_USB_DP              20
  #define WY_USB_DN              19

// ═══════════════════════════════════════════════════════════════════
// ESP32-C3 1.28" Round LCD (GC9A01, 240×240 IPS)
// ESP32-C3, 4MB Flash, WiFi + BLE, 1.28" 240×240 round display
// Popular AliExpress dev board — also sold as "ESP32-C3 LVGL"
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_ESP32C3_GC9A01_128)
  #define WY_BOARD_NAME          "ESP32-C3 GC9A01 1.28\""
  #define WY_MCU                 "ESP32-C3"
  #define WY_ARCH                "riscv32"
  #define WY_FLASH_MB            4
  #define WY_PSRAM_MB            0
  #define WY_CPU_MHZ             160
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  // GC9A01 display (SPI)
  #define WY_DISPLAY_DRIVER      "GC9A01"
  #define WY_DISPLAY_W           240
  #define WY_DISPLAY_H           240
  #define WY_DISPLAY_ROUND       1
  #define WY_TFT_MOSI            7
  #define WY_TFT_SCK             6
  #define WY_TFT_CS              10
  #define WY_TFT_DC              2
  #define WY_TFT_RST             3
  #define WY_TFT_BL              11
  // I2C
  #define WY_I2C_SDA             4
  #define WY_I2C_SCL             5

// ═══════════════════════════════════════════════════════════════════
// Freenove ESP32-S3 WROOM CAM
// ESP32-S3-WROOM-1, 8MB Flash, 8MB PSRAM, OV2640 camera
// 44 GPIO exposed, USB OTG, LED flash GPIO48
// https://github.com/Freenove/Freenove_ESP32_S3_WROOM_Board
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_FREENOVE_ESP32S3_CAM)
  #define WY_BOARD_NAME          "Freenove ESP32-S3 WROOM CAM"
  #define WY_MCU                 "ESP32-S3"
  #define WY_ARCH                "xtensa-lx7"
  #define WY_FLASH_MB            8
  #define WY_PSRAM_MB            8
  #define WY_CPU_MHZ             240
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  #define WY_HAS_CAMERA          1
  // OV2640 camera (same as ESP32-CAM pinout)
  #define WY_CAM_PWDN            -1
  #define WY_CAM_RESET           -1
  #define WY_CAM_XCLK            15
  #define WY_CAM_SIOD            4
  #define WY_CAM_SIOC            5
  #define WY_CAM_D7              16
  #define WY_CAM_D6              17
  #define WY_CAM_D5              18
  #define WY_CAM_D4              12
  #define WY_CAM_D3              10
  #define WY_CAM_D2              8
  #define WY_CAM_D1              9
  #define WY_CAM_D0              11
  #define WY_CAM_VSYNC           6
  #define WY_CAM_HREF            7
  #define WY_CAM_PCLK            13
  // LED flash
  #define WY_LED_FLASH           48
  // USB OTG
  #define WY_USB_DP              20
  #define WY_USB_DN              19

// ═══════════════════════════════════════════════════════════════════
// Tscinbuny ESP32-PLUS with OV2640
// ESP32-WROVER (or WROOM), 4MB Flash, OV2640 camera
// Similar to AI-Thinker ESP32-CAM but with extra IO breakout
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_TSCINBUNY_ESP32_PLUS_CAM)
  #define WY_BOARD_NAME          "Tscinbuny ESP32-PLUS CAM"
  #define WY_MCU                 "ESP32"
  #define WY_ARCH                "xtensa-lx6"
  #define WY_FLASH_MB            4
  #define WY_PSRAM_MB            4
  #define WY_CPU_MHZ             240
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  #define WY_HAS_CAMERA          1
  // OV2640 — AI-Thinker ESP32-CAM compatible pinout
  #define WY_CAM_PWDN            32
  #define WY_CAM_RESET           -1
  #define WY_CAM_XCLK            0
  #define WY_CAM_SIOD            26
  #define WY_CAM_SIOC            27
  #define WY_CAM_D7              35
  #define WY_CAM_D6              34
  #define WY_CAM_D5              39
  #define WY_CAM_D4              36
  #define WY_CAM_D3              21
  #define WY_CAM_D2              19
  #define WY_CAM_D1              18
  #define WY_CAM_D0              5
  #define WY_CAM_VSYNC           25
  #define WY_CAM_HREF            23
  #define WY_CAM_PCLK            22
  #define WY_SD_CS               4
  #define WY_LED_FLASH           4

// ═══════════════════════════════════════════════════════════════════
// ESP32-S3 LVGL HMI RGB LCD (AliExpress generic — ST7262 / 800×480)
// ESP32-S3R8, 8MB PSRAM, 16MB Flash, 4.3" 800×480 RGB LCD, GT911 touch
// Commonly sold as "ESP32-S3 LVGL WiFi BLE HMI Development Board"
// Similar to Waveshare ESP32-S3-Touch-LCD-4.3
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_ESP32S3_LVGL_HMI_43)
  #define WY_BOARD_NAME          "ESP32-S3 LVGL HMI 4.3\""
  #define WY_MCU                 "ESP32-S3"
  #define WY_ARCH                "xtensa-lx7"
  #define WY_FLASH_MB            16
  #define WY_PSRAM_MB            8
  #define WY_CPU_MHZ             240
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  #define WY_HAS_SD              1
  #define WY_HAS_TOUCH           1
  // RGB parallel LCD (ST7262 / ILI6485)
  #define WY_DISPLAY_DRIVER      "ST7262"
  #define WY_DISPLAY_W           800
  #define WY_DISPLAY_H           480
  #define WY_DISPLAY_BUS         "RGB"
  // RGB bus pins (standard for this class of board)
  #define WY_RGB_PCLK            7
  #define WY_RGB_DE              5
  #define WY_RGB_VSYNC           3
  #define WY_RGB_HSYNC           46
  #define WY_RGB_B0              14
  #define WY_RGB_B1              38
  #define WY_RGB_B2              18
  #define WY_RGB_B3              17
  #define WY_RGB_B4              10
  #define WY_RGB_G0              39
  #define WY_RGB_G1              0
  #define WY_RGB_G2              45
  #define WY_RGB_G3              48
  #define WY_RGB_G4              47
  #define WY_RGB_G5              21
  #define WY_RGB_R0              11
  #define WY_RGB_R1              12
  #define WY_RGB_R2              13
  #define WY_RGB_R3              1
  #define WY_RGB_R4              2
  // GT911 touch (I2C)
  #define WY_TOUCH_DRIVER        "GT911"
  #define WY_I2C_SDA             8
  #define WY_I2C_SCL             9
  #define WY_TOUCH_INT           4
  #define WY_TOUCH_RST           -1
  // Backlight
  #define WY_TFT_BL              -1  // always on or controlled via expander

// ═══════════════════════════════════════════════════════════════════
// ILI9488 SPI Touchscreen (generic 3.5" / 4" module)
// 480×320, XPT2046 resistive touch, SPI interface
// Common on many carrier boards; virtual board for pin mapping
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_ILI9488_SPI_GENERIC)
  #define WY_BOARD_NAME          "ILI9488 SPI Generic"
  #define WY_MCU                 ""   // peripheral only — pair with any MCU board
  #define WY_DISPLAY_DRIVER      "ILI9488"
  #define WY_DISPLAY_W           480
  #define WY_DISPLAY_H           320
  #define WY_DISPLAY_BUS         "SPI"
  #define WY_HAS_TOUCH           1
  #define WY_TOUCH_DRIVER        "XPT2046"
  // Typical wiring when used with ESP32 (customise per project)
  #define WY_TFT_MOSI            23
  #define WY_TFT_MISO            19
  #define WY_TFT_SCK             18
  #define WY_TFT_CS              15
  #define WY_TFT_DC              2
  #define WY_TFT_RST             4
  #define WY_TFT_BL              -1
  #define WY_TOUCH_CS            5
  #define WY_TOUCH_IRQ           33


// ═══════════════════════════════════════════════════════════════════
// Guition JC3248W535 — 3.5" 320×480 ESP32-S3 HMI
// ESP32-S3R8, 8MB PSRAM, 16MB Flash, ST7796 320×480 SPI, GT911 touch
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_GUITION_3248W535)
  #define WY_BOARD_NAME          "Guition JC3248W535 3.5\""
  #define WY_MCU                 "ESP32-S3"
  #define WY_ARCH                "xtensa-lx7"
  #define WY_FLASH_MB            16
  #define WY_PSRAM_MB            8
  #define WY_CPU_MHZ             240
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  #define WY_HAS_SD              1
  #define WY_HAS_TOUCH           1
  #define WY_DISPLAY_DRIVER      "ST7796"
  #define WY_DISPLAY_W           320
  #define WY_DISPLAY_H           480
  #define WY_DISPLAY_BUS         "SPI"
  #define WY_TFT_MOSI            13
  #define WY_TFT_MISO            12
  #define WY_TFT_SCK             11
  #define WY_TFT_CS              10
  #define WY_TFT_DC              9
  #define WY_TFT_RST             8
  #define WY_TFT_BL              0
  #define WY_TOUCH_DRIVER        "GT911"
  #define WY_I2C_SDA             4
  #define WY_I2C_SCL             5
  #define WY_TOUCH_INT           3
  #define WY_SD_CS               14

// ═══════════════════════════════════════════════════════════════════
// Guition JC8048W550 — 5" 800×480 ESP32-S3 HMI
// ESP32-S3R8, 8MB PSRAM, 16MB Flash, 800×480 RGB LCD, GT911 touch
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_GUITION_8048W550)
  #define WY_BOARD_NAME          "Guition JC8048W550 5\""
  #define WY_MCU                 "ESP32-S3"
  #define WY_ARCH                "xtensa-lx7"
  #define WY_FLASH_MB            16
  #define WY_PSRAM_MB            8
  #define WY_CPU_MHZ             240
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  #define WY_HAS_SD              1
  #define WY_HAS_TOUCH           1
  #define WY_DISPLAY_DRIVER      "ST7262"
  #define WY_DISPLAY_W           800
  #define WY_DISPLAY_H           480
  #define WY_DISPLAY_BUS         "RGB"
  // RGB pins (same family as 4.3" board)
  #define WY_RGB_PCLK            7
  #define WY_RGB_DE              5
  #define WY_RGB_VSYNC           3
  #define WY_RGB_HSYNC           46
  #define WY_RGB_B0              14
  #define WY_RGB_B1              38
  #define WY_RGB_B2              18
  #define WY_RGB_B3              17
  #define WY_RGB_B4              10
  #define WY_RGB_G0              39
  #define WY_RGB_G1              0
  #define WY_RGB_G2              45
  #define WY_RGB_G3              48
  #define WY_RGB_G4              47
  #define WY_RGB_G5              21
  #define WY_RGB_R0              11
  #define WY_RGB_R1              12
  #define WY_RGB_R2              13
  #define WY_RGB_R3              1
  #define WY_RGB_R4              2
  #define WY_TOUCH_DRIVER        "GT911"
  #define WY_I2C_SDA             8
  #define WY_I2C_SCL             9
  #define WY_TOUCH_INT           4
  #define WY_SD_CS               -1

// ═══════════════════════════════════════════════════════════════════
// Guition JC2432W328 / JC3232W328 — 2.8/3.2" 320×240 ESP32-S3 HMI
// ESP32-S3, ILI9341 320×240 SPI, XPT2046 resistive touch
// ═══════════════════════════════════════════════════════════════════
#elif defined(WY_BOARD_GUITION_3232W328)
  #define WY_BOARD_NAME          "Guition JC3232W328 3.2\""
  #define WY_MCU                 "ESP32-S3"
  #define WY_ARCH                "xtensa-lx7"
  #define WY_FLASH_MB            8
  #define WY_PSRAM_MB            8
  #define WY_CPU_MHZ             240
  #define WY_HAS_WIFI            1
  #define WY_HAS_BLE             1
  #define WY_HAS_TOUCH           1
  #define WY_DISPLAY_DRIVER      "ILI9341"
  #define WY_DISPLAY_W           320
  #define WY_DISPLAY_H           240
  #define WY_TFT_MOSI            13
  #define WY_TFT_MISO            12
  #define WY_TFT_SCK             11
  #define WY_TFT_CS              10
  #define WY_TFT_DC              9
  #define WY_TFT_RST             8
  #define WY_TFT_BL              0
  #define WY_TOUCH_DRIVER        "XPT2046"
  #define WY_TOUCH_CS            7
  #define WY_TOUCH_IRQ           6

