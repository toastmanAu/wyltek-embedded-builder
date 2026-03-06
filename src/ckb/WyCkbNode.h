/*
 * WyCkbNode.h — CKB Node JSON-RPC client for WEB
 * ================================================
 * Polls a CKB full node or light client over WiFi (JSON-RPC 2.0).
 * Returns block height, peer count, and synced state.
 *
 * Requires:
 *   - WyNet (WiFi already connected)
 *   - ArduinoJson (>=6.x)
 *   - HTTPClient (ESP32 Arduino core)
 *
 * Usage:
 *   #include <ckb/WyCkbNode.h>
 *   WyCkbNode node("http://192.168.68.87:8114");
 *   node.update();
 *   Serial.println(node.blockNumber);
 *
 * Also supports CoinGecko price fetch (optional).
 */

#pragma once
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct WyCkbNodeInfo {
    uint64_t blockNumber   = 0;
    uint64_t headerNumber  = 0;   // light client: headers ahead of blocks
    int      peers         = 0;
    bool     synced        = false;
    bool     valid         = false;
    unsigned long fetchedMs = 0;
};

struct WyCkbPrice {
    float    usd           = 0.0f;
    bool     valid         = false;
    unsigned long fetchedMs = 0;
};

class WyCkbNode {
public:
    WyCkbNodeInfo info;
    WyCkbPrice    price;

    WyCkbNode(const char* rpcUrl) : _rpcUrl(rpcUrl) {}

    // Fetch latest block number + peer count from node RPC
    // Returns true on success
    bool update() {
        info.valid = false;

        // get_tip_block_number
        String tipResp = _rpc("get_tip_block_number", "[]");
        if (tipResp.length() == 0) return false;

        StaticJsonDocument<256> tipDoc;
        if (deserializeJson(tipDoc, tipResp) != DeserializationError::Ok) return false;
        const char* tipHex = tipDoc["result"] | "";
        if (strlen(tipHex) < 3) return false;
        info.blockNumber = strtoull(tipHex + 2, nullptr, 16); // strip "0x"

        // get_peers_state or local_node_info for peer count
        String nodeResp = _rpc("local_node_info", "[]");
        if (nodeResp.length() > 0) {
            StaticJsonDocument<1024> nodeDoc;
            if (deserializeJson(nodeDoc, nodeResp) == DeserializationError::Ok) {
                info.peers = nodeDoc["result"]["connections"] | 0;
            }
        }

        info.synced    = (info.peers > 0 && info.blockNumber > 0);
        info.valid     = true;
        info.fetchedMs = millis();
        return true;
    }

    // Fetch CKB/USD price from CoinGecko (free, no key)
    // Call sparingly — rate limited. Suggest every 5 minutes.
    bool updatePrice() {
        price.valid = false;
        HTTPClient http;
        http.begin("https://api.coingecko.com/api/v3/simple/price?ids=nervos-network&vs_currencies=usd");
        http.setTimeout(8000);
        int code = http.GET();
        if (code != 200) { http.end(); return false; }

        String body = http.getString();
        http.end();

        StaticJsonDocument<256> doc;
        if (deserializeJson(doc, body) != DeserializationError::Ok) return false;
        price.usd      = doc["nervos-network"]["usd"] | 0.0f;
        price.valid    = (price.usd > 0.0f);
        price.fetchedMs = millis();
        return price.valid;
    }

    // True if cached data is stale (default: 10 seconds)
    bool isStale(unsigned long maxAgeMs = 10000) {
        return !info.valid || (millis() - info.fetchedMs > maxAgeMs);
    }

    bool isPriceStale(unsigned long maxAgeMs = 300000) { // 5 minutes
        return !price.valid || (millis() - price.fetchedMs > maxAgeMs);
    }

private:
    const char* _rpcUrl;

    String _rpc(const char* method, const char* params) {
        HTTPClient http;
        http.begin(_rpcUrl);
        http.addHeader("Content-Type", "application/json");
        http.setTimeout(5000);

        String body = "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"";
        body += method;
        body += "\",\"params\":";
        body += params;
        body += "}";

        int code = http.POST(body);
        if (code != 200) { http.end(); return ""; }
        String resp = http.getString();
        http.end();
        return resp;
    }
};
