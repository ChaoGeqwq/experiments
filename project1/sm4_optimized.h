#ifndef SM4_OPTIMIZED_H
#define SM4_OPTIMIZED_H

#include <stdint.h>
#include <string.h>
#include <immintrin.h>

// SM4常量定义
extern const uint32_t SM4_FK[4];
extern const uint32_t SM4_CK[32];
extern const uint8_t SM4_SBOX[256];

// T-table优化
extern uint32_t SM4_T0[256];
extern uint32_t SM4_T1[256];
extern uint32_t SM4_T2[256];
extern uint32_t SM4_T3[256];
extern uint32_t SM4_TK0[256];
extern uint32_t SM4_TK1[256];
extern uint32_t SM4_TK2[256];
extern uint32_t SM4_TK3[256];

// 函数声明
void sm4_ttable_init(void);
void sm4_ttable_keygen(const uint32_t user_key[4], uint32_t round_keys[32]);
void sm4_ttable_encrypt(const uint32_t plaintext[4], uint32_t ciphertext[4], const uint32_t round_keys[32]);
void sm4_ttable_decrypt(const uint32_t ciphertext[4], uint32_t plaintext[4], const uint32_t round_keys[32]);

// AESNI优化
void sm4_aesni_encrypt(const uint32_t plaintext[4], uint32_t ciphertext[4], const uint32_t round_keys[32]);
void sm4_aesni_decrypt(const uint32_t ciphertext[4], uint32_t plaintext[4], const uint32_t round_keys[32]);
void sm4_aesni_encrypt_blocks(const uint8_t* plaintext, uint8_t* ciphertext, 
                              const uint32_t round_keys[32], size_t num_blocks);

// 最新指令集优化（GFNI、VPROLD等）
void sm4_gfni_encrypt(const uint32_t plaintext[4], uint32_t ciphertext[4], const uint32_t round_keys[32]);
void sm4_gfni_decrypt(const uint32_t ciphertext[4], uint32_t plaintext[4], const uint32_t round_keys[32]);
void sm4_avx512_encrypt_blocks(const uint8_t* plaintext, uint8_t* ciphertext,
                               const uint32_t round_keys[32], size_t num_blocks);

// GCM模式相关
typedef struct {
    uint32_t round_keys[32];
    uint64_t h[2];        // GCM hash key
    uint64_t counter[2];  // CTR counter
    uint8_t aad_hash[16]; // AAD hash
    size_t aad_len;
    size_t data_len;
} sm4_gcm_ctx_t;

int sm4_gcm_init(sm4_gcm_ctx_t* ctx, const uint8_t key[16], const uint8_t iv[12]);
int sm4_gcm_encrypt(sm4_gcm_ctx_t* ctx, const uint8_t* plaintext, uint8_t* ciphertext, 
                    size_t len, const uint8_t* aad, size_t aad_len, uint8_t tag[16]);
int sm4_gcm_decrypt(sm4_gcm_ctx_t* ctx, const uint8_t* ciphertext, uint8_t* plaintext,
                    size_t len, const uint8_t* aad, size_t aad_len, const uint8_t tag[16]);

// 性能测试
void sm4_performance_test(void);

#endif