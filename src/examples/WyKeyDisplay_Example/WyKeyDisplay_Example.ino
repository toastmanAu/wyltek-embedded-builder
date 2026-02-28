/*
 * WyKeyDisplay_Example — LilyGo T-Keyboard S3 Pro
 * ================================================
 * Demonstrates WyKeyDisplay: 4 mechanical keys, each with its own
 * 128×128 GC9107 display embedded in the keycap.
 *
 * This example shows each key displaying a different metric label —
 * the kind of thing you'd use for a CKB node status pad or SBC
 * control surface.
 *
 * Board: WY_BOARD_LILYGO_TKEYBOARD_S3
 * platformio.ini:
 *   [env:tkeyboard]
 *   platform = espressif32
 *   board = esp32-s3-devkitc-1
 *   build_flags =
 *     -DWY_BOARD_LILYGO_TKEYBOARD_S3
 *     -DARDUINO_USB_MODE=1
 *     -DARDUINO_USB_CDC_ON_BOOT=1
 *   lib_deps =
 *     moononournation/GFX Library for Arduino
 *     toastmanAu/wyltek-embedded-builder
 */

#define WY_BOARD_LILYGO_TKEYBOARD_S3
#include <display/WyKeyDisplay.h>

WyKeyDisplay keys;

/* Example metrics — replace with live data from your node/SBC */
struct Metric { const char *label; const char *value; uint16_t colour; };
Metric metrics[4] = {
    { "BLOCK",   "18.7M",  0x07FF },  /* cyan  — block height */
    { "PEERS",   "21",     0x07E0 },  /* green — peer count   */
    { "HASH",    "83k/s",  0xFD20 },  /* orange — hashrate    */
    { "EPOCH",   "8341",   0xF81F },  /* magenta — epoch      */
};

void setup() {
    Serial.begin(115200);
    keys.begin();

    Serial.println("WyKeyDisplay ready — 4 keys initialised");

    /* Draw a metric on each key */
    for (int i = 0; i < 4; i++) {
        keys.setMetric(i, metrics[i].label, metrics[i].value,
                       WY_GRAY, metrics[i].colour, WY_BLACK);
    }
}

void loop() {
    /* Example: update block height on key 0 every 6 seconds */
    static uint32_t lastUpdate = 0;
    static uint32_t fakeBlock  = 18700000;

    if (millis() - lastUpdate > 6000) {
        lastUpdate = millis();
        fakeBlock += 1;

        char buf[12];
        snprintf(buf, sizeof(buf), "%.1fM", fakeBlock / 1000000.0f);
        keys.setMetric(0, "BLOCK", buf, WY_GRAY, WY_CYAN, WY_BLACK);

        Serial.printf("Block updated: %s\n", buf);
    }
}
