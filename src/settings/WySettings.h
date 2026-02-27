/*
 * WySettings.h — NVS settings + captive portal + boot mode
 * =========================================================
 * Provides persistent key/value storage via NVS (Preferences).
 * On first boot (or if BOOT button held), starts a captive portal
 * WiFi AP so the user can configure settings via browser.
 *
 * Boot modes:
 *   Normal boot   — loads settings from NVS, returns to app
 *   Portal mode   — BOOT button held at power-on, or no settings saved
 *                   Starts AP "WY-Setup-XXXX", serves config page at 192.168.4.1
 *
 * Usage:
 *   WySettings settings;
 *   settings.addString("ssid",     "WiFi SSID",     "");
 *   settings.addString("pass",     "WiFi Password", "");
 *   settings.addString("node_url", "CKB Node URL",  "http://192.168.1.1:8114");
 *   settings.addInt   ("node_port","Node Port",     8114);
 *
 *   settings.begin("myapp");   // namespace, checks boot mode
 *   if (settings.portalActive()) {
 *       while (settings.portalActive()) settings.portalLoop();
 *       ESP.restart();
 *   }
 *   const char* ssid = settings.getString("ssid");
 *
 * Flasher NVS injection:
 *   Build with -DWY_SETTINGS_BAKED_SSID=\"MyNet\" -DWY_SETTINGS_BAKED_PASS=\"pw\"
 *   These are written to NVS on first boot then cleared from defines.
 */

#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include "boards.h"

#ifndef WY_SETTINGS_MAX_KEYS
#define WY_SETTINGS_MAX_KEYS 16
#endif
#ifndef WY_SETTINGS_VAL_LEN
#define WY_SETTINGS_VAL_LEN  128
#endif
#ifndef WY_PORTAL_AP_PREFIX
#define WY_PORTAL_AP_PREFIX  "WY-Setup"
#endif
#ifndef WY_PORTAL_IP
#define WY_PORTAL_IP         "192.168.4.1"
#endif
#ifndef WY_PORTAL_HOLD_MS
#define WY_PORTAL_HOLD_MS    2000   /* hold BOOT btn this long to enter portal */
#endif

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

/* ── Setting descriptor ─────────────────────────────────────────── */
enum WySettingType { WY_SETTING_STRING, WY_SETTING_INT, WY_SETTING_BOOL };

struct WySetting {
    char     key[24];
    char     label[48];
    WySettingType type;
    char     strVal[WY_SETTINGS_VAL_LEN];
    int32_t  intVal;
    bool     boolVal;
    char     strDefault[WY_SETTINGS_VAL_LEN];
    int32_t  intDefault;
    bool     boolDefault;
};

class WySettings {
public:

    /* ── Register settings ────────────────────────────────────────── */
    void addString(const char* key, const char* label, const char* defaultVal) {
        if (_count >= WY_SETTINGS_MAX_KEYS) return;
        WySetting& s = _settings[_count++];
        strncpy(s.key,        key,        sizeof(s.key)-1);
        strncpy(s.label,      label,      sizeof(s.label)-1);
        strncpy(s.strDefault, defaultVal, sizeof(s.strDefault)-1);
        strncpy(s.strVal,     defaultVal, sizeof(s.strVal)-1);
        s.type = WY_SETTING_STRING;
    }

    void addInt(const char* key, const char* label, int32_t defaultVal) {
        if (_count >= WY_SETTINGS_MAX_KEYS) return;
        WySetting& s = _settings[_count++];
        strncpy(s.key,   key,   sizeof(s.key)-1);
        strncpy(s.label, label, sizeof(s.label)-1);
        s.intDefault = defaultVal;
        s.intVal     = defaultVal;
        s.type       = WY_SETTING_INT;
    }

    void addBool(const char* key, const char* label, bool defaultVal) {
        if (_count >= WY_SETTINGS_MAX_KEYS) return;
        WySetting& s = _settings[_count++];
        strncpy(s.key,   key,   sizeof(s.key)-1);
        strncpy(s.label, label, sizeof(s.label)-1);
        s.boolDefault = defaultVal;
        s.boolVal     = defaultVal;
        s.type        = WY_SETTING_BOOL;
    }

    /* ── Initialise ───────────────────────────────────────────────── */
    bool begin(const char* ns = "wysettings") {
        strncpy(_ns, ns, sizeof(_ns)-1);
        _prefs.begin(_ns, false);
        _loadAll();

        /* Write any baked-in defaults from build flags */
        _writeBaked();

        /* Check if we should enter portal */
        _portalActive = _shouldEnterPortal();
        if (_portalActive) _startPortal();
        return true;
    }

