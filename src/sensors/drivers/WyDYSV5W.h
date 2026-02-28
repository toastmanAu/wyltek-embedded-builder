#pragma once
/**
 * WyDYSV5W.h — DY-SV5W / DY-SV8F Serial MP3 Player driver
 * ==========================================================
 * Serial-controlled MP3/WAV player with onboard amp (5W / 8W).
 * Plays audio from SD/TF card via UART commands.
 * TX-only control (send frames, no response needed for basic use).
 *
 * Wiring:
 *   DY-SV5W TX → MCU RX
 *   DY-SV5W RX → MCU TX (optional, for query)
 *   VCC: 5V (or 3.3V — check your module variant)
 *   SPK+/SPK-: speaker (4–8Ω)
 *
 * Protocol: 0xAA cmd len [data...] checksum
 * Checksum: sum of all bytes mod 256
 *
 * Datasheet: https://dl.dwin.com.cn/drive/DWIN_TFT_LCD_DY-SV5W.pdf
 */

#ifndef WY_DYSV5W_UART_BAUD
#define WY_DYSV5W_UART_BAUD  9600
#endif

class WyDYSV5W {
public:
    WyDYSV5W(Stream &serial) : _s(serial) {}

    // Play specific file by index (1-based)
    void playIndex(uint16_t index) {
        sendCmd(0x01, (uint8_t)(index >> 8), (uint8_t)(index & 0xFF));
    }

    // Play by file name number (e.g. "001.mp3" = 1)
    void playNumber(uint16_t num) { playIndex(num); }

    // Pause / resume
    void pause()  { sendCmd3(0x03); }
    void resume() { sendCmd3(0x02); }
    void stop()   { sendCmd3(0x04); }

    // Volume: 0–30
    void setVolume(uint8_t vol) {
        if (vol > 30) vol = 30;
        sendCmd(0x13, vol);
    }

    // Next / previous track
    void next() { sendCmd3(0x06); }
    void prev() { sendCmd3(0x05); }

    // Loop current track
    void loopCurrent() { sendCmd(0x19, 0x02); }
    void loopOff()     { sendCmd(0x19, 0x00); }

    // Play track in folder (folder 1–99, track 1–255)
    void playFolder(uint8_t folder, uint8_t track) {
        sendCmd(0x07, folder, track);
    }

private:
    Stream &_s;

    void sendCmd(uint8_t cmd, uint8_t d1, uint8_t d2) {
        uint8_t buf[] = {0xAA, cmd, 0x02, d1, d2,
                         (uint8_t)(0xAA + cmd + 0x02 + d1 + d2)};
        _s.write(buf, sizeof(buf));
    }
    void sendCmd(uint8_t cmd, uint8_t d1) {
        uint8_t buf[] = {0xAA, cmd, 0x01, d1,
                         (uint8_t)(0xAA + cmd + 0x01 + d1)};
        _s.write(buf, sizeof(buf));
    }
    void sendCmd3(uint8_t cmd) {
        uint8_t buf[] = {0xAA, cmd, 0x00,
                         (uint8_t)(0xAA + cmd)};
        _s.write(buf, sizeof(buf));
    }
};
