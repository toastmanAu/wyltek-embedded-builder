# Wind Speed + Direction Sensors
**Driver:** `WyWind.h` — `WyWindSpeed` (pulse anemometer) + `WyWindDirection` (resistor-ladder vane)

---

## Two sensors, two drivers

- **`WyWindSpeed`** — counts pulses from a cup anemometer, outputs km/h
- **`WyWindDirection`** — reads analog voltage from a resistor-ladder wind vane, outputs compass bearing

Both work with the cheap generic weather station sensors sold on AliExpress for $5–20. Also compatible with Davis Instruments and Misol sensors (same electrical interface).

---

## Wind Speed (WyWindSpeed)

### How it works
Cup anemometers have a reed switch (or hall-effect sensor) that closes once per revolution. Count pulses over time → frequency → speed.

```
speed (km/h) = pulse_frequency (Hz) × calibration_factor
```

Standard calibration factor: **2.4 km/h per Hz** — used by Davis Instruments and most generic sensors. This corresponds to a ~0.5m cup arm radius.

### ⚠️ Voltage — reed switch vs active output
**Reed switch (most common):** Open collector — connects signal wire to GND when closed. Pull signal wire up to **3.3V** on the ESP32 side (use `INPUT_PULLUP`). Safe, no divider needed.

**Active 5V output (some sensors):** Pulls to 5V when triggered. This will damage the ESP32.  
Fix: 10kΩ from sensor → GPIO, 10kΩ from GPIO → GND (voltage divider → 2.5V max).

### ⚠️ Reed switch bounce — debounce is mandatory
Reed switches bounce — one physical contact closure can register 5–20 pulses without debounce. The driver uses a 5ms time-gate: any pulse within 5ms of the last one is ignored. This handles bounce without missing real pulses (even at 100 km/h, pulse rate is ~11 Hz — well under the 200 Hz debounce limit).

For hall-effect sensors (cleaner signal): `setDebounceMs(1)` to reduce gating.

### Wiring (reed switch)
```
Anemometer pulse wire → ESP32 GPIO (interrupt-capable)
Anemometer GND wire   → GND
(Driver uses INPUT_PULLUP — no external resistor needed)
```

### Code
```cpp
auto* spd = sensors.addGPIO<WyWindSpeed>("windspeed", GPIO27);
spd->setCalibration(2.4f);      // km/h per Hz — Davis standard
spd->setAverageSeconds(3);      // 3-second average (good for display)
sensors.begin();

void loop() {
    WySensorData d = sensors.read("windspeed");
    if (d.ok) {
        Serial.printf("Wind: %.1f km/h  Gust: %.1f km/h\n",
            d.raw, d.voltage);
    }
    if (d.raw > 80.0f) spd->resetGust();  // reset gust after logging
    delay(1000);
}
```

### WySensorData (WyWindSpeed)
| Field | Content |
|-------|---------|
| `d.raw` | Wind speed km/h (averaged over window) |
| `d.rawInt` | Pulse count since last read |
| `d.voltage` | Gust speed km/h (peak since last `resetGust()`) |

### Averaging and WMO standards
| Period | Use case |
|--------|----------|
| 3 seconds | Smooth display, gust detection |
| 10 minutes | WMO standard "wind speed" |
| 1 hour | Synoptic reports |

```cpp
spd->setAverageSeconds(600);   // 10-minute WMO standard
```

---

## Wind Direction (WyWindDirection)

### How it works
The vane contains a set of resistors, one per position. As the vane rotates, it connects different resistors between the signal wire and GND. Combined with an external pull-up resistor (usually 10kΩ to VCC), each position produces a distinct voltage.

8 cardinal positions (N, NE, E, SE, S, SW, W, NW) — each ~0.1V apart.  
16-position vanes add intercardinals (NNE, ENE, ESE, SSE, SSW, WSW, WNW, NNW).

### ⚠️ Supply voltage — power from 3.3V
Power the vane from 3.3V and the output stays within ESP32 ADC range (no divider needed). Most vanes work fine at 3.3V — the resistor ratios are unchanged.

If powered from 5V, add a 100kΩ + 100kΩ voltage divider (ratio 0.5) and call `setDividerRatio(0.5f)`.