    /* ── Portal loop — call in loop() while portal is active ──────── */
    void portalLoop() {
        if (!_portalActive) return;
        _dns.processNextRequest();
        _server->handleClient();
    }

    bool portalActive() const { return _portalActive; }
    void stopPortal()   { _portalActive = false; if (_server) { delete _server; _server = nullptr; } }

    /* ── Getters ──────────────────────────────────────────────────── */
    const char* getString(const char* key, const char* fallback = "") {
        WySetting* s = _find(key);
        return (s && s->type == WY_SETTING_STRING) ? s->strVal : fallback;
    }
    int32_t getInt(const char* key, int32_t fallback = 0) {
        WySetting* s = _find(key);
        return (s && s->type == WY_SETTING_INT) ? s->intVal : fallback;
    }
    bool getBool(const char* key, bool fallback = false) {
        WySetting* s = _find(key);
        return (s && s->type == WY_SETTING_BOOL) ? s->boolVal : fallback;
    }

    /* ── Setters (also persist to NVS) ───────────────────────────── */
    void setString(const char* key, const char* val) {
        WySetting* s = _find(key);
        if (!s || s->type != WY_SETTING_STRING) return;
        strncpy(s->strVal, val, sizeof(s->strVal)-1);
        _prefs.putString(key, val);
    }
    void setInt(const char* key, int32_t val) {
        WySetting* s = _find(key);
        if (!s || s->type != WY_SETTING_INT) return;
        s->intVal = val;
        _prefs.putInt(key, val);
    }
    void setBool(const char* key, bool val) {
        WySetting* s = _find(key);
        if (!s || s->type != WY_SETTING_BOOL) return;
        s->boolVal = val;
        _prefs.putBool(key, val);
    }

    /* ── Reset all to defaults ────────────────────────────────────── */
    void resetToDefaults() {
        _prefs.clear();
        for (uint8_t i = 0; i < _count; i++) {
            WySetting& s = _settings[i];
            if      (s.type == WY_SETTING_STRING) strncpy(s.strVal, s.strDefault, sizeof(s.strVal)-1);
            else if (s.type == WY_SETTING_INT)    s.intVal  = s.intDefault;
            else                                   s.boolVal = s.boolDefault;
        }
    }

    /* ── Check if settings have been configured (not all defaults) ── */
    bool isConfigured() { return _prefs.getBool("_configured", false); }

private:
    char        _ns[24]       = "wysettings";
    Preferences _prefs;
    WySetting   _settings[WY_SETTINGS_MAX_KEYS];
    uint8_t     _count        = 0;
    bool        _portalActive = false;
    WebServer*  _server       = nullptr;
    DNSServer   _dns;
    char        _apName[32];

    WySetting* _find(const char* key) {
        for (uint8_t i = 0; i < _count; i++)
            if (strcmp(_settings[i].key, key) == 0) return &_settings[i];
        return nullptr;
    }

    void _loadAll() {
        for (uint8_t i = 0; i < _count; i++) {
            WySetting& s = _settings[i];
            if (s.type == WY_SETTING_STRING) {
                String v = _prefs.getString(s.key, s.strDefault);
                strncpy(s.strVal, v.c_str(), sizeof(s.strVal)-1);
            } else if (s.type == WY_SETTING_INT) {
                s.intVal = _prefs.getInt(s.key, s.intDefault);
            } else {
                s.boolVal = _prefs.getBool(s.key, s.boolDefault);
            }
        }
    }

    void _writeBaked() {
        /* Build-time baked WiFi credentials — written once then NVS is truth */
        bool wrote = false;
        #if defined(WY_SETTINGS_BAKED_SSID)
        if (!isConfigured()) {
            setString("ssid", WY_SETTINGS_BAKED_SSID);
            wrote = true;
        }
        #endif
        #if defined(WY_SETTINGS_BAKED_PASS)
        if (!isConfigured()) {
            setString("pass", WY_SETTINGS_BAKED_PASS);
            wrote = true;
        }
        #endif
        #if defined(WY_SETTINGS_BAKED_NODE)
        if (!isConfigured()) {
            setString("node_url", WY_SETTINGS_BAKED_NODE);
            wrote = true;
        }
        #endif
        if (wrote) _prefs.putBool("_configured", true);
    }

    bool _shouldEnterPortal() {
        /* Enter portal if: not configured, OR BOOT button held */
        if (!isConfigured()) return true;
        #if defined(WY_BOOT_BTN) && WY_BOOT_BTN >= 0
        pinMode(WY_BOOT_BTN, INPUT_PULLUP);
        if (digitalRead(WY_BOOT_BTN) == LOW) {
            uint32_t t = millis();
            while (digitalRead(WY_BOOT_BTN) == LOW) {
                if (millis() - t > WY_PORTAL_HOLD_MS) return true;
                delay(10);
            }
        }
        #endif
        return false;
    }

