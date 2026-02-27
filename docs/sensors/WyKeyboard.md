# WyKeyboard

On-screen touch keyboard that auto-scales to any display size.

## Features

- **QWERTY, Numeric, Symbol** layouts with one-tap switching
- **Auto-scales** — key size computed from display dimensions at `begin()`
- **Password mode** — shows `███` instead of typed characters  
- **Dark and light themes** built-in, custom themes supported
- **Shift / Caps Lock**, backspace, space, enter, cancel
- **Edit mode** — pre-fill with `setValue()`
- **Zero heap allocation** — all buffers on stack

## Quick Start

```cpp
#include <ui/WyKeyboard.h>

WyKeyboard kb;

void setup() {
    display.begin();
    touch.begin();
    kb.begin(display.gfx, display.width, display.height);

    // Show keyboard at bottom of screen
    kb.show("Enter WiFi SSID:", 32);
}

void loop() {
    int tx, ty;
    if (touch.getXY(&tx, &ty)) {
        WyKbResult res = kb.press(tx, ty);
        if (res == WY_KB_DONE)   Serial.println(kb.value());
        if (res == WY_KB_CANCEL) Serial.println("cancelled");
    }
}
```

## API

### `begin(gfx, width, height, theme?)`
Must be called once. Pass your display dimensions. Optional theme (default: dark).

### `show(prompt?, maxLen?, password?, layout?, yOffset?)`
Shows the keyboard. Clears any previous input.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `prompt`  | `nullptr` | Label above input field |
| `maxLen`  | `128`   | Max input length |
| `password`| `false` | Mask input with solid blocks |
| `layout`  | `QWERTY`| Initial layout |
| `yOffset` | auto    | Top of keyboard area in pixels (default: 45% of screen height) |

### `press(tx, ty) → WyKbResult`
Call whenever a touch event occurs. Returns:
- `WY_KB_NONE` — no action  
- `WY_KB_TYPING` — character added/removed  
- `WY_KB_DONE` — Enter pressed  
- `WY_KB_CANCEL` — ESC pressed (input cleared)  

### `value()` → `const char*`
Returns current input string.

### `length()` → `int`
Current input length.

### `active()` → `bool`
True while keyboard is visible.

### `hide()`
Hides keyboard (caller must redraw area).

### `setValue(str)`
Pre-fills input field (edit mode).

### `clear()`
Clears input field.

### `kb_top()` → `int`
Y coordinate where keyboard starts (useful for drawing content above keyboard).

## Layouts

```cpp
kb.show("Enter PIN:", 6, true, WY_KB_NUMERIC);   // numbers only
kb.show("Password:", 32, true, WY_KB_QWERTY);    // alpha + shift for symbols
```

User can switch layouts with the `?123` / `ABC` / `#+=` buttons on-screen.

## Themes

```cpp
kb.begin(display.gfx, display.width, display.height, &WY_KB_THEME_LIGHT);
```

Built-in: `WY_KB_THEME_DARK` (default), `WY_KB_THEME_LIGHT`

Custom theme:
```cpp
WyKbTheme my_theme = {
    .bg          = 0x0000,
    .key_bg      = 0x2104,
    .key_fg      = 0xFFFF,
    .key_special = 0x3186,
    .key_accent  = 0x07E0,   // green enter
    .key_press   = 0x03E0,
    .field_bg    = 0x0000,
    .field_fg    = 0xFFFF,
    .label_fg    = 0x8410,
    .border      = 0x07E0,
};
kb.begin(display.gfx, display.width, display.height, &my_theme);
```

## Convenience Macro

```cpp
WyKbResult result = WY_KB_NONE;

void loop() {
    int tx, ty;
    if (touch.getXY(&tx, &ty)) {
        WY_KB_POLL(kb, touch, result);
        if (result == WY_KB_DONE) { ... }
    }
}
```

## Display Size Behaviour

| Display | Keys/row | Key height | Notes |
|---------|----------|------------|-------|
| 240×240 | 10       | ~28px      | Compact but usable |
| 320×240 | 10       | ~32px      | CYD classic — good |
| 480×320 | 10       | ~40px      | Comfortable |
| 480×480 | 10       | ~48px      | Guition square |
| 800×480 | 10       | ~52px      | Sunton landscape |

The keyboard always occupies the bottom ~55% of the screen (configurable via `yOffset`). Content above is untouched.

## Requirements

- `WY_HAS_DISPLAY=1` (set by your board define in `boards.h`)
- `Arduino_GFX_Library`
- Touch input: any source that provides `(tx, ty)` integers