### ⚠️ Pull-up resistor
The vane signal wire needs a pull-up to VCC. Most modules include a 10kΩ onboard. If yours doesn't, add one externally. The built-in LUT assumes 10kΩ pull-up. Wrong pull-up value = wrong direction readings.

### Wiring (3.3V supply, pull-up included on module)
```
Vane VCC → 3.3V
Vane GND → GND
Vane SIG → ESP32 GPIO34 (ADC1)
```

### Code
```cpp
auto* dir = sensors.addGPIO<WyWindDirection>("winddir", GPIO34);
dir->setSupplyVoltage(3.3f);     // must match actual wiring
dir->setNorthOffset(-15);        // degrees if N arrow isn't true north
sensors.begin();

WySensorData d = sensors.read("winddir");
if (d.ok) {
    Serial.printf("Direction: %.0f° (%s)\n", d.raw, dir->compassLabel());
}
```

### WySensorData (WyWindDirection)
| Field | Content |
|-------|---------|
| `d.raw` | Compass bearing (0–359°, 0=North) |
| `d.rawInt` | LUT index of matched position |
| `d.voltage` | Sensor voltage (for debugging) |
| `d.ok` | false if voltage didn't match any LUT entry |

### 16-point vane
```cpp
dir->use16Point();   // enables 22.5° resolution (NNE, ENE etc.)
```

### Circular averaging — smoothing vane flutter
The vane rattles in turbulent wind. Simple arithmetic averaging fails catastrophically near North (e.g. average of 5° and 355° = 180°, wrong). The driver uses **circular mean** (sin/cos averaging):

```cpp
float smoothBearing = dir->averagedBearing(5);  // 5 readings, 250ms
```

This correctly handles the 0°/360° wraparound.

### Custom LUT
If your vane has different resistor values (produces different voltages), measure each position's voltage and build your own table:

```cpp
static const WyVaneEntry myVane[] = {
    {0.072f, 225, "SW"},   // voltage ratio (V/VCC), degrees, label
    {0.089f, 135, "SE"},
    // ... sorted ascending by ratio
};
dir->setLUT(myVane, 8);
```

---

## Full weather station example

```cpp
auto* spd  = sensors.addGPIO<WyWindSpeed>("windspeed",  GPIO27);
auto* dir  = sensors.addGPIO<WyWindDirection>("winddir", GPIO34);
auto* bme  = sensors.addI2C<WyBME280>("weather", SDA, SCL, 0x76);
auto* rain = sensors.addGPIO<WyRainGaugePulse>("rain",  GPIO26);

spd->setCalibration(2.4f);
spd->setAverageSeconds(10);    // WMO standard
dir->setSupplyVoltage(3.3f);

sensors.begin();

void loop() {
    Serial.printf("Wind: %.1f km/h @ %.0f° (%s)  Gust: %.1f\n",
        sensors.read("windspeed").raw,
        sensors.read("winddir").raw,
        dir->compassLabel(),
        spd->gustKmh());
    Serial.printf("Rain: %.1f mm  Temp: %.1f°C  Humid: %.0f%%\n",
        sensors.read("rain").raw,
        sensors.read("weather").temperature,
        sensors.read("weather").humidity);
    delay(10000);
}
```

## Beaufort scale reference
| km/h | Beaufort | Description |
|------|----------|-------------|
| 0–1 | 0 | Calm |
| 1–5 | 1 | Light air |
| 6–11 | 2 | Light breeze |
| 12–19 | 3 | Gentle breeze |
| 20–28 | 4 | Moderate breeze |
| 29–38 | 5 | Fresh breeze |
| 39–49 | 6 | Strong breeze |
| 50–61 | 7 | Near gale |
| 62–74 | 8 | Gale |
| 75–88 | 9 | Strong gale |
| 89–102 | 10 | Storm |
| 103–117 | 11 | Violent storm |
| 118+ | 12 | Hurricane |

```cpp
// Beaufort from km/h
uint8_t beaufort(float kmh) {
    static const float limits[] = {1,5,11,19,28,38,49,61,74,88,102,117};
    for (uint8_t i = 0; i < 12; i++) if (kmh < limits[i]) return i;
    return 12;
}
```
