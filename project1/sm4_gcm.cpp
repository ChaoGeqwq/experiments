#include "sm4_optimized.h"
#include <iostream>
#include <memory.h>

// GCM模式的多项式：x^128 + x^7 + x^2 + x + 1
static const uint64_t GCM_POLY = 0xE100000000000000ULL;

// 有限域GF(2^128)乘法
static void gf128_mul(const uint64_t a[2], const uint64_t b[2], uint64_t result[2]) {
    uint64_t tmp[4] = {0};
    
    // 使用Karatsuba算法优化
    for (int i = 0; i < 64; i++) {
        if (a[0] & (1ULL << i)) {
            tmp[0] ^= b[0] << i;
            tmp[1] ^= (b[1] << i) | (b[0] >> (64 - i));
        }
        if (a[1] & (1ULL << i)) {
            tmp[1] ^= b[0] << i;
            tmp[2] ^= (b[1] << i) | (b[0] >> (64 - i));
        }
    }
    
    // 模约简
    for (int i = 127; i >= 64; i--) {
        if (tmp[i/64] & (1ULL << (i%64))) {
            tmp[(i-128)/64] ^= GCM_POLY >> (128-i);
            tmp[(i-64)/64] ^= GCM_POLY << (i-64);
        }
    }
    
    result[0] = tmp[0];
    result[1] = tmp[1];
}

// GHASH函数
static void ghash(const uint64_t h[2], const uint8_t* data, size_t len, uint64_t result[2]) {
    result[0] = result[1] = 0;
    
    for (size_t i = 0; i < len; i += 16) {
        uint64_t block[2];
        if (i + 16 <= len) {
            memcpy(block, data + i, 16);
        } else {
            memset(block, 0, 16);
            memcpy(block, data + i, len - i);
        }
        
        // 字节序转换
        block[0] = __builtin_bswap64(block[0]);
        block[1] = __builtin_bswap64(block[1]);
        
        result[0] ^= block[0];
        result[1] ^= block[1];
        
        gf128_mul(result, h, result);
    }
}

// CTR模式加密
static void ctr_encrypt(const uint8_t* input, uint8_t* output, size_t len,
                       const uint32_t round_keys[32], uint64_t counter[2]) {
    uint8_t keystream[16];
    size_t blocks = (len + 15) / 16;
    
    for (size_t i = 0; i < blocks; i++) {
        // 生成密钥流
        uint32_t ctr_block[4];
        ctr_block[0] = (uint32_t)(counter[1] >> 32);
        ctr_block[1] = (uint32_t)(counter[1]);
        ctr_block[2] = (uint32_t)(counter[0] >> 32);
        ctr_block[3] = (uint32_t)(counter[0]);
        
        uint32_t ks_block[4];
        sm4_gfni_encrypt(ctr_block, ks_block, round_keys);
        
        memcpy(keystream, ks_block, 16);
        
        // XOR操作
        size_t block_len = (i == blocks - 1) ? (len - i * 16) : 16;
        for (size_t j = 0; j < block_len; j++) {
            output[i * 16 + j] = input[i * 16 + j] ^ keystream[j];
        }
        
        // 递增计数器
        if (++counter[0] == 0) {
            counter[1]++;
        }
    }
}

// SM4-GCM初始化
int sm4_gcm_init(sm4_gcm_ctx_t* ctx, const uint8_t key[16], const uint8_t iv[12]) {
    if (!ctx || !key || !iv) return -1;
    
    // 生成轮密钥
    sm4_ttable_init();  // 确保T-table已初始化
    sm4_ttable_keygen((uint32_t*)key, ctx->round_keys);
    
    // 计算H = E_K(0^128)
    uint32_t zero_block[4] = {0, 0, 0, 0};
    uint32_t h_block[4];
    sm4_gfni_encrypt(zero_block, h_block, ctx->round_keys);
    
    ctx->h[0] = ((uint64_t)h_block[0] << 32) | h_block[1];
    ctx->h[1] = ((uint64_t)h_block[2] << 32) | h_block[3];
    
    // 初始化计数器：IV || 0^31 || 1
    ctx->counter[1] = ((uint64_t)iv[0] << 56) | ((uint64_t)iv[1] << 48) |
                      ((uint64_t)iv[2] << 40) | ((uint64_t)iv[3] << 32) |
                      ((uint64_t)iv[4] << 24) | ((uint64_t)iv[5] << 16) |
                      ((uint64_t)iv[6] << 8) | iv[7];
    ctx->counter[0] = ((uint64_t)iv[8] << 56) | ((uint64_t)iv[9] << 48) |
                      ((uint64_t)iv[10] << 40) | ((uint64_t)iv[11] << 32) | 1;
    
    // 初始化AAD哈希
    memset(ctx->aad_hash, 0, 16);
    ctx->aad_len = 0;
    ctx->data_len = 0;
    
    return 0;
}

