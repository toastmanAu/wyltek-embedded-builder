# SGP30 — Air Quality Sensor (eCO2 + TVOC)
**Driver:** `WySGP30.h`

---

## What it does
Measures air quality as two numbers:
- **eCO2** (equivalent CO2) in ppm — 400 to 60,000
- **TVOC** (total volatile organic compounds) in ppb — 0 to 60,000

These are *estimated* values based on MOX (metal oxide) sensing, not the same as a true CO2 sensor. Fresh outdoor air is about 400ppm eCO2. A stuffy meeting room might be 1500ppm. VOCs come from paints, solvents, cleaning products, and off-gassing materials.

## What it's not
The SGP30 measures **equivalent** CO2, not actual CO2. It detects hydrogen and ethanol primarily, then estimates CO2 and TVOC from that. For accurate CO2 measurement you need a NDIR sensor (like the SCD30 or MH-Z19). The SGP30 is good for general air quality monitoring and detecting "stale air" or chemical contamination.

## Wiring

| SGP30 Pin | ESP32 |
|-----------|-------|
| VCC | 3.3V |
| GND | GND |
| SDA | GPIO (any I2C SDA) |
| SCL | GPIO (any I2C SCL) |

**I2C address is fixed at `0x58`.** No address select pin.

## Specs

| Parameter | Value |
|-----------|-------|
| eCO2 range | 400–60,000 ppm |
| TVOC range | 0–60,000 ppb |
| Measurement time | 12ms |
| Warm-up time | 15 seconds |
| Interface | I2C, 400kHz |
| I2C address | `0x58` (fixed) |
| Supply | 1.8–3.3V |

## Code Example

```cpp
#include <WySensors.h>

WySensors sensors;

void setup() {
    sensors.addI2C<WySGP30>("air", 21, 22, 0x58);
    sensors.begin();
    Serial.println("Warming up SGP30...");
}

void loop() {
    WySensorData d = sensors.read("air");
    if (d.ok) {
        Serial.printf("eCO2: %.0f ppm  TVOC: %.0f ppb\n",
            d.co2, d.raw);
    } else {
        Serial.printf("Status: %s\n", d.error);  // "warming up"
    }
    delay(1000);  // SGP30 algorithm needs 1-second read intervals
}
```

## WySensorData fields

| Field | Content |
|-------|---------|
| `co2` | eCO2 in ppm |
| `raw` | TVOC in ppb |
| `ok` | `true` after 15-second warm-up completes |
| `error` | `"warming up"` during first 15 seconds, `"CRC fail"` on bad read |

## Warm-up behaviour

For the first 15 seconds after `begin()`, the sensor outputs `eCO2=400` and `TVOC=0`. This is normal — the algorithm is initialising. `d.ok` will be `false` and `d.error` will be `"warming up"` during this period.

## Saving baseline to survive reboots

The SGP30 algorithm improves over hours of continuous operation. To avoid starting fresh every boot, save and restore the baseline registers:

```cpp
#include <Preferences.h>
Preferences prefs;
WySGP30* sgp;

void saveBaseline() {
    auto bl = sgp->getBaseline();
    if (bl.valid) {
        prefs.begin("sgp30", false);
        prefs.putUInt("eco2", bl.eco2);
        prefs.putUInt("tvoc", bl.tvoc);
        prefs.end();
    }
}

void loadBaseline() {
    prefs.begin("sgp30", true);
    uint16_t eco2 = prefs.getUInt("eco2", 0);
    uint16_t tvoc = prefs.getUInt("tvoc", 0);
    prefs.end();
    if (eco2 > 0 && tvoc > 0) {
        sgp->setBaseline(eco2, tvoc);
        Serial.println("SGP30 baseline restored");
    }
}
```

When `setBaseline()` is called with a saved baseline, `d.ok` becomes `true` immediately — no warm-up wait.

## Humidity compensation

If you have a BME280 or SHT31 nearby, feed the absolute humidity to the SGP30 for better accuracy:

```cpp
// Absolute humidity in g/m³
// Rough formula: AH ≈ 216.7 × (RH/100) × 6.112 × exp(17.67T/(T+243.5)) / (T+273.15)
float absHum = 11.2;  // typical indoor value
sgp->setHumidity(absHum);
```

## Gotchas

**CRC on every word.** The SGP30 sends CRC-8 bytes with every 2-byte word. The driver checks all CRCs — a `"CRC fail"` means electrical noise or an I2C timing issue, not sensor malfunction.

**Must read every second.** The SGP30's algorithm assumes you call `read()` approximately every second. If you read less frequently, the internal algorithm degrades. If you need infrequent readings, call `read()` on a 1-second timer and discard results you don't need.

**eCO2=400 and TVOC=0 during warm-up.** This is not an error — the sensor reports baseline values during initialisation. The driver marks these as `ok=false` so you can distinguish warm-up from real readings.

**Not a replacement for a CO2 sensor.** The SGP30 doesn't do NDIR (non-dispersive infrared) CO2 detection. The eCO2 value is an estimate from hydrogen/ethanol detection. For real CO2 monitoring (e.g. COVID ventilation compliance), use a dedicated CO2 sensor.

**Serial number verification.** On `begin()` the driver reads the 9-byte serial number to verify the sensor is present. If `begin()` returns `false`, the serial read returned no data.
