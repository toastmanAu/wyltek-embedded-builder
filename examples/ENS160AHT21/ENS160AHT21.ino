/*
 * ENS160+AHT21 Example — wyltek-embedded-builder
 * =================================================
 * ENS160 digital MOX air quality sensor with AHT21 temperature/humidity
 * compensation. Commonly sold as a combined breakout board.
 *
 * Both sensors share the same I2C bus.
 * ENS160: 0x52 (ADDR LOW) or 0x53 (ADDR HIGH)
 * AHT21:  0x38 (fixed)
 *
 * The AHT21 reading is fed into ENS160 compensate() before each air
 * quality read — this is important for accurate eCO2/TVOC values.
 *
 * Wiring (both on same I2C bus):
 *   SDA → GPIO 21
 *   SCL → GPIO 22
 *   VCC → 3.3V
 *   GND → GND
 */

#include <Arduino.h>
#include "sensors/WySensors.h"
#include "sensors/drivers/WyENS160.h"
#include "sensors/drivers/WyAHT20.h"   /* WyAHT21 alias defined here */

#define SDA_PIN  21
#define SCL_PIN  22

WySensors sensors;
WyENS160* ens = nullptr;
WyAHT20*  aht = nullptr;

void setup() {
    Serial.begin(115200);
    delay(500);

    /* Both sensors on same I2C bus */
    aht = sensors.addI2C<WyAHT20> ("humidity", SDA_PIN, SCL_PIN, 0x38);
    ens = sensors.addI2C<WyENS160>("air",      SDA_PIN, SCL_PIN, 0x52);

    sensors.begin();
}

void loop() {
    /* Read AHT21 first — feed T+RH into ENS160 for compensation */
    WySensorData th = sensors.read("humidity");
    if (th.ok && ens) {
        ens->compensate(th.temperature, th.humidity);
    }

    /* Read ENS160 air quality */
    WySensorData air = sensors.read("air");

    /* Print results */
    if (th.ok) {
        Serial.printf("Temp: %.1f°C  RH: %.1f%%\n",
            th.temperature, th.humidity);
    }

    if (air.ok) {
        Serial.printf("AQI: %d (%s)  eCO2: %.0f ppm  TVOC: %.0f ppb\n",
            air.rawInt, ens->aqiLabel(),
            air.co2, air.raw);
    } else if (air.error) {
        Serial.printf("ENS160: %s\n", air.error);
    }

    Serial.println();
    delay(1000);
}
