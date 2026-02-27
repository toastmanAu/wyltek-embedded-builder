/*
 * drivers/WySi4703.h — Silicon Labs Si4703 FM radio receiver (I2C)
 * ==================================================================
 * Datasheet: https://www.silabs.com/documents/public/data-sheets/Si4702-03-C19-1.pdf
 * AN230 (programming guide): https://www.silabs.com/documents/public/application-notes/AN230.pdf
 * Bundled driver — no external library needed.
 * I2C address: 0x10 (fixed)
 * Registered via WySensors::addI2C<WySi4703>("radio", sda, scl, 0x10)
 *
 * Features:
 *   - FM receive: 76–108 MHz (worldwide band support)
 *   - Automatic frequency control (AFC)
 *   - Hardware seek up/down with RSSI threshold
 *   - Volume: 0–15
 *   - RSSI: received signal strength (0–75 dBuV typical)
 *   - RDS/RBDS: station name (PS), radio text (RT), programme type (PTY)
 *   - Stereo/mono indicator
 *   - Mute / softmute
 *
 * Si4703 REGISTER MAP (16-bit registers, read 0x0A–0x0F, write 0x02–0x07):
 *   The Si4703 has a peculiar I2C protocol:
 *   - READ:  always reads 32 bytes (registers 0x0A–0x0F, then 0x00–0x09)
 *   - WRITE: always writes 8 bytes starting at register 0x02
 *   Registers are 16-bit big-endian.
 *
 * WIRING:
 *   SDA  → I2C SDA (with 4.7kΩ pull-up)
 *   SCL  → I2C SCL (with 4.7kΩ pull-up)
 *   RST  → GPIO (required — held LOW then HIGH to enter I2C mode)
 *   SDIO → same as SDA (some breakouts label it separately)
 *   SEN  → 3.3V or GPIO (I2C mode select — HIGH = I2C)
 *   GPIO2 → optional interrupt pin (seek/tune complete, RDS ready)
 *   3.3V, GND
 *
 *   RST pin is REQUIRED. The Si4703 uses the state of SDIO at reset
 *   to select 2-wire (I2C) vs 3-wire mode. Drive RST LOW, hold SDIO
 *   HIGH (via pull-up), then bring RST HIGH — this locks in I2C mode.
 *
 * Registered with RST pin via pin2:
 *   sensors.addI2C<WySi4703>("radio", SDA, SCL, 0x10)
 *   then: radio->setRstPin(RST_PIN);  // call before sensors.begin()
 *
 * WySensorData:
 *   d.raw      = current frequency × 10 (e.g. 1056 = 105.6 MHz)
 *   d.rawInt   = RSSI (signal strength)
 *   d.light    = stereo indicator (1.0 = stereo, 0.0 = mono)
 *   d.ok       = true when tuned and signal present
 */

#pragma once
#include "../WySensors.h"

/* Si4703 register indices (into shadow register array, 16 registers) */
#define SI4703_REG_DEVICEID   0x00
#define SI4703_REG_CHIPID     0x01
#define SI4703_REG_POWERCFG   0x02
#define SI4703_REG_CHANNEL    0x03
#define SI4703_REG_SYSCONFIG1 0x04
#define SI4703_REG_SYSCONFIG2 0x05
#define SI4703_REG_SYSCONFIG3 0x06
#define SI4703_REG_TEST1      0x07
#define SI4703_REG_TEST2      0x08
#define SI4703_REG_BOOTCONFIG 0x09
#define SI4703_REG_STATUSRSSI 0x0A
#define SI4703_REG_READCHAN   0x0B
#define SI4703_REG_RDSA       0x0C
#define SI4703_REG_RDSB       0x0D
#define SI4703_REG_RDSC       0x0E
#define SI4703_REG_RDSD       0x0F

/* POWERCFG (0x02) bits */
#define SI4703_DSMUTE    0x8000  /* disable softmute */
#define SI4703_DMUTE     0x4000  /* disable mute */
#define SI4703_MONO      0x2000  /* force mono */
#define SI4703_RDSM      0x0800  /* RDS mode (1=verbose) */
#define SI4703_SKMODE    0x0400  /* seek mode (0=wrap, 1=stop at band edge) */
#define SI4703_SEEKUP    0x0200  /* seek direction (1=up) */
#define SI4703_SEEK      0x0100  /* start seek */
#define SI4703_DISABLE   0x0040  /* powerdown */
#define SI4703_ENABLE    0x0001  /* powerup */

/* CHANNEL (0x03) bits */
#define SI4703_TUNE      0x8000  /* set to tune */

