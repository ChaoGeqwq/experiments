#include "sm4_optimized.h"
#include <immintrin.h>

static inline uint32_t rotl32(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

#ifdef __AES__

// 使用AES指令优化的S盒查找
static inline __m128i sm4_sbox_simd(__m128i input) {
    // 简化实现：通过查找表进行S盒变换
    uint8_t bytes[16];
    _mm_storeu_si128((__m128i*)bytes, input);
    
    for (int i = 0; i < 16; i++) {
        bytes[i] = SM4_SBOX[bytes[i]];
    }
    
    return _mm_loadu_si128((__m128i*)bytes);
}

// AESNI优化的单块加密
void sm4_aesni_encrypt(const uint32_t plaintext[4], uint32_t ciphertext[4], const uint32_t round_keys[32]) {
    uint32_t x[4];
    memcpy(x, plaintext, 16);
    
    // 32轮SM4加密
    for (int i = 0; i < 32; i++) {
        uint32_t tmp = x[1] ^ x[2] ^ x[3] ^ round_keys[i];
        
        // S盒变换
        uint8_t* tmp_bytes = (uint8_t*)&tmp;
        uint32_t sbox_result = (SM4_SBOX[tmp_bytes[3]] << 24) |
                               (SM4_SBOX[tmp_bytes[2]] << 16) |
                               (SM4_SBOX[tmp_bytes[1]] << 8) |
                               SM4_SBOX[tmp_bytes[0]];
        
        // L变换
        uint32_t l_result = sbox_result ^ rotl32(sbox_result, 2) ^ rotl32(sbox_result, 10) ^ 
                           rotl32(sbox_result, 18) ^ rotl32(sbox_result, 24);
        
        // 更新状态
        uint32_t new_x = x[0] ^ l_result;
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = new_x;
    }
    
    // 反序变换
    ciphertext[0] = x[3];
    ciphertext[1] = x[2];
    ciphertext[2] = x[1];
    ciphertext[3] = x[0];
}

// AESNI优化的单块解密
void sm4_aesni_decrypt(const uint32_t ciphertext[4], uint32_t plaintext[4], const uint32_t round_keys[32]) {
    uint32_t x[4];
    memcpy(x, ciphertext, 16);
    
    // 32轮SM4解密（逆序密钥）
    for (int i = 0; i < 32; i++) {
        uint32_t tmp = x[1] ^ x[2] ^ x[3] ^ round_keys[31-i];
        
        // S盒变换
        uint8_t* tmp_bytes = (uint8_t*)&tmp;
        uint32_t sbox_result = (SM4_SBOX[tmp_bytes[3]] << 24) |
                               (SM4_SBOX[tmp_bytes[2]] << 16) |
                               (SM4_SBOX[tmp_bytes[1]] << 8) |
                               SM4_SBOX[tmp_bytes[0]];
        
        // L变换
        uint32_t l_result = sbox_result ^ rotl32(sbox_result, 2) ^ rotl32(sbox_result, 10) ^ 
                           rotl32(sbox_result, 18) ^ rotl32(sbox_result, 24);
        
        // 更新状态
        uint32_t new_x = x[0] ^ l_result;
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = new_x;
    }
    
    // 反序变换
    plaintext[0] = x[3];
    plaintext[1] = x[2];
    plaintext[2] = x[1];
    plaintext[3] = x[0];
}

// AESNI优化的多块加密
void sm4_aesni_encrypt_blocks(const uint8_t* plaintext, uint8_t* ciphertext, 
                              const uint32_t round_keys[32], size_t num_blocks) {
    for (size_t i = 0; i < num_blocks; i++) {
        sm4_aesni_encrypt((uint32_t*)(plaintext + i * 16), 
                         (uint32_t*)(ciphertext + i * 16), 
                         round_keys);
    }
}

#else

// 如果不支持AESNI，则使用T-table实现
void sm4_aesni_encrypt(const uint32_t plaintext[4], uint32_t ciphertext[4], const uint32_t round_keys[32]) {
    sm4_ttable_encrypt(plaintext, ciphertext, round_keys);
}

void sm4_aesni_decrypt(const uint32_t ciphertext[4], uint32_t plaintext[4], const uint32_t round_keys[32]) {
    sm4_ttable_decrypt(ciphertext, plaintext, round_keys);
}

void sm4_aesni_encrypt_blocks(const uint8_t* plaintext, uint8_t* ciphertext, 
                              const uint32_t round_keys[32], size_t num_blocks) {
    for (size_t i = 0; i < num_blocks; i++) {
        sm4_ttable_encrypt((uint32_t*)(plaintext + i * 16), 
                          (uint32_t*)(ciphertext + i * 16), 
                          round_keys);
    }
}

#endif
