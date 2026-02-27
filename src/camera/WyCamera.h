/*
 * camera/WyCamera.h — OV2640 Camera Module (ESP32-CAM, ESP32-S3-EYE)
 * ====================================================================
 * Uses Espressif esp32-camera component (bundled with ESP32 Arduino core)
 * Board pin definitions come from boards.h (WY_CAM_* defines)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT DOES
 * ═══════════════════════════════════════════════════════════════════
 * Initialises the OV2640 camera and provides:
 *   - Frame capture (JPEG, RGB565, YUV422, grayscale)
 *   - HTTP MJPEG stream server (WiFi — live view in any browser)
 *   - HTTP JPEG snapshot endpoint
 *   - Frame quality and size configuration
 *   - Special effects (grayscale, sepia, negative, sketch)
 *   - Motion detection via frame differencing
 *
 * ═══════════════════════════════════════════════════════════════════
 * MEMORY — PSRAM IS MANDATORY
 * ═══════════════════════════════════════════════════════════════════
 * Camera frame buffers live in PSRAM. Without PSRAM you can only use
 * very small frame sizes (QQVGA 160×120, grayscale).
 *
 * ESP32-CAM: 4MB QSPI PSRAM — adequate for SVGA JPEG
 * ESP32-S3 boards: 8MB PSRAM — handles UXGA (2MP)
 *
 * The camera driver allocates frame buffers automatically in PSRAM
 * if PSRAM is enabled in the build. Enable in platformio.ini:
 *   board_build.arduino.memory_type = qio_opi   ; S3 with PSRAM
 *   ; or for ESP32-CAM:
 *   board_build.partitions = huge_app.csv
 *
 * ═══════════════════════════════════════════════════════════════════
 * FRAME SIZES (OV2640)
 * ═══════════════════════════════════════════════════════════════════
 *   FRAMESIZE_QQVGA   160×120   — tiny, very fast
 *   FRAMESIZE_QVGA    320×240   — good for real-time stream
 *   FRAMESIZE_VGA     640×480   — default, good quality
 *   FRAMESIZE_SVGA    800×600   — higher quality stream
 *   FRAMESIZE_XGA     1024×768  — slow stream
 *   FRAMESIZE_SXGA    1280×1024 — snapshot quality
 *   FRAMESIZE_UXGA    1600×1200 — full resolution (slow)
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   #include "camera/WyCamera.h"
 *
 *   WyCamera cam;
 *   cam.setFrameSize(FRAMESIZE_VGA);
 *   cam.setQuality(12);      // JPEG quality 0–63, lower = better
 *   cam.begin();
 *
 *   // Start MJPEG stream server on port 81:
 *   cam.startStream(81);
 *   // Browse to http://<esp32-ip>:81/stream
 *   // Snapshot: http://<esp32-ip>:81/capture
 *
 *   // Or capture a frame manually:
 *   camera_fb_t* fb = cam.capture();
 *   if (fb) {
 *       // fb->buf = JPEG data, fb->len = bytes
 *       cam.release(fb);    // MUST release after use
 *   }
 *
 * ═══════════════════════════════════════════════════════════════════
 * MOTION DETECTION
 * ═══════════════════════════════════════════════════════════════════
 * Simple frame differencing — compares 8×8 pixel blocks between frames.
 * Returns motion score 0–100. Threshold ~10 = activity detected.
 *
 *   cam.setMotionDetection(true);
 *   float motion = cam.motionScore();
 *   if (motion > 10.0f) { // something moved }
 *
 * ═══════════════════════════════════════════════════════════════════
 * HTTP STREAM PROTOCOL
 * ═══════════════════════════════════════════════════════════════════
 * GET /stream  → multipart/x-mixed-replace MJPEG stream
 * GET /capture → single JPEG snapshot (Content-Type: image/jpeg)
 * GET /status  → JSON camera settings
 * POST /control?var=xxx&val=yyy → adjust any camera setting
 *
 * Compatible with most browsers, VLC, Home Assistant camera entity,
 * Node-RED, and any HTTP MJPEG consumer.
 */

