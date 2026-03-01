// test_settings.cpp — WySettings logic layer validation
// No Arduino SDK, no NVS, no WiFi — tests pure in-memory key/value logic.
//
// Build: g++ -std=c++17 -DHOST_TEST test/test_settings.cpp -o test/test_settings

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

static int _pass = 0, _fail = 0;
#define PASS(n)       do { printf("  PASS: %s\n", n); _pass++; } while(0)
#define FAIL(n, m)    do { printf("  FAIL: %s  (%s)\n", n, m); _fail++; } while(0)
#define CHECK(c,n,m)  do { if(c) PASS(n); else FAIL(n,m); } while(0)
#define SECTION(s)    printf("\n  [%s]\n", s)

#ifndef WY_SETTINGS_MAX_KEYS
#define WY_SETTINGS_MAX_KEYS 16
#endif
#ifndef WY_SETTINGS_VAL_LEN
#define WY_SETTINGS_VAL_LEN  128
#endif

enum WySettingType { WY_SETTING_STRING, WY_SETTING_INT, WY_SETTING_BOOL };

struct WySetting {
    char          key[24];
    char          label[48];
    WySettingType type;
    char          strVal[WY_SETTINGS_VAL_LEN];
    int32_t       intVal;
    bool          boolVal;
    char          strDefault[WY_SETTINGS_VAL_LEN];
    int32_t       intDefault;
    bool          boolDefault;
};

// Logic layer — mirrors WySettings, Arduino/NVS deps stripped
class WySettingsLogic {
public:
    void addString(const char* key, const char* label, const char* defaultVal) {
        if (_count >= WY_SETTINGS_MAX_KEYS) return;
        WySetting& s = _settings[_count++];
        strncpy(s.key,        key,        sizeof(s.key)-1);        s.key[sizeof(s.key)-1]=0;
        strncpy(s.label,      label,      sizeof(s.label)-1);      s.label[sizeof(s.label)-1]=0;
        strncpy(s.strDefault, defaultVal, sizeof(s.strDefault)-1); s.strDefault[sizeof(s.strDefault)-1]=0;
        strncpy(s.strVal,     defaultVal, sizeof(s.strVal)-1);     s.strVal[sizeof(s.strVal)-1]=0;
        s.type = WY_SETTING_STRING;
    }
    void addInt(const char* key, const char* label, int32_t defaultVal) {
        if (_count >= WY_SETTINGS_MAX_KEYS) return;
        WySetting& s = _settings[_count++];
        strncpy(s.key,   key,   sizeof(s.key)-1);   s.key[sizeof(s.key)-1]=0;
        strncpy(s.label, label, sizeof(s.label)-1); s.label[sizeof(s.label)-1]=0;
        s.intDefault = defaultVal; s.intVal = defaultVal; s.type = WY_SETTING_INT;
    }
    void addBool(const char* key, const char* label, bool defaultVal) {
        if (_count >= WY_SETTINGS_MAX_KEYS) return;
        WySetting& s = _settings[_count++];
        strncpy(s.key,   key,   sizeof(s.key)-1);   s.key[sizeof(s.key)-1]=0;
        strncpy(s.label, label, sizeof(s.label)-1); s.label[sizeof(s.label)-1]=0;
        s.boolDefault = defaultVal; s.boolVal = defaultVal; s.type = WY_SETTING_BOOL;
    }
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
    void setString(const char* key, const char* val) {
        WySetting* s = _find(key);
        if (!s || s->type != WY_SETTING_STRING) return;
        strncpy(s->strVal, val, sizeof(s->strVal)-1); s->strVal[sizeof(s->strVal)-1]=0;
    }
    void setInt(const char* key, int32_t val) {
        WySetting* s = _find(key); if (!s || s->type != WY_SETTING_INT) return; s->intVal = val;
    }
    void setBool(const char* key, bool val) {
        WySetting* s = _find(key); if (!s || s->type != WY_SETTING_BOOL) return; s->boolVal = val;
    }
    void resetToDefaults() {
        for (uint8_t i = 0; i < _count; i++) {
            WySetting& s = _settings[i];
            if      (s.type == WY_SETTING_STRING) strncpy(s.strVal, s.strDefault, sizeof(s.strVal)-1);
            else if (s.type == WY_SETTING_INT)    s.intVal  = s.intDefault;
            else                                   s.boolVal = s.boolDefault;
        }
    }
    uint8_t    count()           const { return _count; }
    WySetting* findKey(const char* key) { return _find(key); }

private:
    WySetting _settings[WY_SETTINGS_MAX_KEYS] = {};
    uint8_t   _count = 0;
    WySetting* _find(const char* key) {
        for (uint8_t i = 0; i < _count; i++)
            if (strcmp(_settings[i].key, key) == 0) return &_settings[i];
        return nullptr;
    }
};

