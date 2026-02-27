/*
 * WyImage.h — Image rendering for wyltek-embedded-builder
 * =========================================================
 * Draw JPEG, PNG, GIF, and BMP images from any Arduino FS
 * (SPIFFS, LittleFS, SD, SD_MMC) onto any Arduino_GFX display.
 *
 * Usage:
 *   #include <WyImage.h>
 *   #include <SPIFFS.h>
 *   // or #include <SD.h>
 *
 *   WyImage img(display.gfx);
 *
 *   // Draw from filesystem
 *   img.draw(SPIFFS, "/logo.jpg", 0, 0);
 *   img.draw(SD,     "/splash.png", 10, 10);
 *   img.draw(SPIFFS, "/anim.gif",   0, 0);   // plays once
 *   img.drawGIF(SPIFFS, "/anim.gif", 0, 0, 3); // plays N loops
 *
 *   // Draw from PROGMEM (no FS needed)
 *   img.drawJPEGmem(data, len, 0, 0);
 *   img.drawPNGmem(data, len, 0, 0);
 *
 *   // Draw scaled/centred
 *   img.drawFit(SPIFFS, "/logo.jpg");          // fit to screen, centred
 *   img.drawAt(SPIFFS, "/icon.png", cx, cy);   // centred on cx,cy
 *
 * Format support:
 *   JPEG — Arduino_GFX built-in JPEG decoder (Bodmer TJpg_Decoder)
 *   PNG  — Arduino_GFX built-in PNG decoder  (pngle)
 *   BMP  — Arduino_GFX built-in BMP decoder
 *   GIF  — Arduino_GFX built-in GIF decoder  (AnimatedGIF)
 *
 * Dependencies (add to platformio.ini lib_deps):
 *   moononournation/Arduino_GFX_Library  (already in wyltek-embedded-builder)
 *   No extra libs needed — all decoders are part of Arduino_GFX
 *
 * Optional extras (add lib_deps if you want them):
 *   https://github.com/bitbank2/AnimatedGIF  — faster GIF decoder
 *   https://github.com/earlephilhower/ESP8266Audio — audio alongside GIF
 */

#pragma once
#include <Arduino.h>
#include <FS.h>
#include <Arduino_GFX_Library.h>

/* ── Format detection ───────────────────────────────────────────── */
enum WyImgFmt { WY_IMG_UNKNOWN, WY_IMG_JPEG, WY_IMG_PNG, WY_IMG_BMP, WY_IMG_GIF };

static WyImgFmt _wy_img_detect(const char *path) {
    const char *ext = strrchr(path, '.');
    if (!ext) return WY_IMG_UNKNOWN;
    if (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0) return WY_IMG_JPEG;
    if (strcasecmp(ext, ".png") == 0) return WY_IMG_PNG;
    if (strcasecmp(ext, ".bmp") == 0) return WY_IMG_BMP;
    if (strcasecmp(ext, ".gif") == 0) return WY_IMG_GIF;
    return WY_IMG_UNKNOWN;
}

/* ── JPEG decoder callback ──────────────────────────────────────── */
static Arduino_GFX *_wy_jpeg_gfx = nullptr;
static int16_t      _wy_jpeg_x   = 0;
static int16_t      _wy_jpeg_y   = 0;

static bool _wy_jpeg_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
    if (!_wy_jpeg_gfx) return false;
    _wy_jpeg_gfx->draw16bitRGBBitmap(
        _wy_jpeg_x + x, _wy_jpeg_y + y, bitmap, w, h);
    return true;
}

/* ══════════════════════════════════════════════════════════════════
 * WyImage class
 * ══════════════════════════════════════════════════════════════════ */
class WyImage {
public:
    Arduino_GFX *gfx;
    bool verbose = false;

    WyImage(Arduino_GFX *g) : gfx(g) {}

