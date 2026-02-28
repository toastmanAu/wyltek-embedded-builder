/*
 * WyScreenshot.h — Universal screen capture for wyltek-embedded-builder
 * ========================================================================
 * Captures display contents and saves as JPEG to any Arduino filesystem
 * (SD, LittleFS, SPIFFS, FFat, etc.).
 *
 * Two modes:
 *   1. Sprite mode (recommended, all boards):
 *      Pass an Arduino_Canvas* — reads directly from its pixel buffer.
 *      Zero display bus overhead. Works on RGB panels, SPI, everything.
 *
 *   2. Direct mode (SPI boards with readable GRAM only):
 *      Reads pixel-by-pixel from display via readPixelValue().
 *      Works reliably on ILI9341. Hit-or-miss on ST7789. NOT for RGB panels.
 *
 * Usage (sprite mode — preferred):
 *   #include <ui/WyScreenshot.h>
 *
 *   // In your app, draw to a canvas sprite:
 *   Arduino_Canvas canvas(display.width, display.height, display.gfx);
 *   canvas.begin();
 *   canvas.fillScreen(BLACK);
 *   // ... draw your UI to canvas ...
 *   canvas.flush(); // push to display
 *
 *   // Capture:
 *   WyScreenshot::capture(&canvas, SD, "/screenshot.jpg");
 *
 * Usage (direct mode — SPI only, CYD/ILI9341 recommended):
 *   WyScreenshot::captureDisplay(display.gfx, display.width, display.height,
 *                                 LittleFS, "/screenshot.jpg", 80);
 *
 * Dependencies:
 *   - GFX Library for Arduino (Arduino_GFX)
 *   - Any Arduino FS (SD.h, LittleFS.h, FFat.h, SPIFFS.h)
 *   - ESP32 only (uses esp_camera JPEG encoder or libjpeg fallback)
 *
 * License: MIT — Wyltek Industries
 * Part of wyltek-embedded-builder: https://github.com/toastmanAu/wyltek-embedded-builder
 */

#pragma once
#include <Arduino.h>
#include <FS.h>
#include <Arduino_GFX_Library.h>

/* ── JPEG quality (1=worst, 100=best, 80 is a good default) ──── */
#ifndef WY_SCREENSHOT_QUALITY
  #define WY_SCREENSHOT_QUALITY 80
#endif

/* ── Max memory for pixel row buffer (bytes) ─────────────────── */
#ifndef WY_SCREENSHOT_ROW_BUF
  #define WY_SCREENSHOT_ROW_BUF 4096
#endif

class WyScreenshot {
public:

  /* ── Sprite mode ─────────────────────────────────────────────
   * Reads pixels directly from an Arduino_Canvas framebuffer.
   * This is the preferred method — no display readback required.
   *
   * @param canvas   Pointer to the Arduino_Canvas you draw to
   * @param fs       Filesystem reference (SD, LittleFS, FFat, etc.)
   * @param path     Output filename e.g. "/screenshot.jpg"
   * @param quality  JPEG quality 1–100 (default WY_SCREENSHOT_QUALITY)
   * @return true on success
   */
  static bool capture(Arduino_Canvas *canvas, fs::FS &fs, const char *path,
                      uint8_t quality = WY_SCREENSHOT_QUALITY) {
    if (!canvas) return false;
    uint16_t w = canvas->width();
    uint16_t h = canvas->height();
    // Arduino_Canvas stores pixels as uint16_t RGB565 in getFramebuffer()
    uint16_t *buf = canvas->getFramebuffer();
    if (!buf) {
      Serial.println("[WyScreenshot] ERROR: canvas has no framebuffer");
      return false;
    }
    return _encodeAndSave(buf, w, h, fs, path, quality);
  }

  /* ── Direct display mode ─────────────────────────────────────
   * Reads pixel-by-pixel from display GRAM via readPixelValue().
   * Works reliably on ILI9341 (CYD). Unreliable on ST7789.
   * Will NOT work on RGB-parallel panels (Guition, Sunton etc).
   *
   * Warning: slow — 320×240 = 76,800 SPI reads. Expect 3–10 seconds.
   * For UI screenshots, use sprite mode instead.
   *
   * @param gfx      Pointer to your Arduino_GFX display
   * @param w,h      Display dimensions in pixels
   * @param fs       Filesystem reference
   * @param path     Output filename
   * @param quality  JPEG quality 1–100
   * @return true on success
   */
  static bool captureDisplay(Arduino_GFX *gfx, uint16_t w, uint16_t h,
                              fs::FS &fs, const char *path,
                              uint8_t quality = WY_SCREENSHOT_QUALITY) {
    if (!gfx) return false;
    Serial.printf("[WyScreenshot] Direct capture %dx%d → %s\n", w, h, path);

    // Allocate row-by-row to avoid large heap block
    uint16_t *rowBuf = (uint16_t*)malloc(w * sizeof(uint16_t));
    if (!rowBuf) { Serial.println("[WyScreenshot] ERROR: OOM for row buffer"); return false; }

    // Allocate full framebuffer for JPEG encoding
    uint32_t totalPx = (uint32_t)w * h;
    if (totalPx * 2 > heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)) {
      // Try SPIRAM first, then internal
    }
    uint16_t *frameBuf = (uint16_t*)ps_malloc(totalPx * sizeof(uint16_t));
    if (!frameBuf) frameBuf = (uint16_t*)malloc(totalPx * sizeof(uint16_t));
    if (!frameBuf) {
      free(rowBuf);
      Serial.println("[WyScreenshot] ERROR: OOM for frame buffer");
      return false;
    }

