// test_sensor_math.cpp — pure math validation for sensor conversion formulas
// No Arduino SDK required — tests the calculation layer only.
//
// Build: g++ -std=c++17 -DHOST_TEST -Isrc test/test_sensor_math.cpp -o test/test_sensor_math -lm
//
// Covers:
//   WyMQ    — ADC → voltage → Rs → ppm (power-law curve)
//   WyGP2Y  — voltage → distance (IR power-law curve, 3 sensor models)
//   WyHCSR04— echo duration → distance (temperature-compensated SoS)
//   WyGUVAS12SD — ADC → UVI conversion
//   WySensorData — struct defaults and field behaviour

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdint.h>

static int _pass=0, _fail=0;
#define PASS(n)         do{printf("  PASS: %s\n",n);_pass++;}while(0)
#define FAIL(n,m)       do{printf("  FAIL: %s  (%s)\n",n,m);_fail++;}while(0)
#define CHECK(c,n,m)    do{if(c)PASS(n);else FAIL(n,m);}while(0)
#define SECTION(s)      printf("\n  [%s]\n",s)
#define NEAR(a,b,eps)   (fabsf((float)(a)-(float)(b)) < (float)(eps))

// ─────────────────────────────────────────────────────────────────────────────
// WySensorData minimal mirror (no Arduino dependency)
// ─────────────────────────────────────────────────────────────────────────────
struct WySensorData {
    float       temperature = 0;   // °C
    float       humidity    = 0;   // %RH (or category index for UV)
    float       pressure    = 0;   // hPa
    float       light       = 0;   // lux (or sensor voltage for UV)
    float       co2         = 0;   // ppm CO2 / primary target gas ppm
    float       raw         = 0;   // raw ADC / UVI / distance cm
    float       voltage     = 0;   // mV (or mW/cm² for UV)
    int32_t     rawInt      = 0;   // integer raw (ADC counts, AQI, µs)
    const char* error       = nullptr;
    bool        ok          = false;
};

// ─────────────────────────────────────────────────────────────────────────────
// WyMQ math (mirrors WyMQ.h formulas exactly)
// ─────────────────────────────────────────────────────────────────────────────
#define WY_MQ_ADC_BITS  12
#define WY_MQ_VREF_MV   3300.0f
#define WY_MQ_RLOAD_KOHM 10.0f

// raw ADC (0..4095) → Rs (kΩ)
static float mq_adcToRs(int32_t raw) {
    float vMV = (raw / (float)((1 << WY_MQ_ADC_BITS) - 1)) * WY_MQ_VREF_MV;
    if (vMV <= 0) return 1e9f;
    return (WY_MQ_VREF_MV - vMV) / vMV * WY_MQ_RLOAD_KOHM;
}

// Rs, R0, curve constants → ppm
static float mq_rsToPpm(float rs, float r0, float a, float b) {
    if (r0 <= 0) return 0;
    float ratio = rs / r0;
    return a * powf(ratio, b);
}

// ─────────────────────────────────────────────────────────────────────────────
// WyGP2Y math
// ─────────────────────────────────────────────────────────────────────────────
#define WY_GP2Y_VREF_MV  3300.0f
#define WY_GP2Y_ADC_BITS 12

struct WyGP2YCurve { float A; float B; float minCm; float maxCm; };
// GP2Y0A02 (20–150cm)
static const WyGP2YCurve GP2Y_A02 = {29.988f, -1.173f, 10.0f, 80.0f};
// GP2Y0A710K (100–550cm)
static const WyGP2YCurve GP2Y_A710 = {1081.0f, -0.895f, 100.0f, 550.0f};
// GP2Y0A41 (4–30cm short range)
static const WyGP2YCurve GP2Y_A41 = {61.573f, -1.1015f, 20.0f, 150.0f};

static float gp2y_adcToVoltageV(int32_t raw) {
    return (raw / (float)((1 << WY_GP2Y_ADC_BITS) - 1)) * (WY_GP2Y_VREF_MV / 1000.0f);
}

