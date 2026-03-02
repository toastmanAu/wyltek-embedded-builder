#pragma once
/*
 * WyAuth.h — CKB transaction signing for ESP32
 *
 * Wraps ckb-auth signing primitives (nervosnetwork/ckb-auth) for embedded use.
 * Targets Omnilock — the audited, mainnet-deployed universal lock — so no new
 * lock script is needed on-chain.
 *
 * Supported signing algorithms (matching AuthAlgorithmIdType):
 *   WYAUTH_ALG_CKB        (0x00) — secp256k1 Blake2b, native CKB
 *   WYAUTH_ALG_ETHEREUM   (0x01) — secp256k1 Keccak256, ETH-compatible
 *   WYAUTH_ALG_BITCOIN    (0x04) — secp256k1 Bitcoin message signing
 *
 * Design:
 *   - Key material stays in RAM (no flash storage here — use WyKeystore)
 *   - All signing is pure software (mbedTLS secp256k1 or micro-ecc)
 *   - Produces 65-byte compact signatures: [v(1)] [r(32)] [s(32)]
 *   - Message hashing follows ckb-auth conventions per algorithm
 *
 * Usage:
 *   WyAuth auth;
 *   auth.begin(privkey_32bytes, WYAUTH_ALG_CKB);
 *   uint8_t sig[65];
 *   auth.sign(tx_hash_32bytes, sig);
 *   // sig is now ready to embed in a CKB witness
 *
 * Attribution: wraps nervosnetwork/ckb-auth (MIT)
 * Fork: toastmanAu/ckb-auth (if diverged)
 */

#include <Arduino.h>
#include <stdint.h>
#include <string.h>

/* ── Algorithm IDs (matches ckb-auth EnumAuthAlgorithmIdType) ─────────────── */
#define WYAUTH_ALG_CKB        0x00
#define WYAUTH_ALG_ETHEREUM   0x01
#define WYAUTH_ALG_BITCOIN    0x04

/* ── Sizes ────────────────────────────────────────────────────────────────── */
#define WYAUTH_PRIVKEY_BYTES  32
#define WYAUTH_PUBKEY_BYTES   33   /* compressed secp256k1 */
#define WYAUTH_SIG_BYTES      65   /* [v][r][s] */
#define WYAUTH_HASH_BYTES     32
#define WYAUTH_AUTH160_BYTES  20   /* Blake2b(pubkey)[0..19] — lock arg */

/* ── Error codes ──────────────────────────────────────────────────────────── */
#define WYAUTH_OK             0
#define WYAUTH_ERR_NO_KEY     1    /* begin() not called */
#define WYAUTH_ERR_ALG        2    /* unsupported algorithm */
#define WYAUTH_ERR_SIGN       3    /* signing operation failed */
#define WYAUTH_ERR_PARAM      4    /* null pointer or bad length */

class WyAuth {
public:
    WyAuth() : _ready(false), _alg(WYAUTH_ALG_CKB) {}
    ~WyAuth() { wipe(); }

    /*
     * begin() — load private key and select algorithm.
     * privkey: 32-byte raw secp256k1 private key scalar.
     * alg:     WYAUTH_ALG_CKB / WYAUTH_ALG_ETHEREUM / WYAUTH_ALG_BITCOIN
     * Returns WYAUTH_OK or error code.
     */
    int begin(const uint8_t privkey[WYAUTH_PRIVKEY_BYTES],
              uint8_t alg = WYAUTH_ALG_CKB);

    /*
     * sign() — sign a 32-byte message hash.
     * msg_hash: 32 bytes. For CKB this is the transaction signing hash.
     * sig_out:  65-byte buffer [recovery_id(1)][r(32)][s(32)]
     * Returns WYAUTH_OK or error code.
     */
    int sign(const uint8_t msg_hash[WYAUTH_HASH_BYTES],
             uint8_t sig_out[WYAUTH_SIG_BYTES]);

    /*
     * pubkey() — get compressed public key (33 bytes).
     */
    int pubkey(uint8_t pubkey_out[WYAUTH_PUBKEY_BYTES]);

    /*
     * lockArg() — derive the 20-byte Omnilock/secp256k1 lock arg.
     * This is Blake2b(compressed_pubkey)[0..19].
     * Pass this as the lock script arg when building a CKB address.
     */
    int lockArg(uint8_t arg_out[WYAUTH_AUTH160_BYTES]);

    /*
     * ckbAddress() — encode a full CKB address string (bech32m, mainnet).
     * buf: caller-supplied buffer of at least 70 bytes.
     */
    int ckbAddress(char *buf, size_t buf_len);

    /*
     * hashCkb() — hash msg with Blake2b personalisation "ckb-default-hash".
     * Convenience for building the signing hash from raw tx data.
     */
    static int hashCkb(const uint8_t *msg, size_t len,
                        uint8_t hash_out[WYAUTH_HASH_BYTES]);

    /*
     * hashEthereum() — apply Ethereum personal_sign prefix + Keccak256.
     */
    static int hashEthereum(const uint8_t *msg, size_t len,
                             uint8_t hash_out[WYAUTH_HASH_BYTES]);

    /*
     * wipe() — zero key material from RAM.
     */
    void wipe();

    bool isReady() const { return _ready; }
    uint8_t algorithm() const { return _alg; }

private:
    bool    _ready;
    uint8_t _alg;
    uint8_t _privkey[WYAUTH_PRIVKEY_BYTES];
    uint8_t _pubkey[WYAUTH_PUBKEY_BYTES];

    int _derivePublicKey();
    int _signCkb(const uint8_t *hash, uint8_t *sig_out);
    int _signEthereum(const uint8_t *hash, uint8_t *sig_out);
    int _signBitcoin(const uint8_t *hash, uint8_t *sig_out);
};
