/*
 * WyNet.h — WiFi manager + OTA for wyltek-embedded-builder
 * =========================================================
 * Simple WiFi connection with:
 *   - Retry with timeout
 *   - Reconnect watchdog in loop()
 *   - Status callbacks
 *   - OTA update support (ArduinoOTA)
 *   - mDNS hostname
 *
 * Usage:
 *   WyNet net;
 *   net.begin(ssid, password);        // blocks until connected or timeout
 *   net.setHostname("ckb-node");      // optional, before begin()
 *   net.enableOTA("ota_password");    // optional OTA
 *
 *   void loop() {
 *       net.loop();                   // handles reconnect + OTA
 *   }
 *
 * Callbacks:
 *   net.onConnect([]()    { Serial.println("WiFi up"); });
 *   net.onDisconnect([]() { Serial.println("WiFi down"); });
 */

#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>

#ifndef WY_NET_CONNECT_TIMEOUT_MS
#define WY_NET_CONNECT_TIMEOUT_MS  15000
#endif
#ifndef WY_NET_RECONNECT_INTERVAL_MS
#define WY_NET_RECONNECT_INTERVAL_MS  10000
#endif

class WyNet {
public:

    /* ── Config (call before begin) ─────────────────────────────── */
    void setHostname(const char* hostname) {
        strncpy(_hostname, hostname, sizeof(_hostname)-1);
    }
    void onConnect(void (*cb)())    { _onConnect = cb; }
    void onDisconnect(void (*cb)()) { _onDisconnect = cb; }

    /* ── Connect ────────────────────────────────────────────────── */
    bool begin(const char* ssid, const char* password,
               uint32_t timeoutMs = WY_NET_CONNECT_TIMEOUT_MS) {
        if (!ssid || strlen(ssid) == 0) {
            Serial.println("[WyNet] no SSID configured");
            return false;
        }
        strncpy(_ssid, ssid, sizeof(_ssid)-1);
        strncpy(_pass, password, sizeof(_pass)-1);

        WiFi.setHostname(_hostname);
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _pass);

        Serial.printf("[WyNet] connecting to \"%s\"", _ssid);
        uint32_t t = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - t > timeoutMs) {
                Serial.println(" timeout");
                return false;
            }
            delay(250);
            Serial.print(".");
        }
        Serial.printf(" OK  IP: %s\n", WiFi.localIP().toString().c_str());

        if (_hostname[0]) MDNS.begin(_hostname);
        _lastReconnect = millis();
        _wasConnected = true;
        if (_onConnect) _onConnect();
        return true;
    }

    /* ── OTA ────────────────────────────────────────────────────── */
    void enableOTA(const char* password = nullptr) {
        _otaEnabled = true;
        #include <ArduinoOTA.h>
        ArduinoOTA.setHostname(_hostname);
        if (password) ArduinoOTA.setPassword(password);
        ArduinoOTA.onStart([]()  { Serial.println("[OTA] start"); });
        ArduinoOTA.onEnd([]()    { Serial.println("[OTA] done"); });
        ArduinoOTA.onError([](ota_error_t e) { Serial.printf("[OTA] error %u\n", e); });
        ArduinoOTA.onProgress([](uint prog, uint total) {
            Serial.printf("[OTA] %u%%\r", (prog * 100) / total);
        });
        if (WiFi.isConnected()) ArduinoOTA.begin();
    }

    /* ── Loop — reconnect watchdog + OTA ────────────────────────── */
    void loop() {
        if (WiFi.status() != WL_CONNECTED) {
            if (_wasConnected && _onDisconnect) { _onDisconnect(); _wasConnected = false; }
            if (millis() - _lastReconnect > WY_NET_RECONNECT_INTERVAL_MS) {
                _lastReconnect = millis();
                Serial.println("[WyNet] reconnecting...");
                WiFi.disconnect();
                WiFi.begin(_ssid, _pass);
            }
        } else {
            if (!_wasConnected) {
                _wasConnected = true;
                Serial.printf("[WyNet] reconnected  IP: %s\n", WiFi.localIP().toString().c_str());
                if (_otaEnabled) {
                    #include <ArduinoOTA.h>
                    ArduinoOTA.begin();
                }
                if (_onConnect) _onConnect();
            }
            if (_otaEnabled) {
                #include <ArduinoOTA.h>
                ArduinoOTA.handle();
            }
        }
    }

    /* ── Status ─────────────────────────────────────────────────── */
    bool isConnected()          { return WiFi.status() == WL_CONNECTED; }
    String localIP()            { return WiFi.localIP().toString(); }
    int8_t rssi()               { return WiFi.RSSI(); }

private:
    char     _ssid[64]     = {};
    char     _pass[64]     = {};
    char     _hostname[32] = "wyltek-device";
    bool     _otaEnabled   = false;
    bool     _wasConnected = false;
    uint32_t _lastReconnect = 0;
    void (*_onConnect)()    = nullptr;
    void (*_onDisconnect)() = nullptr;
};
