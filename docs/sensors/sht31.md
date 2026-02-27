# SHT31 / SHT35 — Temperature & Humidity
**Driver:** `WySHT31.h`

---

## What it does
Measures temperature and relative humidity over I2C. The SHT31 is Sensirion's workhorse — accurate, reliable, and resistant to contamination. The SHT35 is the higher-accuracy version. Both use the same driver.

## Why it's better than DHT22
- **Faster reads** — 20ms measurement vs 2s interval
- **More accurate** — ±2% RH vs ±5%
- **I2C** — no bit-banging timing issues
- **CRC checking** — detects corrupted reads

## Wiring

| SHT31 Pin | ESP32 |
|-----------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO (any I2C SDA) |
| SCL | GPIO (any I2C SCL) |
| ADDR | GND for 0x44, 3.3V for 0x45 |
| nRESET | 3.3V (or floating — internal pull-up) |
| Alert | Not connected (optional interrupt) |

## I2C Addresses

| ADDR Pin | Address |
|---------|---------|
| GND (default) | `0x44` |
| 3.3V | `0x45` |

## Specs

| Parameter | SHT31 | SHT35 |
|-----------|-------|-------|
| Temperature range | -40°C to +125°C | same |
| Temperature accuracy | ±0.3°C | ±0.1°C |
| Humidity range | 0–100% RH | same |
| Humidity accuracy | ±2% RH | ±1.5% RH |
| Measurement time | ~15ms (high rep) | same |
| Interface | I2C, 400kHz | same |

## Code Example

```cpp
#include <WySensors.h>

WySensors sensors;

void setup() {
    sensors.addI2C<WySHT31>("climate", 21, 22, 0x44);
    sensors.begin();
}

void loop() {
    WySensorData d = sensors.read("climate");
    if (d.ok) {
        Serial.printf("Temp: %.2f°C  Humidity: %.1f%%\n",
            d.temperature, d.humidity);
    }
    delay(1000);
}
```

## WySensorData fields

| Field | Content |
|-------|---------|
| `temperature` | Temperature in °C |
| `humidity` | Relative humidity 0–100% |
| `ok` | `true` if read succeeded |
| `error` | `"no data"` if I2C read failed |

## Measurement commands

The driver uses high-repeatability mode (`0x2400`). This gives the best accuracy at 15ms measurement time. The command table is in the driver if you want to customise:

| Mode | Command | Time | Use case |
|------|---------|------|---------|
| High rep | `0x2400` | 15ms | Default — best accuracy |
| Medium rep | `0x240B` | 6ms | Faster, slightly less accurate |
| Low rep | `0x2416` | 4ms | Fastest, lowest power |

## Gotchas

**The sensor reads twice on `begin()`.** The driver sends a soft reset, then reads the status register to verify comms. If `begin()` returns `false`, the sensor isn't responding — check wiring, address pin, and pull-up resistors.

**CRC bytes are there but the driver skips them.** The comment in the source says "CRC check (optional — skip for speed)". If you're seeing occasional garbage readings in a noisy environment, you may want to add CRC checking. The poly is CRC-8 `0x31`, init `0xFF`.

**High-repeatability mode blocks for 20ms.** Every `read()` call inserts `delay(20)`. Don't call it from a high-priority ISR. For interrupt-driven systems, trigger the measurement in advance and read the result after the delay.

**Humidity reads >100% or <0%?** Condensation or sensor contamination. The SHT31 has a PTFE membrane filter on some modules that helps. If the sensor is wet from condensation, let it dry out at room temperature.

**Two SHT31s on one bus.** Totally fine — put one at 0x44 and one at 0x45:
```cpp
sensors.addI2C<WySHT31>("inside",  21, 22, 0x44);
sensors.addI2C<WySHT31>("outside", 21, 22, 0x45);
```
