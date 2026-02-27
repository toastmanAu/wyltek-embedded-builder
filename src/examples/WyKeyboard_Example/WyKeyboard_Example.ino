/*
 * WyKeyboard_Example.ino
 * ======================
 * Demonstrates WyKeyboard on any wyltek-embedded-builder board.
 *
 * Shows:
 *   1. Text input (WiFi SSID)
 *   2. Password input (masked)
 *   3. Numeric input
 *   4. Layout switching (QWERTY → symbols → back)
 *
 * Define your board before including wyltek.h:
 *   #define WY_BOARD_GUITION_4848S040   // 480×480 RGB
 *   #define WY_BOARD_CYD_28            // 320×240 SPI
 *   #define WY_BOARD_SUNTON_8048S043   // 800×480 RGB
 *   etc.
 */

#define WY_BOARD_CYD_28   /* change to your board */

#include <wyltek.h>
#include <display/WyDisplay.h>
#include <display/WyFont.h>
#include <touch/WyTouch.h>
#include <ui/WyKeyboard.h>

WyDisplay  display;
WyTouch    touch;
WyFont     font;
WyKeyboard kb;

/* Demo state */
enum DemoState { DEMO_SSID, DEMO_PASSWORD, DEMO_NUMBER, DEMO_RESULT };
DemoState state = DEMO_SSID;

char ssid[64]     = "";
char password[64] = "";
char number[32]   = "";

void drawPromptScreen(const char *title, const char *body) {
    display.gfx->fillScreen(0x0000);
    font.begin(display.gfx);
    font.centreH(title, 0, 0, display.width, 0,
                 0xFFFF, nullptr, 2);
    font.centreH(body, 0, 30, display.width, 0,
                 0x8C71, nullptr, 1);
}

void startSSID() {
    state = DEMO_SSID;
    drawPromptScreen("WiFi Setup", "");
    kb.show("WiFi network name:", 32, false, WY_KB_QWERTY);
}

void startPassword() {
    state = DEMO_PASSWORD;
    drawPromptScreen("WiFi Setup", ssid);
    kb.show("Password:", 32, true, WY_KB_QWERTY);
}

void startNumber() {
    state = DEMO_NUMBER;
    drawPromptScreen("Port Number", "");
    kb.show("Port (1-65535):", 5, false, WY_KB_NUMERIC);
}

void showResult() {
    state = DEMO_RESULT;
    display.gfx->fillScreen(0x0000);
    font.begin(display.gfx);

    font.centreH("Input Complete", 0, 0, display.width, 0,
                 0x07E0, nullptr, 2);  /* green */

    char line[80];
    snprintf(line, sizeof(line), "SSID: %s", ssid);
    display.gfx->setTextColor(0xFFFF);
    display.gfx->setCursor(10, 60);
    display.gfx->print(line);

    snprintf(line, sizeof(line), "Pass: %s", password[0] ? "****" : "(none)");
    display.gfx->setCursor(10, 80);
    display.gfx->print(line);

    snprintf(line, sizeof(line), "Port: %s", number);
    display.gfx->setCursor(10, 100);
    display.gfx->print(line);

    display.gfx->setCursor(10, 140);
    display.gfx->setTextColor(0x8C71);
    display.gfx->print("Tap to restart demo");
}

void setup() {
    Serial.begin(115200);

    display.begin();
    touch.begin();
    font.begin(display.gfx);

    kb.begin(display.gfx, display.width, display.height);

    startSSID();
}

void loop() {
    int tx, ty;
    bool touched = touch.getXY(&tx, &ty);

    if (kb.active() && touched) {
        WyKbResult res = kb.press(tx, ty);

        if (res == WY_KB_DONE) {
            switch (state) {
                case DEMO_SSID:
                    strncpy(ssid, kb.value(), sizeof(ssid) - 1);
                    startPassword();
                    break;
                case DEMO_PASSWORD:
                    strncpy(password, kb.value(), sizeof(password) - 1);
                    startNumber();
                    break;
                case DEMO_NUMBER:
                    strncpy(number, kb.value(), sizeof(number) - 1);
                    showResult();
                    break;
                default:
                    break;
            }
        } else if (res == WY_KB_CANCEL) {
            /* Restart demo */
            startSSID();
        }
    }

    /* Tap result screen to restart */
    if (state == DEMO_RESULT && touched) {
        delay(200);
        startSSID();
    }

    delay(10);
}