static float gp2y_voltToCm(float vV, const WyGP2YCurve& c) {
    // vV=0 → powf(0, negative) = +inf → cm=inf > maxCm → returns -1 naturally
    float cm = (vV <= 0) ? 1e30f : c.A * powf(vV, c.B);
    if (cm < c.minCm) return -1.0f;
    if (cm > c.maxCm) return -1.0f;
    return cm;
}

// ─────────────────────────────────────────────────────────────────────────────
// WyHCSR04 math
// ─────────────────────────────────────────────────────────────────────────────
static float hcsr04_durationToCm(uint32_t durationUs, float tempC) {
    float speedCmUs = (331.4f + 0.606f * tempC) / 10000.0f;
    return (durationUs * speedCmUs) / 2.0f;
}

// ─────────────────────────────────────────────────────────────────────────────
// WyGUVAS12SD math
// ─────────────────────────────────────────────────────────────────────────────
#define WY_UVI_PER_MW_CM2  40.0f   // UVI = irradiance(mW/cm²) × 40

static float guvas_adcToUVI(uint16_t raw, float darkV=0.0f,
                              float sensitivity=0.1f, float divRatio=1.0f) {
    float adcV    = (raw / 4095.0f) * 3.3f;
    float sensorV = adcV / divRatio;
    float uvV     = sensorV - darkV;
    if (uvV < 0) uvV = 0;
    float irradiance = uvV / sensitivity;
    float uvi = irradiance * WY_UVI_PER_MW_CM2;
    return uvi < 0 ? 0 : uvi;
}