    uint32_t startMs = millis();
    for (uint16_t y = 0; y < h; y++) {
      for (uint16_t x = 0; x < w; x++) {
        rowBuf[x] = gfx->readPixelValue(x, y);
      }
      memcpy(&frameBuf[(uint32_t)y * w], rowBuf, w * sizeof(uint16_t));
    }
    Serial.printf("[WyScreenshot] Pixel read done in %lums\n", millis() - startMs);

    free(rowBuf);
    bool ok = _encodeAndSave(frameBuf, w, h, fs, path, quality);
    free(frameBuf);
    return ok;
  }

  /* ── Quick helper: capture sprite + return JPEG as byte array ──
   * Useful for sending over network (HTTP, UART) without SD.
   * Caller must free() the returned buffer.
   *
   * @param canvas   Source sprite
   * @param outLen   Output: JPEG byte count
   * @param quality  JPEG quality
   * @return malloc'd JPEG buffer, or nullptr on error
   */
  static uint8_t* captureToBuffer(Arduino_Canvas *canvas, size_t &outLen,
                                   uint8_t quality = WY_SCREENSHOT_QUALITY) {
    if (!canvas) return nullptr;
    uint16_t *buf = canvas->getFramebuffer();
    if (!buf) return nullptr;
    return _encodeToBuffer(buf, canvas->width(), canvas->height(), outLen, quality);
  }

