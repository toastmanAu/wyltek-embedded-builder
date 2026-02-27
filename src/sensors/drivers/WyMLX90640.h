/*
 * drivers/WyMLX90640.h — MLX90640 32×24 IR Thermal Camera (I2C)
 * ==============================================================
 * Datasheet: https://www.melexis.com/en/documents/documentation/datasheets/datasheet-mlx90640
 * Application note: AN#0101 (calibration data extraction)
 * Bundled driver — no external library needed.
 *
 * I2C address: 0x33 (default, pin-selectable 0x30–0x37)
 * I2C speed:   400kHz minimum, 1MHz recommended for full frame rate
 * Supply:      3.3V only
 *
 * ═══════════════════════════════════════════════════════════════════
 * WHAT IT DOES
 * ═══════════════════════════════════════════════════════════════════
 * 32×24 pixel array of thermopiles. Each pixel measures IR radiation
 * and reports a temperature. Output: 768 float temperatures in °C.
 *
 * Field of view:
 *   MLX90640-BAA: 55° × 35° (wide — landscape scenes, rooms)
 *   MLX90640-BAB: 110° × 75° (ultra-wide — close-range, wide coverage)
 *
 * Frame rates: 0.5, 1, 2 (default), 4, 8, 16, 32 Hz
 *   Higher = faster refresh, more I2C bandwidth, more noise
 *   2Hz is a good starting point. 8Hz for real-time feel.
 *   32Hz requires 1MHz I2C and fast pixel processing.
 *
 * Resolution: 0.1°C sensitivity, ±1.5°C accuracy (typical)
 * Range: -40°C to +300°C (standard), -40°C to +85°C ambient operating
 *
 * ═══════════════════════════════════════════════════════════════════
 * HOW IT WORKS (simplified)
 * ═══════════════════════════════════════════════════════════════════
 * The sensor outputs raw ADC values that must be converted to
 * temperature using a complex calibration algorithm. The calibration
 * constants are stored in EEPROM on the sensor itself (832 bytes).
 *
 * Process each frame:
 *   1. Read 832 words of EEPROM once on begin() — stored in RAM
 *   2. Extract ~50 calibration parameters from EEPROM
 *   3. Each frame: read 832 words of frame data
 *   4. Apply compensation: ambient temp, supply voltage, per-pixel
 *      gain, offset, sensitivity, emissivity, alpha coefficients
 *   5. Output 768 floats in °C
 *
 * ═══════════════════════════════════════════════════════════════════
 * MEMORY
 * ═══════════════════════════════════════════════════════════════════
 * Heavy sensor on RAM:
 *   _params:   calibration struct ~300 bytes
 *   _frame:    832 × uint16_t = 1664 bytes (raw frame buffer)
 *   _pixels:   768 × float = 3072 bytes (temperature output)
 *   Total: ~5KB — fine on ESP32 (520KB SRAM)
 *
 * ═══════════════════════════════════════════════════════════════════
 * WIRING
 * ═══════════════════════════════════════════════════════════════════
 *   MLX90640 SDA → ESP32 SDA (2.2kΩ pull-up to 3.3V for fast I2C)
 *   MLX90640 SCL → ESP32 SCL (2.2kΩ pull-up to 3.3V)
 *   MLX90640 VDD → 3.3V
 *   MLX90640 GND → GND
 *   AD0/AD1/AD2  → GND (address = 0x33)
 *
 * ⚠️ DECOUPLING IS CRITICAL:
 *   Place 100µF electrolytic + 100nF ceramic right at VDD pin.
 *   The sensor draws current spikes during reads that can corrupt
 *   EEPROM data if VDD glitches. Without caps, calibration fails.
 *
 * ⚠️ PULL-UP STRENGTH:
 *   Standard 4.7kΩ may be too weak at 400kHz+.
 *   Use 2.2kΩ for reliable operation above 400kHz.
 *
 * ═══════════════════════════════════════════════════════════════════
 * USAGE
 * ═══════════════════════════════════════════════════════════════════
 *   auto* cam = sensors.addI2C<WyMLX90640>("thermal", SDA, SCL, 0x33);
 *   cam->setFrameRate(MLX_FPS_4);
 *   cam->setEmissivity(0.95f);    // 0.95 = most surfaces, 1.0 = blackbody
 *   sensors.begin();
 *
 *   WySensorData d = sensors.read("thermal");
 *   if (d.ok) {
 *       float* frame = cam->pixels();     // 768 floats, row-major [row*32+col]
 *       float hotspot = cam->maxTemp();
 *       float centre  = cam->centerTemp();
 *   }
 *
 * WySensorData:
 *   d.temperature = ambient sensor temp (Ta, °C)
 *   d.raw         = hottest pixel temperature (°C)
 *   d.rawInt      = index of hottest pixel (0–767)
 *   d.ok          = true when frame is valid
 *
 * ═══════════════════════════════════════════════════════════════════
 * FALSE COLOUR DISPLAY
 * ═══════════════════════════════════════════════════════════════════
 *   float* px = cam->pixels();
 *   float lo = cam->minTemp(), hi = cam->maxTemp();
 *   for (int i = 0; i < 768; i++) {
 *       float t = (px[i] - lo) / (hi - lo);   // normalise 0.0–1.0
 *       uint16_t c = WyMLX90640::ironbow(t);   // RGB565
 *       tft.fillRect((i%32)*scale, (i/32)*scale, scale, scale, c);
 *   }
 *
 * Or use the templated render() method for a one-liner.
 * Ironbow: black → blue → purple → red → orange → yellow → white
 */

