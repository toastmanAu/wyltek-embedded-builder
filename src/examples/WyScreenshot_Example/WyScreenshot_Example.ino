/*
 * WyScreenshot_Example.ino
 * ========================
 * Demonstrates WyScreenshot on a CYD (ESP32-2432S028R) with SD card.
 * Draws a UI, captures it to /screenshot.jpg on SD.
 *
 * For boards without SD: swap SD for LittleFS —
 *   #include <LittleFS.h>
 *   LittleFS.begin(true);
 *   WyScreenshot::capture(&canvas, LittleFS, "/screenshot.jpg");
 *
 * JPEG encoder priority:
 *   1. esp_camera (if in lib_deps) — best quality
 *   2. bitbank2/JPEGENC (if in lib_deps) — good quality, no camera dep
 *   3. Built-in BMP fallback — always works, larger files
 *
 * lib_deps recommendation:
 *   moononournation/GFX Library for Arduino
 *   bitbank2/JPEGENC          ← add this for JPEG output
 *
 * Board: WY_BOARD_CYD
 * Platform: espressif32
 * Framework: Arduino
 */

#define WY_BOARD_CYD

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <display/WyDisplay.h>
#include <ui/WyScreenshot.h>

WyDisplay display;

// Canvas sprite — draw your UI here, flush to display
// Screenshot reads from this buffer directly (fast, no SPI readback)
Arduino_Canvas *canvas = nullptr;

void drawSampleUI() {
  canvas->fillScreen(0x0000);                              // black
  canvas->fillRect(0, 0, canvas->width(), 40, 0x001F);    // blue header bar
  canvas->setTextColor(0xFFFF);
  canvas->setTextSize(2);
  canvas->setCursor(10, 12);
  canvas->print("Wyltek Industries");

  canvas->setTextColor(0x07E0);  // green
  canvas->setTextSize(1);
  canvas->setCursor(10, 60);
  canvas->print("WyScreenshot test — this is a canvas capture");

  // Draw some coloured rectangles as UI elements
  canvas->fillRoundRect(10, 90, 80, 40, 6, 0xF800);   // red
  canvas->fillRoundRect(100, 90, 80, 40, 6, 0x07E0);  // green
  canvas->fillRoundRect(190, 90, 80, 40, 6, 0x001F);  // blue
  canvas->fillRoundRect(100, 150, 120, 30, 5, 0xFFE0); // yellow
  canvas->setTextColor(0x0000);
  canvas->setTextSize(1);
  canvas->setCursor(120, 162);
  canvas->print("SCREENSHOT");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("WyScreenshot Example");

  // Init display
  display.begin();

  // Init canvas sprite at full display resolution
  canvas = new Arduino_Canvas(display.width, display.height, display.gfx);
  canvas->begin();

  // Init SD card (CYD has SD on GPIO 5 CS)
  if (!SD.begin(5)) {
    Serial.println("SD init failed — check card");
    display.gfx->fillScreen(0xF800);
    display.gfx->setTextColor(0xFFFF);
    display.gfx->setCursor(10, 10);
    display.gfx->println("SD FAILED");
    return;
  }
  Serial.println("SD OK");

  // Draw UI to canvas
  drawSampleUI();

  // Flush canvas to display (user sees the UI)
  canvas->flush();

  // ── Capture screenshot ─────────────────────────────────────
  Serial.println("Capturing screenshot...");
  bool ok = WyScreenshot::capture(canvas, SD, "/screenshot.jpg");

  if (ok) {
    Serial.println("Screenshot saved: /screenshot.jpg");
    // Show brief success indicator
    canvas->fillRect(0, display.height - 30, display.width, 30, 0x07E0);
    canvas->setTextColor(0x0000);
    canvas->setTextSize(1);
    canvas->setCursor(10, display.height - 18);
    canvas->print("Saved: /screenshot.jpg");
    canvas->flush();
  } else {
    Serial.println("Screenshot FAILED");
  }

  // ── Optional: also capture to buffer and print size ────────
  size_t jpegLen = 0;
  uint8_t *jpegBuf = WyScreenshot::captureToBuffer(canvas, jpegLen);
  if (jpegBuf) {
    Serial.printf("In-memory JPEG: %d bytes\n", (int)jpegLen);
    // Could send over WiFi here: client.write(jpegBuf, jpegLen);
    free(jpegBuf);
  }
}

void loop() {
  // Nothing — screenshot was one-shot in setup
  delay(10000);
}
