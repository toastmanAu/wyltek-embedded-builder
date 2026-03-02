/*
 * WyAuth.cpp — CKB transaction signing for ESP32
 *
 * Crypto backend: uECC (micro-ecc) for secp256k1.
 * micro-ecc is ~3KB flash, no heap allocation, constant-time.
 *
 * For Blake2b: uses the implementation in ckb-c-stdlib (or standalone).
 * For Keccak256 (Ethereum): uses a small standalone implementation.
 *
 * Build flags:
 *   WYAUTH_USE_MBEDTLS   — use mbedTLS instead of micro-ecc (larger, available
 *                          on ESP32-S3 via idf-component-manager)
 */

#include "WyAuth.h"

/* ── Pull in crypto ───────────────────────────────────────────────────────── */
#ifdef ARDUINO
  /* micro-ecc: add via lib_deps = kmackay/micro-ecc */
  #include <uECC.h>
  /* Blake2b: standalone 2-file implementation */
  #include "blake2b.h"
  /* Keccak256: standalone */
  #include "keccak256.h"
#else
  /* Host test stubs */
  #include "crypto_stubs.h"
#endif

/* ── Internal helpers ─────────────────────────────────────────────────────── */

static const uint8_t CKB_PERSONALISATION[16] = "ckb-default-hash";

/* secp256k1 curve via micro-ecc */
static const struct uECC_Curve_t *_curve() {
    return uECC_secp256k1();
}

/* ── begin() ─────────────────────────────────────────────────────────────── */
int WyAuth::begin(const uint8_t privkey[WYAUTH_PRIVKEY_BYTES], uint8_t alg) {
    if (!privkey) return WYAUTH_ERR_PARAM;
    if (alg != WYAUTH_ALG_CKB &&
        alg != WYAUTH_ALG_ETHEREUM &&
        alg != WYAUTH_ALG_BITCOIN) return WYAUTH_ERR_ALG;

    memcpy(_privkey, privkey, WYAUTH_PRIVKEY_BYTES);
    _alg = alg;
    _ready = false;

    int r = _derivePublicKey();
    if (r != WYAUTH_OK) {
        wipe();
        return r;
    }

    _ready = true;
    return WYAUTH_OK;
}

/* ── _derivePublicKey() ───────────────────────────────────────────────────── */
int WyAuth::_derivePublicKey() {
    uint8_t uncompressed[64];
    if (!uECC_compute_public_key(_privkey, uncompressed, _curve())) {
        return WYAUTH_ERR_SIGN;
    }
    /* Convert to compressed form: 0x02/0x03 prefix + x coordinate */
    _pubkey[0] = (uncompressed[63] & 1) ? 0x03 : 0x02;
    memcpy(_pubkey + 1, uncompressed, 32);
    return WYAUTH_OK;
}

/* ── pubkey() ────────────────────────────────────────────────────────────── */
int WyAuth::pubkey(uint8_t pubkey_out[WYAUTH_PUBKEY_BYTES]) {
    if (!_ready) return WYAUTH_ERR_NO_KEY;
    memcpy(pubkey_out, _pubkey, WYAUTH_PUBKEY_BYTES);
    return WYAUTH_OK;
}

/* ── lockArg() ───────────────────────────────────────────────────────────── */
int WyAuth::lockArg(uint8_t arg_out[WYAUTH_AUTH160_BYTES]) {
    if (!_ready) return WYAUTH_ERR_NO_KEY;

    uint8_t hash[WYAUTH_HASH_BYTES];
    int r = hashCkb(_pubkey, WYAUTH_PUBKEY_BYTES, hash);
    if (r != WYAUTH_OK) return r;

    memcpy(arg_out, hash, WYAUTH_AUTH160_BYTES);
    return WYAUTH_OK;
}

/* ── sign() ──────────────────────────────────────────────────────────────── */
int WyAuth::sign(const uint8_t msg_hash[WYAUTH_HASH_BYTES],
                 uint8_t sig_out[WYAUTH_SIG_BYTES]) {
    if (!_ready)   return WYAUTH_ERR_NO_KEY;
    if (!msg_hash) return WYAUTH_ERR_PARAM;

    switch (_alg) {
        case WYAUTH_ALG_CKB:      return _signCkb(msg_hash, sig_out);
        case WYAUTH_ALG_ETHEREUM: return _signEthereum(msg_hash, sig_out);
        case WYAUTH_ALG_BITCOIN:  return _signBitcoin(msg_hash, sig_out);
        default:                  return WYAUTH_ERR_ALG;
    }
}

