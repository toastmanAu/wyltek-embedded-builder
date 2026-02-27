# VL53L0X — Laser Time-of-Flight Distance Sensor
**Driver:** `WyVL53L0X.h`

---

## What it does
Measures distance using an invisible 940nm laser and timing how long the light takes to bounce back. Unlike ultrasonic sensors, it works on any surface material and has a tight beam (about 25°). Range is 30mm to 2000mm in typical conditions, up to 8m in ideal (dark, reflective targets) conditions.

## Why it's better than HC-SR04 in some situations
- No minimum range (works at 30mm, HC-SR04 needs 20mm)
- Works on soft materials (fabric, foam) that absorb ultrasound
- Faster — continuous mode updates at ~50Hz
- Narrow beam — measures a specific point, not a wide cone

## Wiring

| VL53L0X Pin | ESP32 |
|------------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO (any I2C SDA) |
| SCL | GPIO (any I2C SCL) |
| XSHUT | Optional — tie to 3.3V if not used |
| GPIO1 | Optional interrupt output |

**XSHUT:** Pulled to 3.3V internally. Pull LOW to put sensor in hardware standby. Useful for power management or for switching between multiple VL53L0X sensors on the same bus.

## Specs

| Parameter | Value |
|-----------|-------|
| Technology | Time-of-Flight (ToF) |
| Range | 30–2000mm (typical), up to 8m ideal |
| Resolution | 1mm |
| Field of view | ~25° |
| Wavelength | 940nm (invisible IR) |
| Interface | I2C, 400kHz |
| I2C address | `0x29` (default, software-changeable) |
| Supply | 3.3V |

## Code Example — Single shot mode

```cpp
#include <WySensors.h>

WySensors sensors;

void setup() {
    sensors.addI2C<WyVL53L0X>("laser", 21, 22, 0x29);
    sensors.begin();
}

void loop() {
    WySensorData d = sensors.read("laser");
    if (d.ok) {
        Serial.printf("Distance: %.0f mm\n", d.distance);
    } else {
        Serial.println("Out of range");
    }
    delay(100);
}
```

## Continuous mode (faster)

```cpp
// Continuous mode: sensor keeps measuring, read() just fetches latest result
sensors.addI2C<WyVL53L0X>("laser", 21, 22, 0x29, VL53L0X_MODE_CONTINUOUS);
```

## Multiple sensors on one bus

The VL53L0X address is software-changeable. Use XSHUT pins to enable one at a time during address assignment:

```cpp
// Wire XSHUT of sensor 1 to GPIO 4, sensor 2 to GPIO 5
// They share SDA/SCL

// During setup, enable one at a time:
pinMode(4, OUTPUT); digitalWrite(4, LOW);  // sensor1 off
pinMode(5, OUTPUT); digitalWrite(5, LOW);  // sensor2 off

// Enable sensor1, assign address 0x30
digitalWrite(4, HIGH); delay(10);
auto* s1 = sensors.addI2C<WyVL53L0X>("front", 21, 22, 0x29);
sensors.begin_one(s1);
s1->setAddress(0x30);

// Enable sensor2 (still at default 0x29)
digitalWrite(5, HIGH); delay(10);
sensors.addI2C<WyVL53L0X>("rear", 21, 22, 0x29);
```

## WySensorData fields

| Field | Content |
|-------|---------|
| `distance` | Distance in mm |
| `ok` | `true` if measurement valid |
| `error` | `"out of range"` if mm == 0 or ≥ 8190 |

## Gotchas

**Complex init sequence.** The VL53L0X has a notoriously fiddly init sequence requiring SPAD calibration and reference calibration. The driver uses a simplified version (based on the essential registers from ST's official API). It works for most use cases but doesn't guarantee the same accuracy as ST's full API. For production use with strict accuracy requirements, use ST's official `VL53L0X_API`.

**Device identity check.** On `begin()` the driver reads registers `0xC0` and `0xC1` expecting `0xEE` and `0xAA`. If it fails, the sensor isn't responding — check wiring and address.

**Out-of-range returns 8190.** When the sensor can't get a valid reading (too far, too dark, too close), it returns 8190mm. The driver converts this to `d.error = "out of range"`.

**Infrared interference.** Strong IR sources (sunlight, halogen lamps, some IR remote controls) can interfere with readings. The sensor has some filtering but works best indoors away from direct sunlight.

**Glossy surfaces.** Highly reflective or transparent surfaces (glass, mirrors, polished metal) can cause erratic readings due to specular reflection. Matte surfaces work best.

**The full ST API is available as a library.** If you need guaranteed accuracy and SPAD calibration, consider the `vl53l0x-arduino` library instead of this bundled driver.
