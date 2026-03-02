/*
 * WyCkbTransfer.ino — Full CKB transfer on ESP32
 *
 * Demonstrates the complete flow:
 *   WyAuth (sign) + WyMolecule (build tx) + HTTPClient (submit to RPC)
 *
 * Fills in all the real fields you'd need for a secp256k1/blake2b transfer:
 *   - Lock script for recipient (standard single-sig)
 *   - CellDep for the secp256k1 dep group (mainnet)
 *   - Input cell from a known OutPoint
 *   - Output cell with correct capacity
 *   - Change output back to sender
 *   - Sign → WitnessArgs → submit
 *
 * lib_deps:
 *   toastmanAu/wyltek-embedded-builder
 *   kmackay/micro-ecc @ ^1.0.0
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WyAuth.h>
#include "../../src/ckb/WyMolecule.h"

/* ── Config — fill these in ────────────────────────────────────────────────── */
const char *WIFI_SSID     = "your-wifi";
const char *WIFI_PASS     = "your-password";
const char *CKB_RPC_URL   = "http://mainnet.ckb.dev/rpc";  /* or your node */

/* NEVER hardcode a real key in firmware — this is a demo key only */
static const uint8_t SENDER_PRIVKEY[32] = {
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
};

/* Input cell to spend (get from ckb_getliveCell or indexer) */
static const uint8_t INPUT_TX_HASH[32] = {
    /* Replace with a real tx hash that has an unspent cell at index 0 */
    0xaa,0xbb,0xcc,0xdd,0xaa,0xbb,0xcc,0xdd,
    0xaa,0xbb,0xcc,0xdd,0xaa,0xbb,0xcc,0xdd,
    0xaa,0xbb,0xcc,0xdd,0xaa,0xbb,0xcc,0xdd,
    0xaa,0xbb,0xcc,0xdd,0xaa,0xbb,0xcc,0xdd,
};
const uint32_t INPUT_INDEX = 0;
const uint64_t INPUT_CAPACITY = 10000ULL * 100000000ULL;  /* 10000 CKB in shannons */

/* Recipient lock arg (Blake2b(compressed_pubkey)[0..19]) */
static const uint8_t RECIPIENT_LOCK_ARG[20] = {
    0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,
    0xbb,0xcc,0xdd,0xee,0xff,0x00,0x11,0x22,0x33,0x44,
};
const uint64_t SEND_AMOUNT    = 100ULL * 100000000ULL;  /* 100 CKB */
const uint64_t TX_FEE         = 1000ULL;                /* 1000 shannons ~= 0.00001 CKB */

/* secp256k1 code hash (mainnet) */
static const uint8_t SECP256K1_CODE_HASH[32] = {
    0x9b,0xd7,0xe0,0x6f,0x3e,0xcf,0x4b,0xe0,
    0xf2,0xfc,0xd2,0x18,0x8b,0x23,0xf1,0xb9,
    0xfc,0xc8,0x8e,0x5d,0x4b,0x65,0xa8,0x63,
    0x7b,0x17,0x72,0x3b,0xbd,0xa3,0xcc,0xe8
};

/* secp256k1 dep group tx (mainnet genesis) */
static const uint8_t SECP256K1_DEP_TX[32] = {
    0x71,0xa7,0xba,0x8f,0xc9,0x63,0x49,0xae,
    0x73,0xbb,0x4c,0x11,0x99,0x85,0xe3,0x63,
    0xe3,0xd5,0xa1,0x7d,0x36,0xc1,0x8c,0xa8,
    0x20,0x45,0xe2,0x58,0x37,0x74,0x89,0x97,
};
/* ──────────────────────────────────────────────────────────────────────────── */

WyAuth auth;