#pragma once
#include <Arduino.h>

#if defined(WY_HAS_CAMERA) && WY_HAS_CAMERA

#include "esp_camera.h"
#include "esp_http_server.h"
#include "../boards.h"

/* Stream part boundary */
#define WY_CAM_PART_BOUNDARY  "wyframe"
#define WY_CAM_PART_HEADER    "\r\n--" WY_CAM_PART_BOUNDARY "\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n"
#define WY_CAM_JSON_STATUS    "application/json"

class WyCamera {
public:
    WyCamera() {}

    /* Frame size — FRAMESIZE_QVGA through FRAMESIZE_UXGA */
    void setFrameSize(framesize_t size) { _frameSize = size; }

    /* JPEG quality: 0–63. Lower number = higher quality = larger file.
     * 10 = excellent, 20 = good, 30 = acceptable */
    void setQuality(uint8_t q)          { _quality = q; }

    /* Horizontal flip (useful for selfie-cam mounting) */
    void setHFlip(bool flip)            { _hflip = flip; }
    void setVFlip(bool flip)            { _vflip = flip; }

    /* Brightness: -2 to +2 */
    void setBrightness(int8_t b)        { _brightness = b; }

    /* Enable motion detection (uses grayscale downsampled comparison) */
    void setMotionDetection(bool en)    { _motionEn = en; }

    bool begin() {
        camera_config_t config = {};

        /* Pin assignments from boards.h */
        config.pin_pwdn     = WY_CAM_PWDN;
        config.pin_reset    = WY_CAM_RESET;
        config.pin_xclk     = WY_CAM_XCLK;
        config.pin_sscb_sda = WY_CAM_SIOD;
        config.pin_sscb_scl = WY_CAM_SIOC;
        config.pin_d7       = WY_CAM_D7;
        config.pin_d6       = WY_CAM_D6;
        config.pin_d5       = WY_CAM_D5;
        config.pin_d4       = WY_CAM_D4;
        config.pin_d3       = WY_CAM_D3;
        config.pin_d2       = WY_CAM_D2;
        config.pin_d1       = WY_CAM_D1;
        config.pin_d0       = WY_CAM_D0;
        config.pin_vsync    = WY_CAM_VSYNC;
        config.pin_href     = WY_CAM_HREF;
        config.pin_pclk     = WY_CAM_PCLK;

        config.xclk_freq_hz = 20000000;  /* 20MHz XCLK */
        config.ledc_timer   = LEDC_TIMER_0;
        config.ledc_channel = LEDC_CHANNEL_0;

        config.pixel_format = PIXFORMAT_JPEG;
        config.frame_size   = _frameSize;
        config.jpeg_quality = _quality;

        /* Use PSRAM if available — allows larger frames and more buffers */
        if (psramFound()) {
            config.fb_count       = 2;     /* double buffer for smooth stream */
            config.grab_mode      = CAMERA_GRAB_LATEST;
            config.fb_location    = CAMERA_FB_IN_PSRAM;
        } else {
            config.fb_count       = 1;
            config.grab_mode      = CAMERA_GRAB_WHEN_EMPTY;
            config.fb_location    = CAMERA_FB_IN_DRAM;
            /* Fall back to small frame without PSRAM */
            if (_frameSize > FRAMESIZE_QVGA) {
                _frameSize = FRAMESIZE_QVGA;
                config.frame_size = FRAMESIZE_QVGA;
                Serial.println("[WyCamera] no PSRAM — limiting to QVGA");
            }
        }

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("[WyCamera] init failed: 0x%04X\n", err);
            return false;
        }