/* SYSCONFIG1 (0x04) bits */
#define SI4703_RDSIEN    0x8000  /* RDS interrupt enable (GPIO2) */
#define SI4703_STCIEN    0x4000  /* seek/tune complete interrupt */
#define SI4703_RDS       0x1000  /* enable RDS */
#define SI4703_DE        0x0800  /* de-emphasis: 0=75µs (US), 1=50µs (EU/AU) */
#define SI4703_AGCD      0x0400  /* AGC disable */

/* SYSCONFIG2 (0x05) bits */
#define SI4703_SEEKTH_SHIFT  8   /* RSSI seek threshold (bits 15:8) */
#define SI4703_BAND_SHIFT    6   /* band select (bits 7:6): 00=87.5-108, 01=76-108, 10=76-90 */
#define SI4703_SPACE_SHIFT   4   /* channel spacing (bits 5:4): 00=200kHz (US), 01=100kHz (EU), 10=50kHz */
#define SI4703_VOLUME_MASK   0x000F

/* STATUSRSSI (0x0A) bits */
#define SI4703_RDSR      0x8000  /* RDS ready */
#define SI4703_STC       0x4000  /* seek/tune complete */
#define SI4703_SF_BL     0x2000  /* seek fail / band limit */
#define SI4703_AFCRL     0x1000  /* AFC rail */
#define SI4703_RDSS      0x0800  /* RDS synchronised */
#define SI4703_STEREO    0x0100  /* stereo indicator */
#define SI4703_RSSI_MASK 0x00FF

/* READCHAN (0x0B) bits */
#define SI4703_READCHAN_MASK 0x03FF

/* Band and spacing config */
#define SI4703_BAND_US_EU   0x00  /* 87.5–108 MHz */
#define SI4703_BAND_WORLD   0x01  /* 76–108 MHz */
#define SI4703_BAND_JAPAN   0x02  /* 76–90 MHz */
#define SI4703_SPACE_200KHZ 0x00  /* US, 200kHz */
#define SI4703_SPACE_100KHZ 0x01  /* Europe/AU, 100kHz */
#define SI4703_SPACE_50KHZ  0x02  /* 50kHz fine */

/* RDS PS name is 8 chars, radio text up to 64 chars */
#define SI4703_RDS_PS_LEN   8
#define SI4703_RDS_RT_LEN   64

class WySi4703 : public WySensorBase {
public:
    WySi4703(WyI2CPins pins) : _pins(pins) {}

    const char* driverName() override { return "Si4703"; }

    void setRstPin(int8_t pin) { _rstPin = pin; }

    bool begin() override {
        if (_rstPin < 0) {
            Serial.println("[Si4703] RST pin required — call setRstPin() before begin()");
            return false;
        }

        /* Reset sequence — forces I2C mode via SDIO state at reset */
        pinMode(_rstPin, OUTPUT);
        pinMode(_pins.sda, OUTPUT);
        digitalWrite(_pins.sda, HIGH);  /* SDIO HIGH = I2C mode */
        delay(1);
        digitalWrite(_rstPin, LOW);
        delay(1);
        digitalWrite(_rstPin, HIGH);
        delay(1);

        /* Now hand SDA back to Wire */
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(_pins.freq > 0 ? _pins.freq : 100000);  /* Si4703 max I2C = 400kHz */
        delay(10);

        /* Read registers to populate shadow copy */
        _readRegisters();

        /* Verify chip — Device ID should be 0x1242 or 0x1243 */
        uint16_t devId = _regs[SI4703_REG_DEVICEID];
        if ((devId & 0xFF00) != 0x1200) {
            Serial.printf("[Si4703] unexpected device ID: 0x%04X\n", devId);
            return false;
        }

        /* Enable oscillator (TEST1 register, bit 15 = XOSCEN) */
        _regs[SI4703_REG_TEST1] = 0x8100;
        _writeRegisters();
        delay(500);  /* oscillator startup */

        /* Power up */
        _readRegisters();
        _regs[SI4703_REG_POWERCFG] = SI4703_DMUTE | SI4703_ENABLE;
        _writeRegisters();
        delay(110);  /* powerup time */

        /* Configure:
         * - RDS enabled
         * - De-emphasis 50µs (Europe/AU — change to 0 for US)
         * - Band: 87.5–108 MHz
         * - Channel spacing: 100kHz (Europe/AU)
         * - Seek threshold: RSSI > 25
         * - Volume: 10 */
        _readRegisters();
        _regs[SI4703_REG_SYSCONFIG1] |= SI4703_RDS | SI4703_DE;
        _regs[SI4703_REG_SYSCONFIG2]  = (25 << SI4703_SEEKTH_SHIFT)
                                       | (SI4703_BAND_US_EU << SI4703_BAND_SHIFT)
                                       | (SI4703_SPACE_100KHZ << SI4703_SPACE_SHIFT)
                                       | 10;  /* volume */
        _writeRegisters();
        delay(10);

        _startFreqMHz = 87.5f;  /* 87.5-108 band */
        _spacingMHz   = 0.1f;   /* 100kHz */

        return true;
    }