    void _startPortal() {
        /* Build AP name with last 4 hex of MAC */
        uint8_t mac[6]; WiFi.macAddress(mac);
        snprintf(_apName, sizeof(_apName), "%s-%02X%02X",
            WY_PORTAL_AP_PREFIX, mac[4], mac[5]);

        WiFi.mode(WIFI_AP);
        WiFi.softAP(_apName);
        WiFi.softAPConfig(
            IPAddress(192,168,4,1),
            IPAddress(192,168,4,1),
            IPAddress(255,255,255,0));

        /* DNS — redirect all to portal */
        _dns.start(53, "*", IPAddress(192,168,4,1));

        _server = new WebServer(80);
        _server->on("/",       [this](){ _handleRoot(); });
        _server->on("/save",   HTTP_POST, [this](){ _handleSave(); });
        _server->on("/reset",  [this](){ _handleReset(); });
        _server->onNotFound(   [this](){ _server->sendHeader("Location","http://192.168.4.1/"); _server->send(302); });
        _server->begin();

        Serial.printf("[WySettings] Portal active — connect to \"%s\" → http://%s\n",
            _apName, WY_PORTAL_IP);
    }

    void _handleRoot() {
        String html = "<!DOCTYPE html><html><head>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>Device Setup</title>"
            "<style>body{font-family:sans-serif;max-width:480px;margin:20px auto;padding:0 16px}"
            "h2{color:#1a73e8}label{display:block;margin-top:12px;font-size:0.9em;color:#555}"
            "input{width:100%;padding:8px;box-sizing:border-box;border:1px solid #ccc;border-radius:4px;margin-top:4px}"
            "input[type=submit]{background:#1a73e8;color:#fff;border:none;padding:12px;cursor:pointer;margin-top:20px;font-size:1em}"
            "input[type=submit]:hover{background:#1558c0}"
            ".reset{color:#c00;font-size:0.85em;margin-top:20px;display:block}</style></head>"
            "<body><h2>&#9881; Device Setup</h2><form method='POST' action='/save'>";

        for (uint8_t i = 0; i < _count; i++) {
            WySetting& s = _settings[i];
            html += "<label>"; html += s.label; html += "</label>";
            if (s.type == WY_SETTING_STRING) {
                /* Password fields */
                bool isPass = (strstr(s.key,"pass") || strstr(s.key,"password") || strstr(s.key,"secret"));
                html += "<input type='"; html += isPass ? "password" : "text";
                html += "' name='"; html += s.key;
                html += "' value='"; html += isPass ? "" : s.strVal;
                html += "'>";
            } else if (s.type == WY_SETTING_INT) {
                html += "<input type='number' name='"; html += s.key;
                html += "' value='"; html += s.intVal; html += "'>";
            } else {
                html += "<input type='checkbox' name='"; html += s.key;
                html += "'"; if (s.boolVal) html += " checked"; html += ">";
            }
        }

        html += "<input type='submit' value='Save &amp; Reboot'>"
                "</form>"
                "<a class='reset' href='/reset'>&#9888; Reset to defaults</a>"
                "</body></html>";

        _server->send(200, "text/html", html);
    }

    void _handleSave() {
        for (uint8_t i = 0; i < _count; i++) {
            WySetting& s = _settings[i];
            if (_server->hasArg(s.key)) {
                String val = _server->arg(s.key);
                if (s.type == WY_SETTING_STRING) {
                    if (val.length() > 0 || !(strstr(s.key,"pass") || strstr(s.key,"password")))
                        setString(s.key, val.c_str());
                } else if (s.type == WY_SETTING_INT) {
                    setInt(s.key, val.toInt());
                } else {
                    setBool(s.key, val == "on" || val == "1" || val == "true");
                }
            } else if (s.type == WY_SETTING_BOOL) {
                setBool(s.key, false);  /* unchecked checkbox */
            }
        }
        _prefs.putBool("_configured", true);
        _server->send(200, "text/html",
            "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;margin-top:60px'>"
            "<h2>&#10003; Saved!</h2><p>Rebooting...</p></body></html>");
        delay(1500);
        ESP.restart();
    }

    void _handleReset() {
        resetToDefaults();
        _prefs.putBool("_configured", false);
        _server->send(200, "text/html",
            "<!DOCTYPE html><html><body style='font-family:sans-serif;text-align:center;margin-top:60px'>"
            "<h2>Reset complete</h2><p>Rebooting...</p></body></html>");
        delay(1500);
        ESP.restart();
    }
};
