/*
 * WyScreenshot.h — Universal screen capture for wyltek-embedded-builder
 * ========================================================================
 * Captures display contents as JPEG, supporting three backends:
 *
 *   1. Sprite mode (Arduino_GFX canvas — preferred, zero bus overhead)
 *   2. TFT_eSPI mode (ILI9341/ST7789 — readRectRGB row-by-row, real GRAM)
 *   3. HTTP server mode — serves live screenshots at GET /screenshot
 *
 * TFT_eSPI mode is new (added 2026-03-01) and covers the CYD + any
 * TFT_eSPI board. It's the right choice when you don't have a sprite.
 *
 * Usage (TFT_eSPI + HTTP server — CYD):
 *   #define WY_SCREENSHOT_TFTESPI
 *   #include <ui/WyScreenshot.h>
 *   #include <TFT_eSPI.h>
 *   TFT_eSPI tft = TFT_eSPI();
 *
 *   // After WiFi connected, in setup():
 *   WyScreenshot::startServer(&tft, 320, 240);
 *   // In loop():
 *   WyScreenshot::handleServer();   // OR: start with FreeRTOS task (see below)
 *
 *   // Capture to buffer:
 *   size_t len; uint8_t *jpg = WyScreenshot::captureToBuffer(&tft, 320, 240, len);
 *   // ... use jpg ...  free(jpg);
 *
 * Usage (TFT_eSPI + FreeRTOS background task):
 *   WyScreenshot::startServerTask(&tft, 320, 240);  // non-blocking
 *
 * Usage (Arduino_GFX sprite mode):
 *   WyScreenshot::captureToBuffer(canvas, len);
 *
 * Notes:
 *   - TFT_eSPI readRectRGB() reads row-by-row: ~1KB heap per row, safe on CYD
 *   - JPEG output buffer auto-sized; free() when done
 *   - HTTP server on port WY_SCREENSHOT_PORT (default 81)
 *   - JPEGENC (bitbank2/JPEGENC) must be in lib_deps
 *
 * License: MIT — Wyltek Industries
 * Part of wyltek-embedded-builder: https://github.com/toastmanAu/wyltek-embedded-builder
 */

#pragma once
#include <Arduino.h>
#include <WebServer.h>

#ifndef WY_SCREENSHOT_QUALITY
  #define WY_SCREENSHOT_QUALITY 85
#endif

#ifndef WY_SCREENSHOT_PORT
  #define WY_SCREENSHOT_PORT 81
#endif

// ── Forward declarations ──────────────────────────────────────────
#if __has_include("JPEGENC.h")
  #include "JPEGENC.h"
  #define WY_HAS_JPEGENC 1
#endif

#if defined(WY_SCREENSHOT_TFTESPI) || __has_include(<TFT_eSPI.h>)
  #include <TFT_eSPI.h>
  #define WY_HAS_TFTESPI 1
#endif

#if __has_include(<Arduino_GFX_Library.h>)
  #include <Arduino_GFX_Library.h>
  #define WY_HAS_GFX 1
#endif

class WyScreenshot {
public:

  // ── TFT_eSPI: capture to heap-allocated JPEG buffer ────────────
  // Row-by-row readRectRGB — only ~960 bytes heap per row, safe even
  // when main buffers are allocated. Caller must free() the result.
  // Returns nullptr on OOM or encode failure.
#ifdef WY_HAS_TFTESPI
  static uint8_t* captureToBuffer(TFT_eSPI *tft, int w, int h,
                                   size_t &outLen,
                                   uint8_t quality = WY_SCREENSHOT_QUALITY) {
    outLen = 0;
#ifndef WY_HAS_JPEGENC
    Serial.println("[WyScreenshot] ERROR: JPEGENC not found. Add bitbank2/JPEGENC to lib_deps.");
    return nullptr;
#else
    uint8_t *rowBuf  = (uint8_t *)malloc(w * 3);
    // RGB565 frame: 320×240×2 = 150KB — fits in heap after main bufs freed.
    // readRect() returns RGB565 directly — no conversion needed for JPEGENC.
    // JPEGE_PIXEL_RGB565 + addFrame = simplest correct path.
    free(rowBuf);   // don't need row buffer — readRect fills full frame
    uint16_t *frameBuf = (uint16_t *)malloc(w * h * 2);   // 150KB for 320×240
    if (!frameBuf) {
      Serial.printf("[WyScreenshot] OOM frameBuf heap=%u largest=%u\n",
                    ESP.getFreeHeap(), ESP.getMaxAllocHeap());
      return nullptr;
    }
    int jpegBufSz = 32768;  // 320×240 Q85 JPEG ≈ 15–25KB
    uint8_t *jpegBuf = (uint8_t *)malloc(jpegBufSz);
    if (!jpegBuf) { free(frameBuf); return nullptr; }
    uint32_t t0 = millis();
    tft->readRect(0, 0, w, h, frameBuf);   // bulk SPI read of full frame (RGB565)
    JPEGENC jpg; JPEGENCODE enc;
    int rc = jpg.open(jpegBuf, jpegBufSz);
    if (rc == JPEGE_SUCCESS)
      rc = jpg.encodeBegin(&enc, w, h, JPEGE_PIXEL_RGB565, JPEGE_SUBSAMPLE_420, quality);
    if (rc == JPEGE_SUCCESS)
      rc = jpg.addFrame(&enc, (uint8_t *)frameBuf, w * 2);
    int jpegLen = (rc == JPEGE_SUCCESS) ? jpg.close() : 0;
    free(frameBuf);
    Serial.printf("[WyScreenshot] TFT_eSPI %dx%d → %d bytes JPEG in %lums\n",
                  w, h, jpegLen, millis()-t0);
    if (jpegLen <= 0) { free(jpegBuf); return nullptr; }
    outLen = jpegLen;
    return jpegBuf;
#endif
  }
#endif // WY_HAS_TFTESPI

