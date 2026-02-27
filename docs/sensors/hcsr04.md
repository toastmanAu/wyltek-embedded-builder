# HC-SR04 / JSN-SR04T — Ultrasonic Distance Sensor
**Driver:** `WyHCSR04.h`

---

## What it does
Measures distance using ultrasonic pulses — sends a sound burst and times how long it takes to bounce back. Range is 2cm to 400cm (about 4 metres). The JSN-SR04T is the waterproof tube version using the same protocol but with a 20cm minimum range.

## Wiring

| HC-SR04 Pin | ESP32 |
|-------------|-------|
| VCC | 5V (ESP32 VIN or external 5V) |
| GND | GND |
| TRIG | Any GPIO (output) |
| ECHO | Any GPIO (input) |

**The HC-SR04 runs on 5V.** The ECHO pin outputs 5V logic, which is above the ESP32's 3.3V maximum GPIO input. Use a voltage divider:

```
ECHO ──┬── 2.2kΩ ── ESP32 GPIO
       └── 1kΩ  ── GND
```

Or use a level shifter module. Some HC-SR04 clones work fine at 3.3V — try it and check if readings make sense before adding the divider.

## JSN-SR04T (waterproof)
Same wiring, same driver. The JSN-SR04T is a sealed probe for wet environments. Alias:
```cpp
sensors.addGPIO<WyJSNSR04T>("tank", TRIG, ECHO);
```

## Specs

| Parameter | HC-SR04 | JSN-SR04T |
|-----------|---------|-----------|
| Measuring range | 2–400cm | 20–450cm |
| Resolution | ~3mm | ~3mm |
| Beam angle | 15° | ~75° (wide) |
| Frequency | 40kHz | 40kHz |
| Trigger pulse | 10µs HIGH | 10µs HIGH |
| Supply | 5V | 5V |

## Code Example

```cpp
#include <WySensors.h>

WySensors sensors;

void setup() {
    // TRIG=5, ECHO=18
    sensors.addGPIO<WyHCSR04>("distance", 5, 18);
    sensors.begin();
}

void loop() {
    WySensorData d = sensors.read("distance");
    if (d.ok) {
        Serial.printf("Distance: %.0f mm  (%.1f cm)\n",
            d.distance, d.raw);
    } else {
        Serial.println("Out of range or no object");
    }
    delay(100);
}
```

## WySensorData fields

| Field | Content |
|-------|---------|
| `distance` | Distance in mm |
| `raw` | Distance in cm |
| `ok` | `true` if measurement valid |
| `error` | `"timeout / out of range"` if no echo |

## Temperature compensation

The speed of sound changes with temperature. At 20°C it's 343 m/s. At 0°C it's 331 m/s — about 3.5% slower. For most applications this doesn't matter. If you have a BME280 or DS18B20 nearby, you can feed the current temperature:

```cpp
auto* sonic = sensors.addGPIO<WyHCSR04>("range", TRIG, ECHO);
// Later, with temp reading available:
sonic->setTemperature(28.5);  // update if temp changes significantly
```

## Gotchas

**Only one measurement at a time.** The driver is blocking — `read()` triggers a pulse and waits up to 25ms for the echo. Don't call from ISRs or high-frequency loops if you need precise timing elsewhere.

**Minimum range is 2cm.** Objects closer than 2cm (20mm) cause the driver to return `error`. The sensor physically cannot measure closer — the outgoing pulse hasn't finished by the time the echo starts.

**Angled surfaces and soft materials absorb ultrasound.** Foam, fabric, and angled walls return weak or no echoes. Works best on flat, hard surfaces perpendicular to the beam.

**Multiple HC-SR04 sensors can interfere.** If two sensors fire at the same time, one can hear the other's echo. Stagger reads with at least 50ms between sensors, or use different brands/frequencies.

**Electrical noise on TRIG.** If TRIG has glitches, the sensor fires randomly. Keep the TRIG wire short and away from high-current lines. Add a 100Ω series resistor on TRIG if you see noise.

**begin() does a test measurement.** If it returns `false`, the first test read returned out-of-range (0 or >5000mm). This usually means the echo pin isn't connected or the sensor has no power.

**JSN-SR04T minimum range is 20cm.** The waterproof version uses a longer transducer housing. Objects closer than 20cm return no echo. Account for this in your tank level calculations.
