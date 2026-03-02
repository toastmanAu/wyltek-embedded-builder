#pragma once
/* Host test stubs — swap in real implementations for device build */
#ifdef HOST_TEST
#include <stdint.h>
#include <string.h>

/* Minimal Blake2b for host tests — real impl used on device */
typedef struct { uint8_t state[64]; uint8_t buf[128]; size_t count; uint8_t outlen; } blake2b_state;
void blake2b_InitPersonal(blake2b_state*, size_t, const uint8_t*, size_t);
void blake2b_Update(blake2b_state*, const void*, size_t);
void blake2b_Final(blake2b_state*, void*, size_t);

/* Keccak256 stub */
typedef struct { uint8_t state[200]; } keccak256_ctx_t;
void keccak256_init(keccak256_ctx_t*);
void keccak256_update(keccak256_ctx_t*, const uint8_t*, size_t);
void keccak256_final(keccak256_ctx_t*, uint8_t*);

/* uECC stub types */
typedef void* uECC_Curve;
typedef struct { void* h; } uECC_HashContext;
#define uECC_HashContext_SHA256 NULL
const uECC_Curve* uECC_secp256k1();
int uECC_compute_public_key(const uint8_t*, uint8_t*, const uECC_Curve*);
int uECC_sign_deterministic(const uint8_t*, const uint8_t*, size_t,
                             void*, uint8_t*, const uECC_Curve*);
int uECC_verify_with_recovery(const uint8_t*, const uint8_t*, size_t,
                               uint8_t*, const uECC_Curve*);
#endif
