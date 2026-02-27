/*
 * drivers/WyDFPlayer.h — DFPlayer Mini / DFR0299 MP3 Player Module (UART)
 * =========================================================================
 * Compatible with: DFPlayer Mini (DFR0299), YX5200, MH2024K-24SS, and
 *                  most clone DFPlayer modules (same serial protocol)
 *
 * Bundled driver — no external library needed.
 * Uses UART — registered via WySensors::addUART<WyDFPlayer>("mp3", TX, RX, 9600)
 * OR use standalone: WyDFPlayer mp3(TX_PIN, RX_PIN);
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT DOES
 * ═══════════════════════════════════════════════════════════════════
 * Plays MP3/WAV/WMA/FLAC files from a micro-SD card (up to 32GB FAT32).
 * Built-in amplifier drives speakers up to 3W (4Ω) directly.
 * Hardware EQ, volume, loop, shuffle, playlist support.
 * Busy pin goes LOW while playing — use for play-complete detection.
 *
 * ═══════════════════════════════════════════════════════════════════
 * SD CARD FILE NAMING — CRITICAL
 * ═══════════════════════════════════════════════════════════════════
 * Files MUST be named with leading zeros: 0001.mp3, 0002.mp3, etc.
 * Folder mode: /01/001.mp3, /01/002.mp3, /02/001.mp3, etc.
 * Folder names 01–99, file names 001–255.
 *
 * File ORDER on the SD card matters — not alphabetical, but the order
 * files were written to FAT32. To guarantee order:
 *   Format SD card (FAT32)
 *   Copy files one by one in order, or use a tool like SD Card Formatter
 *   Avoid dragging folders — copy files individually
 *
 * Special folders:
 *   /MP3/     — play by number from 0001.mp3 to 9999.mp3 (playMP3Folder)
 *   /ADVERT/  — interrupt current playback, play, resume (playAdvertise)
 *
 * ═══════════════════════════════════════════════════════════════════
 * SERIAL PROTOCOL
 * ═══════════════════════════════════════════════════════════════════
 * 9600 baud, 8N1
 * Frame: [7E][FF][06][CMD][FB][PAR_H][PAR_L][CHK_H][CHK_L][EF]
 *   7E = start byte
 *   FF = version (always 0xFF)
 *   06 = data length (always 6)
 *   CMD = command byte
 *   FB = feedback request: 0x01=yes, 0x00=no
 *   PAR_H/PAR_L = 16-bit parameter (big-endian)
 *   CHK_H/CHK_L = checksum = -(0xFF + 0x06 + CMD + FB + PAR_H + PAR_L)
 *   EF = end byte
 *
 * ⚠️ Some clone modules (MH2024K-24SS) use a slightly different protocol
 *    and don't respond to feedback requests. If your module doesn't
 *    respond to queries, call setFeedback(false).
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 *   Module VCC    → 3.3V–5V (5V recommended for max volume)
 *   Module GND    → GND
 *   Module TX     → ESP32 RX pin (via 1kΩ resistor — see note)
 *   Module RX     → ESP32 TX pin (direct, 3.3V signal OK)
 *   Module SPK_1  → Speaker + terminal
 *   Module SPK_2  → Speaker - terminal
 *   Module BUSY   → ESP32 GPIO (optional — LOW while playing)
 *
 * ⚠️ 1kΩ on TX→RX line: The DFPlayer TX is 3.3V but can interfere with
 *    the ESP32 UART bootloader on some boards. The 1kΩ resistor prevents
 *    the DFPlayer from holding the RX line and blocking firmware upload.
 *    Always include it — it costs nothing and prevents headaches.
 *
 * ⚠️ SPEAKER: Built-in amplifier is 3W max. Use 4Ω or 8Ω speaker.
 *    For headphones or line-out: use DAC pins (L_OUT, R_OUT) with
 *    a 1kΩ series resistor to headphone jack — DO NOT connect
 *    headphones to SPK_1/SPK_2 (amplifier output — too loud, will distort).
 *
 * ⚠️ POWER NOISE: The amplifier causes voltage spikes on VCC.
 *    Add 100µF electrolytic + 100nF ceramic capacitor across VCC/GND
 *    right at the module. Without this, other I2C sensors on the same
 *    rail may glitch during playback.
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE (standalone)
 * ═══════════════════════════════════════════════════════════════════
 *   WyDFPlayer mp3;
 *   mp3.begin(TX_PIN, RX_PIN);
 *   mp3.setVolume(20);        // 0–30
 *   mp3.play(1);              // play 0001.mp3
 *   mp3.playFolder(1, 3);     // play /01/003.mp3
 *
 * USAGE (WySensors registry):
 *   auto* mp3 = sensors.addUART<WyDFPlayer>("mp3", TX, RX, 9600);
 *   sensors.begin();
 *   sensors.get<WyDFPlayer>("mp3")->play(1);
 *
 * WySensorData (from read()):
 *   d.rawInt   = current track number (1-based)
 *   d.raw      = playback status: 0=stopped, 1=playing, 2=paused
 *   d.voltage  = volume (0–30)
 *   d.ok       = true when module responded
 */

