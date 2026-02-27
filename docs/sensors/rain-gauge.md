# Optical Rain Gauge (RS485 + Pulse)
**Driver:** `WyRainGauge.h` — RS485/Modbus RTU + Pulse output variants

---

## What it is
Infrared optical rain gauge with 0.1mm resolution. An IR beam detects individual raindrops falling through the sensor head — no moving parts, no tipping bucket to clog or stick. Each detection event increments an internal counter. One count = 0.1mm of rainfall.

Two output interfaces on the same module:
- **RS485** (Modbus RTU) — digital, accurate, works over long cable runs
- **Pulse output** — one low pulse per 0.1mm, simpler wiring

## ⚠️ Poorly documented hardware
This is a Chinese RS485 weather sensor with minimal English documentation. The register map below is **reverse-engineered** from common patterns in similar modules. Your specific unit may have different register addresses.

**The driver includes probe tools specifically for this situation** — see the Reverse Engineering section below.

---

## Interface 1: RS485 / Modbus RTU (recommended)

### What you need
- MAX485 or SP3485 RS485 transceiver module (~$0.50 on AliExpress)
- ESP32 UART pins (TX, RX)
- One extra GPIO for DE/RE (direction control)

### Wiring
```
Sensor A (RS485+) ──── MAX485 A
Sensor B (RS485-) ──── MAX485 B
MAX485 RO ──────────── ESP32 RX
MAX485 DI ──────────── ESP32 TX
MAX485 DE+RE ────────── ESP32 DE pin (tie DE and RE together)
MAX485 VCC ──────────── 3.3V or 5V
Sensor VCC ──────────── 12V (check your module — some are 5V, some 9-24V)
Common GND
```

> **DE pin**: HIGH = transmit mode, LOW = receive mode. Must switch before/after each transmission. The driver handles this automatically if you call `setDEPin()`.

### Code
```cpp
auto* rain = sensors.addUART<WyRainGauge>("rainfall", TX_PIN, RX_PIN, 9600);
rain->setDEPin(DE_PIN);
rain->setModbusAddr(0x01);  // default address
sensors.begin();

WySensorData d = sensors.read("rainfall");
if (d.ok) {
    Serial.printf("Rainfall: %.1f mm\n", d.raw);
    Serial.printf("Rate: %.1f mm/hr\n", d.voltage);
}
```

### Assumed Modbus register map
| Register | Content | Scale |
|----------|---------|-------|
| 0x0000 | Accumulated rainfall | count × 0.1 = mm |
| 0x0001 | Rainfall intensity | × 0.1 = mm/min (may not exist) |
| 0x0002 | Status flags | check your datasheet |
| 0x0003 | Firmware version | |

**These may be wrong for your unit.** Verify with `probeRegisters()`.

---

## Interface 2: Pulse output (simpler)

### Wiring
```
Sensor PULSE/OUT → ESP32 GPIO (interrupt-capable)
Sensor VCC → 12V
Sensor GND → ESP32 GND (common ground required)
Add 10kΩ pull-up to 3.3V on the pulse pin
```

### Code
```cpp
auto* rain = sensors.addGPIO<WyRainGaugePulse>("rainfall", PULSE_PIN);
sensors.begin();

void loop() {
    WySensorData d = sensors.read("rainfall");
    if (d.ok) {
        Serial.printf("Total: %.1f mm  Rate: %.1f mm/hr\n",
            d.raw, d.voltage);
    }
    delay(10000);  // check every 10 seconds
}
```

### ⚠️ Use interrupts — do not poll
The pulse is 100–250ms wide. In heavy rain (50mm/hr = ~1.4 pulses/second) you could miss pulses if your loop is doing other work. The driver uses `attachInterrupt(FALLING)` — pulses are counted in hardware regardless of what the main loop is doing.

---

## Reverse engineering your unit

### Step 1 — find the baud rate
```cpp
rain->probeBaud();
```
Tries 1200, 2400, 4800, 9600, 19200, 38400, 115200 and reports which one gets a response.

### Step 2 — find the address
```cpp
rain->probeAddress();  // scans 0x01–0x10
```

### Step 3 — dump all registers
```cpp
rain->probeRegisters(0x00, 16);  // read registers 0x00–0x0F
```
Run this in **dry conditions**, note the values. Then simulate rain (drip water through the sensor head) and run again. The register that incremented is your accumulated rainfall register.

### Step 4 — watch intensity
Leave it running in rain and watch registers 0x01–0x04 for anything that changes during rain but returns to zero when it stops — that's likely the intensity/rate register.

### Step 5 — update the driver
Once confirmed:
```cpp
rain->setAccumReg(0x0000);     // your confirmed register
rain->setIntensityReg(0x0001); // or 0xFFFF to disable
```

---

## Common Modbus gotchas

**Half-duplex timing** — RS485 is half-duplex. You must switch the DE pin HIGH to transmit, then LOW to receive before the response arrives. The timing window is tight. If you get no responses, increase the delay between setting DE LOW and starting to listen. The driver uses 100µs — try 500µs if unreliable.

**CRC byte order** — Modbus CRC is little-endian (low byte first). Some implementations get this wrong. The driver uses the correct Modbus poly (0xA001) and byte order.

**Termination** — RS485 bus needs 120Ω termination at each end. For a short bench run (under 1m) you can usually skip it. For outdoor cable runs over a few metres, put a 120Ω resistor across A and B at the sensor end.

**12V supply** — most of these modules need 12V to power the IR LED and electronics. Don't try to run them from ESP32 5V — the IR detection distance will be wrong and it may not work at all.

---

## Rainfall calculations

```
accumulated_mm = pulse_count × 0.1

rate_mm_per_hour = (pulses_in_last_hour × 0.1)

intensity scale (approximate):
  < 2.5 mm/hr   — light rain / drizzle
  2.5–7.5 mm/hr — moderate rain
  7.5–50 mm/hr  — heavy rain
  > 50 mm/hr    — very heavy / storm
```

## Daily reset
For a weather station you want daily accumulated rainfall that resets at midnight:
```cpp
// In your time-sync / midnight check:
if (isNewDay()) {
    rain->resetAccumulated();  // Modbus write to reset register
    // or for pulse mode:
    rainPulse->reset();
}
```

## Outdoor mounting
- Mount vertically, collection aperture facing up
- Keep clear of roof overhangs, trees, walls (splash interference)
- Ensure drainage hole at bottom isn't blocked
- Calibration check: pour a measured amount of water (e.g. 100ml) through and verify count matches expected mm (depends on collector area — check spec)