#pragma once
#include "../WySensors.h"
#include <Wire.h>

/* Frame rate codes (control register bits [9:7]) */
#define MLX_FPS_0_5   0x00
#define MLX_FPS_1     0x01
#define MLX_FPS_2     0x02   /* default */
#define MLX_FPS_4     0x03
#define MLX_FPS_8     0x04
#define MLX_FPS_16    0x05
#define MLX_FPS_32    0x06   /* requires 1MHz I2C */

/* MLX90640 register addresses */
#define MLX_REG_STATUS      0x8000
#define MLX_REG_CTRL1       0x800D
#define MLX_REG_EEPROM_BASE 0x2400
#define MLX_REG_FRAME_BASE  0x0400

#define MLX_PIXEL_COUNT  768   /* 32 × 24 */
#define MLX_WORDS        832   /* EEPROM and frame data size */

/* Extracted calibration parameters */
struct MLXParams {
    int16_t  offset[MLX_PIXEL_COUNT];
    float    alpha[MLX_PIXEL_COUNT];
    float    kta[MLX_PIXEL_COUNT];
    float    kv[MLX_PIXEL_COUNT];

    float    KsTa;
    float    KsTo[5];
    float    ct[5];
    float    tgc;
    float    cpAlpha[2];
    int16_t  cpOffset[2];
    float    cpKta, cpKv;
    int16_t  gainEE;
    float    vdd25, KvVdd;
    float    KvPtat, KtPtat, alphaPTAT;
    int16_t  ptatRef;
    uint8_t  resolution;
    bool     valid;
};

class WyMLX90640 : public WySensorBase {
public:
    WyMLX90640(WyI2CPins pins) : _pins(pins) {}

    const char* driverName() override { return "MLX90640"; }

    void setFrameRate(uint8_t fps)  { _fps = fps & 0x07; }
    void setEmissivity(float e)     { _emissivity = constrain(e, 0.01f, 1.0f); }

    float*  pixels()     { return _pixels; }
    float   minTemp()    { return _tmin; }
    float   maxTemp()    { return _tmax; }
    float   centerTemp() { return _pixels[11 * 32 + 15]; }

    /* Ironbow false-colour palette (RGB565) — 0.0=cold, 1.0=hot */
    static uint16_t ironbow(float t) {
        t = constrain(t, 0.0f, 1.0f);
        /* Ironbow: black→indigo→blue→magenta→red→orange→yellow→white */
        /* Sampled RGB565 palette at 32 evenly-spaced points */
        static const uint16_t LUT[33] = {
            0x0000, 0x000B, 0x0013, 0x080E, 0x1009, 0x200A, 0x380C,
            0x5010, 0x6815, 0x801B, 0x9020, 0xA020, 0xB010, 0xC001,
            0xC801, 0xD001, 0xD800, 0xE000, 0xE800, 0xF000, 0xF800,
            0xF900, 0xFA00, 0xFB00, 0xFC00, 0xFCC0, 0xFDA0, 0xFE80,
            0xFF60, 0xFF20, 0xFF80, 0xFFC0, 0xFFFF
        };
        uint8_t idx = (uint8_t)(t * 32.0f);
        if (idx > 32) idx = 32;
        return LUT[idx];
    }