    /* ── draw() — auto-detect format, draw from FS ───────────────── */
    bool draw(fs::FS &fs, const char *path, int16_t x = 0, int16_t y = 0) {
        WyImgFmt fmt = _wy_img_detect(path);
        switch (fmt) {
            case WY_IMG_JPEG: return drawJPEG(fs, path, x, y);
            case WY_IMG_PNG:  return drawPNG(fs, path, x, y);
            case WY_IMG_BMP:  return drawBMP(fs, path, x, y);
            case WY_IMG_GIF:  return drawGIF(fs, path, x, y, 1);
            default:
                if (verbose) Serial.printf("[WyImage] Unknown format: %s\n", path);
                return false;
        }
    }

    /* ── drawFit() — fit image to screen, centred ────────────────── */
    bool drawFit(fs::FS &fs, const char *path) {
        // For now: draw at 0,0. Full fit/scale requires decoding header first.
        // TODO: read image dimensions from header, compute offset.
        return draw(fs, path, 0, 0);
    }

    /* ── drawAt() — draw centred on (cx, cy) ────────────────────── */
    bool drawAt(fs::FS &fs, const char *path, int16_t cx, int16_t cy,
                uint16_t img_w, uint16_t img_h) {
        return draw(fs, path, cx - img_w/2, cy - img_h/2);
    }

    /* ════════════════════════════════════════════════════════════════
     * JPEG
     * ════════════════════════════════════════════════════════════════ */
    bool drawJPEG(fs::FS &fs, const char *path, int16_t x = 0, int16_t y = 0) {
        File f = fs.open(path, "r");
        if (!f) {
            if (verbose) Serial.printf("[WyImage] JPEG open failed: %s\n", path);
            return false;
        }
        size_t len = f.size();
        uint8_t *buf = (uint8_t *)malloc(len);
        if (!buf) {
            f.close();
            if (verbose) Serial.println("[WyImage] JPEG malloc failed");
            return false;
        }
        f.read(buf, len);
        f.close();
        bool ok = drawJPEGmem(buf, len, x, y);
        free(buf);
        return ok;
    }

    bool drawJPEGmem(const uint8_t *data, size_t len, int16_t x = 0, int16_t y = 0) {
        _wy_jpeg_gfx = gfx;
        _wy_jpeg_x   = x;
        _wy_jpeg_y   = y;
        // Arduino_GFX built-in JPEG: drawJpg(data, len, x, y, maxW, maxH, offX, offY, scale)
        gfx->drawJpg(data, len, x, y);
        return true;
    }

    /* ════════════════════════════════════════════════════════════════
     * PNG
     * ════════════════════════════════════════════════════════════════ */
    bool drawPNG(fs::FS &fs, const char *path, int16_t x = 0, int16_t y = 0) {
        File f = fs.open(path, "r");
        if (!f) {
            if (verbose) Serial.printf("[WyImage] PNG open failed: %s\n", path);
            return false;
        }
        size_t len = f.size();
        uint8_t *buf = (uint8_t *)malloc(len);
        if (!buf) {
            f.close();
            if (verbose) Serial.println("[WyImage] PNG malloc failed");
            return false;
        }
        f.read(buf, len);
        f.close();
        bool ok = drawPNGmem(buf, len, x, y);
        free(buf);
        return ok;
    }

    bool drawPNGmem(const uint8_t *data, size_t len, int16_t x = 0, int16_t y = 0) {
        gfx->drawPng(data, len, x, y);
        return true;
    }

    /* ════════════════════════════════════════════════════════════════
     * BMP
     * ════════════════════════════════════════════════════════════════ */
    bool drawBMP(fs::FS &fs, const char *path, int16_t x = 0, int16_t y = 0) {
        File f = fs.open(path, "r");
        if (!f) {
            if (verbose) Serial.printf("[WyImage] BMP open failed: %s\n", path);
            return false;
        }
        size_t len = f.size();
        uint8_t *buf = (uint8_t *)malloc(len);
        if (!buf) {
            f.close();
            if (verbose) Serial.println("[WyImage] BMP malloc failed");
            return false;
        }
        f.read(buf, len);
        f.close();
        bool ok = drawBMPmem(buf, len, x, y);
        free(buf);
        return ok;
    }