    WySensorData read() override {
        WySensorData d;
        _readRegisters();
        uint16_t status = _regs[SI4703_REG_STATUSRSSI];
        uint8_t  rssi   = status & SI4703_RSSI_MASK;
        bool     stereo = (status & SI4703_STEREO) != 0;
        uint16_t chan   = _regs[SI4703_REG_READCHAN] & SI4703_READCHAN_MASK;
        float    freq   = _startFreqMHz + (chan * _spacingMHz);

        d.raw    = freq * 10.0f;   /* e.g. 1056 = 105.6 MHz */
        d.rawInt = rssi;
        d.light  = stereo ? 1.0f : 0.0f;
        d.ok     = (rssi > 5);
        return d;
    }

    /* ── Tuning ──────────────────────────────────────────────────── */

    /* Tune to specific frequency in MHz (e.g. 105.6) */
    bool tune(float freqMHz) {
        uint16_t chan = (uint16_t)((freqMHz - _startFreqMHz) / _spacingMHz + 0.5f);
        _readRegisters();
        _regs[SI4703_REG_CHANNEL] = SI4703_TUNE | (chan & 0x03FF);
        _writeRegisters();

        /* Wait for STC (seek/tune complete) */
        uint32_t t = millis();
        do {
            delay(10);
            _readRegisters();
        } while (!(_regs[SI4703_REG_STATUSRSSI] & SI4703_STC) && millis()-t < 3000);

        /* Clear TUNE bit */
        _regs[SI4703_REG_CHANNEL] &= ~SI4703_TUNE;
        _writeRegisters();
        _currentFreqMHz = freqMHz;
        return (_regs[SI4703_REG_STATUSRSSI] & SI4703_STC) != 0;
    }

    /* Seek up or down from current frequency */
    float seek(bool up = true) {
        _readRegisters();
        if (up) _regs[SI4703_REG_POWERCFG] |=  SI4703_SEEKUP;
        else    _regs[SI4703_REG_POWERCFG] &= ~SI4703_SEEKUP;
        _regs[SI4703_REG_POWERCFG] |= SI4703_SEEK;
        _writeRegisters();

        uint32_t t = millis();
        do {
            delay(20);
            _readRegisters();
        } while (!(_regs[SI4703_REG_STATUSRSSI] & SI4703_STC) && millis()-t < 10000);

        _regs[SI4703_REG_POWERCFG] &= ~SI4703_SEEK;
        _writeRegisters();

        /* Read tuned channel */
        _readRegisters();
        uint16_t chan = _regs[SI4703_REG_READCHAN] & SI4703_READCHAN_MASK;
        _currentFreqMHz = _startFreqMHz + (chan * _spacingMHz);
        return _currentFreqMHz;
    }

    float seekUp()   { return seek(true);  }
    float seekDown() { return seek(false); }

    /* ── Volume / mute ───────────────────────────────────────────── */

    /* Volume: 0 (mute) to 15 (max) */
    void setVolume(uint8_t vol) {
        vol = constrain(vol, 0, 15);
        _readRegisters();
        _regs[SI4703_REG_SYSCONFIG2] &= ~SI4703_VOLUME_MASK;
        _regs[SI4703_REG_SYSCONFIG2] |= vol;
        if (vol == 0) _regs[SI4703_REG_POWERCFG] &= ~SI4703_DMUTE;
        else           _regs[SI4703_REG_POWERCFG] |=  SI4703_DMUTE;
        _writeRegisters();
        _volume = vol;
    }

    void volumeUp()   { setVolume(_volume + 1); }
    void volumeDown() { if (_volume > 0) setVolume(_volume - 1); }
    void mute()       { setVolume(0); }
    uint8_t volume()  { return _volume; }

    void setMono(bool mono) {
        _readRegisters();
        if (mono) _regs[SI4703_REG_POWERCFG] |=  SI4703_MONO;
        else      _regs[SI4703_REG_POWERCFG] &= ~SI4703_MONO;
        _writeRegisters();
    }

    /* ── Status ──────────────────────────────────────────────────── */

    float   currentFreq() { return _currentFreqMHz; }
    uint8_t rssi() {
        _readRegisters();
        return _regs[SI4703_REG_STATUSRSSI] & SI4703_RSSI_MASK;
    }
    bool isStereo() {
        _readRegisters();
        return (_regs[SI4703_REG_STATUSRSSI] & SI4703_STEREO) != 0;
    }

    /* ── RDS ─────────────────────────────────────────────────────── */