// SM4-GCM加密
int sm4_gcm_encrypt(sm4_gcm_ctx_t* ctx, const uint8_t* plaintext, uint8_t* ciphertext,
                    size_t len, const uint8_t* aad, size_t aad_len, uint8_t tag[16]) {
    if (!ctx || !plaintext || !ciphertext || !tag) return -1;
    
    // 1. 处理AAD
    uint64_t aad_hash[2] = {0, 0};
    if (aad && aad_len > 0) {
        ghash(ctx->h, aad, aad_len, aad_hash);
    }
    
    // 2. CTR模式加密
    uint64_t ctr = ctx->counter[0];
    ctx->counter[0]++;  // J0 + 1
    ctr_encrypt(plaintext, ciphertext, len, ctx->round_keys, ctx->counter);
    
    // 3. 计算密文的GHASH
    uint64_t cipher_hash[2] = {0, 0};
    ghash(ctx->h, ciphertext, len, cipher_hash);
    
    // 4. 组合AAD和密文的哈希
    uint64_t combined_hash[2];
    combined_hash[0] = aad_hash[0] ^ cipher_hash[0];
    combined_hash[1] = aad_hash[1] ^ cipher_hash[1];
    
    // 5. 添加长度信息
    uint8_t len_block[16] = {0};
    uint64_t aad_bits = aad_len * 8;
    uint64_t data_bits = len * 8;
    
    // 大端序存储长度
    len_block[0] = (aad_bits >> 56) & 0xFF;
    len_block[1] = (aad_bits >> 48) & 0xFF;
    len_block[2] = (aad_bits >> 40) & 0xFF;
    len_block[3] = (aad_bits >> 32) & 0xFF;
    len_block[4] = (aad_bits >> 24) & 0xFF;
    len_block[5] = (aad_bits >> 16) & 0xFF;
    len_block[6] = (aad_bits >> 8) & 0xFF;
    len_block[7] = aad_bits & 0xFF;
    
    len_block[8] = (data_bits >> 56) & 0xFF;
    len_block[9] = (data_bits >> 48) & 0xFF;
    len_block[10] = (data_bits >> 40) & 0xFF;
    len_block[11] = (data_bits >> 32) & 0xFF;
    len_block[12] = (data_bits >> 24) & 0xFF;
    len_block[13] = (data_bits >> 16) & 0xFF;
    len_block[14] = (data_bits >> 8) & 0xFF;
    len_block[15] = data_bits & 0xFF;
    
    uint64_t len_hash[2];
    ghash(ctx->h, len_block, 16, len_hash);
    
    combined_hash[0] ^= len_hash[0];
    combined_hash[1] ^= len_hash[1];
    
    // 6. 生成认证标签
    uint32_t j0_block[4];
    j0_block[0] = (uint32_t)(ctx->counter[1] >> 32);
    j0_block[1] = (uint32_t)(ctx->counter[1]);
    j0_block[2] = (uint32_t)(ctx->counter[0] >> 32);
    j0_block[3] = (uint32_t)(ctr);  // 原始J0
    
    uint32_t tag_mask[4];
    sm4_gfni_encrypt(j0_block, tag_mask, ctx->round_keys);
    
    // 应用标签掩码
    uint64_t* tag64 = (uint64_t*)tag;
    tag64[0] = combined_hash[0] ^ (((uint64_t)tag_mask[0] << 32) | tag_mask[1]);
    tag64[1] = combined_hash[1] ^ (((uint64_t)tag_mask[2] << 32) | tag_mask[3]);
    
    return 0;
}

