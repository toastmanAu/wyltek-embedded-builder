/*
 * CkbStatsC6.ino — CKB Node Stats Display for LilyGo T-QT C6
 * ============================================================
 * Polls a CKB full node RPC over WiFi and displays live stats
 * on the 0.85" GC9107 (128×128) round display.
 *
 * Shows:
 *   - Block height (updates every 6s — CKB block time)
 *   - Peer count
 *   - CKB/USD price (updates every 5 min via CoinGecko)
 *   - WiFi signal strength
 *
 * Board:    LilyGo T-QT C6 (-DWY_BOARD_LILYGO_TQTC6)
 * Library:  wyltek-embedded-builder
 * Requires: ArduinoJson, Arduino_GFX_Library
 *
 * platformio.ini:
 *   [env:lilygo-tqtc6]
 *   platform = espressif32
 *   board = esp32-c6-devkitc-1
 *   framework = arduino
 *   build_flags =
 *     -DWY_BOARD_LILYGO_TQTC6
 *     -DWIFI_SSID=\"YourSSID\"
 *     -DWIFI_PASS=\"YourPass\"
 *     -DCKB_NODE_URL=\"http://192.168.68.87:8114\"
 *   lib_deps =
 *     toastmanAu/wyltek-embedded-builder
 *     bblanchon/ArduinoJson@^6.21
 *     moononournation/GFX Library for Arduino
 */

#include <wyltek.h>
#include <display/WyDisplay.h>
#include <net/WyNet.h>
#include <ckb/WyCkbNode.h>

// ── Config ────────────────────────────────────────────────────────
#ifndef WIFI_SSID
  #define WIFI_SSID "YourSSID"
#endif
#ifndef WIFI_PASS
  #define WIFI_PASS "YourPass"
#endif
#ifndef CKB_NODE_URL
  #define CKB_NODE_URL "http://192.168.68.87:8114"
#endif

// ── Globals ───────────────────────────────────────────────────────
WyDisplay  display;
WyCkbNode  ckb(CKB_NODE_URL);

// Refresh intervals
static const unsigned long BLOCK_INTERVAL_MS = 6000;   // ~CKB block time
static const unsigned long PRICE_INTERVAL_MS = 300000; // 5 min
static unsigned long lastBlock = 0;
static unsigned long lastPrice = 0;

// Colour palette (RGB565)
#define C_BG        0x0861   // very dark blue
#define C_ACCENT    0x07FF   // cyan
#define C_GREEN     0x07E0   // green
#define C_YELLOW    0xFFE0   // yellow
#define C_RED       0xF800   // red
#define C_WHITE     0xFFFF
#define C_MUTED     0x4A49   // grey

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);

    // Init display
    display.begin();
    display.fillScreen(C_BG);
    display.setTextColor(C_ACCENT);
    display.setTextSize(1);
    display.setCursor(10, 55);
    display.print("Connecting WiFi...");

    // Connect WiFi
    WyNet::connect(WIFI_SSID, WIFI_PASS);
    unsigned long t = millis();
    while (!WyNet::connected() && millis() - t < 15000) delay(200);

    if (!WyNet::connected()) {
        display.fillScreen(C_BG);
        display.setTextColor(C_RED);
        display.setCursor(10, 55);
        display.print("WiFi failed");
        while (1) delay(1000);
    }

    display.fillScreen(C_BG);
    drawLayout();

    // Initial fetch
    ckb.update();
    ckb.updatePrice();
    drawStats();

    lastBlock = millis();
    lastPrice = millis();
}

// ── Main loop ─────────────────────────────────────────────────────
void loop() {
    unsigned long now = millis();

    bool updated = false;

    if (now - lastBlock >= BLOCK_INTERVAL_MS) {
        ckb.update();
        lastBlock = now;
        updated = true;
    }

    if (now - lastPrice >= PRICE_INTERVAL_MS) {
        ckb.updatePrice();
        lastPrice = now;
        updated = true;
    }

    if (updated) drawStats();

    delay(500);
}

// ── UI ────────────────────────────────────────────────────────────
void drawLayout() {
    display.fillScreen(C_BG);

    // Title
    display.setTextColor(C_ACCENT);
    display.setTextSize(1);
    display.setCursor(35, 6);
    display.print("CKB NODE");

    // Divider
    display.drawFastHLine(8, 18, 112, C_ACCENT);

    // Labels
    display.setTextColor(C_MUTED);
    display.setCursor(8, 26);  display.print("BLOCK");
    display.setCursor(8, 58);  display.print("PEERS");
    display.setCursor(8, 90);  display.print("CKB/USD");
    display.setCursor(8, 112); display.print("WIFI");

    // Dividers between rows
    display.drawFastHLine(8, 54, 112, 0x2124);
    display.drawFastHLine(8, 86, 112, 0x2124);
    display.drawFastHLine(8, 108, 112, 0x2124);
}

void clearValue(int y, int h) {
    display.fillRect(8, y, 112, h, C_BG);
}

void drawStats() {
    // Block number
    clearValue(34, 18);
    display.setTextColor(C_WHITE);
    display.setTextSize(2);
    if (ckb.info.valid) {
        // Format with commas: 18,773,052
        char buf[16];
        uint64_t bn = ckb.info.blockNumber;
        if (bn >= 1000000)
            snprintf(buf, sizeof(buf), "%llu", (unsigned long long)bn);
        else
            snprintf(buf, sizeof(buf), "%llu", (unsigned long long)bn);
        display.setCursor(8, 36);
        display.print(buf);
    } else {
        display.setTextColor(C_RED);
        display.setCursor(8, 36);
        display.print("--");
    }

    // Peers
    clearValue(62, 20);
    display.setTextSize(2);
    if (ckb.info.valid) {
        display.setTextColor(ckb.info.peers >= 5 ? C_GREEN :
                             ckb.info.peers > 0  ? C_YELLOW : C_RED);
        display.setCursor(8, 64);
        display.print(ckb.info.peers);
    } else {
        display.setTextColor(C_RED);
        display.setCursor(8, 64);
        display.print("--");
    }

    // Price
    clearValue(92, 14);
    display.setTextSize(1);
    if (ckb.price.valid) {
        char buf[12];
        snprintf(buf, sizeof(buf), "$%.4f", ckb.price.usd);
        display.setTextColor(C_YELLOW);
        display.setCursor(8, 96);
        display.print(buf);
    } else {
        display.setTextColor(C_MUTED);
        display.setCursor(8, 96);
        display.print("fetching...");
    }

    // WiFi RSSI
    clearValue(114, 14);
    display.setTextSize(1);
    int rssi = WyNet::rssi();
    display.setTextColor(rssi > -60 ? C_GREEN : rssi > -75 ? C_YELLOW : C_RED);
    display.setCursor(8, 116);
    char wbuf[16];
    snprintf(wbuf, sizeof(wbuf), "%ddBm", rssi);
    display.print(wbuf);
}
