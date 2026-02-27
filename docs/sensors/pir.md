# PIR Motion Sensors
**Driver:** `WyPIR.h` — Covers MH-SR602, HC-SR501, AM312, RCWL-0516

---

## What they are
Passive Infrared (PIR) sensors detect movement by sensing changes in infrared radiation (body heat) across their field of view. They don't emit anything — purely passive detection. All common modules output a simple digital HIGH when motion is detected.

## Sensor comparison

| Sensor | Size | Supply | Range | Delay | Adjustable | Notes |
|--------|------|--------|-------|-------|------------|-------|
| HC-SR501 | Large | 5–20V | ~7m | 0.5–200s | Yes (pots) | Most common, most flexible |
| MH-SR602 | Mini | 2.7–12V | ~3m | ~2s | No | Tiny, 3.3V compatible |
| AM312 | Micro | 3.3V | ~2m | ~2s | No | Smallest, 3.3V native |
| RCWL-0516 | Small | 4–28V | ~7m | ~2s | No | Microwave (not PIR), sees through walls |

## Wiring
All sensors: VCC, GND, OUT. Output is 3.3V HIGH on detection, LOW when clear. HC-SR501 outputs ~3.3V even from a 5V supply — safe for ESP32 GPIO.

## ⚠️ The warm-up period
**PIR sensors need 30–60 seconds after power-on before reliable detection.** During warm-up they may fire randomly or not at all. This is normal — the pyroelectric element is settling to ambient temperature.

The driver tracks warm-up time. `isWarmedUp()` returns false during this period. For the AM312 (no warm-up needed), call `skipWarmup()`.

## HC-SR501 trim pots
The HC-SR501 has two adjustment potentiometers:
- **Left pot (Sx — sensitivity)**: clockwise = more sensitive / longer range. Start at midpoint.
- **Right pot (Tx — delay time)**: clockwise = longer HIGH output after motion stops (0.5s to ~3 minutes)

And a jumper:
- **H position (retriggerable)**: timer resets if motion continues — stays HIGH as long as you're moving
- **L position (non-retriggerable)**: one fixed-length pulse per trigger regardless of ongoing motion

For most projects, **H (retriggerable)** is what you want.

## Edge detection vs level detection
```cpp
// Level — true while motion is happening
if (pir->motion()) { ... }

// Edge — true only on the first frame of new motion
if (pir->motionStarted()) { Serial.println("Motion started!"); }

// Edge — true only when motion clears
if (pir->motionEnded()) { Serial.println("Motion stopped"); }
```

## RCWL-0516 — not actually a PIR
The RCWL-0516 uses **microwave Doppler radar** (3.2GHz), not infrared. Key differences:
- Works through walls, cardboard, thin plastic
- Detects motion, not body heat — will trigger on swinging objects
- More sensitive to small movements at longer range
- Higher false-positive rate in windy environments
- Same digital output interface — use `WyRCWL0516` alias

## False trigger causes
- AC power lines running parallel to sensor wiring
- Fluorescent lights starting/stopping (emit IR on warm-up)
- Heating/cooling vents nearby (air temperature changes)
- Sunlight moving across walls as clouds pass
- Small animals (cats, birds outside windows)

## Multiple zone coverage
```cpp
sensors.addGPIO<WyPIR>("hallway",  PIN_HALL);
sensors.addGPIO<WyPIR>("entrance", PIN_ENTRY);
sensors.addGPIO<WyPIR>("garden",   PIN_GARDEN);
```

Each sensor independently tracked. No cross-interference.