    /* Rainbow palette alternative (blue→cyan→green→yellow→red) */
    static uint16_t rainbow(float t) {
        t = constrain(t, 0.0f, 1.0f);
        float r = 0, g = 0, b = 0;
        if      (t < 0.25f) { b = 1.0f; g = t * 4.0f; }
        else if (t < 0.5f)  { b = 1.0f - (t - 0.25f) * 4.0f; g = 1.0f; }
        else if (t < 0.75f) { g = 1.0f; r = (t - 0.5f) * 4.0f; }
        else                { g = 1.0f - (t - 0.75f) * 4.0f; r = 1.0f; }
        return ((uint16_t)(r * 31) << 11) | ((uint16_t)(g * 63) << 5) | (uint16_t)(b * 31);
    }

    /* Render frame to display — works with TFT_eSPI or Arduino_GFX.
     * scale=6 → 192×144 px on a 320×240 screen (CYD)
     * scale=10 → 320×240 (fills CYD screen perfectly, 32×10=320, 24×10=240) */
    template<typename TFT>
    void render(TFT* tft, int16_t x, int16_t y, uint8_t scale = 6,
                float tmin = 0.0f, float tmax = 0.0f) {
        if (tmin == tmax) { tmin = _tmin; tmax = _tmax; }
        float range = (tmax - tmin > 0.1f) ? (tmax - tmin) : 0.1f;
        for (int row = 0; row < 24; row++) {
            for (int col = 0; col < 32; col++) {
                float t = (_pixels[row * 32 + col] - tmin) / range;
                tft->fillRect(x + col * scale, y + row * scale,
                              scale, scale, ironbow(t));
            }
        }
    }

    bool begin() override {
        Wire.begin(_pins.sda, _pins.scl);
        Wire.setClock(400000);
        delay(80);  /* sensor boot */

        /* Read calibration EEPROM */
        uint16_t eeprom[MLX_WORDS];
        if (!_readWords(MLX_REG_EEPROM_BASE, eeprom, MLX_WORDS)) {
            Serial.println("[MLX90640] EEPROM read failed — check wiring and decoupling caps");
            return false;
        }

        _extractParams(eeprom);
        if (!_params.valid) {
            Serial.println("[MLX90640] calibration extraction failed");
            return false;
        }

        /* Set frame rate in control register */
        uint16_t ctrl = 0x1901;  /* default after reset */
        ctrl = (ctrl & ~(0x07 << 7)) | ((_fps & 0x07) << 7);
        _writeReg(MLX_REG_CTRL1, ctrl);

        if (_fps >= MLX_FPS_8) Wire.setClock(1000000);

        Serial.printf("[MLX90640] ready — FPS:%s emissivity:%.2f\n", _fpsStr(), _emissivity);
        return true;
    }

    WySensorData read() override {
        WySensorData d;

        /* Poll status register for new data flag (bit 3) */
        uint32_t deadline = millis() + 3000;
        uint16_t status = 0;
        while (!(status & 0x0008) && millis() < deadline) {
            if (!_readReg(MLX_REG_STATUS, status)) { d.error = "status fail"; return d; }
            if (!(status & 0x0008)) delay(5);
        }
        if (!(status & 0x0008)) { d.error = "frame timeout"; return d; }

        /* Read frame */
        uint16_t frame[MLX_WORDS];
        if (!_readWords(MLX_REG_FRAME_BASE, frame, MLX_WORDS)) {
            d.error = "frame read fail"; return d;
        }

        /* Clear data-ready flag */
        _writeReg(MLX_REG_STATUS, status & ~0x0008);

        /* Compute temperatures */
        float Ta = _calcTa(frame);
        _calcPixels(frame, Ta);

        d.temperature = Ta;
        d.raw    = _tmax;
        d.rawInt = _tmaxIdx;
        d.ok     = true;
        return d;
    }

private:
    WyI2CPins _pins;
    uint8_t   _fps        = MLX_FPS_2;
    float     _emissivity = 1.0f;
    MLXParams _params     = {};
    float     _pixels[MLX_PIXEL_COUNT] = {};
    float     _tmin = 0.0f, _tmax = 0.0f;
    int       _tmaxIdx = 0;

