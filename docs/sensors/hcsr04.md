# HC-SR04 / JSN-SR04T — Ultrasonic Distance Sensor
**Driver:** `WyHCSR04.h` — also covers JSN-SR04T, AJ-SR04M, HC-SR04P

---

## What it does
Fires a burst of 40kHz ultrasonic sound, listens for the echo, measures the time. Distance = time × speed-of-sound / 2. Simple, cheap, works from 2cm to 4 metres.

## Variants

| Sensor | Range | Min | Waterproof | Supply | Notes |
|--------|-------|-----|------------|--------|-------|
| HC-SR04 | 2–400cm | 2cm | No | **5V** | Most common, needs voltage divider on ECHO |
| HC-SR04P | 2–400cm | 2cm | No | **3.3V** | Direct ESP32 connection, no divider |
| JSN-SR04T | 20–600cm | 20cm | Yes | 5V | External probe, tanks/outdoor/liquid |
| AJ-SR04M | 20–700cm | 20cm | Yes | 5V | Similar to JSN, longer range |

**Buy the HC-SR04P if you can** — it's 3.3V native, no voltage divider needed, same price.

## ⚠️ Voltage — the thing that kills ESP32s

Classic HC-SR04 ECHO output is **5V**. Connecting it directly to an ESP32 GPIO will damage the pin over time and eventually destroy the ESP32.

**Option 1 — HC-SR04P** (recommended): 3.3V version, connect directly. Done.

**Option 2 — voltage divider on ECHO:**
```
HC-SR04 ECHO → 1kΩ → ESP32 GPIO
                    → 2kΩ → GND
```
This gives 5V × (2/3) = 3.33V max. Acceptable.

**TRIG is fine either way** — the HC-SR04 input threshold is ~2V, and ESP32 outputs 3.3V. No divider needed on TRIG.

## Wiring (HC-SR04P — simplest)
```
VCC  → 3.3V
GND  → GND
TRIG → ESP32 any GPIO (e.g. GPIO5)
ECHO → ESP32 any GPIO (e.g. GPIO18)
```

## Code
```cpp
// pin = TRIG, pin2 = ECHO
auto* us = sensors.addGPIO<WyHCSR04>("dist", TRIG_PIN, ECHO_PIN);
us->setSamples(3);           // median of 3 readings (default)
us->setTemperature(25.0f);   // optional — improves accuracy slightly
sensors.begin();

WySensorData d = sensors.read("dist");
if (d.ok) {
    Serial.printf("Distance: %.1f cm\n", d.raw);
} else {
    Serial.printf("Out of range: %s\n", d.error);
}
```

## Median filtering — why you need it
Ultrasonic sensors produce occasional spikes — a sound pulse from a fan, a passing object, a nearby surface reflecting at an angle. Without filtering, those spikes cause erratic readings.

```cpp
us->setSamples(1);   // raw — fastest, noisiest
us->setSamples(3);   // median of 3 — good balance (default)
us->setSamples(5);   // median of 5 — smoothest, ~300ms per call
```

The median specifically kills outliers better than averaging. One bad reading out of 5 doesn't affect the median at all.

## WySensorData fields
| Field | Content |
|-------|---------|
| `d.raw` | Distance in cm |
| `d.distance` | Distance in mm |
| `d.rawInt` | Valid sample count (when using median) |
| `d.ok` | true when reading within valid range |
| `d.error` | "out of range" or "all samples failed" |

## Temperature compensation
Speed of sound: **331.4 + 0.606 × T°C** m/s.  
At 0°C: 331 m/s. At 40°C: 355 m/s. That's ~7% difference.

For a 1-metre distance, uncompensated error: ~3.5cm between 0°C and 40°C. For most indoor use this doesn't matter. For precise level sensing or outdoor use across temperature extremes, it does:

```cpp
WySensorData weather = sensors.read("bme280");
us->setTemperature(weather.temperature);  // auto-compensate
```

## The minimum range blind spot
The sensor can't hear its own echo for a few milliseconds after firing — the 40kHz burst is still ringing down when a very close echo arrives.

- **HC-SR04**: can't detect objects closer than ~2cm
- **JSN-SR04T**: can't detect objects closer than ~20cm (longer transducer)
- At minimum range: readings become erratic or show maximum range value

The driver returns `d.ok = false` for readings outside the min/max range.

## JSN-SR04T — waterproof variant
```cpp
auto* tank = sensors.addGPIO<WyJSNSR04T>("level", TRIG_PIN, ECHO_PIN);
sensors.begin();
```

`WyJSNSR04T` is a subclass that pre-sets min=20cm, max=600cm. Same driver, same API.

**Common uses:**
- Water tank level monitoring (mount at top, measure distance to water surface)
- Underground sump/pit level
- Outdoor obstacle detection
- Bin/hopper fill level

**Tank level calculation:**
```cpp
float tankDepthCm  = 100.0f;  // total tank height
float sensorOffset = 3.0f;    // cm from sensor to full-level mark
float distCm = sensors.read("level").raw;
float fillPct = 100.0f * (1.0f - (distCm - sensorOffset) / tankDepthCm);
fillPct = constrain(fillPct, 0.0f, 100.0f);
Serial.printf("Tank: %.0f%%\n", fillPct);
```

## Gotchas

**60ms minimum between readings.** The sensor needs time for the ultrasonic burst to completely die out before firing again. Fire too fast and it hears its own echo from the previous pulse. The driver enforces a 60ms minimum. With 3-sample median at 60ms per sample: ~180ms per `read()` call — plan your loop timing accordingly.

**Sound absorbing surfaces.** Foam, fabric, carpet, and soft furnishings absorb ultrasound and return weak echoes or no echo at all. The sensor works best on hard, flat, perpendicular surfaces.

**Angled surfaces.** If the target surface is tilted more than ~15° from perpendicular to the sensor axis, the echo reflects away and the sensor reads nothing (timeout). This is why it's unreliable for measuring water with surface ripples in a tilted container.

**Narrow objects.** The sensor beam is about 15° wide. Very narrow objects (a rod, a wire) may not return enough echo to register. Works best on surfaces wider than ~10cm at the target distance.

**Interference between multiple sensors.** If you run two HC-SR04s simultaneously, they can hear each other's pulses. Stagger the readings so only one fires at a time. The driver already enforces the inter-measurement gap per sensor.
