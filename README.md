# wyltek-embedded-builder

A modular embedded SDK for ESP32 — display, touch, settings, UI, sensors, and networking.

Board-agnostic hardware abstraction for rapid firmware development.

## Philosophy

Write your application once. Point it at a board. It works.

No copy-pasting display init sequences. No re-implementing captive portals. No per-project settings boilerplate. Just include what you need and build the thing.

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

MIT
