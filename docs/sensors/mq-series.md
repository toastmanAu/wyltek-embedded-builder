# MQ Gas Sensor Series
**Driver:** `WyMQ.h` — Covers MQ-2, MQ-3, MQ-4, MQ-5, MQ-6, MQ-7, MQ-8, MQ-9, MQ-135, MQ-136, MQ-137

---

## What they are
MOX (Metal Oxide) resistive gas sensors. A heated ceramic element changes resistance in the presence of target gases. Cheap, widely available, good for detection and alarm — not laboratory-grade measurement.

| Sensor | Primary target | Also sensitive to |
|--------|---------------|-------------------|
| MQ-2 | LPG, propane | Smoke, H2, CH4, CO |
| MQ-3 | Alcohol/ethanol | Benzene, CH4 |
| MQ-4 | Methane (natural gas) | LPG, smoke |
| MQ-5 | LPG, natural gas | H2, CO |
| MQ-6 | LPG, butane | CH4 |
| MQ-7 | Carbon monoxide (CO) | H2 |
| MQ-8 | Hydrogen (H2) | CH4, LPG |
| MQ-9 | CO + flammable gases | LPG, CH4 |
| MQ-135 | Air quality / CO2 approx | NH3, NOx, alcohol, benzene |
| MQ-136 | Hydrogen sulphide (H2S) | SO2 |
| MQ-137 | Ammonia (NH3) | — |

## How they work
The sensor has a heater (H pins) that keeps the ceramic at ~300°C. At this temperature, target gases oxidise on the surface and change the element's resistance (Rs). The higher the gas concentration, the lower Rs gets. The module converts this to an analog voltage.

**Formula:** `ppm = A × (Rs/R0)^B`
- Rs = sensor resistance now
- R0 = sensor resistance in clean air (your calibration point)
- A, B = curve constants from the datasheet characteristic curve

## ⚠️ The warm-up problem — this is the #1 gotcha
**The heater takes 20–60 seconds to reach operating temperature.** During warm-up, readings are completely meaningless. The driver delays 20 seconds in `begin()`.

For best accuracy, MOX sensors need **24 hours** of continuous power before their first R0 calibration. The ceramic element conditions itself over time.

**Practical rule:** Power the sensor continuously. Don't switch it on/off — each cold start burns through the calibration stability.

## Calibration — R0 is everything
R0 is the sensor resistance in **clean air**. Every individual unit has a different R0 — the defaults in the driver are typical values, but your specific unit could be 2–3× off.

### How to calibrate
1. Power the sensor for at least 20 minutes (ideally 24 hours) in **clean outdoor air**
2. Call `sensor->calibrateR0()` — this averages 50 readings over ~25 seconds
3. Store the result in NVS and restore it on boot

```cpp
// First boot calibration
mq135->calibrateR0();
float r0 = mq135->getR0();
prefs.putFloat("mq135_r0", r0);

// Subsequent boots
float r0 = prefs.getFloat("mq135_r0", 76.63f);  // default if not set
mq135->setR0(r0);
```

## The Rs/R0 ratio — use it for diagnostics
`sensor->getRsR0()` returns the raw ratio. This is more stable than ppm across units because it cancels out absolute resistance variation. Use it for relative comparisons ("air quality better or worse than baseline").

## Temperature and humidity affect readings
MOX sensors are significantly sensitive to ambient T and RH. Most datasheets provide correction curves (usually a graph, not a formula). General rule:
- High humidity → Rs decreases → reads higher concentration (false alarm tendency)
- Low temperature → slower reaction → reads lower
- Correction tables exist but vary per sensor; for alarm purposes just accept the variance

## MQ-7 CO sensor — special heating cycle
The MQ-7 is the odd one out. For accurate CO measurement it needs a **PWM heating cycle**:
- 60 seconds at 5V heater
- 90 seconds at 1.4V heater (use PWM at ~28% duty on a 5V supply)
- Repeat

The driver uses fixed 5V which is simpler but ~20% less accurate. For safety-critical CO alarms, implement the heating cycle. For "is there CO in the air?" detection, fixed 5V is fine.

## AOUT voltage range
5V modules output 0–5V on AOUT. With ESP32's 3.3V ADC you have two options:
- **Voltage divider** (recommended): 3.3kΩ + 6.8kΩ on AOUT → 0.49× scaling
- **Set `WY_MQ_VREF_MV=5000`** if using a 5V-capable ADC reference (not standard ESP32)

Most ambient air readings are in the 0.5–2V range, so clipping is rare in practice — but for alarms where you care about high-concentration detection, use the divider.

## The DOUT digital output pin
Every MQ module also has a DOUT pin with a comparator. Adjust the trim pot to set a threshold — DOUT goes HIGH when the threshold is crossed. This is useful for simple alarms without any calibration or math. Pair with `WyPIR`-style polling (it's just a digital pin).

## MQ-135 for CO2 — important caveat
MQ-135 does NOT measure CO2 directly. It's a general air quality sensor that responds to many gases. The CO2 ppm reading from the driver is an **approximation** based on the assumption that CO2 is the dominant variable gas. In a room with people it's useful as a proxy. For accurate CO2 measurement use the **MH-Z19B** NDIR sensor (coming soon to the driver library).

## Example — simple gas alarm
```cpp
auto* lpg = sensors.addGPIO<WyMQ2>("lpg_alarm", 34);
lpg->skipPreheat();  // if sensor has been on for a while
sensors.begin();
lpg->setR0(9.83f);   // your calibrated value

void loop() {
    WySensorData d = sensors.read("lpg_alarm");
    if (d.ok && d.co2 > 1000) {  // >1000 ppm LPG
        Serial.println("⚠️ LPG detected!");
        // trigger alarm...
    }
}
```
