/*
 * drivers/WySN65HVD230.h — SN65HVD230 / VP230 CAN bus transceiver
 * =================================================================
 * Datasheet: https://www.ti.com/lit/ds/symlink/sn65hvd230.pdf
 * Bundled driver — no external library needed.
 * Uses ESP32 built-in TWAI (Two-Wire Automotive Interface) controller.
 * Registered via WySensors::addGPIO<WySN65HVD230>("can", TX_PIN, RX_PIN)
 *
 * ═══════════════════════════════════════════════════════════════════
 * SCOPE NOTE — CAN bus is a large topic. This driver provides:
 *   [x] Hardware init (TWAI peripheral + transceiver enable)
 *   [x] Send/receive raw CAN frames
 *   [x] OBD-II PIDs (SAE J1979) — engine, speed, temps, fuel
 *   [ ] TODO: Extended PID library (full SAE J1979 table)
 *   [ ] TODO: UDS (ISO 14229) diagnostics
 *   [ ] TODO: CAN DBC file parsing (decode manufacturer-specific frames)
 *   [ ] TODO: J1939 (heavy vehicle / truck protocol)
 *   [ ] TODO: ISOTP (ISO 15765-2) multi-frame transport
 *   [ ] TODO: Manufacturer-specific buses (Toyota, BMW, VAG, Ford)
 * ═══════════════════════════════════════════════════════════════════
 *
 * HARDWARE:
 *   SN65HVD230 breakout board wiring:
 *     CTX (D)  → ESP32 TX pin (TWAI TX)
 *     CRX (R)  → ESP32 RX pin (TWAI RX)
 *     VCC      → 3.3V
 *     GND      → GND
 *     CANH     → CAN bus high wire
 *     CANL     → CAN bus low wire
 *     Rs (S)   → GND for normal mode (or 10kΩ to GND for slope control)
 *
 *   VP230 is pin-compatible — use identically.
 *
 * TERMINATION:
 *   CAN bus requires 120Ω between CANH and CANL at EACH END of the bus.
 *   Most breakout boards have a solder jumper to enable the onboard 120Ω.
 *   If connecting to a vehicle OBD-II port — the car already has termination,
 *   do NOT add another 120Ω or you'll disturb the bus.
 *
 * BAUD RATES:
 *   OBD-II / passenger cars: 500 kbps (most), 250 kbps (some older)
 *   J1939 trucks:            250 kbps
 *   High-speed (CAN FD):     up to 8 Mbps (SN65HVD230 supports classic CAN only)
 *   LIN bus:                 Not CAN — different protocol, not this driver
 *
 * OBD-II USAGE:
 *   Connect CANH/CANL to OBD-II port pins 6/14.
 *   Engine must be running (or ACC/ON) for ECU to respond.
 *   Request: send frame to 0x7DF (broadcast) or 0x7E0-0x7E7 (specific ECU)
 *   Response: ECU replies from 0x7E8-0x7EF
 *
 * ESP32 TWAI NOTES:
 *   - Only one TWAI peripheral on ESP32 (ESP32-S3 also has one)
 *   - ESP32-C3/S2: also one TWAI peripheral
 *   - Cannot share TWAI with other uses
 *   - Uses driver_twai.h from ESP-IDF (exposed via Arduino ESP32 SDK)
 *
 * SAFETY WARNING:
 *   Writing to a vehicle CAN bus can affect safety systems.
 *   NEVER write arbitrary frames to an unknown bus.
 *   OBD-II diagnostic requests (0x7DF) are read-only by design.
 *   Test on a bench setup or dedicated test bus first.
 */

#pragma once
#include "../WySensors.h"
#include "driver/twai.h"   /* ESP32 TWAI (CAN) peripheral driver */

