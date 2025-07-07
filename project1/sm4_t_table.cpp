#include <iostream>
#include <cstring>
#include <vector>
#include <chrono>

using namespace std;

// SM4 S盒
const uint8_t SBox[256] = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7, 0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3, 0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a, 0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
    0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95, 0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
    0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba, 0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
    0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b, 0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
    0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2, 0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
    0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52, 0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
    0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5, 0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
    0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55, 0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
    0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60, 0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
    0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f, 0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
    0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f, 0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
    0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd, 0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
    0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e, 0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
    0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20, 0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48
};

// FK常量
const uint32_t FK[4] = {0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc};

// CK常量
const uint32_t CK[32] = {
    0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
    0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
    0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
    0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
    0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
    0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
    0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
    0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279
};

// T表：预计算S盒替换和线性变换的结果
// T0[i] = L(S(i || 0x00 || 0x00 || 0x00))
// T1[i] = L(S(0x00 || i || 0x00 || 0x00))
// T2[i] = L(S(0x00 || 0x00 || i || 0x00))
// T3[i] = L(S(0x00 || 0x00 || 0x00 || i))
uint32_t T0[256], T1[256], T2[256], T3[256];

// 密钥扩展用的T表
// TK0[i] = L'(S(i || 0x00 || 0x00 || 0x00))
// TK1[i] = L'(S(0x00 || i || 0x00 || 0x00))
// TK2[i] = L'(S(0x00 || 0x00 || i || 0x00))
// TK3[i] = L'(S(0x00 || 0x00 || 0x00 || i))
uint32_t TK0[256], TK1[256], TK2[256], TK3[256];

