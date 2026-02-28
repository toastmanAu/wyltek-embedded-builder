/*
 * WySensors.h — Sensor registry for wyltek-embedded-builder
 * ===========================================================
 * Self-describing sensor constructors — each sensor declares exactly
 * the pins it needs. The registry manages initialisation and provides
 * a uniform read interface.
 *
 * Usage:
 *
 *   // I2C sensors — bus shared, just need addr
 *   sensors.addI2C<WyBME280>("env", 0x76);
 *   sensors.addI2C<WySHT31> ("temp", 0x44);
 *
 *   // SPI sensors — each gets its own CS, shares MOSI/MISO/SCK
 *   sensors.addSPI<WyMAX6675>("thermocouple", MOSI=23, MISO=19, SCK=18, CS=5);
 *
 *   // Single-wire / GPIO sensors
 *   sensors.addGPIO<WyDHT22> ("humidity", PIN=4);
 *   sensors.addGPIO<WyDS18B20>("water_temp", PIN=13);
 *
 *   // Begin all at once
 *   sensors.begin();
 *
 *   // Read — returns WySensorData (typed union)
 *   WySensorData d = sensors.read("env");
 *   Serial.printf("temp=%.1f hum=%.1f\n", d.temperature, d.humidity);
 *
 *   // Or get sensor directly
 *   auto* bme = sensors.get<WyBME280>("env");
 *   if (bme) bme->doSomethingSpecific();
 *
 * Adding a new sensor type:
 *   1. Create WyMyNewSensor.h in sensors/drivers/
 *   2. Inherit from WySensorBase
 *   3. Implement: begin(), read(), sensorType(), pinSchema()
 *   4. Done — registry handles everything else
 */

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

/* ── Max sensors in registry ────────────────────────────────────── */
#ifndef WY_SENSORS_MAX
#define WY_SENSORS_MAX 8
#endif

/* ══════════════════════════════════════════════════════════════════
 * WySensorData — typed result struct
 * All drivers populate what they support, leave rest at NaN / 0
 * ══════════════════════════════════════════════════════════════════ */
#define WY_INVALID  (float)NAN

struct WySensorData {
    float temperature  = WY_INVALID;  /* °C */
    float humidity     = WY_INVALID;  /* % RH */
    float pressure     = WY_INVALID;  /* hPa */
    float altitude     = WY_INVALID;  /* m */
    float light        = WY_INVALID;  /* lux */
    float co2          = WY_INVALID;  /* ppm */
    float distance     = WY_INVALID;  /* mm */
    float voltage      = WY_INVALID;  /* V */
    float current      = WY_INVALID;  /* A */
    float weight       = WY_INVALID;  /* g */
    float raw          = WY_INVALID;  /* generic ADC/raw */
    uint32_t rawInt    = 0;
    bool ok            = false;
    const char* error  = nullptr;

    bool valid(float v) { return !isnan(v); }
};

/* ══════════════════════════════════════════════════════════════════
 * WySensorBase — interface all drivers implement
 * ══════════════════════════════════════════════════════════════════ */
class WySensorBase {
public:
    virtual ~WySensorBase() {}
    virtual bool         begin()      = 0;
    virtual WySensorData read()       = 0;
    virtual const char*  driverName() = 0;  /* "BME280", "DHT22", etc. */
    bool ready = false;
};

/* ══════════════════════════════════════════════════════════════════
 * Pin config structs — passed to addSPI / addI2C / addGPIO
 * Named fields: user can't accidentally swap MOSI and SCK
 * ══════════════════════════════════════════════════════════════════ */
struct WySPIPins {
    int8_t mosi;
    int8_t miso;
    int8_t sck;
    int8_t cs;
    uint32_t freq    = 1000000;
    uint8_t  spiMode = 0;       /* SPI mode 0-3 */
    uint8_t  spiPort = VSPI;    /* VSPI or HSPI */
};

struct WyI2CPins {
    int8_t  sda;
    int8_t  scl;
    uint8_t addr;
    uint32_t freq = 400000;
};

struct WyGPIOPins {
    int8_t pin;
    int8_t pin2 = -1;   /* some sensors need 2 pins (e.g. UART TX/RX) */
};

struct WyUARTPins {
    int8_t tx;
    int8_t rx;
    uint32_t baud = 9600;
    uint8_t  port = 1;  /* UART port 0-2 */
};

/* ══════════════════════════════════════════════════════════════════
 * WySensorEntry — registry slot
 * ══════════════════════════════════════════════════════════════════ */
struct WySensorEntry {
    char          name[24]  = {};
    WySensorBase* driver    = nullptr;
    bool          inUse     = false;
};

/* ══════════════════════════════════════════════════════════════════
 * WySensors — the registry
 * ══════════════════════════════════════════════════════════════════ */
class WySensors {
public:

    /* ── Add I2C sensor ─────────────────────────────────────────── */
    template<typename T>
    T* addI2C(const char* name, WyI2CPins pins) {
        WySensorEntry* slot = _alloc(name);
        if (!slot) return nullptr;
        T* drv = new T(pins);
        slot->driver = drv;
        return drv;
    }

