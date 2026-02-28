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

| Board | Display | Touch | Board JSON |
|-------|---------|-------|-----------|
| ESP32 CYD (2432S028R) | ILI9341 320×240 SPI | XPT2046 resistive | `boards/esp32-2432s028r.json` |
| Guition ESP32-S3-4848S040 | ST7701S 480×480 RGB | GT911 I2C | `boards/guition4848s040.json` |

More boards coming.

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

Early development — born from real firmware projects in the [CKB ecosystem](https://github.com/toastmanAu).

Confirmed working:
- ✅ GT911 touch on Guition 4848S040
- ✅ ST7701S RGB panel on Guition 4848S040  
- ✅ XPT2046 + ILI9341 on CYD (ESP32-2432S028R)
- ✅ NVS settings persistence

## License

Wyltek Embedded Builder is MIT licensed.

Third-party components wrapped within this library retain their original licenses. Each `Wy*` wrapper file that incorporates third-party code includes the original license, author, and source URL in its header comment. See individual files and `LICENSES/` for full text of third-party licenses.