// 循环左移
uint32_t RotL(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// 线性变换L
uint32_t L(uint32_t x) {
    return x ^ RotL(x, 2) ^ RotL(x, 10) ^ RotL(x, 18) ^ RotL(x, 24);
}

// 密钥扩展线性变换L'
uint32_t LPrime(uint32_t x) {
    return x ^ RotL(x, 13) ^ RotL(x, 23);
}

// 生成T表
void GenerateTTables() {
    cout << "生成T表..." << endl;
    
    for (int i = 0; i < 256; i++) {
        uint8_t s = SBox[i];
        
        // 生成加密用的T表
        uint32_t val0 = (s << 24);
        uint32_t val1 = (s << 16);
        uint32_t val2 = (s << 8);
        uint32_t val3 = s;
        
        T0[i] = L(val0);
        T1[i] = L(val1);
        T2[i] = L(val2);
        T3[i] = L(val3);
        
        // 生成密钥扩展用的T表
        TK0[i] = LPrime(val0);
        TK1[i] = LPrime(val1);
        TK2[i] = LPrime(val2);
        TK3[i] = LPrime(val3);
    }
    
    cout << "T表生成完成!" << endl;
}

// 使用T表的非线性变换
uint32_t TauWithTTable(uint32_t x) {
    return T0[(x >> 24) & 0xFF] ^ T1[(x >> 16) & 0xFF] ^ 
           T2[(x >> 8) & 0xFF] ^ T3[x & 0xFF];
}

// 使用T表的密钥扩展非线性变换
uint32_t TauKeyWithTTable(uint32_t x) {
    return TK0[(x >> 24) & 0xFF] ^ TK1[(x >> 16) & 0xFF] ^ 
           TK2[(x >> 8) & 0xFF] ^ TK3[x & 0xFF];
}

// 传统方式的非线性变换（用于对比）
uint32_t TauTraditional(uint32_t x) {
    uint32_t result = (SBox[x >> 24] << 24) | (SBox[(x >> 16) & 0xFF] << 16) |
                     (SBox[(x >> 8) & 0xFF] << 8) | SBox[x & 0xFF];
    return L(result);
}

// 传统方式的密钥扩展非线性变换
uint32_t TauKeyTraditional(uint32_t x) {
    uint32_t result = (SBox[x >> 24] << 24) | (SBox[(x >> 16) & 0xFF] << 16) |
                     (SBox[(x >> 8) & 0xFF] << 8) | SBox[x & 0xFF];
    return LPrime(result);
}

// 使用T表的密钥扩展
void KeyExpansionWithTTable(const uint32_t MK[4], uint32_t rk[32]) {
    uint32_t K[36];
    for (int i = 0; i < 4; i++) {
        K[i] = MK[i] ^ FK[i];
    }
    for (int i = 0; i < 32; i++) {
        K[i + 4] = K[i] ^ TauKeyWithTTable(K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ CK[i]);
        rk[i] = K[i + 4];
    }
}

// 传统方式的密钥扩展
void KeyExpansionTraditional(const uint32_t MK[4], uint32_t rk[32]) {
    uint32_t K[36];
    for (int i = 0; i < 4; i++) {
        K[i] = MK[i] ^ FK[i];
    }
    for (int i = 0; i < 32; i++) {
        K[i + 4] = K[i] ^ TauKeyTraditional(K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ CK[i]);
        rk[i] = K[i + 4];
    }
}

// 使用T表的加密/解密
void SM4CryptWithTTable(const uint32_t input[4], uint32_t output[4], const uint32_t rk[32], bool encrypt) {
    uint32_t X[36];
    for (int i = 0; i < 4; i++) {
        X[i] = input[i];
    }
    for (int i = 0; i < 32; i++) {
        int idx = encrypt ? i : 31 - i;
        X[i + 4] = X[i] ^ TauWithTTable(X[i + 1] ^ X[i + 2] ^ X[i + 3] ^ rk[idx]);
    }
    for (int i = 0; i < 4; i++) {
        output[i] = X[35 - i];
    }
}

// 传统方式的加密/解密
void SM4CryptTraditional(const uint32_t input[4], uint32_t output[4], const uint32_t rk[32], bool encrypt) {
    uint32_t X[36];
    for (int i = 0; i < 4; i++) {
        X[i] = input[i];
    }
    for (int i = 0; i < 32; i++) {
        int idx = encrypt ? i : 31 - i;
        X[i + 4] = X[i] ^ TauTraditional(X[i + 1] ^ X[i + 2] ^ X[i + 3] ^ rk[idx]);
    }
    for (int i = 0; i < 4; i++) {
        output[i] = X[35 - i];
    }
}

// 性能测试函数
void PerformanceTest() {
    const int TEST_ROUNDS = 1000000;
    uint32_t plaintext[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t ciphertext_ttable[4], ciphertext_traditional[4];
    uint32_t rk_ttable[32], rk_traditional[32];
    
    cout << "\n=== 性能测试 ===" << endl;
    cout << "测试轮数: " << TEST_ROUNDS << endl;
    
    // 测试T表方式的密钥扩展
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < TEST_ROUNDS; i++) {
        KeyExpansionWithTTable(key, rk_ttable);
    }
    auto end = chrono::high_resolution_clock::now();
    auto ttable_key_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    // 测试传统方式的密钥扩展
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < TEST_ROUNDS; i++) {
        KeyExpansionTraditional(key, rk_traditional);
    }
    end = chrono::high_resolution_clock::now();
    auto traditional_key_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    // 测试T表方式的加密
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < TEST_ROUNDS; i++) {
        SM4CryptWithTTable(plaintext, ciphertext_ttable, rk_ttable, true);
    }
    end = chrono::high_resolution_clock::now();
    auto ttable_encrypt_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    // 测试传统方式的加密
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < TEST_ROUNDS; i++) {
        SM4CryptTraditional(plaintext, ciphertext_traditional, rk_traditional, true);
    }
    end = chrono::high_resolution_clock::now();
    auto traditional_encrypt_time = chrono::duration_cast<chrono::microseconds>(end - start).count();
    
    // 输出结果
    cout << "\n密钥扩展性能:" << endl;
    cout << "T表方式:   " << ttable_key_time << " 微秒" << endl;
    cout << "传统方式:  " << traditional_key_time << " 微秒" << endl;
    cout << "性能提升:  " << (double)traditional_key_time / ttable_key_time << "x" << endl;
    
    cout << "\n加密性能:" << endl;
    cout << "T表方式:   " << ttable_encrypt_time << " 微秒" << endl;
    cout << "传统方式:  " << traditional_encrypt_time << " 微秒" << endl;
    cout << "性能提升:  " << (double)traditional_encrypt_time / ttable_encrypt_time << "x" << endl;
    
    cout << "\n总体性能:" << endl;
    cout << "T表方式:   " << ttable_key_time + ttable_encrypt_time << " 微秒" << endl;
    cout << "传统方式:  " << traditional_key_time + traditional_encrypt_time << " 微秒" << endl;
    cout << "性能提升:  " << (double)(traditional_key_time + traditional_encrypt_time) / (ttable_key_time + ttable_encrypt_time) << "x" << endl;
}