/* ── _signCkb() ──────────────────────────────────────────────────────────── */
int WyAuth::_signCkb(const uint8_t *hash, uint8_t *sig_out) {
    /*
     * CKB secp256k1/blake2b signature format (65 bytes):
     *   [0]     recovery_id  (0 or 1)
     *   [1..32] r
     *   [33..64] s
     *
     * micro-ecc produces [r(32)][s(32)], recovery id computed separately.
     */
    uint8_t rs[64];
    uint8_t recovery_id = 0;

    if (!uECC_sign_deterministic(
            _privkey, hash, WYAUTH_HASH_BYTES,
            &uECC_HashContext_SHA256 /* micro-ecc RFC6979 */,
            rs, _curve())) {
        return WYAUTH_ERR_SIGN;
    }

    /* Determine recovery id by trying both and checking pubkey recovery */
    uint8_t recovered[64];
    for (uint8_t v = 0; v <= 1; v++) {
        uint8_t sig65_test[65];
        sig65_test[0] = v;
        memcpy(sig65_test + 1, rs, 64);
        if (uECC_verify_with_recovery(sig65_test, hash, WYAUTH_HASH_BYTES,
                                       recovered, _curve())) {
            /* Check recovered pub matches ours */
            uint8_t uncompressed[64];
            memcpy(uncompressed, _pubkey + 1, 32); /* x */
            if (memcmp(recovered, uncompressed, 32) == 0) {
                recovery_id = v;
                break;
            }
        }
    }

    sig_out[0] = recovery_id;
    memcpy(sig_out + 1, rs, 64);
    return WYAUTH_OK;
}

/* ── _signEthereum() ─────────────────────────────────────────────────────── */
int WyAuth::_signEthereum(const uint8_t *hash, uint8_t *sig_out) {
    /*
     * Ethereum uses the same secp256k1 sig format but recovery_id is 27 or 28
     * (legacy) for personal_sign. ckb-auth uses 0/1 — match that.
     */
    return _signCkb(hash, sig_out);  /* same wire format for ckb-auth */
}

/* ── _signBitcoin() ──────────────────────────────────────────────────────── */
int WyAuth::_signBitcoin(const uint8_t *hash, uint8_t *sig_out) {
    /* Bitcoin message signing: same secp256k1, v = 31 or 32 (compressed key).
     * ckb-auth normalises this back to 0/1 internally — use raw 0/1. */
    return _signCkb(hash, sig_out);
}

/* ── hashCkb() ───────────────────────────────────────────────────────────── */
int WyAuth::hashCkb(const uint8_t *msg, size_t len,
                    uint8_t hash_out[WYAUTH_HASH_BYTES]) {
    if (!msg || !hash_out) return WYAUTH_ERR_PARAM;

    blake2b_state S;
    blake2b_InitPersonal(&S, WYAUTH_HASH_BYTES,
                         CKB_PERSONALISATION, sizeof(CKB_PERSONALISATION));
    blake2b_Update(&S, msg, len);
    blake2b_Final(&S, hash_out, WYAUTH_HASH_BYTES);
    return WYAUTH_OK;
}

/* ── hashEthereum() ──────────────────────────────────────────────────────── */
int WyAuth::hashEthereum(const uint8_t *msg, size_t len,
                          uint8_t hash_out[WYAUTH_HASH_BYTES]) {
    if (!msg || !hash_out) return WYAUTH_ERR_PARAM;

    /* "\x19Ethereum Signed Message:\n" + len_str + msg */
    char prefix[40];
    int plen = snprintf(prefix, sizeof(prefix),
                        "\x19" "Ethereum Signed Message:\n%u", (unsigned)len);

    keccak256_ctx_t ctx;
    keccak256_init(&ctx);
    keccak256_update(&ctx, (const uint8_t *)prefix, plen);
    keccak256_update(&ctx, msg, len);
    keccak256_final(&ctx, hash_out);
    return WYAUTH_OK;
}

/* ── ckbAddress() ────────────────────────────────────────────────────────── */
int WyAuth::ckbAddress(char *buf, size_t buf_len) {
    if (!_ready || !buf) return WYAUTH_ERR_PARAM;

    /*
     * CKB full address (bech32m, mainnet):
     *   payload = 0x00 (full format) || code_hash(32) || hash_type(1) || args
     * For secp256k1/blake2b single-sig (default lock):
     *   code_hash = 0x9bd7e06f3ecf4be0f2fcd2188b23f1b9fcc88e5d4b65a8637b17723bbda3cce8
     *   hash_type  = 0x01 (type)
     *   args       = lock_arg (20 bytes = lockArg())
     *
     * Encoded with bech32m, HRP "ckb".
     * We use a minimal bech32m encoder here.
     */
    uint8_t arg[WYAUTH_AUTH160_BYTES];
    int r = lockArg(arg);
    if (r != WYAUTH_OK) return r;

    /* secp256k1/blake2b code hash (mainnet, type) */
    static const uint8_t CODE_HASH[32] = {
        0x9b,0xd7,0xe0,0x6f,0x3e,0xcf,0x4b,0xe0,
        0xf2,0xfc,0xd2,0x18,0x8b,0x23,0xf1,0xb9,
        0xfc,0xc8,0x8e,0x5d,0x4b,0x65,0xa8,0x63,
        0x7b,0x17,0x72,0x3b,0xbd,0xa3,0xcc,0xe8
    };
    static const uint8_t HASH_TYPE = 0x01;  /* type */

    /* Build payload: [0x00][code_hash(32)][hash_type(1)][arg(20)] = 54 bytes */
    uint8_t payload[54];
    payload[0] = 0x00;
    memcpy(payload + 1, CODE_HASH, 32);
    payload[33] = HASH_TYPE;
    memcpy(payload + 34, arg, 20);

    return bech32m_encode(buf, buf_len, "ckb", payload, sizeof(payload));
}

/* ── wipe() ──────────────────────────────────────────────────────────────── */
void WyAuth::wipe() {
    memset(_privkey, 0, sizeof(_privkey));
    memset(_pubkey,  0, sizeof(_pubkey));
    _ready = false;
}
