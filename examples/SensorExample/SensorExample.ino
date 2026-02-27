/*
 * SensorExample — wyltek-embedded-builder
 * =========================================
 * Demonstrates WySensors registry with multiple sensor types.
 * 
 * Wiring assumed:
 *   BME280  — I2C, SDA=21, SCL=22, addr=0x76
 *   SHT31   — I2C, SDA=21, SCL=22, addr=0x44 (shared bus)
 *   DHT22   — data pin GPIO 4
 *   MAX6675 — SPI MOSI=23, MISO=19, SCK=18, CS=5
 */

#include <Arduino.h>
#include "sensors/WySensors.h"
#include "sensors/drivers/WyBME280.h"
#include "sensors/drivers/WySHT31.h"
#include "sensors/drivers/WyDHT22.h"
#include "sensors/drivers/WyMAX6675.h"

WySensors sensors;

void setup() {
    Serial.begin(115200);
    delay(1000);

    /* Register sensors — each declares its own required pins */
    sensors.addI2C<WyBME280>  ("environment", 21, 22, 0x76);
    sensors.addI2C<WySHT31>   ("humidity",    21, 22, 0x44);
    sensors.addGPIO<WyDHT22>  ("outdoor",     4);
    sensors.addSPI<WyMAX6675> ("oven_temp",   23, 19, 18, 5);

    /* Init all in one call */
    sensors.begin();
    sensors.list();
}

void loop() {
    /* Read all sensors */
    WySensorData env  = sensors.read("environment");
    WySensorData hum  = sensors.read("humidity");
    WySensorData out  = sensors.read("outdoor");
    WySensorData oven = sensors.read("oven_temp");

    if (env.ok)  Serial.printf("[BME280]   T=%.1f°C  H=%.1f%%  P=%.1fhPa  Alt=%.0fm\n",
        env.temperature, env.humidity, env.pressure, env.altitude);
    if (hum.ok)  Serial.printf("[SHT31]    T=%.1f°C  H=%.1f%%\n",
        hum.temperature, hum.humidity);
    if (out.ok)  Serial.printf("[DHT22]    T=%.1f°C  H=%.1f%%\n",
        out.temperature, out.humidity);
    if (oven.ok) Serial.printf("[MAX6675]  T=%.2f°C\n",
        oven.temperature);

    /* Direct driver access for sensor-specific features */
    // WyBME280* bme = sensors.get<WyBME280>("environment");
    // if (bme) { /* bme->someSpecificMethod() */ }

    delay(2000);
}
