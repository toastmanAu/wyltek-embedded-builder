# X9C — Digital Potentiometer
**Driver:** `WyX9C.h`

---

## What it does
A digital potentiometer — the same as a mechanical knob, but controlled by a microcontroller. 100 wiper positions (0–99). Use it wherever you'd use a variable resistor: volume control, bias adjustment, LED brightness, gain control on op-amps.

## Variants

All are electrically identical except the resistance value:

| Class | Part | Resistance |
|-------|------|-----------|
| `WyX9C102` | X9C102 | 1kΩ |
| `WyX9C103` | X9C103 | 10kΩ |
| `WyX9C503` | X9C503 | 50kΩ |
| `WyX9C104` | X9C104 | 100kΩ |

## Wiring

The X9C uses a 3-wire GPIO interface (not standard SPI, despite looking similar).

| X9C Pin | ESP32 | Notes |
|---------|-------|-------|
| CS | GPIO | Active LOW chip select |
| INC | GPIO | Falling edge = one wiper step |
| U/D | GPIO | HIGH = up (increment), LOW = down (decrement) |
| VH | Signal or voltage | High terminal |
| VL | GND or reference | Low terminal |
| VW | Wiper output | Variable tap |
| VCC | 5V (2.7–5.5V) | Power supply |
| GND | GND | |

## Typical use — voltage divider

```
VH = 3.3V
VL = GND
VW = variable output (0V to 3.3V)
     VW = 3.3V × (position / 99)
```

## Code Example

```cpp
#include <WySensors.h>
#include <sensors/drivers/WyX9C.h>

WySensors sensors;
WyX9C103* pot;

#define CS_PIN  5
#define INC_PIN 18
#define UD_PIN  19

void setup() {
    pot = sensors.addGPIO<WyX9C103>("volume", CS_PIN, INC_PIN);
    pot->setUDPin(UD_PIN);  // must call before begin()
    sensors.begin();

    pot->set(50);   // start at 50% (midpoint)
}

void loop() {
    // Fade up slowly
    for (int i = 0; i <= 99; i++) {
        pot->set(i);
        delay(20);
    }
    delay(1000);

    // Fade down
    for (int i = 99; i >= 0; i--) {
        pot->set(i);
        delay(20);
    }
    delay(1000);
}
```

## Position control

```cpp
pot->set(75);          // absolute position 0–99
pot->setPercent(0.5f); // 50% = position 49
pot->increment();       // +1 step
pot->decrement();       // -1 step
pot->move(5);           // +5 steps
pot->move(-10);         // -10 steps

int pos  = pot->position();  // current position 0–99
float pct = pot->percent();  // 0.0 – 1.0
```

## Non-volatile memory (NV)

The X9C has built-in EEPROM — it remembers the last stored position through power cycles:

```cpp
pot->set(75);    // set position
pot->store();    // save to NV memory — survives reboot
// On next boot, sensor initialises to position 0 (home),
// unless you load the stored value and restore it
```

**Don't call `store()` on every change.** NV write endurance is ~100,000 cycles. Store only when the user finishes adjusting.

## WySensorData fields

| Field | Content |
|-------|---------|
| `raw` | Current wiper position (0–99) |
| `voltage` | Position as fraction (0.0–1.0) |
| `ok` | Always `true` |

## Gotchas

**`setUDPin()` must be called before `begin()`.** If you forget, `begin()` returns `false` and prints an error. The INC and UD pins are both required.

**`begin()` drives the wiper to position 0.** On power-on the X9C restores its wiper to the last stored position (from NV). The driver doesn't know this position, so it drives 99 steps downward to ensure it's at 0. This means the wiper always starts at 0 after `begin()`, regardless of what it was previously.

**Pulse timing is 2µs minimum.** The driver uses `WY_X9C_PULSE_US = 2`. If your clock speed or timing is off and pulses are too short, the wiper may skip steps. Increase with `-DWY_X9C_PULSE_US=5`.

**NV store requires CS rising while INC is LOW.** The store sequence is exactly: `CS→LOW`, `INC→LOW`, `CS→HIGH`, wait 20ms, `INC→HIGH`. Getting this wrong (e.g. CS rises while INC is HIGH) doesn't store. The driver handles this correctly.

**Maximum steps per `set()` call.** Moving from 0 to 99 requires 99 pulses × 4µs each = ~396µs for the pulses, plus `CS` assert/deassert overhead. For real-time audio applications this is fast enough. For critical timing loops, pre-calculate the position change.

**5V logic levels.** The X9C is a 5V device. While it accepts 3.3V logic on its control pins (GPIO signals), the VCC must be 2.7–5.5V. Use 3.3V supply if your resistance accuracy requirements aren't tight — the wiper resistance and terminal resistance are slightly affected by supply voltage.
