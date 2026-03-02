#pragma once
/*
 * bech32m.h — minimal bech32m encoder for CKB addresses
 * Spec: BIP-350 (bech32m), CKB address format RFC
 */
#include <stdint.h>
#include <string.h>
#include "WyAuth.h"  /* for error codes */

static const char BECH32M_CHARSET[] = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
static const uint32_t BECH32M_CONST = 0x2bc830a3;

static uint32_t _bech32_polymod(const uint8_t *v, size_t len) {
    uint32_t c = 1;
    for (size_t i = 0; i < len; i++) {
        uint8_t c0 = c >> 25;
        c = ((c & 0x1ffffff) << 5) ^ v[i];
        if (c0 & 1)  c ^= 0x3b6a57b2;
        if (c0 & 2)  c ^= 0x26508e6d;
        if (c0 & 4)  c ^= 0x1ea119fa;
        if (c0 & 8)  c ^= 0x3d4233dd;
        if (c0 & 16) c ^= 0x2a1462b3;
    }
    return c;
}

static int bech32m_encode(char *out, size_t out_len,
                          const char *hrp, const uint8_t *data, size_t data_len) {
    /* Convert data bytes to 5-bit groups */
    size_t max_5bit = (data_len * 8 + 4) / 5;
    uint8_t d5[128];
    if (max_5bit > sizeof(d5)) return WYAUTH_ERR_PARAM;

    size_t n5 = 0;
    uint32_t acc = 0; int bits = 0;
    for (size_t i = 0; i < data_len; i++) {
        acc = (acc << 8) | data[i];
        bits += 8;
        while (bits >= 5) {
            bits -= 5;
            d5[n5++] = (acc >> bits) & 0x1f;
        }
    }
    if (bits > 0) d5[n5++] = (acc << (5 - bits)) & 0x1f;

    size_t hrp_len = strlen(hrp);
    /* Total length: hrp + '1' + n5 chars + 6 checksum chars */
    size_t total = hrp_len + 1 + n5 + 6;
    if (total >= out_len) return WYAUTH_ERR_PARAM;

    /* Build polymod input: [hrp high bits][0][hrp low bits][data5][0,0,0,0,0,0] */
    uint8_t pm[256]; size_t pi = 0;
    for (size_t i = 0; i < hrp_len; i++) pm[pi++] = hrp[i] >> 5;
    pm[pi++] = 0;
    for (size_t i = 0; i < hrp_len; i++) pm[pi++] = hrp[i] & 0x1f;
    for (size_t i = 0; i < n5; i++) pm[pi++] = d5[i];
    for (int i = 0; i < 6; i++) pm[pi++] = 0;
    uint32_t mod = _bech32_polymod(pm, pi) ^ BECH32M_CONST;

    /* Encode */
    size_t oi = 0;
    for (size_t i = 0; i < hrp_len; i++) out[oi++] = hrp[i];
    out[oi++] = '1';
    for (size_t i = 0; i < n5; i++) out[oi++] = BECH32M_CHARSET[d5[i]];
    for (int i = 0; i < 6; i++)
        out[oi++] = BECH32M_CHARSET[(mod >> (5 * (5 - i))) & 0x1f];
    out[oi] = '\0';
    return WYAUTH_OK;
}