// SM4-GCM解密
int sm4_gcm_decrypt(sm4_gcm_ctx_t* ctx, const uint8_t* ciphertext, uint8_t* plaintext,
                    size_t len, const uint8_t* aad, size_t aad_len, const uint8_t tag[16]) {
    if (!ctx || !ciphertext || !plaintext || !tag) return -1;
    
    // 验证标签（与加密过程相同的哈希计算）
    uint8_t computed_tag[16];
    
    // 临时加密上下文用于计算预期标签
    sm4_gcm_ctx_t temp_ctx = *ctx;
    
    // 1. 处理AAD
    uint64_t aad_hash[2] = {0, 0};
    if (aad && aad_len > 0) {
        ghash(ctx->h, aad, aad_len, aad_hash);
    }
    
    // 2. 计算密文的GHASH
    uint64_t cipher_hash[2] = {0, 0};
    ghash(ctx->h, ciphertext, len, cipher_hash);
    
    // 3-6. 与加密相同的标签计算过程
    uint64_t combined_hash[2];
    combined_hash[0] = aad_hash[0] ^ cipher_hash[0];
    combined_hash[1] = aad_hash[1] ^ cipher_hash[1];
    
    // 长度块处理
    uint8_t len_block[16] = {0};
    uint64_t aad_bits = aad_len * 8;
    uint64_t data_bits = len * 8;
    
    len_block[0] = (aad_bits >> 56) & 0xFF;
    len_block[1] = (aad_bits >> 48) & 0xFF;
    len_block[2] = (aad_bits >> 40) & 0xFF;
    len_block[3] = (aad_bits >> 32) & 0xFF;
    len_block[4] = (aad_bits >> 24) & 0xFF;
    len_block[5] = (aad_bits >> 16) & 0xFF;
    len_block[6] = (aad_bits >> 8) & 0xFF;
    len_block[7] = aad_bits & 0xFF;
    
    len_block[8] = (data_bits >> 56) & 0xFF;
    len_block[9] = (data_bits >> 48) & 0xFF;
    len_block[10] = (data_bits >> 40) & 0xFF;
    len_block[11] = (data_bits >> 32) & 0xFF;
    len_block[12] = (data_bits >> 24) & 0xFF;
    len_block[13] = (data_bits >> 16) & 0xFF;
    len_block[14] = (data_bits >> 8) & 0xFF;
    len_block[15] = data_bits & 0xFF;
    
    uint64_t len_hash[2];
    ghash(ctx->h, len_block, 16, len_hash);
    
    combined_hash[0] ^= len_hash[0];
    combined_hash[1] ^= len_hash[1];
    
    // 生成预期的认证标签
    uint32_t j0_block[4];
    uint64_t ctr = ctx->counter[0];
    j0_block[0] = (uint32_t)(ctx->counter[1] >> 32);
    j0_block[1] = (uint32_t)(ctx->counter[1]);
    j0_block[2] = (uint32_t)(ctx->counter[0] >> 32);
    j0_block[3] = (uint32_t)(ctr);
    
    uint32_t tag_mask[4];
    sm4_gfni_encrypt(j0_block, tag_mask, ctx->round_keys);
    
    uint64_t* computed_tag64 = (uint64_t*)computed_tag;
    computed_tag64[0] = combined_hash[0] ^ (((uint64_t)tag_mask[0] << 32) | tag_mask[1]);
    computed_tag64[1] = combined_hash[1] ^ (((uint64_t)tag_mask[2] << 32) | tag_mask[3]);
    
    // 验证标签
    if (memcmp(tag, computed_tag, 16) != 0) {
        return -1;  // 认证失败
    }
    
    // 标签验证通过，进行解密
    ctx->counter[0]++;  // J0 + 1
    ctr_encrypt(ciphertext, plaintext, len, ctx->round_keys, ctx->counter);
    
    return 0;
}
