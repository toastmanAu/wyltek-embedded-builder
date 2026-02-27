/*
 * DoubleEyeDemo — Animated robot eyes on dual GC9A01 round displays
 * =================================================================
 * Board: Double EYE module (WY_BOARD_DOUBLE_EYE)
 *
 * platformio.ini:
 *   [env:double_eye]
 *   platform      = espressif32
 *   board         = esp32dev
 *   framework     = arduino
 *   monitor_speed = 115200
 *   build_flags   =
 *     -DWY_BOARD_DOUBLE_EYE
 *     ; Optional pin overrides:
 *     ; -DWY_DISPLAY_DC=2
 *     ; -DWY_DISPLAY_CS=5       (CS1 — left eye)
 *     ; -DWY_EYE_CS2=15         (CS2 — right eye)
 *     ; -DWY_DISPLAY_SCK=18
 *     ; -DWY_DISPLAY_MOSI=23
 *     ; -DWY_DISPLAY_RST=4
 *     ; -DWY_DISPLAY_BL=21
 */

#include "ui/WyEyes.h"

WyEyes eyes;

/* Expression sequence for demo */
const WyExpression demoSequence[] = {
    EYES_IDLE,
    EYES_LOOK_LEFT,
    EYES_LOOK_RIGHT,
    EYES_LOOK_UP,
    EYES_LOOK_DOWN,
    EYES_BLINK,
    EYES_HAPPY,
    EYES_IDLE,
    EYES_ANGRY,
    EYES_SAD,
    EYES_SURPRISED,
    EYES_SLEEPY,
    EYES_IDLE,
    EYES_DEAD,
    EYES_IDLE,
};
const uint8_t demoLen = sizeof(demoSequence) / sizeof(demoSequence[0]);
uint8_t demoIdx = 0;
uint32_t lastChange = 0;

void setup() {
    Serial.begin(115200);
    randomSeed(analogRead(0));

    eyes.setIrisColor(EYES_CYAN);   /* cyan iris */
    eyes.begin();

    Serial.println("Double EYE demo running");
    Serial.println("Open serial monitor to see expression names");
}

void loop() {
    /* Cycle through expressions every 2.5 seconds */
    if (millis() - lastChange > 2500) {
        WyExpression e = demoSequence[demoIdx];
        eyes.setExpression(e);

        const char* names[] = {
            "IDLE","BLINK","LOOK_LEFT","LOOK_RIGHT","LOOK_UP","LOOK_DOWN",
            "ANGRY","HAPPY","SAD","SURPRISED","SLEEPY","DEAD"
        };
        if (e < EYES_EXPRESSION_COUNT) {
            Serial.printf("[eyes] %s\n", names[e]);
        }

        demoIdx = (demoIdx + 1) % demoLen;
        lastChange = millis();
    }

    eyes.update();
    delay(16);   /* ~60fps */
}