    /* ── Calibration extraction (AN#0101) ─────────────────────────── */
    void _extractParams(uint16_t* ee) {
        /* Gain */
        _params.gainEE = (int16_t)ee[0x30];

        /* Vdd */
        int16_t KvVdd_raw = (int8_t)(ee[0x33] >> 8);
        _params.KvVdd = KvVdd_raw * 32.0f;
        int16_t vdd25_raw = (int8_t)(ee[0x33] & 0xFF) - 256;
        _params.vdd25 = (float)((vdd25_raw - 256) * 32 - 8192);

        /* Ta */
        int16_t KvPtat_raw = (int16_t)(ee[0x32] >> 10);
        if (KvPtat_raw > 31) KvPtat_raw -= 64;
        _params.KvPtat = KvPtat_raw / 4096.0f;

        int16_t KtPtat_raw = (int16_t)(ee[0x32] & 0x03FF);
        if (KtPtat_raw > 511) KtPtat_raw -= 1024;
        _params.KtPtat = KtPtat_raw / 8.0f;

        _params.ptatRef   = (int16_t)ee[0x31];
        _params.alphaPTAT = (float)((ee[0x10] >> 12) & 0xF) / 4.0f + 8.0f;
        _params.resolution = (uint8_t)((ee[0x39] >> 12) & 0x03);

        /* KsTa */
        _params.KsTa = (int8_t)(ee[0x3C] >> 8) / 8192.0f;

        /* KsTo */
        float ksToScale = 1.0f / (float)(1 << ((ee[0x3F] & 0xF) + 8));
        _params.KsTo[0] = (int8_t)( ee[0x3D] & 0xFF)         * ksToScale;
        _params.KsTo[1] = (int8_t)((ee[0x3D] >> 8) & 0xFF)   * ksToScale;
        _params.KsTo[2] = (int8_t)( ee[0x3E] & 0xFF)         * ksToScale;
        _params.KsTo[3] = (int8_t)((ee[0x3E] >> 8) & 0xFF)   * ksToScale;
        _params.KsTo[4] = -0.0002f;
        _params.ct[0] = -40.0f; _params.ct[1] = 0.0f;
        _params.ct[2] = ((ee[0x3F] >> 4) & 0xF) * 10.0f;
        _params.ct[3] = ((ee[0x3F] >> 8) & 0xFF) * 10.0f;
        _params.ct[4] = 400.0f;

        /* TGC */
        _params.tgc = (int8_t)(ee[0x3C] & 0xFF) / 32.0f;

        /* Alpha scale */
        uint8_t alphaScale = ((ee[0x20] >> 12) & 0xF) + 30;
        float   alphaBase  = (float)ee[0x21];
        float   alphaScaleF = 1.0f / (float)(1UL << alphaScale);

        /* Per-pixel: offset, alpha, kta, kv */
        int16_t occRef = (int16_t)(ee[0x11] >> 10);
        uint8_t ktaScale1 = ((ee[0x3A] >> 4) & 0xF) + 8;
        uint8_t ktaScale2 =  (ee[0x3A] & 0xF);
        uint8_t kvScale   = ((ee[0x38] >> 8) & 0xF);

        /* Row and column base arrays */
        int16_t occRow[24], occCol[32];
        for (int i = 0; i < 6; i++) {
            uint16_t w = ee[0x12 + i];
            occRow[i*4+0] = _s4((w>>0)&0xF); occRow[i*4+1] = _s4((w>>4)&0xF);
            occRow[i*4+2] = _s4((w>>8)&0xF); occRow[i*4+3] = _s4((w>>12)&0xF);
        }
        for (int i = 0; i < 8; i++) {
            uint16_t w = ee[0x18 + i];
            occCol[i*4+0] = _s4((w>>0)&0xF); occCol[i*4+1] = _s4((w>>4)&0xF);
            occCol[i*4+2] = _s4((w>>8)&0xF); occCol[i*4+3] = _s4((w>>12)&0xF);
        }
        uint8_t aRow[24], aCol[32];
        for (int i = 0; i < 6; i++) {
            uint16_t w = ee[0x22 + i];
            aRow[i*4+0] = (w>>0)&0xF; aRow[i*4+1] = (w>>4)&0xF;
            aRow[i*4+2] = (w>>8)&0xF; aRow[i*4+3] = (w>>12)&0xF;
        }
        for (int i = 0; i < 8; i++) {
            uint16_t w = ee[0x28 + i];
            aCol[i*4+0] = (w>>0)&0xF; aCol[i*4+1] = (w>>4)&0xF;
            aCol[i*4+2] = (w>>8)&0xF; aCol[i*4+3] = (w>>12)&0xF;
        }
        int8_t ktaCol[2] = { _s4((ee[0x3B]>>8)&0xF), _s4((ee[0x3B])&0xF) };
        int8_t kvRow[2]  = { _s4((ee[0x34]>>12)&0xF), _s4((ee[0x34]>>8)&0xF) };
        int8_t kvCol[2]  = { _s4((ee[0x34]>>4)&0xF),  _s4((ee[0x34])&0xF) };

        for (int i = 0; i < MLX_PIXEL_COUNT; i++) {
            int row = i / 32, col = i % 32;
            /* Pixel data at EEPROM offset 0x40 = 64 words in */
            uint16_t w = ee[0x40 + i];

            /* Offset: bits [15:10], 6-bit signed */
            int16_t pOff = _s6((w >> 10) & 0x3F);
            _params.offset[i] = pOff + occRef + occRow[row] + occCol[col];

            /* Alpha: bits [9:4], 6-bit unsigned + row/col/reference */
            uint8_t pAlpha = (w >> 4) & 0x3F;
            _params.alpha[i] = (alphaBase +
                (float)aRow[row] * (float)(1 << (alphaScale - 4)) +
                (float)aCol[col] * (float)(1 << (alphaScale - 4)) +
                (float)pAlpha) * alphaScaleF;

            /* Kta: bit [3] = row parity contribution */
            int8_t pKta = (w >> 3) & 0x01 ? 1 : -1;
            _params.kta[i] = (float)(ktaCol[col & 1] + pKta * (1 << ktaScale2)) /
                              (float)(1 << ktaScale1);

            /* Kv: row/col base only */
            _params.kv[i] = (float)(kvRow[row & 1] + kvCol[col & 1]) /
                             (float)(1 << kvScale);
        }

        /* Compensation pixel */
        _params.cpAlpha[0]  = (float)((ee[0x3F] >> 10) & 0x3F) * alphaScaleF;
        _params.cpAlpha[1]  = _params.cpAlpha[0] * (1.0f + (float)(ee[0x3F] & 0x3F) / 128.0f);
        _params.cpOffset[0] = _s6((ee[0x3E] >> 10) & 0x3F);
        _params.cpOffset[1] = _params.cpOffset[0] + _s6((ee[0x3E] >> 4) & 0x3F);
        _params.cpKta = (float)(int8_t)(ee[0x3E] & 0x0F) / (float)(1 << ktaScale1);
        _params.cpKv  = (float)(int8_t)((ee[0x3E] >> 4) & 0x0F) / (float)(1 << kvScale);

        _params.valid = true;
    }

