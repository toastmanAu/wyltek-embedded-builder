# DHT22 / DHT11 — Temperature & Humidity (Single-Wire)
**Driver:** `WyDHT22.h`

---

## What it does
Temperature and humidity sensor using a single GPIO pin and a proprietary one-wire protocol. Common and cheap, but slower and trickier than I2C alternatives. The DHT11 (blue) reads 0–50°C and 20–90% RH with 1-degree resolution. The DHT22/AM2302 (white) reads -40°C to +80°C and 0–100% RH with 0.1-degree resolution.

## When to use it
When you have a spare GPIO and no I2C bus available, or you already have a DHT22 on hand. For new designs, prefer SHT31 or AHT20.

## Wiring

| DHT22 Pin | ESP32 |
|-----------|-------|
| Pin 1 (VCC) | 3.3V or 5V |
| Pin 2 (DATA) | GPIO + 10kΩ to 3.3V |
| Pin 3 | Not connected |
| Pin 4 (GND) | GND |

**The pull-up resistor is required.** 10kΩ is specified. Some breakout boards include it — check for a small SMD resistor near the DATA pin before adding another.

For long cable runs, reduce to 4.7kΩ.

## Specs

| Parameter | DHT11 | DHT22 / AM2302 |
|-----------|-------|----------------|
| Temperature range | 0–50°C | -40°C to +80°C |
| Temperature accuracy | ±2°C | ±0.5°C |
| Humidity range | 20–90% RH | 0–100% RH |
| Humidity accuracy | ±5% RH | ±2–5% RH |
| Sampling interval | ≥1s | ≥2s |
| Resolution | 1°C, 1% | 0.1°C, 0.1% |

## Code Example

```cpp
#include <WySensors.h>

WySensors sensors;

void setup() {
    // DHT22 on GPIO 4
    sensors.addGPIO<WyDHT22>("temp_hum", 4);
    sensors.begin();  // applies 2s power-on delay
}

void loop() {
    WySensorData d = sensors.read("temp_hum");
    if (d.ok) {
        Serial.printf("Temp: %.1f°C  Humidity: %.1f%%\n",
            d.temperature, d.humidity);
    } else {
        Serial.printf("Error: %s\n", d.error);
    }
    delay(3000);  // DHT22 needs 2s between reads
}
```

## Using DHT11

```cpp
// Pass model=11 for DHT11
sensors.addGPIO<WyDHT22>("dht11", 4);
// Or use the alias:
sensors.addGPIO<WyDHT11>("dht11", 4);
```

The alias `WyDHT11` passes `model=11`, which changes:
- Start pulse: 18ms LOW (vs 1ms for DHT22)
- Data parsing: integer-only bytes (vs 0.1-resolution 16-bit values)

## WySensorData fields

| Field | Content |
|-------|---------|
| `temperature` | Temperature in °C |
| `humidity` | Relative humidity 0–100% |
| `ok` | `true` if read succeeded |
| `error` | Failure reason (see below) |

## Error messages

| Error | Likely cause |
|-------|-------------|
| `"no response (low)"` | Sensor not pulling bus LOW after start — check wiring |
| `"no response (high)"` | Sensor not releasing bus HIGH — check power, pull-up |
| `"no response (sync)"` | Sync pulse missing — marginal timing |
| `"read timeout"` | Bit read timed out — interrupt interference |
| `"bit timeout"` | Individual bit too long — interference |
| `"checksum fail"` | Data corrupted — usually timing/interference |

## Gotchas

**2-second interval is hard-coded in the hardware.** The DHT22 refuses to take a new measurement within 2 seconds of the last one. Call `read()` faster and you'll get a checksum error (corrupted data) or stale data. The driver doesn't enforce the minimum interval — that's your job. Use `delay(2500)` between reads to be safe.

**The `begin()` call delays 2000ms.** This is the DHT22's mandatory power-on settle time. Don't be surprised when your `setup()` takes 2 extra seconds.

**Timing is everything.** The DHT22 uses bit-banging with microsecond-level timing. The driver uses `noInterrupts()` during reads to prevent corruption. If you have frequent interrupts from WiFi, Bluetooth, or your own code, you'll see checksum failures. Solutions:
- Use I2C sensors (SHT31, AHT20) instead
- Reduce interrupt frequency during reads
- Retry on failure (read 3 times, take first success)

**Cable length matters.** Keep the cable under 20cm for reliable operation. Longer cables need a stronger pull-up (4.7kΩ) and slower speeds are more tolerant.

**GPIO 36 and 39 are input-only on ESP32.** Don't use them for DHT22 — the driver switches the pin between INPUT and OUTPUT. Use GPIO 1-39 that support output.

**Negative temperatures on DHT22.** The MSB of the temperature bytes signals negative. The driver handles this correctly: `data[2] & 0x80` is the sign bit.