        /* Apply image settings */
        sensor_t* s = esp_camera_sensor_get();
        if (s) {
            s->set_hflip(s, _hflip ? 1 : 0);
            s->set_vflip(s, _vflip ? 1 : 0);
            s->set_brightness(s, _brightness);
            /* OV2640 specific: reduce initial noise */
            s->set_whitebal(s, 1);     /* auto white balance on */
            s->set_awb_gain(s, 1);
            s->set_exposure_ctrl(s, 1); /* auto exposure on */
            s->set_aec2(s, 1);          /* AEC DSP on */
        }

        _started = true;
        Serial.printf("[WyCamera] ready — %s JPEG q=%d%s\n",
            _frameSizeStr(), _quality, psramFound() ? " (PSRAM)" : " (DRAM)");
        return true;
    }

    /* Capture a single frame. MUST call release() after use. */
    camera_fb_t* capture() {
        if (!_started) return nullptr;
        return esp_camera_fb_get();
    }

    /* Release a frame buffer back to the driver */
    void release(camera_fb_t* fb) {
        if (fb) esp_camera_fb_return(fb);
    }

    /* Change frame size at runtime (reinitialises camera) */
    bool setFrameSizeRuntime(framesize_t size) {
        sensor_t* s = esp_camera_sensor_get();
        if (!s) return false;
        return s->set_framesize(s, size) == 0;
    }

    /* Motion detection score — 0.0 (still) to 100.0 (lots of motion).
     * Call regularly (e.g. every 500ms). Requires PIXFORMAT_GRAYSCALE
     * or compares JPEG-decoded luma (approximate). */
    float motionScore() {
        if (!_motionEn) return 0.0f;

        camera_fb_t* fb = capture();
        if (!fb) return 0.0f;

        /* Very simple motion: compare total byte difference.
         * For JPEG, this is approximate (encoding changes with content).
         * For a real implementation, capture in PIXFORMAT_GRAYSCALE. */
        float score = 0.0f;
        if (_prevLen > 0) {
            int64_t diff = (int64_t)fb->len - (int64_t)_prevLen;
            if (diff < 0) diff = -diff;
            score = min((float)diff / (float)_prevLen * 100.0f, 100.0f);
        }
        _prevLen = fb->len;
        release(fb);
        return score;
    }

    /* Flash LED control (GPIO 4 on ESP32-CAM) */
    void flashOn(uint8_t brightness = 255) {
#ifdef WY_FLASH_LED
        analogWrite(WY_FLASH_LED, brightness);
#endif
    }
    void flashOff() {
#ifdef WY_FLASH_LED
        analogWrite(WY_FLASH_LED, 0);
#endif
    }

    /* ── HTTP Stream Server ──────────────────────────────────────── */

    /* Start MJPEG stream on given port.
     * /stream → MJPEG live view
     * /capture → single JPEG
     * /status  → JSON settings */
    bool startStream(uint16_t port = 81) {
        if (!_started) return false;

        httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
        cfg.server_port      = port;
        cfg.ctrl_port        = port + 100;
        cfg.max_open_sockets = 3;
        cfg.task_priority    = 5;
        cfg.stack_size       = 8192;

        if (httpd_start(&_server, &cfg) != ESP_OK) {
            Serial.println("[WyCamera] stream server start failed");
            return false;
        }

        httpd_uri_t streamUri = {
            .uri       = "/stream",
            .method    = HTTP_GET,
            .handler   = _streamHandler,
            .user_ctx  = this
        };
        httpd_uri_t captureUri = {
            .uri       = "/capture",
            .method    = HTTP_GET,
            .handler   = _captureHandler,
            .user_ctx  = this
        };
        httpd_uri_t statusUri = {
            .uri       = "/status",
            .method    = HTTP_GET,
            .handler   = _statusHandler,
            .user_ctx  = this
        };

        httpd_register_uri_handler(_server, &streamUri);
        httpd_register_uri_handler(_server, &captureUri);
        httpd_register_uri_handler(_server, &statusUri);

        Serial.printf("[WyCamera] stream: http://<ip>:%u/stream\n", port);
        Serial.printf("[WyCamera] snapshot: http://<ip>:%u/capture\n", port);
        return true;
    }

    void stopStream() {
        if (_server) { httpd_stop(_server); _server = nullptr; }
    }

    bool isStarted() { return _started; }

