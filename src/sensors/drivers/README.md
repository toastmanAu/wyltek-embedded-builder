# Sensor Drivers

Each driver is a single self-contained header file. No external libraries required.  
All drivers implement `WySensorBase` — just `begin()`, `read()`, `driverName()`.

## Adding a new sensor

1. Copy `WyDriverTemplate.h` → `WyMyNewSensor.h`
2. Find your sensor's datasheet — look for:
   - **I2C address** (hex, often selectable via ADDR pin)
   - **WHO_AM_I / Chip ID register** — verify you're talking to the right chip
   - **Configuration register** — how to wake it up / set mode
   - **Data registers** — where to read the measurement bytes from
   - **Conversion formula** — how raw ADC counts become real units
3. Fill in the `TODO` sections in the template
4. Register it in your sketch: `sensors.addI2C<WyMyNewSensor>("name", sda, scl, addr)`

## Bundled drivers

| Driver | Sensor(s) | Bus | Measures |
|--------|-----------|-----|----------|
| `WyBME280.h`  | BME280 / BMP280              | I2C        | Temp, Humidity, Pressure, Altitude |
| `WySHT31.h`   | SHT31 / SHT35                | I2C        | Temp, Humidity |
| `WyAHT20.h`   | AHT20 / AHT21 / AHT10       | I2C        | Temp, Humidity |
| `WyDHT22.h`   | DHT22 / DHT11                | GPIO       | Temp, Humidity |
| `WyDS18B20.h` | DS18B20                      | 1-Wire     | Temp, multi-sensor bus |
| `WyBH1750.h`  | BH1750                       | I2C        | Light (lux) |
| `WyHCSR04.h`  | HC-SR04 / JSN-SR04T          | GPIO 2-pin | Distance (mm) |
| `WyINA219.h`  | INA219                       | I2C        | Voltage, Current, Power |
| `WyVL53L0X.h` | VL53L0X                      | I2C        | Distance (mm), ToF laser |
| `WyMAX6675.h` | MAX6675                      | SPI        | Thermocouple 0–1023°C |
| `WyMPU6050.h` | MPU-6050                     | I2C        | Accel XYZ (g), Gyro XYZ (°/s), Roll/Pitch |
| `WySGP30.h`   | SGP30                        | I2C        | eCO2 (ppm), TVOC (ppb), baseline + humidity comp |
| `WyHX711.h`   | HX711                        | GPIO 2-pin | Weight / load cell (g), tare, calibration |
| `WyGM861S.h`  | Grow GM861S                  | UART       | Barcode/QR scan, trigger control |
| `WyHSQR204.h` | HS-QR204 / HSP04             | UART       | Thermal receipt printer (ESC/POS), QR print |
| `WyMQ.h`      | MQ-2,3,4,5,6,7,8,9,135,136,137 | Analog  | Gas: CO, LPG, CH4, H2, NH3, alcohol, smoke |

## MQ sensor guide

All MQ classes are in `WyMQ.h` — one file, all variants:

| Class | Sensor | Primary target gas | Typical R0 |
|-------|--------|--------------------|------------|
| `WyMQ2`   | MQ-2   | LPG / propane / smoke    | 9.83 kΩ  |
| `WyMQ3`   | MQ-3   | Alcohol / ethanol        | 60 kΩ    |
| `WyMQ4`   | MQ-4   | Methane (natural gas)    | 4.4 kΩ   |
| `WyMQ5`   | MQ-5   | LPG / natural gas / H2   | 6.5 kΩ   |
| `WyMQ6`   | MQ-6   | LPG / butane             | 10 kΩ    |
| `WyMQ7`   | MQ-7   | Carbon monoxide (CO)     | 27.5 kΩ  |
| `WyMQ8`   | MQ-8   | Hydrogen (H2)            | 1000 kΩ  |
| `WyMQ9`   | MQ-9   | CO + flammable gases     | 9.6 kΩ   |
| `WyMQ135` | MQ-135 | Air quality / CO2 approx | 76.63 kΩ |
| `WyMQ136` | MQ-136 | Hydrogen sulphide (H2S)  | 3.5 kΩ   |
| `WyMQ137` | MQ-137 | Ammonia (NH3)            | 35 kΩ    |

All MQ sensors use `ppm = a × (Rs/R0)^b`. R0 varies between individual units —
**always calibrate in clean air** after ≥20 min warm-up: `sensor->calibrateR0()`

## Drivers planned

| Sensor | Bus | Measures | Notes |
|--------|-----|----------|-------|
| VEML7700  | I2C  | Light (lux)          | High dynamic range |
| MH-Z19B   | UART | CO2 ppm              | NDIR, self-calibrating |
| PMS5003   | UART | PM1/2.5/10           | Particulate matter |
| QMC5883L  | I2C  | Magnetometer         | Compass bearing |
| APDS9960  | I2C  | Gesture/light/prox   | |
| ADS1115   | I2C  | ADC 16-bit 4-ch      | Expand analog inputs |
| MAX31865  | SPI  | PT100/PT1000 RTD     | Precision temperature |
| PZEM-004T | UART | AC V/I/P/energy      | Modbus RTU |

## Contribution guidelines

- One sensor per file (or one file for a family — see WyMQ.h)
- No external library dependencies — talk directly to registers
- Use the helper methods in the template (`_readReg8`, `_readRegBuf`, etc.)
- Populate only the `WySensorData` fields your sensor actually provides
- Include the datasheet register names as `#define` constants — makes the code readable
- Add a link to the datasheet in a comment at the top of the file

## Register addressing patterns

Most sensors follow one of these I2C patterns:

```
// 8-bit register address (most common)
Wire.write(0xF7);  // register address

// 16-bit register address (e.g. GT911 touch, some environmental sensors)
Wire.write(0xF7 >> 8);   // high byte first
Wire.write(0xF7 & 0xFF); // low byte

// Auto-increment (burst read multiple registers)
// Works on most sensors — read N bytes starting at base register
Wire.requestFrom(addr, 6);  // reads reg, reg+1, reg+2, reg+3, reg+4, reg+5
```