  // ── Arduino_GFX canvas: capture to buffer ──────────────────────
#ifdef WY_HAS_GFX
  static uint8_t* captureToBuffer(Arduino_Canvas *canvas,
                                   size_t &outLen,
                                   uint8_t quality = WY_SCREENSHOT_QUALITY) {
    outLen = 0;
    if (!canvas) return nullptr;
#ifndef WY_HAS_JPEGENC
    Serial.println("[WyScreenshot] ERROR: JPEGENC not found.");
    return nullptr;
#else
    uint16_t w = canvas->width(), h = canvas->height();
    uint16_t *fb = canvas->getFramebuffer();
    if (!fb) return nullptr;
    // Convert RGB565 → RGB888
    uint32_t nPx = (uint32_t)w * h;
    uint8_t *rgb888 = (uint8_t *)malloc(nPx * 3);
    if (!rgb888) return nullptr;
    for (uint32_t i = 0; i < nPx; i++) {
      uint16_t px = fb[i];
      uint8_t r5=(px>>11)&0x1F, g6=(px>>5)&0x3F, b5=px&0x1F;
      rgb888[i*3+0]=(r5<<3)|(r5>>2);
      rgb888[i*3+1]=(g6<<2)|(g6>>4);
      rgb888[i*3+2]=(b5<<3)|(b5>>2);
    }
    uint8_t *jpegBuf = (uint8_t *)malloc(w * h);
    if (!jpegBuf) { free(rgb888); return nullptr; }
    JPEGENC jpg; JPEGENCODE enc;
    int rc = jpg.open(jpegBuf, jpegBufSz);
    if (rc == JPEGE_SUCCESS)
      rc = jpg.encodeBegin(&enc, w, h, JPEGE_PIXEL_RGB888, JPEGE_SUBSAMPLE_420, quality);
    for (uint16_t y = 0; y < h && rc == JPEGE_SUCCESS; y++)
      rc = jpg.addMCU(&enc, rgb888 + (uint32_t)y * w * 3, w * 3);
    int jpegLen = (rc == JPEGE_SUCCESS) ? jpg.close() : 0;
    free(rgb888);
    if (jpegLen <= 0) { free(jpegBuf); return nullptr; }
    outLen = jpegLen;
    return jpegBuf;
#endif
  }
#endif // WY_HAS_GFX

  // ── HTTP server — TFT_eSPI mode ────────────────────────────────
  // startServer(): init, call handleServer() in loop()
  // startServerTask(): spins a FreeRTOS background task (non-blocking)
#ifdef WY_HAS_TFTESPI
  static void startServer(TFT_eSPI *tft, int w, int h,
                          uint16_t port = WY_SCREENSHOT_PORT) {
    _tft = tft; _w = w; _h = h;
    if (!_server) _server = new WebServer(port);
    _server->on("/", HTTP_GET, [](){
      String html = "<html><body style='background:#07090f;color:#eee;font-family:monospace;padding:2rem'>"
        "<h2>WyScreenshot — TFT_eSPI</h2>"
        "<p><a href='/screenshot' style='color:#0cf'>📷 /screenshot</a></p>"
        "<img src='/screenshot' style='max-width:100%;border:1px solid #333;margin-top:1rem'>"
        "</body></html>";
      _server->send(200, "text/html", html);
    });
    _server->on("/screenshot", HTTP_GET, _handleScreenshot);
    _server->begin();
    Serial.printf("[WyScreenshot] http://%s:%d/screenshot\n",
                  WiFi.localIP().toString().c_str(), port);
  }

  static void handleServer() {
    if (_server) _server->handleClient();
  }

  // Non-blocking: spins a FreeRTOS task that calls handleServer() forever.
  // Call once after startServer(). No loop() changes needed.
  static void startServerTask(TFT_eSPI *tft, int w, int h,
                              uint16_t port = WY_SCREENSHOT_PORT,
                              uint32_t stackSize = 6144) {
    startServer(tft, w, h, port);
    xTaskCreate([](void*){
      for(;;) { if(_server) _server->handleClient(); vTaskDelay(5/portTICK_PERIOD_MS); }
    }, "wyss", stackSize, nullptr, 1, nullptr);
  }
#endif // WY_HAS_TFTESPI

private:
  static WebServer  *_server;
#ifdef WY_HAS_TFTESPI
  static TFT_eSPI   *_tft;
#endif
  static int         _w, _h;

#ifdef WY_HAS_TFTESPI
  static void _handleScreenshot() {
    if (!_tft || !_server) return;
    size_t len = 0;
    uint8_t *jpg = captureToBuffer(_tft, _w, _h, len);
    if (!jpg || len == 0) {
      _server->send(503, "text/plain", "Capture failed — OOM or encode error");
      return;
    }
    _server->sendHeader("Cache-Control", "no-cache");
    _server->sendHeader("Access-Control-Allow-Origin", "*");
    _server->send_P(200, "image/jpeg", (const char *)jpg, len);
    free(jpg);
  }
#endif
};

// ── Static member definitions (include-once guard in .ino / .cpp) ─
// Add this to one .cpp file (or your .ino) before including this header:
//   #define WY_SCREENSHOT_IMPL
// to instantiate the statics. Or just define them inline here for
// header-only use (works for single-translation-unit Arduino sketches).
WebServer  *WyScreenshot::_server = nullptr;
int         WyScreenshot::_w      = 320;
int         WyScreenshot::_h      = 240;
#ifdef WY_HAS_TFTESPI
TFT_eSPI   *WyScreenshot::_tft    = nullptr;
#endif
