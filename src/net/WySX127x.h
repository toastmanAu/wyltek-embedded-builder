#pragma once
/**
 * WySX127x.h — SX1276/SX1278 LoRa module driver wrapper
 * =======================================================
 * Thin wrapper around RadioLib (recommended) or LoRa library.
 * Defines pin constants for common SX127x module variants.
 *
 * Supported modules:
 *   SX1276 (868/915MHz) — Ra-01H, HopeRF RFM95W
 *   SX1278 (433MHz)     — Ra-02, HopeRF RFM96W
 *   SX1262              — newer variant, see WySX126x.h
 *
 * Standard SPI wiring (customise with WY_LORA_* defines):
 *   NSS/CS  → WY_LORA_CS   (default: 18)
 *   DIO0    → WY_LORA_IRQ  (default: 26)
 *   RESET   → WY_LORA_RST  (default: 14)
 *   MOSI    → SPI MOSI
 *   MISO    → SPI MISO
 *   SCK     → SPI SCK
 *
 * Usage with RadioLib:
 *   #include <RadioLib.h>
 *   SX1276 radio = new Module(WY_LORA_CS, WY_LORA_IRQ, WY_LORA_RST);
 *
 * Usage with arduino-LoRa:
 *   #include <LoRa.h>
 *   LoRa.setPins(WY_LORA_CS, WY_LORA_RST, WY_LORA_IRQ);
 *   LoRa.begin(915E6);
 *
 * Module variants (set WY_LORA_MODULE):
 *   WY_LORA_SX1276  — 868/915MHz
 *   WY_LORA_SX1278  — 433MHz
 *   WY_LORA_RFM95W  — HopeRF 868/915MHz (SX1276 compatible)
 *   WY_LORA_RFM96W  — HopeRF 433MHz (SX1278 compatible)
 *   WY_LORA_RA01    — AI-Thinker Ra-01 433MHz (SX1278)
 *   WY_LORA_RA01H   — AI-Thinker Ra-01H 868MHz (SX1276)
 */

// Default pin definitions (override before including)
#ifndef WY_LORA_CS
  #define WY_LORA_CS   18
#endif
#ifndef WY_LORA_IRQ
  #define WY_LORA_IRQ  26
#endif
#ifndef WY_LORA_RST
  #define WY_LORA_RST  14
#endif
#ifndef WY_LORA_BUSY
  #define WY_LORA_BUSY -1   // SX126x only
#endif

// Frequency presets (Hz)
#define WY_LORA_FREQ_433   433000000UL
#define WY_LORA_FREQ_868   868000000UL
#define WY_LORA_FREQ_915   915000000UL
#define WY_LORA_FREQ_923   923000000UL   // AU/AS 923MHz plan

// Module family identification
#if defined(WY_LORA_SX1276) || defined(WY_LORA_RFM95W) || defined(WY_LORA_RA01H)
  #define WY_LORA_FAMILY_SX127X  1
  #define WY_LORA_DEFAULT_FREQ   WY_LORA_FREQ_915
#elif defined(WY_LORA_SX1278) || defined(WY_LORA_RFM96W) || defined(WY_LORA_RA01)
  #define WY_LORA_FAMILY_SX127X  1
  #define WY_LORA_DEFAULT_FREQ   WY_LORA_FREQ_433
#elif defined(WY_LORA_SX1262)
  #define WY_LORA_FAMILY_SX126X  1
  #define WY_LORA_DEFAULT_FREQ   WY_LORA_FREQ_915
#endif

// Convenience: print module info to Serial
inline void WySX127x_printInfo() {
    Serial.println(F("LoRa module info:"));
#if defined(WY_LORA_SX1276)
    Serial.println(F("  Module: SX1276 (868/915MHz)"));
#elif defined(WY_LORA_SX1278)
    Serial.println(F("  Module: SX1278 (433MHz)"));
#elif defined(WY_LORA_RFM95W)
    Serial.println(F("  Module: HopeRF RFM95W (868/915MHz)"));
#elif defined(WY_LORA_RFM96W)
    Serial.println(F("  Module: HopeRF RFM96W (433MHz)"));
#elif defined(WY_LORA_RA01)
    Serial.println(F("  Module: AI-Thinker Ra-01 (433MHz)"));
#elif defined(WY_LORA_RA01H)
    Serial.println(F("  Module: AI-Thinker Ra-01H (868MHz)"));
#elif defined(WY_LORA_SX1262)
    Serial.println(F("  Module: SX1262 (868/915MHz)"));
#endif
    Serial.print(F("  CS=")); Serial.print(WY_LORA_CS);
    Serial.print(F(" IRQ=")); Serial.print(WY_LORA_IRQ);
    Serial.print(F(" RST=")); Serial.println(WY_LORA_RST);
}