    /* ── Temperature computation ──────────────────────────────────── */

    float _calcTa(uint16_t* frame) {
        /* VDD from RAM word 810 */
        int16_t vddRaw = (int16_t)frame[810];
        float vdd = (float)_params.gainEE / (float)(1 << _params.resolution) *
                    ((float)vddRaw - _params.vdd25) / _params.KvVdd + 3.3f;

        /* PTAT from RAM word 800 */
        int16_t ptatRaw = (int16_t)frame[800];
        float dV = vdd - 3.3f;
        float Ta = (float)ptatRaw / (_params.KvPtat * dV + _params.KtPtat) -
                   _params.alphaPTAT + 25.0f;
        return Ta;
    }

    void _calcPixels(uint16_t* frame, float Ta) {
        /* Gain from RAM word 778 */
        int16_t gainRaw = (int16_t)frame[778];
        float gain = (float)_params.gainEE / (float)gainRaw;

        /* Vdd */
        int16_t vddRaw = (int16_t)frame[810];
        float vdd = gain * ((float)vddRaw - _params.vdd25) / _params.KvVdd + 3.3f;
        float dV  = vdd - 3.3f;
        float dTa = Ta - 25.0f;

        /* Compensation pixel */
        float cpPix[2];
        for (int sp = 0; sp < 2; sp++) {
            int16_t cpRaw = (int16_t)frame[808 + sp];
            cpPix[sp] = gain * (float)cpRaw
                        - _params.cpOffset[sp] * (1.0f + _params.cpKta * dTa)
                                                * (1.0f + _params.cpKv * dV);
        }

        _tmin = 999.0f; _tmax = -999.0f;

        /* Thermal gradient compensation reference */
        float tgcCp = _params.tgc * ((cpPix[0] + cpPix[1]) * 0.5f);

        for (int i = 0; i < MLX_PIXEL_COUNT; i++) {
            int row = i / 32;
            int sp  = (row & 1);  /* chess pattern subpage */

            int16_t pixRaw = (int16_t)frame[i];
            float pix = gain * (float)pixRaw
                        - _params.offset[i] * (1.0f + _params.kta[i] * dTa)
                                            * (1.0f + _params.kv[i]  * dV);

            /* Subtract TGC-compensated reference */
            float Vir = (pix - tgcCp) / _emissivity;

            /* Alpha (sensitivity) */
            float alpha = _params.alpha[i] - _params.tgc * _params.cpAlpha[sp];
            alpha /= (1.0f + _params.KsTa * dTa);

            /* IR compensated (object temperature estimate) */
            float TaK4 = (Ta + 273.15f);
            TaK4 = TaK4 * TaK4 * TaK4 * TaK4;

            float To = sqrtf(sqrtf(Vir / alpha + TaK4)) - 273.15f;

            /* KsTo range correction */
            for (int r = 0; r < 4; r++) {
                if (To >= _params.ct[r] && To < _params.ct[r + 1]) {
                    float correction = 1.0f;
                    for (int c = 0; c <= r; c++) correction *= 1.0f + _params.KsTo[c] * 10.0f;
                    To /= correction;
                    break;
                }
            }

            _pixels[i] = To;
            if (To < _tmin) _tmin = To;
            if (To > _tmax) { _tmax = To; _tmaxIdx = i; }
        }
    }