/* CAN baud rates (use with begin()) */
#define WY_CAN_BAUD_25K    TWAI_TIMING_CONFIG_25KBITS()
#define WY_CAN_BAUD_50K    TWAI_TIMING_CONFIG_50KBITS()
#define WY_CAN_BAUD_100K   TWAI_TIMING_CONFIG_100KBITS()
#define WY_CAN_BAUD_125K   TWAI_TIMING_CONFIG_125KBITS()
#define WY_CAN_BAUD_250K   TWAI_TIMING_CONFIG_250KBITS()
#define WY_CAN_BAUD_500K   TWAI_TIMING_CONFIG_500KBITS()   /* OBD-II standard */
#define WY_CAN_BAUD_800K   TWAI_TIMING_CONFIG_800KBITS()
#define WY_CAN_BAUD_1M     TWAI_TIMING_CONFIG_1MBITS()

/* OBD-II standard CAN IDs */
#define OBD2_REQUEST_ID     0x7DF   /* broadcast to all ECUs */
#define OBD2_RESPONSE_BASE  0x7E8   /* ECU 1 (engine) response */
#define OBD2_ECU_ENGINE     0x7E0   /* direct request to engine ECU */

/* OBD-II service modes */
#define OBD2_MODE_CURRENT   0x01   /* current data */
#define OBD2_MODE_FREEZE    0x02   /* freeze frame */
#define OBD2_MODE_DTC       0x03   /* read DTCs */
#define OBD2_MODE_CLEAR_DTC 0x04   /* clear DTCs */
#define OBD2_MODE_O2        0x05   /* O2 sensor test results */
#define OBD2_MODE_TEST      0x06   /* onboard test results */
#define OBD2_MODE_PENDING   0x07   /* pending DTCs */
#define OBD2_MODE_VIN       0x09   /* vehicle info */

/* OBD-II PIDs (Mode 01 — current data) */
#define OBD2_PID_PIDS_SUPPORTED    0x00
#define OBD2_PID_ENGINE_LOAD       0x04   /* % */
#define OBD2_PID_COOLANT_TEMP      0x05   /* °C = A-40 */
#define OBD2_PID_FUEL_PRESSURE     0x0A   /* kPa = 3×A */
#define OBD2_PID_INTAKE_MAP        0x0B   /* kPa = A */
#define OBD2_PID_RPM               0x0C   /* rpm = (256A+B)/4 */
#define OBD2_PID_SPEED             0x0D   /* km/h = A */
#define OBD2_PID_TIMING_ADVANCE    0x0E   /* ° = A/2-64 */
#define OBD2_PID_INTAKE_TEMP       0x0F   /* °C = A-40 */
#define OBD2_PID_MAF               0x10   /* g/s = (256A+B)/100 */
#define OBD2_PID_THROTTLE          0x11   /* % = 100×A/255 */
#define OBD2_PID_O2_B1S1           0x14   /* V = A/200, trim = B*100/128-100 % */
#define OBD2_PID_OBD_STANDARD      0x1C
#define OBD2_PID_RUNTIME           0x1F   /* seconds = 256A+B */
#define OBD2_PID_DISTANCE_WITH_MIL 0x21   /* km = 256A+B */
#define OBD2_PID_FUEL_LEVEL        0x2F   /* % = 100×A/255 */
#define OBD2_PID_WARMUPS_SINCE_DTC 0x30
#define OBD2_PID_DISTANCE_SINCE_DTC 0x31  /* km = 256A+B */
#define OBD2_PID_BARO_PRESSURE     0x33   /* kPa = A */
#define OBD2_PID_O2_SENSOR_WIDE    0x34
#define OBD2_PID_CATALYST_TEMP_B1S1 0x3C  /* °C = (256A+B)/10 - 40 */
#define OBD2_PID_CONTROL_MODULE_V  0x42   /* V = (256A+B)/1000 */
#define OBD2_PID_AMBIENT_TEMP      0x46   /* °C = A-40 */
#define OBD2_PID_ACCEL_PEDAL_D     0x49   /* % = 100×A/255 */
#define OBD2_PID_ACCEL_PEDAL_E     0x4A   /* % = 100×A/255 */
#define OBD2_PID_THROTTLE_ACTUATOR 0x4C   /* % = 100×A/255 */
#define OBD2_PID_FUEL_RATE         0x5E   /* L/h = (256A+B)/20 */
#define OBD2_PID_ENGINE_TORQUE_PCT 0x62   /* % = A-125 */

