# Turbidity Sensor
**Driver:** `WyTurbidity.h` — SEN0189, blue Arduino turbidity module, and compatible analog sensors

---

## What it measures
How cloudy the liquid is. An IR LED shines through the water; a phototransistor on the other side measures how much light arrives. Clear water lets most light through. Murky water scatters the light — less reaches the detector.

Output: **NTU** (Nephelometric Turbidity Units) — the international standard for water clarity.

| NTU | What it looks like |
|-----|--------------------|
| 0 | Perfectly clear — distilled water |
| 1 | EU drinking water standard |
| 4 | US EPA drinking water limit |
| 10 | Slightly hazy — mildly dirty aquarium |
| 50 | Noticeably murky |
| 100 | Visibly turbid — river after light rain |
| 500 | Muddy-looking |
| 1000+ | Very turbid — stormwater runoff |
| 3000 | Nearly opaque |

## ⚠️ Voltage divider required (5V sensor → 3.3V ESP32)

Most of these modules are designed for 5V Arduino. At 5V supply, the analog output can reach ~4.5V. The ESP32 ADC input is 3.3V max — **connecting 5V signal directly will damage the ESP32**.

**Required voltage divider:**
```
Sensor AO → 100kΩ → [junction] → ESP32 ADC1 pin
                   → 100kΩ → GND
```
The junction voltage = sensor voltage × 0.5 (max ~2.25V — safely within 3.3V ADC range).

Then tell the driver about the divider:
```cpp
turb->setDividerRatio(0.5f);   // 100kΩ + 100kΩ = ÷2
```

**Alternative:** If your module has a 3.3V compatible version or you power it from 3.3V, skip the divider and call `setSupplyVoltage(3.3f)`.

## Wiring (5V sensor)
```
Sensor VCC → 5V
Sensor GND → GND (common with ESP32)
Sensor AO  → 100kΩ → ESP32 GPIO34 (ADC1)
                    → 100kΩ → GND
Sensor DO  → ESP32 GPIO (optional — digital threshold)
```

### ADC1 only
GPIO32–39 on ESP32. ADC2 shares the WiFi radio and reads garbage when WiFi is active.

## Code
```cpp
auto* turb = sensors.addGPIO<WyTurbidity>("turbidity", GPIO34);
turb->setDividerRatio(0.5f);    // 100kΩ+100kΩ voltage divider
turb->setSupplyVoltage(5.0f);   // sensor powered from 5V
sensors.begin();

void loop() {
    WySensorData d = sensors.read("turbidity");
    if (d.ok) {
        const char* quality[] = {"Clear", "Good", "Fair", "Poor"};
        Serial.printf("%.1f NTU — %s (%.3fV)\n",
            d.raw, quality[(int)d.humidity], d.voltage);
    }
    delay(1000);
}
```

## WySensorData fields
| Field | Content |
|-------|---------|
| `d.raw` | Turbidity in NTU |
| `d.voltage` | Sensor output voltage (corrected for divider) |
| `d.rawInt` | Raw ADC value (0–4095) |
| `d.humidity` | Quality category: 0=Clear, 1=Good, 2=Fair, 3=Poor |
| `d.ok` | true when reading valid |

## Temperature compensation
The photodiode sensitivity drifts with temperature — about 0.5% per °C. For drinking water monitoring or anything where accuracy matters, pair with a DS18B20 and compensate:

```cpp
WySensorData temp = sensors.read("watertemp");
turb->setTemperature(temp.temperature);   // compensates for temp drift
WySensorData t = sensors.read("turbidity");
```

For fish tanks and casual monitoring, skip it — 5% drift over a 10°C range usually doesn't matter.

## Calibration
The built-in NTU curve is from the SEN0189 datasheet plus community measurements. It's approximate. For better accuracy:

**Two-point calibration:**
1. Distilled water (0 NTU) — note the voltage
2. A known turbid standard — note the voltage

```cpp
// Custom calibration table (descending voltage = increasing turbidity)
static const float myV[]   = {4.0f, 3.2f, 2.1f, 1.2f};
static const float myNTU[] = {0.0f, 150.f, 600.f, 2000.f};
turb->setCalibration(myV, myNTU, 4);
```

For anything claiming to measure drinking water standards (< 4 NTU), buy proper Formazin calibration standards or StablCal — they're the only way to accurately calibrate to absolute NTU.

## The four practical gotchas

**Bubbles give false readings.** Air bubbles scatter light like particles do — the sensor can't tell the difference. Let your sample settle for 30+ seconds before reading. De-gas agitated water first.

**Ambient light overwhelms the IR.** Direct sunlight, or even a bright lamp shining on the detector, saturates it and gives artificially low NTU readings. Mount in a housing or cover. The driver averages 16 samples — this helps with fluorescent flicker but not direct sunlight.

**Probe fouling creeps up slowly.** Algae, biofilm, and mineral deposits gradually coat the optical window. The sensor drifts high over weeks to months. Clean periodically with a soft cloth. Dilute white vinegar removes mineral scale. For permanent installations, schedule monthly cleaning.

**The NTU curve isn't linear.** It's a steep curve near clear water (small voltage change = large NTU change) and flatter at high turbidity. This means the sensor has good resolution at low turbidity but poor resolution at high turbidity — which is actually fine for most use cases (you care more about small changes near "clean" than big changes in "obviously muddy").

## Pairing with pH and temperature
The classic water quality triad: turbidity + pH + temperature. All three together give a solid picture of water quality. In a fish tank or water treatment monitoring setup:

```cpp
auto* turb = sensors.addGPIO<WyTurbidity>("turbidity", GPIO34);
auto* ph   = sensors.addGPIO<WyPH>("ph", GPIO35);
auto* temp = sensors.addGPIO<WyDS18B20>("temp", GPIO4);

// Temperature feeds both pH and turbidity compensation
WySensorData t = sensors.read("temp");
ph->setTemperature(t.temperature);
turb->setTemperature(t.temperature);
```

## vs more expensive sensors
This ~$3 optical module gives you 0–3000 NTU range with roughly ±10–15% accuracy after calibration. Commercial turbidimeters ($100–$1000) use nephelometric geometry (90° scatter measurement) which is more accurate to the NTU standard and less affected by colour.

For drinking water compliance testing, use certified equipment. For fish tank automation, irrigation water quality, swimming pool monitoring, or process control where trend matters more than absolute accuracy — this sensor is perfectly adequate.