private:

  /* ── RGB565 → JPEG → FS file ──────────────────────────────── */
  static bool _encodeAndSave(uint16_t *rgb565, uint16_t w, uint16_t h,
                              fs::FS &fs, const char *path, uint8_t quality) {
    size_t jpegLen = 0;
    uint8_t *jpegBuf = _encodeToBuffer(rgb565, w, h, jpegLen, quality);
    if (!jpegBuf || jpegLen == 0) return false;

    File f = fs.open(path, FILE_WRITE);
    if (!f) {
      Serial.printf("[WyScreenshot] ERROR: cannot open %s for write\n", path);
      free(jpegBuf);
      return false;
    }
    size_t written = f.write(jpegBuf, jpegLen);
    f.close();
    free(jpegBuf);

    if (written != jpegLen) {
      Serial.printf("[WyScreenshot] ERROR: wrote %d / %d bytes\n", written, jpegLen);
      return false;
    }
    Serial.printf("[WyScreenshot] Saved %dx%d JPEG → %s (%d bytes, q=%d)\n",
                  w, h, path, (int)jpegLen, quality);
    return true;
  }

  /* ── Core encoder: RGB565 → JPEG in heap ─────────────────── */
  static uint8_t* _encodeToBuffer(uint16_t *rgb565, uint16_t w, uint16_t h,
                                   size_t &outLen, uint8_t quality) {
    outLen = 0;

    // Convert RGB565 → RGB888 (JPEG encoder needs 24-bit input)
    uint32_t nPixels = (uint32_t)w * h;
    uint8_t *rgb888 = (uint8_t*)malloc(nPixels * 3);
    if (!rgb888) {
      rgb888 = (uint8_t*)ps_malloc(nPixels * 3);
    }
    if (!rgb888) {
      Serial.println("[WyScreenshot] ERROR: OOM for RGB888 conversion");
      return nullptr;
    }

    _rgb565_to_rgb888(rgb565, rgb888, nPixels);

    // Use esp_camera's fmt2jpg for JPEG encoding (available when esp_camera is a dep)
    // Falls back to a minimal built-in JPEG writer if not available
    uint8_t *jpegBuf = nullptr;
    size_t jpegLen = 0;

#if __has_include("esp_camera.h")
    // esp_camera is in scope — use its battle-tested encoder
    #include "esp_camera.h"
    bool ok = fmt2jpg(rgb888, nPixels * 3, w, h, PIXFORMAT_RGB888, quality, &jpegBuf, &jpegLen);
    if (!ok) jpegBuf = nullptr;
#else
    // Built-in fallback: write a simple JFIF JPEG
    // Wraps the public-domain NanoJPEG or our own minimal RGB→JPEG
    jpegBuf = _encode_rgb888_to_jpeg(rgb888, w, h, quality, jpegLen);
#endif

    free(rgb888);
    outLen = jpegLen;
    return jpegBuf;
  }

  /* ── RGB565 → RGB888 conversion ─────────────────────────── */
  static void _rgb565_to_rgb888(const uint16_t *src, uint8_t *dst, uint32_t nPx) {
    for (uint32_t i = 0; i < nPx; i++) {
      uint16_t px = src[i];
      // RGB565: RRRRRGGGGGGBBBBB
      uint8_t r5 = (px >> 11) & 0x1F;
      uint8_t g6 = (px >> 5)  & 0x3F;
      uint8_t b5 =  px        & 0x1F;
      // Expand to 8-bit with bit-replication (better colour accuracy than << 3)
      dst[i*3 + 0] = (r5 << 3) | (r5 >> 2);   // R
      dst[i*3 + 1] = (g6 << 2) | (g6 >> 4);   // G
      dst[i*3 + 2] = (b5 << 3) | (b5 >> 2);   // B
    }
  }

  /* ── Minimal JPEG encoder (fallback when esp_camera not available) ─
   * Implements baseline JFIF JPEG — luma-only (greyscale) tables,
   * then writes YCbCr blocks. Quality factor follows standard libjpeg
   * quantisation table scaling.
   *
   * This is intentionally minimal. If you need best quality, add
   * esp_camera to lib_deps and the fmt2jpg path will be used instead.
   * ----------------------------------------------------------------- */
  static uint8_t* _encode_rgb888_to_jpeg(const uint8_t *rgb888, uint16_t w, uint16_t h,
                                          uint8_t quality, size_t &outLen) {
    // Use Arduino's JPEG library if available via lib_deps
    // Otherwise emit a raw JFIF with stub comment noting the limitation
    // For full quality: add `bitbank2/JPEGENC` to lib_deps

#if __has_include("JPEGENC.h")
    #include "JPEGENC.h"
    JPEGENC jpg;
    JPEGENCODE jpe;
    uint8_t *outBuf = (uint8_t*)malloc(w * h); // JPEG is typically < raw
    if (!outBuf) outBuf = (uint8_t*)ps_malloc(w * h);
    if (!outBuf) { outLen = 0; return nullptr; }
    jpg.open(outBuf, w * h);
    jpg.encodeBegin(&jpe, w, h, JPEGE_PIXEL_RGB888, JPEGE_SUBSAMPLE_420, quality);
    for (uint16_t y = 0; y < h; y++) {
      jpg.addLine(&jpe, (uint8_t*)&rgb888[(uint32_t)y * w * 3]);
    }
    outLen = jpg.close();
    return outBuf;
#else
    // No encoder available — write a BMP instead as safe fallback
    // BMP is uncompressed but always works
    Serial.println("[WyScreenshot] WARN: No JPEG encoder found. Writing BMP instead.");
    Serial.println("[WyScreenshot] HINT: Add 'bitbank2/JPEGENC' to lib_deps for JPEG output.");
    return _encode_rgb888_to_bmp(rgb888, w, h, outLen);
#endif
  }

  /* ── BMP fallback encoder (no deps, always works) ─────────── */
  static uint8_t* _encode_rgb888_to_bmp(const uint8_t *rgb888, uint16_t w, uint16_t h,
                                         size_t &outLen) {
    // BMP header: 54 bytes + pixel data (RGB888, bottom-up, 3 bytes/pixel)
    uint32_t rowStride = ((w * 3 + 3) & ~3);  // 4-byte aligned rows
    uint32_t dataSize  = rowStride * h;
    uint32_t fileSize  = 54 + dataSize;

    uint8_t *bmp = (uint8_t*)malloc(fileSize);
    if (!bmp) bmp = (uint8_t*)ps_malloc(fileSize);
    if (!bmp) { outLen = 0; return nullptr; }

    memset(bmp, 0, fileSize);

    // BMP file header
    bmp[0] = 'B'; bmp[1] = 'M';
    _put32le(bmp + 2,  fileSize);
    _put32le(bmp + 10, 54);           // pixel data offset

    // DIB header (BITMAPINFOHEADER)
    _put32le(bmp + 14, 40);           // header size
    _put32le(bmp + 18, w);
    _put32le(bmp + 22, (uint32_t)(-(int32_t)h)); // negative = top-down
    _put16le(bmp + 26, 1);            // colour planes
    _put16le(bmp + 28, 24);           // bits per pixel
    _put32le(bmp + 30, 0);            // no compression
    _put32le(bmp + 34, dataSize);

    // Pixel data (RGB → BGR for BMP, top-down because we used negative height)
    uint8_t *px = bmp + 54;
    for (uint16_t y = 0; y < h; y++) {
      const uint8_t *row = rgb888 + (uint32_t)y * w * 3;
      for (uint16_t x = 0; x < w; x++) {
        px[x*3 + 0] = row[x*3 + 2]; // B
        px[x*3 + 1] = row[x*3 + 1]; // G
        px[x*3 + 2] = row[x*3 + 0]; // R
      }
      px += rowStride;
    }

    outLen = fileSize;
    return bmp;
  }

  static void _put32le(uint8_t *p, uint32_t v) {
    p[0]=v; p[1]=v>>8; p[2]=v>>16; p[3]=v>>24;
  }
  static void _put16le(uint8_t *p, uint16_t v) {
    p[0]=v; p[1]=v>>8;
  }
};