    /* Read RDS Programme Service name (station name, 8 chars) into buf.
     * Returns true if a complete name was decoded.
     * Call repeatedly — RDS sends the name in 2-char groups over several blocks. */
    bool readRDS_PS(char* buf) {
        _readRegisters();
        if (!(_regs[SI4703_REG_STATUSRSSI] & SI4703_RDSR)) return false;

        uint16_t b = _regs[SI4703_REG_RDSB];
        uint8_t  groupType = (b >> 12) & 0x0F;
        bool     versionB  = (b >> 11) & 0x01;

        if (groupType == 0 && !versionB) {  /* Group 0A — PS name */
            uint8_t addr = (b & 0x03) * 2;  /* 2 chars per group, 4 groups = 8 chars */
            uint16_t d_reg = _regs[SI4703_REG_RDSD];
            _psName[addr]     = (char)(d_reg >> 8);
            _psName[addr + 1] = (char)(d_reg & 0xFF);
            _psName[SI4703_RDS_PS_LEN] = 0;
            _psReceived |= (1 << addr/2);
        }

        if (_psReceived == 0x0F) {  /* all 4 groups received */
            strncpy(buf, _psName, SI4703_RDS_PS_LEN + 1);
            _psReceived = 0;
            return true;
        }
        return false;
    }

    /* Read RDS Radio Text (up to 64 chars) — call repeatedly until returns true */
    bool readRDS_RT(char* buf) {
        _readRegisters();
        if (!(_regs[SI4703_REG_STATUSRSSI] & SI4703_RDSR)) return false;

        uint16_t b = _regs[SI4703_REG_RDSB];
        uint8_t  groupType = (b >> 12) & 0x0F;
        bool     versionB  = (b >> 11) & 0x01;

        if (groupType == 2 && !versionB) {  /* Group 2A — Radio Text */
            uint8_t addr = (b & 0x0F) * 4;
            uint16_t c_reg = _regs[SI4703_REG_RDSC];
            uint16_t d_reg = _regs[SI4703_REG_RDSD];
            if (addr + 3 < SI4703_RDS_RT_LEN) {
                _rt[addr]   = (char)(c_reg >> 8);
                _rt[addr+1] = (char)(c_reg & 0xFF);
                _rt[addr+2] = (char)(d_reg >> 8);
                _rt[addr+3] = (char)(d_reg & 0xFF);
            }
            if (b & 0x0010) {  /* Text A/B flag toggled = new message */
                _rt[addr] = 0;
                strncpy(buf, _rt, SI4703_RDS_RT_LEN);
                return true;
            }
        }
        return false;
    }

    /* PTY (programme type) code 0–31 */
    uint8_t readRDS_PTY() {
        _readRegisters();
        if (!(_regs[SI4703_REG_STATUSRSSI] & SI4703_RDSR)) return 0;
        return (_regs[SI4703_REG_RDSB] >> 5) & 0x1F;
    }

    /* ── Power ───────────────────────────────────────────────────── */

    void powerDown() {
        _readRegisters();
        _regs[SI4703_REG_POWERCFG] = SI4703_ENABLE | SI4703_DISABLE;
        _writeRegisters();
    }

private:
    WyI2CPins _pins;
    int8_t    _rstPin        = -1;
    uint16_t  _regs[16]      = {};
    float     _startFreqMHz  = 87.5f;
    float     _spacingMHz    = 0.1f;
    float     _currentFreqMHz = 87.5f;
    uint8_t   _volume        = 10;

    /* RDS buffers */
    char    _psName[SI4703_RDS_PS_LEN + 1] = {};
    char    _rt[SI4703_RDS_RT_LEN + 1]     = {};
    uint8_t _psReceived = 0;

    /* Si4703 I2C read protocol:
     * Always reads 32 bytes = 16 registers × 2 bytes, starting from reg 0x0A.
     * The register array wraps: 0x0A, 0x0B, ..., 0x0F, 0x00, 0x01, ..., 0x09 */
    void _readRegisters() {
        Wire.requestFrom(_pins.addr, (uint8_t)32);
        if (Wire.available() < 32) return;
        for (uint8_t i = 0; i < 16; i++) {
            uint8_t idx = (0x0A + i) & 0x0F;
            uint8_t hi  = Wire.read();
            uint8_t lo  = Wire.read();
            _regs[idx] = ((uint16_t)hi << 8) | lo;
        }
    }

    /* Si4703 I2C write protocol:
     * Always writes 8 bytes = registers 0x02–0x05 (4 registers × 2 bytes) */
    void _writeRegisters() {
        Wire.beginTransmission(_pins.addr);
        for (uint8_t reg = 0x02; reg <= 0x07; reg++) {
            Wire.write(_regs[reg] >> 8);
            Wire.write(_regs[reg] & 0xFF);
        }
        Wire.endTransmission();
    }
};