    /* ── I2C low level ────────────────────────────────────────────── */
    /* MLX90640 uses 16-bit register addresses + 16-bit data words */

    bool _readReg(uint16_t reg, uint16_t& val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write((uint8_t)(reg >> 8));
        Wire.write((uint8_t)(reg & 0xFF));
        if (Wire.endTransmission(false) != 0) return false;
        Wire.requestFrom(_pins.addr, (uint8_t)2);
        if (Wire.available() < 2) return false;
        val = ((uint16_t)Wire.read() << 8) | Wire.read();
        return true;
    }

    void _writeReg(uint16_t reg, uint16_t val) {
        Wire.beginTransmission(_pins.addr);
        Wire.write((uint8_t)(reg >> 8));
        Wire.write((uint8_t)(reg & 0xFF));
        Wire.write((uint8_t)(val >> 8));
        Wire.write((uint8_t)(val & 0xFF));
        Wire.endTransmission();
    }

    /* Read N 16-bit words starting at reg into buf */
    bool _readWords(uint16_t regStart, uint16_t* buf, uint16_t count) {
        /* MLX90640 I2C protocol: send 2-byte register address,
         * then read count×2 bytes. Max 32 bytes per Wire transaction
         * on most Arduino cores — must chunk into pages. */
        const uint8_t CHUNK = 16;  /* 16 words = 32 bytes per transaction */
        for (uint16_t w = 0; w < count; w += CHUNK) {
            uint16_t reg  = regStart + w;
            uint16_t todo = min((uint16_t)(count - w), (uint16_t)CHUNK);

            Wire.beginTransmission(_pins.addr);
            Wire.write((uint8_t)(reg >> 8));
            Wire.write((uint8_t)(reg & 0xFF));
            if (Wire.endTransmission(false) != 0) return false;

            Wire.requestFrom(_pins.addr, (uint8_t)(todo * 2));
            for (uint16_t j = 0; j < todo; j++) {
                if (Wire.available() < 2) return false;
                buf[w + j] = ((uint16_t)Wire.read() << 8) | Wire.read();
            }
        }
        return true;
    }

    /* ── Bit helpers ──────────────────────────────────────────────── */
    static int8_t  _s4(uint8_t v)  { return (v & 0x8) ? (int8_t)(v | 0xF0) : (int8_t)v; }
    static int8_t  _s6(uint8_t v)  { return (v & 0x20) ? (int8_t)(v | 0xC0) : (int8_t)v; }
    static int8_t  _s1(uint8_t v)  { return v ? -1 : 1; }

    const char* _fpsStr() {
        static const char* fps[] = {"0.5","1","2","4","8","16","32"};
        return (_fps < 7) ? fps[_fps] : "?";
    }
};