static uint8_t guvas_uviCategory(float uvi) {
    if (uvi < 3)  return 0;
    if (uvi < 6)  return 1;
    if (uvi < 8)  return 2;
    if (uvi < 11) return 3;
    return 4;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests
// ─────────────────────────────────────────────────────────────────────────────
int main() {
    printf("\n========================================\n");
    printf("  Sensor math unit tests\n");
    printf("========================================\n");

    // ── WyMQ ADC → Rs conversion ───────────────────────────────────────────
    SECTION("WyMQ: ADC → voltage → Rs");
    {
        // At full scale (4095): vMV = 3300, Rs = 0
        float rs = mq_adcToRs(4095);
        CHECK(NEAR(rs, 0.0f, 0.01f), "ADC=4095 → Rs≈0 kΩ (full scale)", "non-zero");

        // At half scale (2047): vMV≈1650, Rs = (3300-1650)/1650*10 ≈ 10 kΩ
        rs = mq_adcToRs(2047);
        CHECK(NEAR(rs, 10.0f, 0.5f), "ADC=2047 → Rs≈10 kΩ (half scale)", "wrong");

        // At 1/4 scale (1024): vMV≈824.4, Rs = (3300-824.4)/824.4*10 ≈ 30 kΩ
        rs = mq_adcToRs(1024);
        CHECK(rs > 25.0f && rs < 35.0f, "ADC=1024 → Rs in [25,35] kΩ", "out of range");

        // At zero: returns very large (infinite Rs)
        rs = mq_adcToRs(0);
        CHECK(rs > 1e6f, "ADC=0 → Rs very large (no current)", "too small");

        // Rs must be non-negative
        CHECK(mq_adcToRs(2000) >= 0, "ADC=2000 → Rs >= 0", "negative");
        CHECK(mq_adcToRs(3000) >= 0, "ADC=3000 → Rs >= 0", "negative");
    }

    // ── WyMQ ppm curve ────────────────────────────────────────────────────
    SECTION("WyMQ: Rs/R0 → ppm (power-law curve)");
    {
        // MQ-135 CO2: a=110.47, b=-2.862
        // At ratio=1.0 (Rs=R0 = clean air): ppm = 110.47 * 1^(-2.862) = 110.47
        float ppm = mq_rsToPpm(10.0f, 10.0f, 110.47f, -2.862f);
        CHECK(NEAR(ppm, 110.47f, 2.0f), "MQ135 ratio=1.0 → ppm≈110 (clean air)", "wrong");

        // At ratio=0.5 (Rs=R0/2 — higher gas concentration): ppm should be > clean air
        ppm = mq_rsToPpm(5.0f, 10.0f, 110.47f, -2.862f);
        CHECK(ppm > 110.47f, "MQ135 ratio=0.5 → ppm > clean air value", "not higher");

        // MQ-2 LPG: a=574.25, b=-2.222 — check clean air baseline
        ppm = mq_rsToPpm(10.0f, 10.0f, 574.25f, -2.222f);
        CHECK(NEAR(ppm, 574.25f, 10.0f), "MQ2 ratio=1.0 → ppm≈574 (clean air)", "wrong");

        // R0=0 returns 0 (guard)
        ppm = mq_rsToPpm(10.0f, 0.0f, 110.47f, -2.862f);
        CHECK(ppm == 0.0f, "R0=0 → ppm=0 (guard)", "non-zero");

        // Monotonic: higher Rs/R0 ratio → lower ppm (negative exponent)
        float ppm_hi = mq_rsToPpm(20.0f, 10.0f, 110.47f, -2.862f); // ratio=2 → lower
        float ppm_lo = mq_rsToPpm(5.0f,  10.0f, 110.47f, -2.862f); // ratio=0.5 → higher
        CHECK(ppm_lo > ppm_hi, "negative exponent: higher ratio → lower ppm", "not monotonic");
    }

    // ── WyMQ calibration math ─────────────────────────────────────────────
    SECTION("WyMQ: R0 calibration");
    {
        // If 100 clean-air ADC readings average to 2047 → Rs≈10 kΩ → R0=10
        // Simulate average of uniform readings
        float sum = 0; int N = 100;
        for (int i = 0; i < N; i++) {
            int32_t raw = 2047;
            float vMV = (raw / (float)((1<<12)-1)) * 3300.0f;
            float rs  = (3300.0f - vMV) / vMV * 10.0f;
            sum += rs;
        }
        float r0 = sum / N;
        CHECK(NEAR(r0, 10.0f, 0.5f), "calibration of 100 identical ADC=2047 → R0≈10 kΩ", "wrong");

        // Different ADC values give different R0
        float sum2 = 0;
        for (int i = 0; i < N; i++) {
            int32_t raw = 3000;
            float vMV = (raw / (float)((1<<12)-1)) * 3300.0f;
            sum2 += (3300.0f - vMV) / vMV * 10.0f;
        }
        float r0b = sum2 / N;
        CHECK(r0b < r0, "ADC=3000 clean air → R0 lower than ADC=2047 (higher voltage = lower Rs)", "wrong");
    }

    // ── WyGP2Y distance formulas ──────────────────────────────────────────
    SECTION("WyGP2Y0A02: voltage → distance");
    {
        // At 1.5V: cm = 29.988 * 1.5^(-1.173) ≈ 18.5 cm (within range 10–80)
        float vV = 1.5f;
        float cm = GP2Y_A02.A * powf(vV, GP2Y_A02.B);
        CHECK(cm > 10.0f && cm < 80.0f, "GP2Y A02: 1.5V → in-range distance", "out of range");

        // Higher voltage → shorter distance (inverse relationship)
        float cm_hi = GP2Y_A02.A * powf(2.0f, GP2Y_A02.B);
        float cm_lo = GP2Y_A02.A * powf(1.0f, GP2Y_A02.B);
        CHECK(cm_hi < cm_lo, "GP2Y: higher voltage → shorter distance", "not inverse");

        // Out of range clamps to -1
        CHECK(gp2y_voltToCm(3.0f, GP2Y_A02) == -1.0f,
              "GP2Y A02: 3.0V too close → -1 (below minCm)", "wrong");

        // Zero voltage → maxCm (no return)
        CHECK(gp2y_voltToCm(0.0f, GP2Y_A02) == -1.0f,
              "GP2Y A02: 0V → -1 (above maxCm)", "wrong");
    }

    SECTION("WyGP2Y: ADC → voltage → distance pipeline");
    {
        // ADC mid-range on A02: raw=2000 → vV≈1.61V → should give valid distance
        float vV = gp2y_adcToVoltageV(2000);
        CHECK(NEAR(vV, 1.610f, 0.05f), "ADC=2000 → vV≈1.61V", "wrong");
        float cm = gp2y_voltToCm(vV, GP2Y_A02);
        char msg[40]; snprintf(msg,sizeof(msg),"cm=%.1f",cm);
        CHECK(cm > 0, "ADC=2000 → valid distance on A02", msg);

        // ADC full scale: vV=3.3V → distance very small → out of A02 range
        float vV_fs = gp2y_adcToVoltageV(4095);
        CHECK(NEAR(vV_fs, 3.3f, 0.05f), "ADC=4095 → vV≈3.3V", "wrong");

        // GP2Y0A710K (long range) — needs higher ADC for valid reading
        // At raw=500: vV≈0.40V → cm = 1081*0.40^(-0.895)
        float vV2 = gp2y_adcToVoltageV(500);
        float cm2 = gp2y_voltToCm(vV2, GP2Y_A710);
        CHECK(cm2 < 0 || cm2 > 100.0f, "GP2Y A710: low voltage → near maxrange or -1", "wrong");
    }

    // ── WyHCSR04 echo timing → distance ───────────────────────────────────
    SECTION("WyHCSR04: echo duration → distance");
    {
        // At 20°C: speed = (331.4 + 0.606*20)/10000 = 0.033532 cm/µs
        // 1160µs → 1160 * 0.033532 / 2 ≈ 19.45 cm
        float cm = hcsr04_durationToCm(1160, 20.0f);
        CHECK(NEAR(cm, 19.45f, 0.5f), "1160µs at 20°C → ≈19.4 cm", "wrong");

        // At 0°C: speed = 331.4/10000 = 0.03314 cm/µs
        // 1000µs → 1000 * 0.03314 / 2 = 16.57 cm
        float cm0 = hcsr04_durationToCm(1000, 0.0f);
        CHECK(NEAR(cm0, 16.57f, 0.3f), "1000µs at 0°C → ≈16.6 cm", "wrong");

        // Temperature compensation: warmer → faster sound → longer reading
        float cm_cold = hcsr04_durationToCm(2000, 0.0f);
        float cm_warm = hcsr04_durationToCm(2000, 40.0f);
        CHECK(cm_warm > cm_cold, "higher temp → faster sound → larger cm reading", "wrong");

        // Known value: at 20°C, speed=0.034352 cm/µs; 2000µs → 2000*0.034352/2=34.35cm
        float cm_346 = hcsr04_durationToCm(2000, 20.0f);
        CHECK(NEAR(cm_346, 34.35f, 0.3f), "2000µs at 20°C → ≈34.4 cm", "wrong");

        // Zero duration → 0 cm
        CHECK(hcsr04_durationToCm(0, 20.0f) == 0.0f, "0µs → 0 cm", "non-zero");

        // Speed formula: 331.4 + 0.606*T
        float speed20 = (331.4f + 0.606f * 20.0f) / 10000.0f;
        CHECK(NEAR(speed20, 0.034352f, 0.0001f), "speed at 20°C = 0.03435 cm/µs", "wrong");
    }

    // ── WyGUVAS12SD UVI conversion ────────────────────────────────────────
    SECTION("WyGUVAS12SD: ADC → UV Index");
    {
        // No UV: ADC=0 → vV=0 → UVI=0
        float uvi = guvas_adcToUVI(0);
        CHECK(uvi == 0.0f, "ADC=0 → UVI=0 (no UV)", "non-zero");

        // At ADC=1000: adcV=1000/4095*3.3≈0.806V, sensorV=0.806V (divRatio=1)
        // irradiance = 0.806 / 0.1 = 8.06 mW/cm²  → UVI = 8.06 * 40 = 322 (bright sun)
        uvi = guvas_adcToUVI(1000);
        CHECK(NEAR(uvi, 322.0f, 5.0f), "ADC=1000 → UVI≈322 (no dark offset)", "wrong");

        // With dark offset: darkV=0.1V subtracts from sensorV
        // adcV same, uvV = 0.806 - 0.1 = 0.706V → irr=7.06 → UVI≈282
        float uvi_dark = guvas_adcToUVI(1000, 0.1f);
        CHECK(uvi_dark < uvi, "dark offset reduces UVI", "not smaller");
        CHECK(NEAR(uvi_dark, 282.0f, 5.0f), "ADC=1000 with 0.1V dark → UVI≈282", "wrong");

        // Negative uvV clamps to 0
        float uvi_neg = guvas_adcToUVI(100, 2.0f);  // darkV > sensorV
        CHECK(uvi_neg == 0.0f, "dark offset > signal → UVI clamped to 0", "non-zero");

        // Voltage divider: divRatio=2 → sensorV doubled
        float uvi_div = guvas_adcToUVI(500, 0.0f, 0.1f, 2.0f);
        float uvi_nodiv = guvas_adcToUVI(500, 0.0f, 0.1f, 1.0f);
        // divRatio=2 divides sensorV by 2 → half the UVI (accounts for hardware voltage divider)
        CHECK(NEAR(uvi_div * 2.0f, uvi_nodiv, 1.0f), "divRatio=2 → half UVI (voltage divider)", "wrong");
    }

    SECTION("WyGUVAS12SD: UVI category");
    {
        CHECK(guvas_uviCategory(0.0f) == 0,  "UVI 0 → Low (0)", "wrong");
        CHECK(guvas_uviCategory(2.9f) == 0,  "UVI 2.9 → Low (0)", "wrong");
        CHECK(guvas_uviCategory(3.0f) == 1,  "UVI 3 → Moderate (1)", "wrong");
        CHECK(guvas_uviCategory(5.9f) == 1,  "UVI 5.9 → Moderate (1)", "wrong");
        CHECK(guvas_uviCategory(6.0f) == 2,  "UVI 6 → High (2)", "wrong");
        CHECK(guvas_uviCategory(7.9f) == 2,  "UVI 7.9 → High (2)", "wrong");
        CHECK(guvas_uviCategory(8.0f) == 3,  "UVI 8 → Very High (3)", "wrong");
        CHECK(guvas_uviCategory(10.9f) == 3, "UVI 10.9 → Very High (3)", "wrong");
        CHECK(guvas_uviCategory(11.0f) == 4, "UVI 11 → Extreme (4)", "wrong");
        CHECK(guvas_uviCategory(16.0f) == 4, "UVI 16 → Extreme (4)", "wrong");
    }

    // ── WySensorData struct defaults ──────────────────────────────────────
    SECTION("WySensorData: struct defaults and state");
    {
        WySensorData d;
        CHECK(d.temperature == 0.0f,  "temperature default=0", "non-zero");
        CHECK(d.humidity    == 0.0f,  "humidity default=0", "non-zero");
        CHECK(d.pressure    == 0.0f,  "pressure default=0", "non-zero");
        CHECK(d.light       == 0.0f,  "light default=0", "non-zero");
        CHECK(d.co2         == 0.0f,  "co2 default=0", "non-zero");
        CHECK(d.raw         == 0.0f,  "raw default=0", "non-zero");
        CHECK(d.voltage     == 0.0f,  "voltage default=0", "non-zero");
        CHECK(d.rawInt      == 0,     "rawInt default=0", "non-zero");
        CHECK(d.ok          == false, "ok default=false", "true");
        CHECK(d.error       == nullptr, "error default=nullptr", "non-null");

        // Set ok=true and verify fields are writable
        d.ok = true; d.temperature = 23.5f; d.humidity = 55.0f;
        CHECK(d.ok,                    "ok settable to true", "false");
        CHECK(NEAR(d.temperature, 23.5f, 0.001f), "temperature writable", "wrong");
        CHECK(NEAR(d.humidity,    55.0f, 0.001f), "humidity writable", "wrong");

        // Error state
        WySensorData err;
        err.error = "sensor not found";
        CHECK(err.ok == false,          "error state: ok=false", "true");
        CHECK(err.error != nullptr,     "error state: error set", "null");
        CHECK(strcmp(err.error, "sensor not found")==0,
              "error message preserved", "wrong string");
    }

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", _pass, _fail);
    printf("========================================\n");
    return _fail ? 1 : 0;
}
