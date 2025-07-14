#include "sm4_optimized.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>

using namespace std;
using namespace chrono;

// 性能测试数据
const size_t TEST_BLOCKS = 100000;
const size_t BLOCK_SIZE = 16;

void print_hex(const uint8_t* data, size_t len, const string& label) {
    cout << label << ": ";
    for (size_t i = 0; i < len; i++) {
        cout << hex << setfill('0') << setw(2) << (int)data[i];
    }
    cout << dec << endl;
}

void test_correctness() {
    cout << "\n=== 正确性测试 ===" << endl;
    
    // 测试数据
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t plaintext[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t ciphertext[4], decrypted[4];
    uint32_t round_keys[32];
    
    // 初始化T-table
    sm4_ttable_init();
    sm4_ttable_keygen(key, round_keys);
    
    cout << "明文: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << plaintext[i] << " ";
    }
    cout << dec << endl;
    
    // T-table方法测试
    sm4_ttable_encrypt(plaintext, ciphertext, round_keys);
    cout << "T-table加密: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << ciphertext[i] << " ";
    }
    cout << dec << endl;
    
    sm4_ttable_decrypt(ciphertext, decrypted, round_keys);
    cout << "T-table解密: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << decrypted[i] << " ";
    }
    cout << dec << endl;
    
    // 验证解密正确性
    bool correct = true;
    for (int i = 0; i < 4; i++) {
        if (plaintext[i] != decrypted[i]) {
            correct = false;
            break;
        }
    }
    cout << "T-table正确性: " << (correct ? "通过" : "失败") << endl;
    
    // AESNI方法测试
    memset(ciphertext, 0, sizeof(ciphertext));
    memset(decrypted, 0, sizeof(decrypted));
    
    sm4_aesni_encrypt(plaintext, ciphertext, round_keys);
    cout << "AESNI加密: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << ciphertext[i] << " ";
    }
    cout << dec << endl;
    
    sm4_aesni_decrypt(ciphertext, decrypted, round_keys);
    correct = true;
    for (int i = 0; i < 4; i++) {
        if (plaintext[i] != decrypted[i]) {
            correct = false;
            break;
        }
    }
    cout << "AESNI正确性: " << (correct ? "通过" : "失败") << endl;
    
    // GFNI方法测试
    memset(ciphertext, 0, sizeof(ciphertext));
    memset(decrypted, 0, sizeof(decrypted));
    
    sm4_gfni_encrypt(plaintext, ciphertext, round_keys);
    cout << "GFNI加密: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << ciphertext[i] << " ";
    }
    cout << dec << endl;
    
    sm4_gfni_decrypt(ciphertext, decrypted, round_keys);
    correct = true;
    for (int i = 0; i < 4; i++) {
        if (plaintext[i] != decrypted[i]) {
            correct = false;
            break;
        }
    }
    cout << "GFNI正确性: " << (correct ? "通过" : "失败") << endl;
}

void performance_test_single() {
    cout << "\n=== 单块性能测试 ===" << endl;
    
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t plaintext[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t ciphertext[4];
    uint32_t round_keys[32];
    
    sm4_ttable_init();
    sm4_ttable_keygen(key, round_keys);
    
    const int iterations = 1000000;
    
    // T-table性能测试
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        sm4_ttable_encrypt(plaintext, ciphertext, round_keys);
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    cout << "T-table: " << iterations << " 次加密耗时 " 
         << duration.count() << " 微秒 (" 
         << (double)iterations / duration.count() << " MB/s)" << endl;
    
    // AESNI性能测试
    start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        sm4_aesni_encrypt(plaintext, ciphertext, round_keys);
    }
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "AESNI: " << iterations << " 次加密耗时 " 
         << duration.count() << " 微秒 (" 
         << (double)iterations / duration.count() << " MB/s)" << endl;
    
    // GFNI性能测试
    start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        sm4_gfni_encrypt(plaintext, ciphertext, round_keys);
    }
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    cout << "GFNI: " << iterations << " 次加密耗时 " 
         << duration.count() << " 微秒 (" 
         << (double)iterations / duration.count() << " MB/s)" << endl;
}

