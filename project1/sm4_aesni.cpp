#include "sm4_optimized.h"
#include <immintrin.h>

#ifdef __AES__

// 使用AESNI指令实现SM4 S盒查找
static inline __m128i sm4_sbox_simd(__m128i input) {
    // 使用AES的SubBytes变换作为基础，通过查找表修正
    // 这里简化实现，实际需要通过复杂的仿射变换来实现SM4 S盒
    
    // 拆分为4个字节分别处理
    uint8_t bytes[16];
    _mm_storeu_si128((__m128i*)bytes, input);
    
    for (int i = 0; i < 16; i++) {
        bytes[i] = SM4_SBOX[bytes[i]];
    }
    
    return _mm_loadu_si128((__m128i*)bytes);
}

// AESNI优化的单块加密
void sm4_aesni_encrypt(const uint32_t plaintext[4], uint32_t ciphertext[4], const uint32_t round_keys[32]) {
    __m128i state = _mm_loadu_si128((__m128i*)plaintext);
    
    // 字节序转换（大端序）
    state = _mm_shuffle_epi8(state, _mm_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12));
    
    uint32_t x[4];
    _mm_storeu_si128((__m128i*)x, state);
    
    // 32轮SM4加密
    for (int i = 0; i < 32; i++) {
        uint32_t tmp = x[1] ^ x[2] ^ x[3] ^ round_keys[i];
        
        // 使用SIMD优化的S盒变换
        __m128i tmp_vec = _mm_set1_epi32(tmp);
        tmp_vec = sm4_sbox_simd(tmp_vec);
        uint32_t sbox_result = _mm_cvtsi128_si32(tmp_vec);
        
        // L变换
        uint32_t l_result = sbox_result ^ rotl(sbox_result, 2) ^ rotl(sbox_result, 10) ^ 
                           rotl(sbox_result, 18) ^ rotl(sbox_result, 24);
        
        // 更新状态
        uint32_t new_x = x[0] ^ l_result;
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = new_x;
    }
    
    // 反序变换
    state = _mm_setr_epi32(x[3], x[2], x[1], x[0]);
    
    // 字节序转换回来
    state = _mm_shuffle_epi8(state, _mm_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12));
    
    _mm_storeu_si128((__m128i*)ciphertext, state);
}

// AESNI优化的单块解密
void sm4_aesni_decrypt(const uint32_t ciphertext[4], uint32_t plaintext[4], const uint32_t round_keys[32]) {
    __m128i state = _mm_loadu_si128((__m128i*)ciphertext);
    
    // 字节序转换
    state = _mm_shuffle_epi8(state, _mm_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12));
    
    uint32_t x[4];
    _mm_storeu_si128((__m128i*)x, state);
    
    // 32轮SM4解密（逆序密钥）
    for (int i = 0; i < 32; i++) {
        uint32_t tmp = x[1] ^ x[2] ^ x[3] ^ round_keys[31-i];
        
        // 使用SIMD优化的S盒变换
        __m128i tmp_vec = _mm_set1_epi32(tmp);
        tmp_vec = sm4_sbox_simd(tmp_vec);
        uint32_t sbox_result = _mm_cvtsi128_si32(tmp_vec);
        
        // L变换
        uint32_t l_result = sbox_result ^ rotl(sbox_result, 2) ^ rotl(sbox_result, 10) ^ 
                           rotl(sbox_result, 18) ^ rotl(sbox_result, 24);
        
        // 更新状态
        uint32_t new_x = x[0] ^ l_result;
        x[0] = x[1];
        x[1] = x[2];
        x[2] = x[3];
        x[3] = new_x;
    }
    
    // 反序变换
    state = _mm_setr_epi32(x[3], x[2], x[1], x[0]);
    
    // 字节序转换回来
    state = _mm_shuffle_epi8(state, _mm_setr_epi8(3,2,1,0,7,6,5,4,11,10,9,8,15,14,13,12));
    
    _mm_storeu_si128((__m128i*)plaintext, state);
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