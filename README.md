# wyltek-embedded-builder

A modular embedded SDK for ESP32 — display, touch, settings, UI, sensors, and networking.

Board-agnostic hardware abstraction for rapid firmware development.

## Philosophy

Write your application once. Point it at a board. It works.

No copy-pasting display init sequences. No re-implementing captive portals. No per-project settings boilerplate. Just include what you need and build the thing.

## Growth Policy

This library grows with real projects. When a project needs something that doesn't exist here:

1. **Build it here first** as a `Wy*` component — not inside the project repo
2. **External dependencies** — if it wraps a third-party lib, fork it to `toastmanAu/`, attribute clearly in the header, add as `lib_deps`
3. **Use it in the project** via `lib_deps = https://github.com/toastmanAu/wyltek-embedded-builder`

The rule: **every reusable piece lives here**. Project repos only contain project-specific code.

### Attribution format (for wrapped libs)
```cpp
// WyFoo.h — wraps libfoo by Original Author (MIT)
// Fork: https://github.com/toastmanAu/libfoo
// Original: https://github.com/originalauthor/libfoo
// Changes: adapted for Wy* API, stripped platform deps
```

### Licensing rules

- **Check the license before forking anything.** If it's GPL/LGPL, wrapping it in a Wy* header does not escape the copyleft — the combined work must be GPL-compatible.
- **Permissive licenses (MIT, BSD, Apache 2.0, ISC)** — fork freely, attribute, include the original LICENSE file in the fork.
- **LGPL** — can link dynamically (not applicable in embedded/static builds). For ESP32 static builds, treat as GPL unless the original author grants an exception.
- **GPL** — do not incorporate into wyltek-embedded-builder (MIT). Can be used in standalone project repos if the project is also GPL, but document clearly.
- **No-license / All Rights Reserved** — do not use. No exceptions.
- **Attribution is not optional.** Original author name, license, and source URL go in every file header that wraps or derives from third-party code.
- When uncertain about a license interaction — check first, don't assume.

## Modules

| Module | Class | What it does |
|--------|-------|-------------|
| `display/` | `WyDisplay` | Board-specific display init, common draw primitives |
| `touch/` | `WyTouch` | Unified touch API — GT911, XPT2046, CST816 |
| `settings/` | `WySettings` | NVS read/write, captive portal, boot mode detection |
| `ui/` | `WyUI` | Screen manager, buttons, text fields, nav stack |
| `net/` | `WyNet` | WiFi manager, OTA helpers |
| `sensors/` | `WySensors` | I2C sensor abstractions |

## Supported Boards

**43 board targets** defined in `src/boards.h` — select via PlatformIO build flag:

```ini
build_flags = -DWY_BOARD_CYD   ; or any board below
```

**ESP32 classic**  
CYD · CYD2USB · M5Stack Core · ILI9341 Generic · ILI9341 Adafruit · GC9A01 Generic · ST7789 Generic · ILI9488 SPI Generic · Double-Eye · ESP32CAM

**ESP32-S3**  
Guition 4848S040 · Guition 3248W535 · Guition 8048W550 · Guition 3232W328 · Sunton 8048S043 · ESP32-3248S035 · WT32-SC01 Plus · LilyGo T-Display S3 · LilyGo T-Display S3 AMOLED · LilyGo T-Display S3 Long · LilyGo T-Keyboard S3 · LilyGo T-Impulse · LilyGo T-Deck · LilyGo T-Pico S3 · Waveshare 1.47" S3 · Waveshare 2.00" S3 · XIAO S3 Round · ESP32-S3 Eye · Freenove ESP32-S3 Cam · ESP32S3 LVGL HMI 4.3" · LOLIN S3 Pro

**ESP32-C3 / C6**  
ESP32-C3 GC9A01 128 · LilyGo T-QT C6

**LoRa / Cellular / GPS**  
LilyGo A7670SA · LilyGo TSIM7080G-S3 · LilyGo T-Beam · LilyGo T-Beam Meshtastic · LilyGo T-Beam Supreme · TTGO T-Go · TTGO T-Display · Heltec LoRa32 V3

**Wearable**  
T-Watch 2020 V3

**Camera**  
ESP32CAM · ESP32-S3 Eye · Freenove S3 Cam · TSCinBuny ESP32-Plus-Cam

## Quick Start

```cpp
#include <WyDisplay.h>
#include <WyTouch.h>
#include <WySettings.h>

WyDisplay display;
WyTouch touch;
WySettings settings;

void setup() {
    display.begin();           // board auto-detected via compile flags
    touch.begin();
    settings.begin("myapp");   // loads from NVS, captive portal on first boot
}

void loop() {
    if (touch.update()) {
        display.fillCircle(touch.x, touch.y, 10, WY_RED);
    }
}
```

Select your board in `platformio.ini`:
```ini
build_flags = -DWY_BOARD_GUITION4848S040
```

## Status

Active development — born from real firmware projects in the [CKB ecosystem](https://github.com/toastmanAu).

Hardware confirmed working:
- ✅ GT911 touch on Guition 4848S040
- ✅ ST7701S RGB panel on Guition 4848S040  
- ✅ XPT2046 + ILI9341 on CYD (ESP32-2432S028R)
- ✅ NVS settings persistence

Host test suite: **753/753 passing** — boards.h validation + sensor math + settings logic, no Arduino SDK required.

```bash
bash test/run_tests.sh
# → 43/43 boards, 753 tests, 0 failed
```

## License

Wyltek Embedded Builder is MIT licensed.

Third-party components wrapped within this library retain their original licenses. Each `Wy*` wrapper file that incorporates third-party code includes the original license, author, and source URL in its header comment. See individual files and `LICENSES/` for full text of third-party licenses.

## Hardware Test Status

> **Legend:** 🟢 Physically tested on real hardware | 🔵 Virtually tested (code compiles, pinouts from datasheet/community) | ⚪ Untested

| Board / Component | Define | Status | Notes |
|---|---|---|---|
| ESP32-2432S028R (CYD) | `WY_BOARD_CYD` | 🟢 2026-02 | NerdMiner flashed, ckb-light-esp compile target |
| Guition ESP32-S3-4848S040 | `WY_BOARD_GUITION4848S040` | 🟢 2026-02 | Real mainnet CKB TX confirmed, Fiber kiosk node |
| All other boards | — | 🔵 Virtual | Code verified, not physically on-hand yet |
| GM861S Barcode Scanner | `WyGM861S` | 🟢 2026-02 | BlackBox POS — QR invoice scanning confirmed |
| QR204 Thermal Printer | `WyHSQR204` | 🟢 2026-02 | BlackBox POS — thermal QR printing confirmed |
| DFPlayer Mini | `WyDFPlayer` | 🟢 2026-02 | Protocol tested on bench, clone chip workaround confirmed |
| All other sensors | — | 🔵 Virtual | Drivers from datasheet + community-verified pinouts |

Hardware status is also visible on the [Wyltek Hardware Reference](https://wyltekindustries.com/hardware.html) page — each card shows its test badge with date and hover notes.

To mark a component as physically tested: update the `tested` field in `hardware.html` and add a row here.
