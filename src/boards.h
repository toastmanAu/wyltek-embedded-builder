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

#else
  #warning "wyltek-embedded-builder: no board defined. Add -DWY_BOARD_xxx to build_flags."
  #define WY_HAS_DISPLAY  0
  #define WY_HAS_TOUCH    0
  #define WY_HAS_RGB_LED  0
  #define WY_HAS_PSRAM    0
  #define WY_SCREEN_W     0
  #define WY_SCREEN_H     0
#endif