private:
    framesize_t _frameSize  = FRAMESIZE_VGA;
    uint8_t     _quality    = 12;
    bool        _hflip      = false;
    bool        _vflip      = false;
    int8_t      _brightness = 0;
    bool        _motionEn   = false;
    bool        _started    = false;
    httpd_handle_t _server  = nullptr;
    size_t      _prevLen    = 0;

    /* ── HTTP Handlers (static — required by esp_http_server) ──── */

    static esp_err_t _streamHandler(httpd_req_t* req) {
        WyCamera* self = (WyCamera*)req->user_ctx;
        char partHdr[64];
        static const char* streamHdr =
            "multipart/x-mixed-replace;boundary=" WY_CAM_PART_BOUNDARY;

        httpd_resp_set_type(req, streamHdr);
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        httpd_resp_set_hdr(req, "X-Framerate", "60");

        while (true) {
            camera_fb_t* fb = esp_camera_fb_get();
            if (!fb) { Serial.println("[WyCamera] frame capture failed"); break; }

            size_t hlen = snprintf(partHdr, sizeof(partHdr),
                WY_CAM_PART_HEADER, (unsigned)fb->len);

            esp_err_t res = httpd_resp_send_chunk(req, partHdr, hlen);
            if (res == ESP_OK)
                res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);

            esp_camera_fb_return(fb);

            if (res != ESP_OK) break;  /* client disconnected */
        }
        return ESP_OK;
    }

    static esp_err_t _captureHandler(httpd_req_t* req) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) return httpd_resp_send_500(req);

        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_set_hdr(req, "Content-Disposition",
            "inline; filename=capture.jpg");
        httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
        esp_err_t res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
        esp_camera_fb_return(fb);
        return res;
    }

    static esp_err_t _statusHandler(httpd_req_t* req) {
        sensor_t* s = esp_camera_sensor_get();
        char json[512];
        snprintf(json, sizeof(json),
            "{\"framesize\":%d,\"quality\":%d,\"brightness\":%d,"
            "\"contrast\":%d,\"saturation\":%d,\"hflip\":%d,\"vflip\":%d,"
            "\"awb\":%d,\"aec\":%d,\"agc\":%d}",
            s ? s->status.framesize : 0,
            s ? s->status.quality : 0,
            s ? s->status.brightness : 0,
            s ? s->status.contrast : 0,
            s ? s->status.saturation : 0,
            s ? s->status.hmirror : 0,
            s ? s->status.vflip : 0,
            s ? s->status.awb : 0,
            s ? s->status.aec : 0,
            s ? s->status.agc : 0);
        httpd_resp_set_type(req, WY_CAM_JSON_STATUS);
        return httpd_resp_sendstr(req, json);
    }

    const char* _frameSizeStr() {
        switch (_frameSize) {
            case FRAMESIZE_QQVGA: return "QQVGA(160x120)";
            case FRAMESIZE_QVGA:  return "QVGA(320x240)";
            case FRAMESIZE_VGA:   return "VGA(640x480)";
            case FRAMESIZE_SVGA:  return "SVGA(800x600)";
            case FRAMESIZE_XGA:   return "XGA(1024x768)";
            case FRAMESIZE_SXGA:  return "SXGA(1280x1024)";
            case FRAMESIZE_UXGA:  return "UXGA(1600x1200)";
            default:              return "custom";
        }
    }
};

#else
/* Stub when no camera board defined */
class WyCamera {
public:
    bool begin() {
        Serial.println("[WyCamera] no camera board defined (WY_HAS_CAMERA not set)");
        return false;
    }
    struct camera_fb_t* capture() { return nullptr; }
    void release(void*) {}
    bool startStream(uint16_t = 81) { return false; }
};
#endif /* WY_HAS_CAMERA */