/* Raw CAN frame (matches TWAI message structure) */
struct WyCANFrame {
    uint32_t id;
    uint8_t  data[8];
    uint8_t  dlc;        /* data length code (0-8) */
    bool     extended;   /* true = 29-bit ID, false = 11-bit */
    bool     rtr;        /* remote transmission request */
};

class WySN65HVD230 : public WySensorBase {
public:
    /* pin = TX, pin2 = RX */
    WySN65HVD230(WyGPIOPins pins)
        : _txPin(pins.pin), _rxPin(pins.pin2) {}

    const char* driverName() override { return "SN65HVD230"; }

    bool begin() override {
        return beginCAN(TWAI_TIMING_CONFIG_500KBITS());
    }

    /* Begin with specific baud rate */
    bool beginCAN(twai_timing_config_t timing) {
        if (_rxPin < 0) {
            Serial.println("[CAN] RX pin required as pin2");
            return false;
        }

        twai_general_config_t g_config =
            TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)_txPin, (gpio_num_t)_rxPin,
                                        TWAI_MODE_NORMAL);
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (twai_driver_install(&g_config, &timing, &f_config) != ESP_OK) {
            Serial.println("[CAN] driver install failed");
            return false;
        }
        if (twai_start() != ESP_OK) {
            Serial.println("[CAN] start failed");
            return false;
        }
        _running = true;
        Serial.println("[CAN] bus started");
        return true;
    }

    WySensorData read() override {
        WySensorData d;
        WyCANFrame frame;
        if (receive(frame, 10)) {
            d.rawInt = frame.id;
            d.raw    = (float)frame.data[0];
            d.ok     = true;
        }
        return d;
    }

    /* ── Raw CAN frame I/O ───────────────────────────────────────── */

    bool send(const WyCANFrame& frame, uint32_t timeoutMs = 100) {
        twai_message_t msg = {};
        msg.identifier     = frame.id;
        msg.data_length_code = frame.dlc;
        msg.extd           = frame.extended ? 1 : 0;
        msg.rtr            = frame.rtr ? 1 : 0;
        memcpy(msg.data, frame.data, frame.dlc);
        return twai_transmit(&msg, pdMS_TO_TICKS(timeoutMs)) == ESP_OK;
    }

    bool receive(WyCANFrame& frame, uint32_t timeoutMs = 100) {
        twai_message_t msg = {};
        if (twai_receive(&msg, pdMS_TO_TICKS(timeoutMs)) != ESP_OK) return false;
        frame.id       = msg.identifier;
        frame.dlc      = msg.data_length_code;
        frame.extended = msg.extd;
        frame.rtr      = msg.rtr;
        memcpy(frame.data, msg.data, msg.data_length_code);
        return true;
    }

    /* ── OBD-II ──────────────────────────────────────────────────── */

    /* Query a single OBD-II PID (Mode 01). Returns raw A/B bytes.
     * Returns false if no response within timeout. */
    bool queryPID(uint8_t pid, uint8_t& A, uint8_t& B,
                  uint32_t timeoutMs = 200) {
        /* OBD-II request frame: [0x02, mode, pid, 0x55, 0x55, 0x55, 0x55, 0x55] */
        WyCANFrame req = {};
        req.id     = OBD2_REQUEST_ID;
        req.dlc    = 8;
        req.data[0] = 0x02;           /* length: 2 bytes follow */
        req.data[1] = OBD2_MODE_CURRENT;
        req.data[2] = pid;
        req.data[3] = req.data[4] = req.data[5] = req.data[6] = req.data[7] = 0x55;

        if (!send(req)) return false;

        /* Wait for response from ECU (0x7E8–0x7EF) */
        uint32_t deadline = millis() + timeoutMs;
        while (millis() < deadline) {
            WyCANFrame rsp;
            if (receive(rsp, 10)) {
                if (rsp.id >= OBD2_RESPONSE_BASE && rsp.id <= OBD2_RESPONSE_BASE + 7) {
                    /* Response: [len, 0x41, pid, A, B, ...] */
                    if (rsp.data[1] == 0x41 && rsp.data[2] == pid) {
                        A = rsp.data[3];
                        B = rsp.data[4];
                        return true;
                    }
                }
            }
        }
        return false;
    }

    /* Convenience OBD-II queries — return engineering values */

    float rpm() {
        uint8_t A, B;
        return queryPID(OBD2_PID_RPM, A, B) ? ((256.0f*A + B) / 4.0f) : -1;
    }

    float speedKmh() {
        uint8_t A, B;
        return queryPID(OBD2_PID_SPEED, A, B) ? A : -1;
    }

    float coolantTempC() {
        uint8_t A, B;
        return queryPID(OBD2_PID_COOLANT_TEMP, A, B) ? (A - 40.0f) : -999;
    }

    float intakeTempC() {
        uint8_t A, B;
        return queryPID(OBD2_PID_INTAKE_TEMP, A, B) ? (A - 40.0f) : -999;
    }

    float throttlePct() {
        uint8_t A, B;
        return queryPID(OBD2_PID_THROTTLE, A, B) ? (100.0f * A / 255.0f) : -1;
    }

    float engineLoadPct() {
        uint8_t A, B;
        return queryPID(OBD2_PID_ENGINE_LOAD, A, B) ? (100.0f * A / 255.0f) : -1;
    }

    float fuelLevelPct() {
        uint8_t A, B;
        return queryPID(OBD2_PID_FUEL_LEVEL, A, B) ? (100.0f * A / 255.0f) : -1;
    }

    float mafGramsSec() {
        uint8_t A, B;
        return queryPID(OBD2_PID_MAF, A, B) ? ((256.0f*A + B) / 100.0f) : -1;
    }

    float ambientTempC() {
        uint8_t A, B;
        return queryPID(OBD2_PID_AMBIENT_TEMP, A, B) ? (A - 40.0f) : -999;
    }

    float batteryVoltage() {
        uint8_t A, B;
        return queryPID(OBD2_PID_CONTROL_MODULE_V, A, B)
               ? ((256.0f*A + B) / 1000.0f) : -1;
    }

    /* Read DTCs (Mode 03) — fills buf with up to maxCodes fault codes.
     * Returns number of DTCs found. DTC format: "P0300" etc.
     * TODO: full DTC decode (this is a stub returning raw bytes) */
    uint8_t readDTCs(char dtcBuf[][6], uint8_t maxCodes) {
        /* TODO: implement ISO 15765-2 multi-frame (ISOTP) for DTC responses
         * DTC responses can exceed 8 bytes — need flow control frames */
        Serial.println("[CAN] DTC read: ISOTP not yet implemented");
        return 0;
    }

    /* ── Bus management ──────────────────────────────────────────── */

    void stop() {
        if (_running) {
            twai_stop();
            twai_driver_uninstall();
            _running = false;
        }
    }

    bool isRunning() { return _running; }

    twai_status_info_t status() {
        twai_status_info_t s = {};
        twai_get_status_info(&s);
        return s;
    }

    void printStatus() {
        twai_status_info_t s = status();
        Serial.printf("[CAN] state=%d tx_err=%d rx_err=%d tx_q=%d rx_q=%d msgs_tx=%d msgs_rx=%d\n",
            s.state, s.tx_error_counter, s.rx_error_counter,
            s.msgs_to_tx, s.msgs_to_rx,
            s.tx_failed_count, s.rx_missed_count);
    }

private:
    int8_t _txPin, _rxPin;
    bool   _running = false;
};
