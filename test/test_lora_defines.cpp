// test_lora_defines.cpp — WySX127x.h define validation
// Compiles once per LoRa module variant, checks pin and frequency defines.
//
// Build: g++ -std=c++17 -DHOST_TEST -DWY_LORA_SX1276 -Isrc test/test_lora_defines.cpp -o /tmp/t

#include <stdio.h>
#include <stdint.h>

static int _pass=0, _fail=0;
#define PASS(n)      do{printf("  PASS: %s\n",n);_pass++;}while(0)
#define FAIL(n,m)    do{printf("  FAIL: %s  (%s)\n",n,m);_fail++;}while(0)
#define CHECK(c,n,m) do{if(c)PASS(n);else FAIL(n,m);}while(0)
#define SECTION(s)   printf("\n  [%s]\n",s)

// Shim Serial for WySX127x_printInfo
struct FakeSerial {
    void print(const char*) {}
    void print(int) {}
    void println(const char*) {}
    template<typename T> void println(T) {}
    void printf(const char*, ...) {}
} Serial;
#define F(x) x

// Pull in the header under test
#include "net/WySX127x.h"

int main() {
    printf("\n========================================\n");
    printf("  WySX127x define tests\n");
    printf("========================================\n");

    SECTION("Default pin defines");
    CHECK(WY_LORA_CS  == 18, "WY_LORA_CS default = 18",  "wrong");
    CHECK(WY_LORA_IRQ == 26, "WY_LORA_IRQ default = 26", "wrong");
    CHECK(WY_LORA_RST == 14, "WY_LORA_RST default = 14", "wrong");
    CHECK(WY_LORA_BUSY == -1,"WY_LORA_BUSY default = -1 (SX126x only)", "wrong");

    SECTION("Frequency presets");
    CHECK(WY_LORA_FREQ_433 == 433000000UL, "FREQ_433 = 433 MHz", "wrong");
    CHECK(WY_LORA_FREQ_868 == 868000000UL, "FREQ_868 = 868 MHz", "wrong");
    CHECK(WY_LORA_FREQ_915 == 915000000UL, "FREQ_915 = 915 MHz", "wrong");
    CHECK(WY_LORA_FREQ_923 == 923000000UL, "FREQ_923 = 923 MHz (AU/AS)", "wrong");

    SECTION("Frequency ordering");
    CHECK(WY_LORA_FREQ_433 < WY_LORA_FREQ_868, "433 < 868", "wrong");
    CHECK(WY_LORA_FREQ_868 < WY_LORA_FREQ_915, "868 < 915", "wrong");
    CHECK(WY_LORA_FREQ_915 < WY_LORA_FREQ_923, "915 < 923", "wrong");

    SECTION("Module family: SX1276 (set via -DWY_LORA_SX1276)");
#if defined(WY_LORA_SX1276)
    CHECK(1, "WY_LORA_SX1276 defined", "not defined");
    #ifdef WY_LORA_FAMILY_SX127X
    CHECK(WY_LORA_FAMILY_SX127X == 1, "WY_LORA_FAMILY_SX127X = 1", "wrong");
    #else
    FAIL("WY_LORA_FAMILY_SX127X", "not defined");
    #endif
    CHECK(WY_LORA_DEFAULT_FREQ == WY_LORA_FREQ_915, "SX1276 default freq = 915 MHz", "wrong");
#else
    PASS("WY_LORA_SX1276 not set — module family checks skipped");
#endif

    SECTION("Pin values are valid GPIO range (-1 or 0..48)");
    auto validPin = [](int p) { return p == -1 || (p >= 0 && p <= 48); };
    CHECK(validPin(WY_LORA_CS),   "WY_LORA_CS in valid range",   "out of range");
    CHECK(validPin(WY_LORA_IRQ),  "WY_LORA_IRQ in valid range",  "out of range");
    CHECK(validPin(WY_LORA_RST),  "WY_LORA_RST in valid range",  "out of range");
    CHECK(validPin(WY_LORA_BUSY), "WY_LORA_BUSY in valid range", "out of range");

    SECTION("Pins distinct (when not -1)");
    int cs=WY_LORA_CS, irq=WY_LORA_IRQ, rst=WY_LORA_RST;
    if (cs != -1 && irq != -1)  CHECK(cs != irq, "CS != IRQ",  "conflict");
    else PASS("CS or IRQ is -1, skip");
    if (cs != -1 && rst != -1)  CHECK(cs != rst, "CS != RST",  "conflict");
    else PASS("CS or RST is -1, skip");
    if (irq != -1 && rst != -1) CHECK(irq != rst, "IRQ != RST", "conflict");
    else PASS("IRQ or RST is -1, skip");

    SECTION("WySX127x_printInfo compiles and runs (no crash)");
    WySX127x_printInfo();
    PASS("WySX127x_printInfo() returned without crash");

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", _pass, _fail);
    printf("========================================\n");
    return _fail ? 1 : 0;
}
