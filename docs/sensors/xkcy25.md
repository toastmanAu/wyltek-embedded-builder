# XKC-Y25 — Non-Contact Liquid Level Sensor
**Driver:** `WyXKCY25.h`

---

## What it does
Detects the presence of liquid at a specific level through the wall of a non-conductive tank — no holes, no sealing, no contact with the liquid. It uses capacitive sensing to detect the change in dielectric constant when liquid is present. Output is a simple digital HIGH or LOW.

## How it works
The sensor sticks flat against the outside of a plastic, glass, or ceramic tank. When liquid is at that level, it changes the capacitance the sensor detects, and the output pin switches state. Move the sensor up or down to set your trigger level.

**Does not work through metal.** Metal walls block the capacitive field completely.

## Variants

All these variants use the same driver:

| Part | Output logic | Notes |
|------|-------------|-------|
| XKC-Y25-V | NPN — LOW when liquid present | Most common |
| XKC-Y25-PNP | PNP — HIGH when liquid present | Use `WyXKCY25PNP` |
| XKC-Y23-V | Same as Y25 | Smaller body |
| XKC-Y24-V | Same as Y25 | Extended sensing range |

## Wiring (XKC-Y25-V NPN, the common one)

| Wire colour | Connects to |
|------------|------------|
| Brown | 5V–12V supply |
| Blue | GND |
| Black | Signal → ESP32 GPIO |

**Pull-up resistor required for NPN.** Without it the output floats when no liquid is present. Add 10kΩ between signal and 3.3V, or use `INPUT_PULLUP` (the driver does this automatically):

```
  3.3V ──10kΩ──┬── ESP32 GPIO
               └── Sensor Black wire
```

For long wire runs (>50cm), use an external 10kΩ pull-up instead of relying on the ESP32's internal ~45kΩ.

## Wiring (XKC-Y25-PNP)

Same colours. The signal goes HIGH when liquid is present. The driver handles this automatically:
```cpp
sensors.addGPIO<WyXKCY25PNP>("level", PIN);
```

## Multiple level sensors

A common setup is low/mid/high level detection on a tank:

```cpp
sensors.addGPIO<WyXKCY25>("level_low",  PIN_LOW);   // 20% level
sensors.addGPIO<WyXKCY25>("level_high", PIN_HIGH);  // 80% level

bool empty = !sensors.read("level_low").rawInt;
bool full  =  sensors.read("level_high").rawInt;
```

## Specs

| Parameter | Value |
|-----------|-------|
| Supply | 5V–12V DC |
| Output | NPN open-collector (or PNP) |
| Max current | 200mA sink |
| Response time | <100ms |
| Wall thickness | Up to ~14mm |
| Operating temp | -25°C to +70°C |

## Code Example

```cpp
#include <WySensors.h>

WySensors sensors;

void setup() {
    sensors.addGPIO<WyXKCY25>("tank", 4);
    sensors.begin();
}

void loop() {
    WySensorData d = sensors.read("tank");
    bool hasLiquid = d.rawInt;

    Serial.printf("Tank at sensor level: %s\n",
        hasLiquid ? "LIQUID PRESENT" : "empty");
    delay(500);
}
```

## State-change callback

```cpp
auto* level = sensors.addGPIO<WyXKCY25>("tank", 4);
level->onStateChange([](bool present) {
    Serial.printf("Level changed: %s\n", present ? "FULL" : "EMPTY");
});

// In loop:
level->poll();
```

## WySensorData fields

| Field | Content |
|-------|---------|
| `rawInt` | `1` if liquid present, `0` if not |
| `raw` | Same as rawInt (float) |
| `ok` | Always `true` |

## Gotchas

**Mounting matters.** The flat face of the sensor must be flush against the tank wall. Use the included double-sided mounting tape or a cable tie. Any gap or tilt reduces sensitivity.

**Sensitivity can be adjusted.** Some variants have a small potentiometer or trim screw on the body. Turn it to adjust the detection threshold for different wall thicknesses.

**Metal objects nearby interfere.** Metal brackets, pipes, or the tank fill port can affect the capacitive field. Mount at least 20–30mm away from metal objects.

**Doesn't work for conductive tank walls.** Stainless steel tanks, metal jerry cans, aluminium containers — none of these work. Use a float switch or JSN-SR04T ultrasonic for those.

**Debouncing.** The driver does a 3-sample majority vote (500µs between samples) to filter glitches. Change with `-DWY_XKCY25_DEBOUNCE=5` for more stability in electrically noisy environments.

**100ms settle time.** The driver calls `delay(100)` in `begin()` to let the sensor settle after power-on. This is already done — you don't need to add your own delay.