#pragma once
#include "../WySensors.h"

/* DFPlayer command bytes */
#define DFP_CMD_NEXT          0x01
#define DFP_CMD_PREV          0x02
#define DFP_CMD_PLAY_IDX      0x03   /* play by global index */
#define DFP_CMD_VOLUME_UP     0x04
#define DFP_CMD_VOLUME_DOWN   0x05
#define DFP_CMD_VOLUME        0x06   /* set volume 0–30 */
#define DFP_CMD_EQ            0x07   /* 0=Normal,1=Pop,2=Rock,3=Jazz,4=Classic,5=Bass */
#define DFP_CMD_LOOP_IDX      0x08   /* loop track by index */
#define DFP_CMD_SOURCE        0x09   /* 1=USB,2=TF(SD),5=NOR */
#define DFP_CMD_SLEEP         0x0A
#define DFP_CMD_RESET         0x0C
#define DFP_CMD_PLAY          0x0D   /* resume / play */
#define DFP_CMD_PAUSE         0x0E
#define DFP_CMD_FOLDER        0x0F   /* play folder: H=folder, L=file */
#define DFP_CMD_VOLUME_ADJ    0x10   /* gain: H=enable, L=gain 0–31 */
#define DFP_CMD_LOOP_ALL      0x11   /* 1=loop all, 0=stop loop */
#define DFP_CMD_MP3_FOLDER    0x12   /* play /MP3/nnnn.mp3 */
#define DFP_CMD_ADVERTISE     0x13   /* play /ADVERT/nnnn.mp3, then resume */
#define DFP_CMD_FOLDER_LOOP   0x17   /* loop folder */
#define DFP_CMD_RANDOM        0x18   /* shuffle all */
#define DFP_CMD_LOOP_CURRENT  0x19   /* 0=loop current, 1=stop loop */
#define DFP_CMD_DAC           0x1A   /* 0=DAC on, 1=DAC off */
#define DFP_CMD_STOP_ADVERT   0x15   /* stop advert and resume */
#define DFP_CMD_STOP          0x16   /* stop playback */

/* Query commands (require feedback) */
#define DFP_QUERY_STATUS      0x42   /* playing status */
#define DFP_QUERY_VOLUME      0x43
#define DFP_QUERY_EQ          0x44
#define DFP_QUERY_PLAYBACK    0x45   /* current file index */
#define DFP_QUERY_SD_FILES    0x48   /* total files on SD */
#define DFP_QUERY_FOLDER_FILES 0x4E  /* files in folder */
#define DFP_QUERY_FOLDERS     0x4F   /* total folders */

/* Response codes */
#define DFP_RSP_TFCARD_INSERT  0x3A
#define DFP_RSP_TFCARD_REMOVE  0x3B
#define DFP_RSP_FINISH_USB     0x3C
#define DFP_RSP_FINISH_SD      0x3D
#define DFP_RSP_FINISH_FLASH   0x3E
#define DFP_RSP_INIT           0x3F
#define DFP_RSP_ERROR          0x40
#define DFP_RSP_ACK            0x41
#define DFP_RSP_STATUS         0x42
#define DFP_RSP_VOLUME         0x43
#define DFP_RSP_EQ             0x44
#define DFP_RSP_PLAYBACK       0x45
#define DFP_RSP_SD_FILES       0x48

/* EQ modes */
#define DFP_EQ_NORMAL   0
#define DFP_EQ_POP      1
#define DFP_EQ_ROCK     2
#define DFP_EQ_JAZZ     3
#define DFP_EQ_CLASSIC  4
#define DFP_EQ_BASS     5

#ifndef WY_DFP_ACK_TIMEOUT_MS
#define WY_DFP_ACK_TIMEOUT_MS  300
#endif