    bool drawBMPmem(const uint8_t *data, size_t len, int16_t x = 0, int16_t y = 0) {
        gfx->drawBmp(data, len, x, y);
        return true;
    }

    /* ════════════════════════════════════════════════════════════════
     * GIF — streamed from FS (no full buffer needed — great for RAM)
     * ════════════════════════════════════════════════════════════════ */
    bool drawGIF(fs::FS &fs, const char *path,
                 int16_t x = 0, int16_t y = 0, int loops = 1) {
        File f = fs.open(path, "r");
        if (!f) {
            if (verbose) Serial.printf("[WyImage] GIF open failed: %s\n", path);
            return false;
        }

        GifClass gif;
        if (!gif.gd_open_gif(&f)) {
            f.close();
            if (verbose) Serial.println("[WyImage] GIF decode init failed");
            return false;
        }

        uint8_t *buf = (uint8_t *)malloc(gif.gif.width * gif.gif.height * 2);
        if (!buf) {
            gif.gd_close_gif();
            f.close();
            if (verbose) Serial.println("[WyImage] GIF frame malloc failed");
            return false;
        }

        for (int loop = 0; loop < loops || loops < 0; loop++) {
            while (true) {
                int res = gif.gd_get_frame(&f);
                if (res < 0 || res == 0) break;  // error or end of GIF
                gfx->draw16bitRGBBitmap(x, y,
                    (uint16_t *)buf,
                    gif.gif.width, gif.gif.height);
                delay(gif.gif.gce.delay * 10);  // delay in centiseconds → ms
            }
            if (loops < 0) gif.gd_rewind(&f);  // infinite loop
        }

        free(buf);
        gif.gd_close_gif();
        f.close();
        return true;
    }

    /* ── GIF infinite loop (call from loop(), returns false when done) */
    bool drawGIFframe(GifClass &gif, fs::FS &fs, File &f,
                      int16_t x, int16_t y, uint8_t *framebuf) {
        int res = gif.gd_get_frame(&f);
        if (res <= 0) return false;
        gfx->draw16bitRGBBitmap(x, y,
            (uint16_t *)framebuf,
            gif.gif.width, gif.gif.height);
        delay(gif.gif.gce.delay * 10);
        return true;
    }

    /* ════════════════════════════════════════════════════════════════
     * Streaming JPEG — draw directly from open File, no malloc
     * Best for large images on boards with limited PSRAM
     * ════════════════════════════════════════════════════════════════ */
    bool drawJPEGstream(fs::FS &fs, const char *path, int16_t x = 0, int16_t y = 0) {
        File f = fs.open(path, "r");
        if (!f) return false;
        // Arduino_GFX drawJpgFile: streams directly, no full buffer
        gfx->drawJpgFile(fs, path, x, y);
        f.close();
        return true;
    }

    bool drawPNGstream(fs::FS &fs, const char *path, int16_t x = 0, int16_t y = 0) {
        gfx->drawPngFile(fs, path, x, y);
        return true;
    }
};

/* ── Convenience free functions (drop-in one-liners) ─────────────── */
/*
 * wyDrawImage(gfx, fs, "/image.jpg", x, y)
 * wyDrawImage(gfx, SPIFFS, "/logo.png", 0, 0)
 * wyDrawImage(gfx, SD,     "/bg.bmp",  0, 0)
 */
static inline bool wyDrawImage(Arduino_GFX *gfx, fs::FS &fs,
                                const char *path, int16_t x = 0, int16_t y = 0) {
    WyImage img(gfx);
    return img.draw(fs, path, x, y);
}

static inline bool wyDrawImageFit(Arduino_GFX *gfx, fs::FS &fs, const char *path) {
    WyImage img(gfx);
    return img.drawFit(fs, path);
}