void performance_test_blocks() {
    cout << "\n=== 多块性能测试 ===" << endl;
    
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t round_keys[32];
    
    sm4_ttable_init();
    sm4_ttable_keygen(key, round_keys);
    
    // 准备测试数据
    vector<uint8_t> plaintext(TEST_BLOCKS * BLOCK_SIZE);
    vector<uint8_t> ciphertext(TEST_BLOCKS * BLOCK_SIZE);
    
    for (size_t i = 0; i < plaintext.size(); i++) {
        plaintext[i] = i & 0xFF;
    }
    
    // AESNI多块测试
    auto start = high_resolution_clock::now();
    sm4_aesni_encrypt_blocks(plaintext.data(), ciphertext.data(), round_keys, TEST_BLOCKS);
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double throughput = (double)(TEST_BLOCKS * BLOCK_SIZE) / (1024 * 1024) / (duration.count() / 1000000.0);
    cout << "AESNI多块: " << TEST_BLOCKS << " 块加密耗时 " 
         << duration.count() << " 微秒 (" 
         << fixed << setprecision(2) << throughput << " MB/s)" << endl;
    
    // AVX512多块测试
    start = high_resolution_clock::now();
    sm4_avx512_encrypt_blocks(plaintext.data(), ciphertext.data(), round_keys, TEST_BLOCKS);
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    throughput = (double)(TEST_BLOCKS * BLOCK_SIZE) / (1024 * 1024) / (duration.count() / 1000000.0);
    cout << "AVX512多块: " << TEST_BLOCKS << " 块加密耗时 " 
         << duration.count() << " 微秒 (" 
         << fixed << setprecision(2) << throughput << " MB/s)" << endl;
}

void test_gcm_mode() {
    cout << "\n=== SM4-GCM模式测试 ===" << endl;
    
    // 测试数据
    uint8_t key[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    
    uint8_t iv[12] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b
    };
    
    uint8_t plaintext[] = "Hello, SM4-GCM World! This is a test message.";
    size_t plaintext_len = strlen((char*)plaintext);
    
    uint8_t aad[] = "Additional Authenticated Data";
    size_t aad_len = strlen((char*)aad);
    
    vector<uint8_t> ciphertext(plaintext_len);
    vector<uint8_t> decrypted(plaintext_len);
    uint8_t tag[16];
    
    sm4_gcm_ctx_t ctx;
    
    // 初始化
    if (sm4_gcm_init(&ctx, key, iv) != 0) {
        cout << "GCM初始化失败!" << endl;
        return;
    }
    
    cout << "明文: " << (char*)plaintext << endl;
    print_hex(plaintext, plaintext_len, "明文(hex)");
    print_hex(aad, aad_len, "AAD");
    
    // 加密
    auto start = high_resolution_clock::now();
    if (sm4_gcm_encrypt(&ctx, plaintext, ciphertext.data(), plaintext_len, 
                       aad, aad_len, tag) != 0) {
        cout << "GCM加密失败!" << endl;
        return;
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    
    print_hex(ciphertext.data(), plaintext_len, "密文");
    print_hex(tag, 16, "认证标签");
    cout << "GCM加密耗时: " << duration.count() << " 微秒" << endl;
    
    // 重新初始化上下文用于解密
    if (sm4_gcm_init(&ctx, key, iv) != 0) {
        cout << "GCM重新初始化失败!" << endl;
        return;
    }
    
    // 解密
    start = high_resolution_clock::now();
    if (sm4_gcm_decrypt(&ctx, ciphertext.data(), decrypted.data(), plaintext_len,
                       aad, aad_len, tag) != 0) {
        cout << "GCM解密或认证失败!" << endl;
        return;
    }
    end = high_resolution_clock::now();
    duration = duration_cast<microseconds>(end - start);
    
    // 添加字符串结束符
    decrypted.push_back('\0');
    cout << "解密结果: " << (char*)decrypted.data() << endl;
    cout << "GCM解密耗时: " << duration.count() << " 微秒" << endl;
    
    // 验证正确性
    bool correct = (memcmp(plaintext, decrypted.data(), plaintext_len) == 0);
    cout << "GCM正确性: " << (correct ? "通过" : "失败") << endl;
    
    // 测试认证失败情况
    cout << "\n--- 测试认证失败情况 ---" << endl;
    uint8_t wrong_tag[16];
    memcpy(wrong_tag, tag, 16);
    wrong_tag[0] ^= 0x01;  // 修改标签
    
    if (sm4_gcm_init(&ctx, key, iv) != 0) {
        cout << "GCM重新初始化失败!" << endl;
        return;
    }
    
    vector<uint8_t> decrypted2(plaintext_len);
    int result = sm4_gcm_decrypt(&ctx, ciphertext.data(), decrypted2.data(), plaintext_len,
                                aad, aad_len, wrong_tag);
    cout << "错误标签解密结果: " << (result == 0 ? "成功(错误!)" : "失败(正确)") << endl;
}

void sm4_performance_test() {
    cout << "========== SM4优化实现性能测试 ==========" << endl;
    
    test_correctness();
    performance_test_single();
    performance_test_blocks();
    test_gcm_mode();
    
    cout << "\n========== 测试完成 ==========" << endl;
}