void printHex(const char *label, const uint8_t *d, size_t n) {
    Serial.printf("%s (hex): 0x", label);
    for (size_t i = 0; i < n; i++) Serial.printf("%02x", d[i]);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);

    Serial.println("\n=== WyCkbTransfer Demo ===");

    /* ── 1. Init auth ─────────────────────────────────────────────────────── */
    if (auth.begin(SENDER_PRIVKEY, WYAUTH_ALG_CKB) != WYAUTH_OK) {
        Serial.println("auth.begin() failed"); return;
    }
    char addr[80];
    auth.ckbAddress(addr, sizeof(addr));
    Serial.printf("Sender: %s\n", addr);

    /* ── 2. Get sender lock arg ───────────────────────────────────────────── */
    uint8_t sender_arg[20];
    auth.lockArg(sender_arg);

    /* ── 3. Build lock scripts ────────────────────────────────────────────── */
    WyScript recipient_lock;
    recipient_lock.build(SECP256K1_CODE_HASH, 0x01, RECIPIENT_LOCK_ARG, 20);

    WyScript sender_lock;
    sender_lock.build(SECP256K1_CODE_HASH, 0x01, sender_arg, 20);

    /* ── 4. Build cell dep (secp256k1 dep group) ─────────────────────────── */
    WyOutPoint dep_op;
    dep_op.build(SECP256K1_DEP_TX, 0);  /* index 0, dep_type=1 (dep_group) */

    /* ── 5. Build input ───────────────────────────────────────────────────── */
    WyOutPoint input_op;
    input_op.build(INPUT_TX_HASH, INPUT_INDEX);

    WyCellInput input;
    input.build(input_op);  /* since=0 */

    /* ── 6. Build outputs ────────────────────────────────────────────────── */
    WyCellOutput to_output;
    to_output.build(SEND_AMOUNT, recipient_lock);

    uint64_t change = INPUT_CAPACITY - SEND_AMOUNT - TX_FEE;
    WyCellOutput change_output;
    change_output.build(change, sender_lock);

    /* ── 7. Assemble transaction ──────────────────────────────────────────── */
    WyTransaction tx;
    tx.addCellDep(dep_op, 1);       /* dep_group */
    tx.addInput(input);
    tx.addOutput(to_output);        /* no output data for CKB transfer */
    tx.addOutput(change_output);

    /* ── 8. Sign ──────────────────────────────────────────────────────────── */
    uint8_t signing_hash[32];
    if (tx.signingHash(signing_hash) != WYMOL_OK) {
        Serial.println("signingHash() failed"); return;
    }
    printHex("Signing hash", signing_hash, 32);

    uint8_t sig[65];
    if (auth.sign(signing_hash, sig) != WYAUTH_OK) {
        Serial.println("sign() failed"); return;
    }
    printHex("Signature", sig, 65);

    tx.setWitnessSignature(sig);

    /* ── 9. Serialise ─────────────────────────────────────────────────────── */
    size_t tx_len;
    uint8_t *tx_bytes = tx.build(&tx_len);
    if (!tx_bytes) { Serial.println("build() failed"); return; }
    Serial.printf("Tx size: %u bytes\n", tx_len);

    /* Convert to hex string for JSON-RPC */
    String tx_hex = "0x";
    for (size_t i = 0; i < tx_len; i++) {
        char h[3]; snprintf(h, 3, "%02x", tx_bytes[i]);
        tx_hex += h;
    }
    free(tx_bytes);

    /* ── 10. Connect WiFi + submit ───────────────────────────────────────── */
    Serial.printf("Connecting to %s...\n", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries++ < 20) { delay(500); Serial.print("."); }
    if (WiFi.status() != WL_CONNECTED) { Serial.println("\nWiFi failed"); return; }
    Serial.println("\nConnected");

    /* ckb_sendRawTransaction RPC */
    StaticJsonDocument<256> rpc;
    rpc["id"]      = 1;
    rpc["jsonrpc"] = "2.0";
    rpc["method"]  = "ckb_sendRawTransaction";
    rpc["params"][0] = tx_hex;
    rpc["params"][1] = "passthrough";

    String body;
    serializeJson(rpc, body);

    HTTPClient http;
    http.begin(CKB_RPC_URL);
    http.addHeader("Content-Type", "application/json");
    int code = http.POST(body);
    Serial.printf("RPC response: %d\n", code);
    if (code > 0) Serial.println(http.getString());
    http.end();

    Serial.println("Done.");
}

void loop() {}
