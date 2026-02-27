# AHT20 / AHT21 / AHT10 — Temperature & Humidity
**Driver:** `WyAHT20.h`

---

## What it does
Compact I2C temperature and humidity sensor. The AHT20 is a popular, cheap alternative to the DHT22. No pull-up resistor needed on the data line — it's pure I2C. The AHT21 and AM2301B are electrically identical. The AHT10 uses a slightly different init command.

## Variants covered by this driver

| Part | Same as | Notes |
|------|---------|-------|
| AHT20 | — | Main version, init cmd `0xBE` |
| AHT21 | AHT20 | Identical |
| AM2301B | AHT20 | Identical |
| AHT10 | — | Init cmd `0xE1`, pass `isAHT10=true` |

The driver provides `WyAHT10` as a subclass:
```cpp
sensors.addI2C<WyAHT10>("humidity", sda, scl, 0x38);
```

## Wiring

| AHT20 Pin | ESP32 |
|-----------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO (any I2C SDA) |
| SCL | GPIO (any I2C SCL) |

**I2C address is fixed at `0x38`.** There is no address-select pin. You can only have one AHT20 per I2C bus.

## Specs

| Parameter | Value |
|-----------|-------|
| Temperature range | -40°C to +85°C |
| Temperature accuracy | ±0.3°C |
| Humidity range | 0–100% RH |
| Humidity accuracy | ±2% RH |
| Measurement time | ~80ms |
| Interface | I2C, 400kHz |
| I2C address | `0x38` (fixed) |
| Supply voltage | 2.0–5.5V |

## Code Example

```cpp
#include <WySensors.h>

WySensors sensors;

void setup() {
    sensors.addI2C<WyAHT20>("env", 21, 22, 0x38);
    sensors.begin();
}

void loop() {
    WySensorData d = sensors.read("env");
    if (d.ok) {
        Serial.printf("Temp: %.1f°C  Humidity: %.1f%%\n",
            d.temperature, d.humidity);
    }
    delay(2000);
}
```

## WySensorData fields

| Field | Content |
|-------|---------|
| `temperature` | Temperature in °C |
| `humidity` | Relative humidity 0–100% |
| `ok` | `true` if read succeeded and calibrated |
| `error` | `"timeout"` or `"no data"` on failure |

## How the calibration works

On power-on the driver checks the calibration bit in the status register. If the sensor reports "not calibrated" (bit 3 of status byte clear), it sends the init command (`0xBE 0x08 0x00` for AHT20) and waits 10ms. If calibration still fails after that, `begin()` returns `false`. A healthy sensor always calibrates fine — `false` from `begin()` means the sensor isn't responding.

## Gotchas

**Power-on delay.** The AHT20 needs **40ms** after power-on before it will respond. The driver enforces this in `begin()`. If you call `begin()` immediately after power-on in your code, you may see init failures. The driver delays, but if the whole `setup()` runs very fast, the sensor might not have had power for 40ms yet.

**Only one per I2C bus.** The address is fixed at `0x38`. If you need two humidity sensors, use a second I2C bus (ESP32 has multiple), or mix with a SHT31 (which has two selectable addresses).

**80ms measurement time.** Each `read()` triggers a measurement and waits up to 80ms for the result. Don't call it in a tight loop — reads will take at least 80ms each.

**Busy bit timeout.** If the sensor is stuck busy (hardware fault), the driver times out after 100ms and returns `d.error = "timeout"`.

**The AHT10 uses a different init.** If you have the older AHT10, register it as `WyAHT10` not `WyAHT20` — or pass `true` for `isAHT10`:
```cpp
sensors.addI2C<WyAHT10>("old_sensor", 21, 22, 0x38);
```
