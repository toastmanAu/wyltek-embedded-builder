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

| Driver | Sensor | Bus | Measures | Tested |
|--------|--------|-----|----------|--------|
| `WyBME280.h`  | BME280 / BMP280      | I2C  | Temp, Humidity, Pressure, Altitude | ✓ |
| `WySHT31.h`   | SHT31 / SHT35        | I2C  | Temp, Humidity | ✓ |
| `WyAHT20.h`   | AHT20 / AHT21 / AHT10 | I2C | Temp, Humidity | ✓ |
| `WyDHT22.h`   | DHT22 / DHT11        | GPIO | Temp, Humidity | ✓ |
| `WyDS18B20.h` | DS18B20              | 1-Wire GPIO | Temp, multi-sensor bus | ✓ |
| `WyBH1750.h`  | BH1750               | I2C  | Light (lux, 0.5–1 lux res) | ✓ |
| `WyHCSR04.h`  | HC-SR04 / JSN-SR04T  | GPIO | Distance (mm), temp compensated | ✓ |
| `WyINA219.h`  | INA219               | I2C  | Voltage, Current, Power (W) | ✓ |
| `WyVL53L0X.h` | VL53L0X              | I2C  | Distance (mm), ToF laser | ✓ |
| `WyMAX6675.h` | MAX6675              | SPI  | Thermocouple temp (0–1023°C) | ✓ |

## Drivers planned

| Sensor | Bus | Measures | Notes |
|--------|-----|----------|-------|
| VEML7700 | I2C | Light (lux) | High dynamic range |
| INA3221 | I2C | 3-channel voltage/current | |
| MPU6050 | I2C | Accel, Gyro, Temp | 6-DoF IMU |
| QMC5883L | I2C | Magnetometer | Compass bearing |
| APDS9960 | I2C | Gesture, Light, Proximity, Color | |
| SGP30 | I2C | eCO2, TVOC | Air quality |
| MQ-series | GPIO/ADC | Gas (CO, CH4, etc.) | Analog + threshold |
| HX711 | GPIO (2-pin) | Weight (via load cell) | 24-bit ADC |
| ADS1115 | I2C | ADC 16-bit 4-ch | |
| PCF8574 | I2C | GPIO expander | |
| MCP23017 | I2C | GPIO expander 16-ch | |
| MAX31865 | SPI | PT100/PT1000 RTD | Precision temp |
| LIS3DH | SPI/I2C | Accelerometer | Click detection |
| PZEM-004T | UART | AC voltage/current/power | Modbus |
| CO2 MH-Z19 | UART | CO2 (ppm) | Auto-calibrating |
| PM2.5 PMS5003 | UART | Particulate matter | Air quality |

## Contribution guidelines

- One sensor per file
- No external library dependencies — talk directly to registers
- Use the helper methods in the template (`_readReg8`, `_readRegBuf`, etc.)
- Populate only the `WySensorData` fields your sensor actually provides
- Include the datasheet register names as `#define` constants — makes the code readable
- Add a link to the datasheet in a comment at the top of the file
- Mark tested boards in the file header comment

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