#ifndef WY_DFP_INIT_DELAY_MS
#define WY_DFP_INIT_DELAY_MS   2000  /* module boot time */
#endif

class WyDFPlayer : public WySensorBase {
public:
    WyDFPlayer() {}
    WyDFPlayer(WyUARTPins pins) : _pins(pins) {}

    const char* driverName() override { return "DFPlayer Mini"; }

    /* Enable/disable feedback requests.
     * Some clones (MH2024K-24SS) ignore feedback — set false if no response. */
    void setFeedback(bool en)        { _feedback = en; }

    /* Optional BUSY pin — LOW while playing */
    void setBusyPin(int8_t pin)      { _busyPin = pin; }

    /* Standalone begin (when not using WySensors registry) */
    bool begin(int8_t tx, int8_t rx, uint32_t baud = 9600) {
        _pins.tx = tx; _pins.rx = rx; _pins.baud = baud;
        return begin();
    }

    bool begin() override {
        Serial2.begin(_pins.baud, SERIAL_8N1, _pins.rx, _pins.tx);

        if (_busyPin >= 0) {
            pinMode(_busyPin, INPUT_PULLUP);
        }

        /* Flush any garbage */
        delay(200);
        while (Serial2.available()) Serial2.read();

        /* Wait for module init */
        Serial.printf("[DFPlayer] waiting for init (%dms)...\n", WY_DFP_INIT_DELAY_MS);
        delay(WY_DFP_INIT_DELAY_MS);

        /* Select SD card as source */
        _sendCmd(DFP_CMD_SOURCE, 0x02, _feedback);
        delay(200);

        /* Get file count to verify comms */
        uint16_t files = totalFiles();
        if (files == 0 && _feedback) {
            Serial.println("[DFPlayer] no files found or no response");
            Serial.println("[DFPlayer] check SD card and try setFeedback(false) for clones");
            /* Not fatal — module may still work */
        } else if (files > 0) {
            Serial.printf("[DFPlayer] ready — %u files on SD\n", files);
        } else {
            Serial.println("[DFPlayer] ready (feedback disabled)");
        }
        return true;
    }

    /* ── Playback controls ────────────────────────────────────────── */

    /* Play track by global index (1-based, FAT32 write order) */
    void play(uint16_t track = 1)   { _sendCmd(DFP_CMD_PLAY_IDX, track); }

    /* Play file from folder: /01/003.mp3 = playFolder(1, 3) */
    void playFolder(uint8_t folder, uint8_t file) {
        _sendCmd(DFP_CMD_FOLDER, ((uint16_t)folder << 8) | file);
    }

    /* Play from /MP3/ folder by number (supports 0001–9999) */
    void playMP3(uint16_t num)      { _sendCmd(DFP_CMD_MP3_FOLDER, num); }

    /* Play from /ADVERT/ folder — interrupts current, then resumes */
    void playAdvertise(uint16_t num){ _sendCmd(DFP_CMD_ADVERTISE, num); }

    void pause()                    { _sendCmd(DFP_CMD_PAUSE); }
    void resume()                   { _sendCmd(DFP_CMD_PLAY); }
    void stop()                     { _sendCmd(DFP_CMD_STOP); }
    void next()                     { _sendCmd(DFP_CMD_NEXT); }
    void prev()                     { _sendCmd(DFP_CMD_PREV); }

    /* Volume: 0–30 */
    void setVolume(uint8_t vol)     { _vol = min(vol, (uint8_t)30); _sendCmd(DFP_CMD_VOLUME, _vol); }
    void volumeUp()                 { _sendCmd(DFP_CMD_VOLUME_UP); }
    void volumeDown()               { _sendCmd(DFP_CMD_VOLUME_DOWN); }

    /* EQ: DFP_EQ_NORMAL / POP / ROCK / JAZZ / CLASSIC / BASS */
    void setEQ(uint8_t eq)          { _sendCmd(DFP_CMD_EQ, eq); }

    /* Loop modes */
    void loopAll(bool en = true)    { _sendCmd(DFP_CMD_LOOP_ALL, en ? 1 : 0); }
    void loopCurrent(bool en = true){ _sendCmd(DFP_CMD_LOOP_CURRENT, en ? 0 : 1); }
    void loopFolder(uint8_t folder) { _sendCmd(DFP_CMD_FOLDER_LOOP, folder); }
    void shuffle()                  { _sendCmd(DFP_CMD_RANDOM); }

