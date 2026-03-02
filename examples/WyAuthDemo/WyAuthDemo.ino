/*
 * WyAuthDemo.ino — CKB transaction signing on ESP32
 *
 * Demonstrates:
 *  - Loading a private key into WyAuth
 *  - Deriving the CKB address (secp256k1/blake2b, Omnilock compatible)
 *  - Signing a mock transaction hash
 *
 * WARNING: Never hardcode real private keys in firmware.
 *          Use WyKeystore (NVS/eFuse) for production.
 *
 * lib_deps:
 *   toastmanAu/wyltek-embedded-builder
 *   kmackay/micro-ecc @ ^1.0.0
 */

#include <WyAuth.h>

/* Test key — BIP32 path m/44'/309'/0'/0/0 from a known test mnemonic */
/* NEVER USE A REAL KEY HERE */
static const uint8_t TEST_PRIVKEY[32] = {
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
    0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,
};

/* Mock tx hash — in real use this comes from your transaction builder */
static const uint8_t MOCK_TX_HASH[32] = {
    0xde,0xad,0xbe,0xef,0xca,0xfe,0xba,0xbe,
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
    0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,
    0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,
};

WyAuth auth;

void printHex(const char *label, const uint8_t *data, size_t len) {
    Serial.print(label);
    Serial.print(": 0x");
    for (size_t i = 0; i < len; i++) {
        if (data[i] < 0x10) Serial.print("0");
        Serial.print(data[i], HEX);
    }
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    delay(500);

    Serial.println("\n=== WyAuth Demo ===");
    Serial.println("CKB secp256k1/blake2b signing\n");

    /* Load key */
    int r = auth.begin(TEST_PRIVKEY, WYAUTH_ALG_CKB);
    if (r != WYAUTH_OK) {
        Serial.printf("begin() failed: %d\n", r);
        return;
    }
    Serial.println("Key loaded OK");

    /* Print public key */
    uint8_t pubkey[WYAUTH_PUBKEY_BYTES];
    auth.pubkey(pubkey);
    printHex("Public key (compressed)", pubkey, WYAUTH_PUBKEY_BYTES);

    /* Print lock arg (what goes in the lock script args field) */
    uint8_t arg[WYAUTH_AUTH160_BYTES];
    auth.lockArg(arg);
    printHex("Lock arg (Blake2b(pubkey)[0..19])", arg, WYAUTH_AUTH160_BYTES);

    /* Print CKB address */
    char addr[80];
    r = auth.ckbAddress(addr, sizeof(addr));
    if (r == WYAUTH_OK) {
        Serial.print("CKB address: ");
        Serial.println(addr);
    }

    /* Sign a transaction hash */
    Serial.println("\nSigning mock tx hash...");
    uint8_t sig[WYAUTH_SIG_BYTES];
    r = auth.sign(MOCK_TX_HASH, sig);
    if (r != WYAUTH_OK) {
        Serial.printf("sign() failed: %d\n", r);
        return;
    }
    printHex("Signature [v|r|s]", sig, WYAUTH_SIG_BYTES);

    Serial.println("\nEmbed sig[0..64] in CKB witness (secp256k1 lock)");
    Serial.println("Done.");
}

void loop() {}
