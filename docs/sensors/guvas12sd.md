# GUVA-S12SD — UV Index Sensor
**Driver:** `WyGUVAS12SD.h`

---

## What it measures
UV radiation in the 240–370nm range (UV-A and UV-B). Outputs a voltage proportional to UV intensity. The driver converts it to:
- **UV Index** (UVI) — the WHO international standard, 0–16+
- **Irradiance** (mW/cm²) — raw radiometric power

The sensor is a SiC photodiode — completely blind to visible light and infrared. It only responds to UV. No interference from bright lamps, sunshine reflections, or heat.

## UV Index scale (WHO)
| UVI | Category | What to do |
|-----|----------|------------|
| 0–2 | Low | No protection needed |
| 3–5 | Moderate | Sunscreen recommended |
| 6–7 | High | Sunscreen + hat + shade |
| 8–10 | Very High | Avoid midday sun |
| 11+ | Extreme | Stay indoors 10am–4pm |

Typical readings in Adelaide: UVI 10–12 on a clear summer day at noon. UVI 0–1 indoors. UVI 0 at night.

## Wiring
```
Module VCC → 3.3V  (or 5V — but read the voltage warning below)
Module GND → GND
Module OUT → ESP32 ADC1 pin (GPIO32–39)
```

### ⚠️ 5V supply → voltage divider needed
At 5V supply, the op-amp output can reach 3–4V at very high UV. That exceeds the ESP32 ADC's 3.3V limit.

Either:
- **Power from 3.3V** — output stays within range, simpler. (Some modules say 3.3–5V, check yours.)
- **Power from 5V + divider**: 100kΩ + 68kΩ → ratio 0.405, then `setDividerRatio(0.405f)`

### ADC1 only
GPIO32, 33, 34, 35, 36, 39 — ADC2 is unusable with WiFi active.

## Basic usage
```cpp
auto* uv = sensors.addGPIO<WyGUVAS12SD>("uv", GPIO34);
sensors.begin();

void loop() {
    WySensorData d = sensors.read("uv");
    if (d.ok) {
        const char* cat[] = {"Low", "Moderate", "High", "Very High", "Extreme"};
        Serial.printf("UV Index: %.1f (%s)  %.3f mW/cm²\n",
            d.raw, cat[(int)d.humidity], d.voltage);
    }
    delay(1000);
}
```

## WySensorData fields
| Field | Content |
|-------|---------|
| `d.raw` | UV Index (0.0 – 16+) |
| `d.voltage` | Irradiance (mW/cm²) |
| `d.light` | Sensor output voltage (V, dark-corrected) |
| `d.rawInt` | Raw ADC value (0–4095) |
| `d.humidity` | Category: 0=Low, 1=Moderate, 2=High, 3=Very High, 4=Extreme |

## Dark calibration — do this once
The op-amp has a small voltage offset even in total darkness — typically 5–50mV depending on the module. Without correcting for it, indoor readings show UVI 0.5–1.0 when the true value is near zero.

```cpp
// Cover the sensor completely with your finger or a lens cap
uv->calibrateDark();   // measures and stores the offset
// Now uncover it — all readings subtract this offset automatically
```

Call once at startup. If you store the value in NVS, you can skip calibration on subsequent boots.

## Sensitivity tuning
Unit-to-unit variation is ±30% — two sensors from the same batch can differ by 30% at the same UV level. If your readings seem consistently high or low compared to a reference (weather service UVI or a calibrated meter), adjust:

```cpp
uv->setSensitivity(0.13f);   // increase → lowers output NTU; decrease → raises it
// Default: 0.1 V per mW/cm²
```

For serious accuracy, calibrate against a known UV source or compare to your local Bureau of Meteorology UV Index at solar noon on a clear day.

## Mounting — orientation matters
The sensor should face the sky, horizontal. UV Index is defined as the UV dose on a **horizontal surface** — it's an exposure metric, not a directional one.

- Pointing straight up → correct measurement
- Tilted → reads lower (sees less sky)
- Pointing at the sun → reads higher (not what UVI means)
- In shade → reads significantly lower than the open-sky UVI (as expected — that's the point of shade)

## What affects the reading
**Clouds:** Thin cirrus clouds can actually increase UV by scattering — the Broken Cloud Effect can push readings 10–20% above clear-sky values. Dense cloud blocks most UV (typically 70–90% reduction).

**Altitude:** UV increases ~4% per 300m elevation. At 1500m it's ~20% higher than sea level.

**Reflection:** Snow, sand, and water reflect UV back upward — can increase skin dose significantly even in shade.

**Glass:** Standard glass blocks almost all UV. Low-E glass and UV-filtering glass block even more. The sensor behind a window reads near-zero even on a sunny day.

**Ozone:** The ozone layer blocks UV-B specifically. Ozone hole events (southern latitudes, spring) can significantly spike UV-B.

## Pairing with a weather station
```cpp
auto* uv   = sensors.addGPIO<WyGUVAS12SD>("uv",   GPIO34);
auto* lux  = sensors.addI2C<WyVEML7700>("lux",    SDA, SCL, 0x10);
auto* temp = sensors.addI2C<WyBME280>("weather",  SDA, SCL, 0x76);
```

UV + visible lux + temp/humidity/pressure = solid outdoor weather station. The VEML7700 tracks total visible light; the GUVA-S12SD tracks the UV component specifically.

## Gotchas
**Near-zero indoor readings are normal.** Window glass blocks UV-A/B. Fluorescent lights emit tiny amounts of UV — you may see UVI 0.1–0.2 under strong fluorescent lighting. LED lights emit virtually no UV. This is correct behaviour.

**ADC noise at low UV levels.** At UVI 0–1, the sensor output is in the 0–100mV range. ESP32 ADC noise floor is ~5–10mV — that's 5–10% noise at these levels. The 16-sample average helps. For very precise low-UV measurements, add a 100nF cap between OUT and GND to filter high-frequency noise.

**Module-to-module variation is real.** Two sensors side by side in sunlight can read UVI 8 and UVI 11. Always dark-calibrate, and adjust sensitivity against a known reference if absolute accuracy matters.
