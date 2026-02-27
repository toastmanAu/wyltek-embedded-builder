# Resistive Soil Moisture Sensor
**Driver:** `WySoilMoisture.h` — YL-69, HL-69, and all the blue/green Arduino probe modules

---

## What it is
Two exposed metal probes. Stick them in soil. Wet soil conducts electricity between the probes better than dry soil. More conductivity = less resistance = lower output voltage. Read the voltage → infer moisture.

Simple. Cheap ($0.50–$2 each). Works. But has one problem that kills them if you don't know about it.

## ⚠️ Corrosion — the reason these fail
These sensors pass DC current through the soil **continuously**. DC current through metal electrodes in an electrolyte (wet soil) causes **electrolysis** — the positive probe slowly dissolves. Left powered 24/7, the probes are visibly corroded within weeks and useless within months.

**The fix is dead simple: only power the sensor when taking a reading.**

Wire VCC through a GPIO pin instead of directly to 3.3V:
```cpp
auto* soil = sensors.addGPIO<WySoilMoisture>("soil", AOUT_PIN);
soil->setPowerPin(PWR_PIN);   // GPIO drives HIGH to power, LOW between reads
```

The driver drives PWR_PIN HIGH, waits 80ms to settle, reads, then drives it LOW immediately. Your probes will last years instead of weeks.

## Wiring

**Recommended (power-switched):**
```
Sensor VCC → ESP32 GPIO (PWR_PIN) — e.g. GPIO25
Sensor GND → GND
Sensor AO  → ESP32 ADC1 pin — e.g. GPIO34
Sensor DO  → ESP32 digital GPIO (optional threshold alert)
```

**Basic (always-on, shorter probe life):**
```
Sensor VCC → 3.3V
Sensor GND → GND
Sensor AO  → ESP32 ADC1 pin
```

### ⚠️ ADC1 pins only (with WiFi)
ADC2 is shared with the WiFi radio. When WiFi is active, ADC2 reads garbage.  
**Always use ADC1 pins for analog sensors:**  
GPIO32, GPIO33, GPIO34, GPIO35, GPIO36 (VP), GPIO39 (VN)

## Calibration
Raw ADC values are **inverted** — higher number means drier soil. This trips people up.

| Condition | Raw ADC (approx, 3.3V) |
|-----------|------------------------|
| Dry air (open circuit) | 3500–4095 |
| Dry soil | 2500–3200 |
| Moist soil | 1200–2200 |
| Soaking wet / in water | 400–1200 |

Default calibration baked in: wet=1200, dry=3200. For accurate percentages, calibrate for your specific sensor and soil type:

```cpp
// Step 1: hold probes in dry air, call rawValue()
uint16_t myDryRaw = soil->rawValue();   // e.g. 3350

// Step 2: submerge probe tips in water, call rawValue()
uint16_t myWetRaw = soil->rawValue();   // e.g. 950

// Step 3: apply
soil->setCalibration(myWetRaw, myDryRaw);  // (wet, dry)
```

After calibration, `d.raw` returns 0.0–100.0 percent moisture.

## Code
```cpp
auto* soil = sensors.addGPIO<WySoilMoisture>("soil", GPIO34);
soil->setPowerPin(GPIO25);
soil->setCalibration(950, 3350);
sensors.begin();

void loop() {
    WySensorData d = sensors.read("soil");
    if (d.ok) {
        Serial.printf("Moisture: %.1f%%  (raw: %u)\n", d.raw, (uint32_t)d.rawInt);
        if (d.raw < 20.0f) Serial.println("⚠️ Needs watering!");
    }
    delay(60000);  // read once per minute is plenty
}
```

## The DO (digital output) pin
The module has a potentiometer that sets a threshold. When moisture drops below the threshold, DO goes HIGH. Above threshold, DO goes LOW. Useful for a simple dry-soil alert without reading the analog value.

```cpp
soil->setDOPin(GPIO26);
// d.voltage = 1.0 when threshold triggered (soil above moisture threshold)
// isDry() = true when DO is HIGH (below threshold)
if (soil->isDry()) { /* water the plant */ }
```

Adjust the pot with a small screwdriver while watching the DO LED on the module.

## Multiple sensors
Each sensor needs its own analog pin. Power pins can be shared or individual:

```cpp
auto* bed1 = sensors.addGPIO<WySoilMoisture>("bed1", GPIO34);
auto* bed2 = sensors.addGPIO<WySoilMoisture>("bed2", GPIO35);
auto* bed3 = sensors.addGPIO<WySoilMoisture>("bed3", GPIO32);

// Individual power pins — read one at a time, no cross-talk
bed1->setPowerPin(GPIO25);
bed2->setPowerPin(GPIO26);
bed3->setPowerPin(GPIO27);
```

## Noise reduction
The ADC on ESP32 is noisy. The driver averages 8 samples by default. For cleaner readings in electrically noisy environments:
```cpp
soil->setSamples(16);   // more samples, slower read
```

Also useful: add a 100nF capacitor between AO and GND right at the ESP32 pin to filter high-frequency noise.

## These sensors vs capacitive sensors
This resistive probe is the cheap version. A **capacitive soil moisture sensor** (v1.2, v2.0) is a few dollars more and uses capacitance instead of conductance:
- No exposed metal → no corrosion → lasts indefinitely
- Less sensitive at high salinity
- Doesn't need power-switching trick (though it still helps)
- Better choice for permanent outdoor installations

If you're planning a long-term garden or field deployment, go capacitive. For quick experiments or temporary setups, these resistive ones are fine with the power-switching trick.

## Typical moisture thresholds (varies by plant and soil type)
| Value | Condition | Notes |
|-------|-----------|-------|
| 0–20% | Very dry | Most plants need watering |
| 20–40% | Dry | Cacti/succulents happy here |
| 40–70% | Moist | Most plants prefer this range |
| 70–85% | Wet | Good for moisture-loving plants |
| 85–100% | Saturated | Risk of root rot for most plants |
