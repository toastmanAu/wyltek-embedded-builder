/*
 * ESP32CamStream — MJPEG stream + JPEG snapshot + motion detection
 * ================================================================
 * Board: AI-Thinker ESP32-CAM (WY_BOARD_ESP32CAM)
 *
 * platformio.ini:
 *   [env:esp32cam]
 *   platform      = espressif32
 *   board         = esp32cam
 *   framework     = arduino
 *   monitor_speed = 115200
 *   build_flags   = -DWY_BOARD_ESP32CAM
 *                   -DBOARD_HAS_PSRAM
 *                   -mfix-esp32-psram-cache-issue
 *   board_upload.flash_size   = 4MB
 *   board_build.partitions    = huge_app.csv
 *   upload_port   = /dev/ttyUSB0
 *
 * Flash wiring (FTDI → ESP32-CAM):
 *   FTDI GND → GND
 *   FTDI TX  → GPIO3 (RX)
 *   FTDI RX  → GPIO1 (TX)
 *   FTDI 5V  → 5V
 *   IO0      → GND (hold LOW during upload, remove after)
 *
 * After upload: remove IO0→GND bridge, press RESET.
 *
 * Access:
 *   http://<ip>:81/stream   — MJPEG live view (browser, VLC)
 *   http://<ip>:81/capture  — single JPEG snapshot
 *   http://<ip>:81/status   — JSON camera settings
 */

#include <WiFi.h>
#include "camera/WyCamera.h"

/* ── WiFi credentials — use build_flags or separate secrets.h ── */
#ifndef WIFI_SSID
#define WIFI_SSID  "your_network"
#define WIFI_PASS  "your_password"
#endif

WyCamera cam;

/* Motion alert threshold (0–100). Lower = more sensitive. */
const float MOTION_THRESHOLD = 15.0f;

void setup() {
    Serial.begin(115200);
    Serial.println("\n[ESP32-CAM] starting...");

    /* Flash LED on GPIO4 — blink to show startup */
    pinMode(4, OUTPUT);
    digitalWrite(4, LOW);  /* off */

    /* Camera init */
    cam.setFrameSize(FRAMESIZE_VGA);   /* 640×480 — good balance */
    cam.setQuality(12);                /* JPEG quality (lower = better) */
    cam.setHFlip(false);
    cam.setVFlip(false);
    cam.setBrightness(0);              /* -2 to +2 */
    cam.setMotionDetection(true);

    if (!cam.begin()) {
        Serial.println("Camera init failed — halting");
        while (true) {
            digitalWrite(4, HIGH); delay(100);
            digitalWrite(4, LOW);  delay(100);
        }
    }

    /* WiFi */
    Serial.printf("Connecting to %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    uint8_t attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500); Serial.print("."); attempts++;
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nWiFi failed — check credentials");
        while (true) delay(1000);
    }
    Serial.printf("\nConnected: %s\n", WiFi.localIP().toString().c_str());

    /* Start HTTP stream server */
    cam.startStream(81);
    Serial.println("Ready!");

    /* Brief flash to confirm startup */
    for (int i = 0; i < 3; i++) {
        digitalWrite(4, HIGH); delay(50);
        digitalWrite(4, LOW);  delay(50);
    }
}

void loop() {
    /* Motion detection — check every 2 seconds */
    static uint32_t lastMotionCheck = 0;
    if (millis() - lastMotionCheck > 2000) {
        float motion = cam.motionScore();
        if (motion > MOTION_THRESHOLD) {
            Serial.printf("[MOTION] score: %.1f — activity detected!\n", motion);
            /* Flash LED briefly on motion */
            cam.flashOn(64);   /* dim flash — not blinding */
            delay(100);
            cam.flashOff();
        }
        lastMotionCheck = millis();
    }

    /* Print IP reminder every 30s (helpful when reading serial) */
    static uint32_t lastPrint = 0;
    if (millis() - lastPrint > 30000) {
        Serial.printf("[ESP32-CAM] stream: http://%s:81/stream\n",
            WiFi.localIP().toString().c_str());
        lastPrint = millis();
    }

    delay(10);
}