int main() {
    printf("\n========================================\n");
    printf("  WySettings logic unit tests\n");
    printf("========================================\n");

    SECTION("Registration: addString");
    {
        WySettingsLogic s;
        s.addString("ssid", "WiFi SSID", "MyNet");
        CHECK(s.count() == 1,                           "count=1 after addString",  "wrong");
        CHECK(strcmp(s.getString("ssid"),"MyNet")==0,   "default value returned",   "wrong");
        WySetting* e = s.findKey("ssid");
        CHECK(e != nullptr,                             "findKey returns entry",     "null");
        CHECK(e->type == WY_SETTING_STRING,             "type = STRING",            "wrong");
        CHECK(strcmp(e->label, "WiFi SSID")==0,         "label stored",             "wrong");
    }

    SECTION("Registration: addInt");
    {
        WySettingsLogic s;
        s.addInt("port", "Node Port", 8114);
        CHECK(s.count() == 1,                   "count=1 after addInt",  "wrong");
        CHECK(s.getInt("port") == 8114,          "default int returned",  "wrong");
        WySetting* e = s.findKey("port");
        CHECK(e && e->type == WY_SETTING_INT,    "type = INT",            "wrong");
        CHECK(e && e->intDefault == 8114,        "intDefault stored",     "wrong");
    }

    SECTION("Registration: addBool");
    {
        WySettingsLogic s;
        s.addBool("debug", "Debug Mode", true);
        CHECK(s.count() == 1,             "count=1 after addBool",       "wrong");
        CHECK(s.getBool("debug") == true, "default bool=true",           "wrong");
        s.addBool("ota", "OTA", false);
        CHECK(s.count() == 2,             "count=2 after second add",    "wrong");
        CHECK(s.getBool("ota") == false,  "default bool=false",          "wrong");
    }

    SECTION("Registration: mixed types");
    {
        WySettingsLogic s;
        s.addString("ssid", "SSID", "net");
        s.addString("pass", "Pass", "pw");
        s.addInt   ("port", "Port", 8114);
        s.addBool  ("debug","Debug",false);
        CHECK(s.count() == 4, "count=4 for 4 registrations", "wrong");
    }

    SECTION("Capacity guard: max keys");
    {
        WySettingsLogic s;
        char key[8];
        for (int i = 0; i < WY_SETTINGS_MAX_KEYS; i++) {
            snprintf(key, sizeof(key), "k%d", i);
            s.addString(key, "label", "val");
        }
        CHECK(s.count() == WY_SETTINGS_MAX_KEYS, "count == MAX at capacity",         "wrong");
        s.addString("overflow", "Overflow", "x");
        CHECK(s.count() == WY_SETTINGS_MAX_KEYS, "overflow silently dropped",         "count grew");
        CHECK(strcmp(s.getString("k0"),"val")==0, "k0 still accessible after overflow","wrong");
    }

    SECTION("Key truncation at 23 chars");
    {
        WySettingsLogic s;
        const char* longkey = "this_key_is_way_too_long_for_the_buffer";
        s.addString(longkey, "label", "val");
        WySetting* e = s.findKey("this_key_is_way_too_lon");
        CHECK(e != nullptr,           "truncated key findable at 23 chars", "null");
        CHECK(strlen(e->key) == 23,   "key stored at max 23 chars",         "wrong length");
    }

    SECTION("Getters: fallback for missing key");
    {
        WySettingsLogic s;
        s.addString("ssid", "SSID", "net");
        CHECK(strcmp(s.getString("nope","fb"),"fb")==0, "getString missing → fallback", "wrong");
        CHECK(s.getInt ("nope", 99) == 99,              "getInt missing → fallback",    "wrong");
        CHECK(s.getBool("nope", true) == true,          "getBool missing → fallback",   "wrong");
    }

    SECTION("Getters: type mismatch returns fallback");
    {
        WySettingsLogic s;
        s.addString("ssid", "SSID", "net");
        s.addInt("port", "Port", 8114);
        CHECK(s.getInt("ssid", -1) == -1,                "getInt on string key → fallback",  "wrong");
        CHECK(strcmp(s.getString("port","X"),"X")==0,    "getString on int key → fallback",  "wrong");
        CHECK(s.getBool("ssid", true) == true,           "getBool on string key → fallback", "wrong");
    }

    SECTION("Setters: setString");
    {
        WySettingsLogic s;
        s.addString("ssid", "SSID", "default");
        s.setString("ssid", "MyNetwork");
        CHECK(strcmp(s.getString("ssid"),"MyNetwork")==0, "setString updates value",            "wrong");
        WySetting* e = s.findKey("ssid");
        CHECK(strcmp(e->strDefault,"default")==0,          "strDefault preserved after set",     "wrong");
    }

    SECTION("Setters: setInt / setBool");
    {
        WySettingsLogic s;
        s.addInt ("port",  "Port",  8114);
        s.addBool("debug", "Debug", false);
        s.setInt ("port",  9000);
        s.setBool("debug", true);
        CHECK(s.getInt ("port")  == 9000, "setInt updates value",  "wrong");
        CHECK(s.getBool("debug") == true, "setBool updates value", "wrong");
        WySetting* ep = s.findKey("port");
        WySetting* ed = s.findKey("debug");
        CHECK(ep && ep->intDefault  == 8114,  "intDefault preserved",  "wrong");
        CHECK(ed && ed->boolDefault == false, "boolDefault preserved", "wrong");
    }

    SECTION("Setters: type mismatch silently ignored");
    {
        WySettingsLogic s;
        s.addString("ssid", "SSID", "net");
        s.addInt("port", "Port", 8114);
        s.setInt("ssid", 999);
        CHECK(strcmp(s.getString("ssid"),"net")==0, "setInt on string key: no-op", "value changed");
        s.setString("port", "notanint");
        CHECK(s.getInt("port") == 8114,             "setString on int key: no-op", "value changed");
        s.setString("nonexistent","val");
        s.setInt   ("nonexistent", 1);
        s.setBool  ("nonexistent", true);
        CHECK(s.count() == 2, "set on missing key: count unchanged", "wrong");
        PASS("set on missing key: no crash");
    }

    SECTION("Set then get round-trip");
    {
        WySettingsLogic s;
        s.addString("url",     "Node URL", "http://localhost:8114");
        s.addInt   ("retries", "Retries",  3);
        s.addBool  ("tls",     "Use TLS",  false);
        s.setString("url",     "http://192.168.68.87:8114");
        s.setInt   ("retries", 5);
        s.setBool  ("tls",     true);
        CHECK(strcmp(s.getString("url"),"http://192.168.68.87:8114")==0, "url round-trip",     "wrong");
        CHECK(s.getInt("retries") == 5,                                   "retries round-trip", "wrong");
        CHECK(s.getBool("tls")   == true,                                 "tls round-trip",     "wrong");
    }

    SECTION("resetToDefaults restores all values");
    {
        WySettingsLogic s;
        s.addString("ssid",  "SSID",  "HomeNet");
        s.addInt   ("port",  "Port",  8114);
        s.addBool  ("debug", "Debug", false);
        s.setString("ssid",  "OtherNet");
        s.setInt   ("port",  9999);
        s.setBool  ("debug", true);
        s.resetToDefaults();
        CHECK(strcmp(s.getString("ssid"),"HomeNet")==0, "ssid reset to default", "wrong");
        CHECK(s.getInt("port")   == 8114,               "port reset to default", "wrong");
        CHECK(s.getBool("debug") == false,              "debug reset to default","wrong");
    }

    SECTION("resetToDefaults: empty string default");
    {
        WySettingsLogic s;
        s.addString("pass", "Password", "");
        s.setString("pass", "secret");
        s.resetToDefaults();
        CHECK(strcmp(s.getString("pass"),"")==0, "pass reset to empty string", "wrong");
    }

    SECTION("WySetting struct field sizes");
    {
        WySetting d;
        CHECK(sizeof(d.key)        == 24,  "key field = 24 bytes",   "wrong");
        CHECK(sizeof(d.label)      == 48,  "label field = 48 bytes", "wrong");
        CHECK(sizeof(d.strVal)     == 128, "strVal = 128 bytes",     "wrong");
        CHECK(sizeof(d.strDefault) == 128, "strDefault = 128 bytes", "wrong");
    }

    SECTION("Long value clamped to VAL_LEN-1");
    {
        WySettingsLogic s;
        s.addString("k", "label", "");
        char longval[201]; memset(longval,'A',200); longval[200]=0;
        s.setString("k", longval);
        CHECK(strlen(s.getString("k")) == WY_SETTINGS_VAL_LEN-1,
              "long value clamped to VAL_LEN-1", "wrong length");
    }

    SECTION("findKey: linear search");
    {
        WySettingsLogic s;
        s.addString("a","A","1");
        s.addString("b","B","2");
        s.addString("c","C","3");
        CHECK(s.findKey("a") != nullptr,             "findKey('a') found",   "null");
        CHECK(s.findKey("b") != nullptr,             "findKey('b') found",   "null");
        CHECK(s.findKey("c") != nullptr,             "findKey('c') found",   "null");
        CHECK(s.findKey("z") == nullptr,             "findKey('z') = null",  "non-null");
        CHECK(strcmp(s.findKey("b")->strVal,"2")==0, "findKey('b') correct", "wrong");
    }

    SECTION("Negative int values");
    {
        WySettingsLogic s;
        s.addInt("offset", "Offset", -1);
        CHECK(s.getInt("offset") == -1, "default negative int returned", "wrong");
        s.setInt("offset", -999);
        CHECK(s.getInt("offset") == -999, "negative int set and retrieved", "wrong");
        s.resetToDefaults();
        CHECK(s.getInt("offset") == -1, "negative int default restored", "wrong");
    }

    SECTION("Zero-length key edge case");
    {
        WySettingsLogic s;
        s.addString("", "Empty key", "val");
        // Empty key is technically valid (strncpy of "" → key[0]=0)
        WySetting* e = s.findKey("");
        CHECK(e != nullptr,                    "empty key findable", "null");
        CHECK(strcmp(s.getString(""),"val")==0,"empty key get works", "wrong");
    }

    printf("\n========================================\n");
    printf("  Results: %d passed, %d failed\n", _pass, _fail);
    printf("========================================\n");
    return _fail ? 1 : 0;
}
