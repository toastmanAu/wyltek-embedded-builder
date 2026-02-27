/*
 * FullExample — wyltek-embedded-builder
 * ======================================
 * Shows WyDisplay + WyTouch + WySettings + WyNet all working together.
 * Select board in platformio.ini build_flags.
 *
 * On first boot (or BOOT button held 2s): captive portal starts.
 * Connect to "WY-Setup-XXXX" WiFi → open browser → configure → save → reboots.
 * On normal boot: connects to WiFi, shows touch coordinates on screen.
 */

#include <Arduino.h>
#include "boards.h"
#include "display/WyDisplay.h"
#include "touch/WyTouch.h"
#include "settings/WySettings.h"
#include "net/WyNet.h"

WyDisplay  display;
WyTouch    touch;
WySettings settings;
WyNet      net;

void setup() {
    Serial.begin(115200);

    /* Register settings */
    settings.addString("ssid",     "WiFi SSID",     "");
    settings.addString("pass",     "WiFi Password", "");
    settings.addString("node_url", "CKB Node URL",  "http://192.168.1.1:8114");

    /* Init display first so we can show portal status on screen */
    display.begin();

    /* Boot: check for portal mode */
    settings.begin("myapp");

    if (settings.portalActive()) {
        /* Show portal instructions on screen */
        #if WY_HAS_DISPLAY
        display.gfx->fillScreen(WY_BLACK);
        display.gfx->setTextColor(WY_WHITE);
        display.gfx->setTextSize(2);
        display.gfx->setCursor(10, 20);
        display.gfx->print("Setup Mode");
        display.gfx->setTextSize(1);
        display.gfx->setCursor(10, 50);
        display.gfx->print("Connect to WiFi:");
        display.gfx->setTextColor(WY_YELLOW);
        display.gfx->setCursor(10, 65);
        display.gfx->print("WY-Setup-XXXX");
        display.gfx->setTextColor(WY_WHITE);
        display.gfx->setCursor(10, 85);
        display.gfx->print("Then open: http://192.168.4.1");
        #endif

        while (settings.portalActive()) {
            settings.portalLoop();
        }
        ESP.restart();
    }

    /* Normal boot — connect WiFi */
    net.setHostname("wyltek-device");
    net.onConnect([]() {
        Serial.printf("[app] connected: %s\n", net.localIP().c_str());
    });
    net.begin(settings.getString("ssid"), settings.getString("pass"));

    /* Init touch */
    touch.begin();

    /* Draw welcome screen */
    #if WY_HAS_DISPLAY
    display.gfx->fillScreen(WY_BLACK);
    display.gfx->setTextColor(WY_GREEN);
    display.gfx->setTextSize(3);
    display.gfx->setCursor(20, 20);
    display.gfx->print("wyltek ready");
    display.gfx->setTextColor(WY_WHITE);
    display.gfx->setTextSize(2);
    display.gfx->setCursor(20, 70);
    display.gfx->printf("IP: %s", net.localIP().c_str());
    display.gfx->setCursor(20, 100);
    display.gfx->print("Tap anywhere!");
    #endif
}

static int last_x = -1, last_y = -1;

void loop() {
    net.loop();

    if (touch.update()) {
        #if WY_HAS_DISPLAY
        if (last_x >= 0) display.gfx->fillCircle(last_x, last_y, 20, WY_BLACK);
        display.gfx->fillCircle(touch.x, touch.y, 18, WY_RED);
        display.gfx->fillCircle(touch.x, touch.y,  5, WY_WHITE);

        display.gfx->fillRect(0, WY_SCREEN_H - 40, WY_SCREEN_W, 40, WY_BLACK);
        display.gfx->setTextColor(WY_YELLOW);
        display.gfx->setTextSize(2);
        display.gfx->setCursor(10, WY_SCREEN_H - 30);
        display.gfx->printf("x=%d  y=%d  pts=%d", touch.x, touch.y, touch.points);
        last_x = touch.x; last_y = touch.y;
        #endif
        Serial.printf("touch x=%d y=%d\n", touch.x, touch.y);
    }

    delay(10);
}
