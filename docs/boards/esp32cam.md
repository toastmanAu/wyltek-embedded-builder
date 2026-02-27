# ESP32-CAM (AI-Thinker) + OV2640
**Module:** `src/camera/WyCamera.h` — Board: `WY_BOARD_ESP32CAM`

---

## What it is
The AI-Thinker ESP32-CAM is a $5 board combining an ESP32-S (single-core), 4MB flash, 4MB QSPI PSRAM, and an OV2640 2MP camera module. No USB — programs via FTDI adapter. No display — output goes over WiFi.

What you get from `WyCamera`:
- MJPEG live stream at `/stream` — open in any browser or VLC
- JPEG snapshot at `/capture`
- JSON settings at `/status`
- Motion detection via frame differencing
- Flash LED control

## ⚠️ GPIO 4 — shared between flash LED and SD card CS

GPIO 4 is wired to both the bright white flash LED **and** the SD card CS pin. You can't use both simultaneously. Choose one:
- Flash LED: `cam.flashOn()` / `cam.flashOff()`
- SD card: initialise SPIFFS instead, or drive GPIO4 HIGH (deselects SD) before using flash

This is a hardware design quirk on the AI-Thinker module — not fixable in software.

## Programming — no USB, needs FTDI

The ESP32-CAM has no onboard USB-to-serial. You need a 3.3V/5V FTDI adapter:

```
FTDI GND → ESP32-CAM GND
FTDI TX  → GPIO3 (RXD)
FTDI RX  → GPIO1 (TXD)
FTDI 5V  → 5V (or VCC)
IO0      → GND  ← only during upload, remove after!
```

**Upload procedure:**
1. Connect IO0 to GND
2. Power cycle (or press RESET)
3. Upload firmware
4. Disconnect IO0 from GND
5. Press RESET to run

If you skip step 4, it stays in bootloader forever.

## platformio.ini
```ini
[env:esp32cam]
platform   = espressif32
board      = esp32cam
framework  = arduino
monitor_speed = 115200
build_flags =
    -DWY_BOARD_ESP32CAM
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
board_upload.flash_size  = 4MB
board_build.partitions   = huge_app.csv
```

`-mfix-esp32-psram-cache-issue` — required workaround for ESP32-CAM PSRAM cache bug. Without it, random crashes occur when PSRAM and WiFi are both active.

## Frame sizes
| Size | Resolution | Notes |
|------|------------|-------|
| `FRAMESIZE_QQVGA` | 160×120 | Works without PSRAM |
| `FRAMESIZE_QVGA` | 320×240 | Good for slow connections |
| `FRAMESIZE_VGA` | 640×480 | **Default — good balance** |
| `FRAMESIZE_SVGA` | 800×600 | Higher quality stream |
| `FRAMESIZE_SXGA` | 1280×1024 | Snapshots only (stream too slow) |
| `FRAMESIZE_UXGA` | 1600×1200 | Full 2MP — snapshots only |

## Basic usage
```cpp
#include "camera/WyCamera.h"

WyCamera cam;

void setup() {
    cam.setFrameSize(FRAMESIZE_VGA);
    cam.setQuality(12);   // JPEG: 0=best quality, 63=worst
    cam.begin();

    WiFi.begin("ssid", "pass");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    cam.startStream(81);
    // Browse to http://<ip>:81/stream
}
```

## JPEG quality
Lower number = better quality = larger file = slower stream:

| Quality | File size (VGA) | Recommended for |
|---------|----------------|-----------------|
| 5–10 | 50–100 KB | Best quality, slow WiFi OK |
| 10–15 | 20–50 KB | **Good balance** |
| 20–30 | 10–20 KB | Fast stream, weaker WiFi |
| 40+ | <10 KB | Very low quality |

## Manual frame capture
```cpp
// Capture a JPEG frame
camera_fb_t* fb = cam.capture();
if (fb) {
    Serial.printf("Frame: %u bytes\n", fb->len);
    // fb->buf = JPEG data pointer
    // fb->len = data length in bytes
    // Use it here, then:
    cam.release(fb);   // CRITICAL — must release or PSRAM fills up
}
```

**Always call `release()` after using a frame.** The camera has 2 frame buffers — if you hold them both, the next capture blocks forever.

## Motion detection
```cpp
cam.setMotionDetection(true);

void loop() {
    float score = cam.motionScore();  // 0.0 = still, 100.0 = lots of motion
    if (score > 15.0f) {
        Serial.printf("Motion! score=%.1f\n", score);
        // Send alert, save snapshot, trigger relay...
    }
    delay(500);
}
```

The motion score is based on JPEG size change between frames — a crude but surprisingly effective indicator. Static scenes produce consistent JPEG sizes. Moving objects change the scene entropy and compress differently.

## Flash LED
```cpp
cam.flashOn(255);    // full brightness — very bright, don't stare at it
cam.flashOn(64);     // dim — less blinding for motion alerts
cam.flashOff();
```

Remember GPIO 4 is shared with SD card CS. Don't use both.

## Viewing the stream

**Browser:** Navigate to `http://<esp32-ip>:81/stream` — works in Chrome, Firefox, Safari.

**VLC:** Media → Open Network Stream → `http://<esp32-ip>:81/stream`

**Home Assistant:**
```yaml
camera:
  - platform: mjpeg
    name: ESP32-CAM
    mjpeg_url: http://192.168.x.x:81/stream
    still_image_url: http://192.168.x.x:81/capture
```

**Node-RED:** HTTP In node → `http://<ip>:81/capture` → image display

## Power requirements
The ESP32-CAM draws 200–300mA during WiFi + camera operation. Spikes to 500mA during WiFi transmit bursts. Use a proper 5V supply (500mA+). **USB power banks often can't sustain this** — they shut off under the current spikes. Use a direct 5V supply or a power bank rated for 1A+.

## Gotchas

**Single-core limits throughput.** The ESP32-CAM uses an ESP32-S (single-core variant). WiFi and camera compete for the same core. Stream frame rate is typically 5–15 fps at VGA depending on WiFi strength and what else is running.

**PSRAM cache bug requires `-mfix-esp32-psram-cache-issue`.** Without this build flag, the combination of WiFi + PSRAM access causes random crashes every few minutes. It's a silicon errata on older ESP32 revisions — the flag enables a software workaround.

**OV2640 needs warm-up.** The first 2–3 frames after init may be underexposed or discoloured. The auto-exposure and auto-white-balance settle within a few frames. Don't trigger captures immediately after `begin()`.

**Long IO0 to GND bridge = permanent bootloader.** If you forget to remove the IO0 ground bridge after uploading, the board boots into download mode every time and never runs your firmware. The RESET button doesn't help — you need to remove the bridge.

**No OTA without custom bootloader partition.** The default huge_app partition scheme uses all 4MB for the app and OTA storage is minimal. For OTA updates, switch to `min_spiffs.csv` partition scheme.
