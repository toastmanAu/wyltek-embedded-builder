# pH Sensor (BNC Analog Module)
**Driver:** `WyPH.h` — Compatible with PH4502C, SEN0161, DFRobot pH meter v1/v2, and most AliExpress BNC electrode modules.

---

## What it does
Measures the acidity or alkalinity of a liquid on the pH scale (0–14). pH 7 is neutral, below 7 is acidic, above 7 is alkaline. The glass electrode probe generates a tiny voltage that shifts with pH — the module amplifies this to an ADC-readable range.

## How it works
The glass electrode generates a voltage relative to a reference electrode inside the probe. The op-amp on the module amplifies this to roughly:
- ~2.5V at pH 7 (neutral)
- ~3.4V at pH 4 (acidic)
- ~1.7V at pH 10 (alkaline)

The relationship follows the **Nernst equation**: approximately 59.16mV per pH unit at 25°C.

## Wiring
| Module pin | ESP32 |
|------------|-------|
| VCC | 5V |
| GND | GND |
| AOUT / Po | Any ADC-capable GPIO |
| BNC socket | pH probe |

## ⚠️ The 3.3V problem
The module's op-amp runs on 5V. At pH 4 the output can reach **~3.4V** — which exceeds the ESP32's 3.3V ADC limit. Options:
- **Voltage divider** on AOUT: 3.3kΩ + 6.8kΩ gives 0.49× scaling (safe for full range)
- **Accept clipping** below pH ~4.5 (fine for aquarium/hydroponics — you rarely go below 5.5)
- Use a module with a 3.3V op-amp supply (rarer, but they exist)

## Calibration — do this properly
The Nernst slope varies between individual probes and modules by ±10–15%. Without calibration, readings can be off by 0.5–1.0 pH.

### Two-point calibration (recommended)
1. Prepare pH 7.0 and pH 4.0 buffer solutions (usually included as sachets)
2. Rinse probe, place in pH 7 buffer, wait 1 minute
3. Call `ph->calibrate(7.0)` (or whatever is printed on the sachet — often 6.86)
4. Rinse probe thoroughly, place in pH 4 buffer, wait 1 minute
5. Call `ph->calibrate(4.0)`

The driver now has both slope and offset. Readings should be accurate to ±0.1 pH.

### Single-point calibration
Only offset is corrected, slope stays at default 59.16mV/pH. Accurate near pH 7 but drifts at extremes. Better than nothing.

```cpp
ph->calibrate(7.0);  // probe in pH 7 buffer
```

## Temperature compensation — more important than you think
pH shifts ~0.002 pH per °C from the reference temperature (25°C). At 30°C vs 25°C that's 0.01 pH — not huge. But at 15°C vs 25°C it's 0.02 × 10 = **0.2 pH shift** — significant.

**Always use `readTemp()` with a water temperature reading:**
```cpp
float waterTemp = ds18b20.readCelsius();
WySensorData d = ph->readTemp(waterTemp);
```

Pair a **DS18B20** in the same water — it's waterproof, cheap, and gives you the exact temperature for compensation.

## Noise reduction
ESP32 ADC is notoriously noisy — can swing ±50mV randomly. The driver averages 32 samples by default (`WY_PH_SAMPLES`). If still noisy:
- Add a **100nF capacitor** between AOUT and GND right at the ESP32 GPIO pin
- Keep AOUT wire short — it's high impedance and picks up interference
- Keep away from switching power supplies and PWM signals
- Increase `WY_PH_SAMPLES` to 64 or 128 (slower but smoother)

## Fish tank / aquarium notes
- Typical healthy freshwater range: pH 6.5–7.5
- Marine/reef: pH 8.1–8.4
- You'll rarely approach pH 4 or 10, so the 3.3V clipping issue usually doesn't matter
- Recalibrate monthly — glass electrodes drift over time
- Rinse probe in distilled water before storing; never let it dry out completely
- Storage solution (KCl) keeps the electrode hydrated between uses
- Probe tip is fragile glass — don't knock it against tank walls

## Probe care
- Soak new probes in pH 4 buffer or KCl solution for 1–2 hours before first use
- Rinse with distilled water between measurements
- Store in KCl solution or pH 4 buffer (NOT distilled water — that leaches the reference electrolyte)
- Replace probe every 1–2 years — glass electrodes age and drift

## Known issues / battle-tested fixes
*(This section will be updated from Phill's fish tank implementation)*
- [ ] TODO: add real-world noise fixes from live aquarium deployment
- [ ] TODO: drift compensation over multi-day logging
- [ ] TODO: alert thresholds for pH out-of-range

## Example
```cpp
auto* ph = sensors.addGPIO<WyPH>("water_ph", 34);
sensors.begin();

// Calibrate (once, in buffer solutions)
// ph->calibrate(6.86);  // pH 6.86 buffer
// ph->calibrate(4.00);  // pH 4.00 buffer

void loop() {
    float tempC = 24.5;  // from DS18B20
    WySensorData d = ph->readTemp(tempC);
    if (d.ok) {
        Serial.printf("pH: %.2f (%.0f mV, %.1f°C)\n",
            d.raw, d.voltage, d.temperature);
    }
}
```