    /* Convenience: addI2C with just SDA, SCL, addr ─────────────── */
    template<typename T>
    T* addI2C(const char* name, int8_t sda, int8_t scl, uint8_t addr,
              uint32_t freq = 400000) {
        return addI2C<T>(name, {sda, scl, addr, freq});
    }

    /* ── Add SPI sensor ─────────────────────────────────────────── */
    template<typename T>
    T* addSPI(const char* name, WySPIPins pins) {
        WySensorEntry* slot = _alloc(name);
        if (!slot) return nullptr;
        T* drv = new T(pins);
        slot->driver = drv;
        return drv;
    }

    /* Convenience: addSPI with flat args ───────────────────────── */
    template<typename T>
    T* addSPI(const char* name,
              int8_t mosi, int8_t miso, int8_t sck, int8_t cs,
              uint32_t freq = 1000000, uint8_t mode = 0) {
        return addSPI<T>(name, {mosi, miso, sck, cs, freq, mode});
    }

    /* ── Add GPIO sensor (single/dual pin) ──────────────────────── */
    template<typename T>
    T* addGPIO(const char* name, int8_t pin, int8_t pin2 = -1) {
        WySensorEntry* slot = _alloc(name);
        if (!slot) return nullptr;
        T* drv = new T({pin, pin2});
        slot->driver = drv;
        return drv;
    }

    /* ── Add UART sensor ────────────────────────────────────────── */
    template<typename T>
    T* addUART(const char* name, int8_t tx, int8_t rx,
               uint32_t baud = 9600, uint8_t port = 1) {
        WySensorEntry* slot = _alloc(name);
        if (!slot) return nullptr;
        T* drv = new T({tx, rx, baud, port});
        slot->driver = drv;
        return drv;
    }

    /* ── Begin all registered sensors ──────────────────────────── */
    void begin() {
        for (uint8_t i = 0; i < WY_SENSORS_MAX; i++) {
            if (!_slots[i].inUse || !_slots[i].driver) continue;
            bool ok = _slots[i].driver->begin();
            _slots[i].driver->ready = ok;
            if (ok)
                Serial.printf("[WySensors] %-16s %-12s ready\n",
                    _slots[i].name, _slots[i].driver->driverName());
            else
                Serial.printf("[WySensors] %-16s %-12s FAILED\n",
                    _slots[i].name, _slots[i].driver->driverName());
        }
    }

    /* ── Read by name ───────────────────────────────────────────── */
    WySensorData read(const char* name) {
        WySensorEntry* slot = _find(name);
        if (!slot || !slot->driver || !slot->driver->ready) {
            WySensorData err; err.error = "not found or not ready";
            return err;
        }
        return slot->driver->read();
    }

    /* ── Get typed driver pointer by name ───────────────────────── */
    template<typename T>
    T* get(const char* name) {
        WySensorEntry* slot = _find(name);
        if (!slot) return nullptr;
        return static_cast<T*>(slot->driver);
    }

    /* ── List all registered sensors ───────────────────────────── */
    void list() {
        Serial.println("[WySensors] registered:");
        for (uint8_t i = 0; i < WY_SENSORS_MAX; i++) {
            if (!_slots[i].inUse) continue;
            Serial.printf("  %-16s %-12s %s\n",
                _slots[i].name,
                _slots[i].driver ? _slots[i].driver->driverName() : "?",
                _slots[i].driver && _slots[i].driver->ready ? "OK" : "not ready");
        }
    }

    /* ── Count ──────────────────────────────────────────────────── */
    uint8_t count() {
        uint8_t n = 0;
        for (uint8_t i = 0; i < WY_SENSORS_MAX; i++) if (_slots[i].inUse) n++;
        return n;
    }

private:
    WySensorEntry _slots[WY_SENSORS_MAX];

    WySensorEntry* _alloc(const char* name) {
        for (uint8_t i = 0; i < WY_SENSORS_MAX; i++) {
            if (!_slots[i].inUse) {
                strncpy(_slots[i].name, name, sizeof(_slots[i].name)-1);
                _slots[i].inUse = true;
                return &_slots[i];
            }
        }
        Serial.println("[WySensors] registry full");
        return nullptr;
    }

    WySensorEntry* _find(const char* name) {
        for (uint8_t i = 0; i < WY_SENSORS_MAX; i++)
            if (_slots[i].inUse && strcmp(_slots[i].name, name) == 0)
                return &_slots[i];
        return nullptr;
    }
};

// ── Audio & Sound ─────────────────────────────────────────────────
#include "drivers/WyDYSV5W.h"    // DY-SV5W/SV8F serial MP3 player
#include "drivers/WyMAX9814.h"   // MAX9814 auto-gain microphone

// ── Input ─────────────────────────────────────────────────────────
#include "drivers/WyKY040.h"     // KY-040 rotary encoder