int main() {
    // 生成T表
    GenerateTTables();
    
    // 测试数据
    uint32_t plaintext[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t ciphertext_ttable[4], ciphertext_traditional[4];
    uint32_t decrypted_ttable[4], decrypted_traditional[4];
    uint32_t rk_ttable[32], rk_traditional[32];

    // 展示测试数据
    cout << "\n=== SM4 T表优化测试 ===" << endl;
    cout << "明文: ";
    for (int i = 0; i < 4; i++) {
        printf("%08x ", plaintext[i]);
    }
    cout << endl;

    cout << "密钥: ";
    for (int i = 0; i < 4; i++) {
        printf("%08x ", key[i]);
    }
    cout << endl;

    // T表方式测试
    cout << "\n=== T表方式 ===" << endl;
    KeyExpansionWithTTable(key, rk_ttable);
    SM4CryptWithTTable(plaintext, ciphertext_ttable, rk_ttable, true);
    SM4CryptWithTTable(ciphertext_ttable, decrypted_ttable, rk_ttable, false);
    
    cout << "加密结果: ";
    for (int i = 0; i < 4; i++) {
        printf("%08x ", ciphertext_ttable[i]);
    }
    cout << endl;
    
    cout << "解密结果: ";
    for (int i = 0; i < 4; i++) {
        printf("%08x ", decrypted_ttable[i]);
    }
    cout << endl;

    // 传统方式测试
    cout << "\n=== 传统方式 ===" << endl;
    KeyExpansionTraditional(key, rk_traditional);
    SM4CryptTraditional(plaintext, ciphertext_traditional, rk_traditional, true);
    SM4CryptTraditional(ciphertext_traditional, decrypted_traditional, rk_traditional, false);
    
    cout << "加密结果: ";
    for (int i = 0; i < 4; i++) {
        printf("%08x ", ciphertext_traditional[i]);
    }
    cout << endl;
    
    cout << "解密结果: ";
    for (int i = 0; i < 4; i++) {
        printf("%08x ", decrypted_traditional[i]);
    }
    cout << endl;

    // 验证两种方式的结果是否一致
    bool results_match = true;
    for (int i = 0; i < 4; i++) {
        if (ciphertext_ttable[i] != ciphertext_traditional[i] || 
            decrypted_ttable[i] != decrypted_traditional[i]) {
            results_match = false;
            break;
        }
    }
    
    cout << "\n=== 正确性验证 ===" << endl;
    cout << "两种方式结果是否一致: " << (results_match ? "是" : "否") << endl;
    
    // 性能测试
    PerformanceTest();
    
    cout << "\n=== T表优化说明 ===" << endl;
    cout << "T表优化原理:" << endl;
    cout << "1. 预计算S盒替换和线性变换的复合操作" << endl;
    cout << "2. 将原本的多步骤计算合并为一次查表操作" << endl;
    cout << "3. 减少了循环左移和异或操作的次数" << endl;
    cout << "4. 提高了缓存命中率，减少了计算延迟" << endl;
    cout << "5. 空间换时间的经典优化策略" << endl;

    return 0;
}