    /* Stop advert and resume previous track */
    void stopAdvertise()            { _sendCmd(DFP_CMD_STOP_ADVERT); }

    /* DAC on/off (for sleep / standby) */
    void dacOn()                    { _sendCmd(DFP_CMD_DAC, 0x00); }
    void dacOff()                   { _sendCmd(DFP_CMD_DAC, 0x01); }
    void sleep()                    { _sendCmd(DFP_CMD_SLEEP); }
    void reset()                    { _sendCmd(DFP_CMD_RESET); delay(WY_DFP_INIT_DELAY_MS); }

    /* ── Status queries ──────────────────────────────────────────── */

    /* True if BUSY pin is LOW (playing) — fast, no UART needed */
    bool isPlaying() {
        if (_busyPin >= 0) return (digitalRead(_busyPin) == LOW);
        return _query(DFP_QUERY_STATUS) == 1;
    }

    bool isStopped() { return !isPlaying(); }

    uint16_t currentTrack()  { return _query(DFP_QUERY_PLAYBACK); }
    uint8_t  currentVolume() { return (uint8_t)_query(DFP_QUERY_VOLUME); }
    uint16_t totalFiles()    { return _query(DFP_QUERY_SD_FILES); }
    uint16_t totalFolders()  { return _query(DFP_QUERY_FOLDERS); }
    uint16_t filesInFolder(uint8_t folder) {
        return _query(DFP_QUERY_FOLDER_FILES, folder);
    }

    /* Block until track finishes (or timeout ms). Uses BUSY pin if set. */
    bool waitDone(uint32_t timeoutMs = 60000) {
        uint32_t deadline = millis() + timeoutMs;
        delay(200);  /* give playback a moment to start */
        while (isPlaying() && millis() < deadline) delay(50);
        return !isPlaying();
    }

    /* ── WySensorBase read ───────────────────────────────────────── */
    WySensorData read() override {
        WySensorData d;
        d.rawInt  = currentTrack();
        d.raw     = isPlaying() ? 1.0f : 0.0f;
        d.voltage = currentVolume();
        d.ok      = true;
        return d;
    }

private:
    WyUARTPins _pins   = {};
    int8_t     _busyPin = -1;
    bool       _feedback = true;
    uint8_t    _vol    = 20;

    /* ── Protocol implementation ─────────────────────────────────── */

    void _sendCmd(uint8_t cmd, uint16_t param = 0, bool fb = false) {
        uint8_t buf[10];
        buf[0] = 0x7E;   /* start */
        buf[1] = 0xFF;   /* version */
        buf[2] = 0x06;   /* length */
        buf[3] = cmd;
        buf[4] = fb ? 0x01 : 0x00;  /* feedback */
        buf[5] = (uint8_t)(param >> 8);
        buf[6] = (uint8_t)(param & 0xFF);

        /* Checksum: -(sum of bytes 1–6) */
        int16_t chk = 0;
        for (int i = 1; i <= 6; i++) chk -= buf[i];
        buf[7] = (uint8_t)(chk >> 8);
        buf[8] = (uint8_t)(chk & 0xFF);
        buf[9] = 0xEF;   /* end */

        Serial2.write(buf, 10);
        delay(30);  /* inter-command gap — module needs time to process */
    }

    /* Send a query command and return the 16-bit response parameter */
    uint16_t _query(uint8_t cmd, uint16_t param = 0) {
        if (!_feedback) return 0;

        /* Flush stale data */
        while (Serial2.available()) Serial2.read();

        _sendCmd(cmd, param, true);

        /* Read response frame */
        uint8_t buf[10] = {};
        uint32_t deadline = millis() + WY_DFP_ACK_TIMEOUT_MS;
        uint8_t  i = 0;

        /* Find start byte 0x7E */
        while (millis() < deadline) {
            if (Serial2.available()) {
                uint8_t b = Serial2.read();
                if (b == 0x7E) { buf[0] = b; i = 1; break; }
            }
        }
        if (i == 0) return 0;  /* timeout */

        /* Read remaining 9 bytes */
        while (i < 10 && millis() < deadline) {
            if (Serial2.available()) buf[i++] = Serial2.read();
        }
        if (i < 10) return 0;
        if (buf[9] != 0xEF) return 0;

        /* buf[3] = response command, buf[5:6] = parameter */
        return ((uint16_t)buf[5] << 8) | buf[6];
    }
};
