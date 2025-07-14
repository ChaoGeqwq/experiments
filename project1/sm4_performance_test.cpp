#include "sm4_optimized.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <iomanip>
#include <cstring>

using namespace std;
using namespace chrono;

void print_hex(const uint8_t* data, size_t len, const string& label) {
    cout << label << ": ";
    for (size_t i = 0; i < len; i++) {
        cout << hex << setfill('0') << setw(2) << (int)data[i];
    }
    cout << dec << endl;
}

void test_correctness() {
    cout << "\n=== 正确性测试 ===" << endl;
    
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t plaintext[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t ciphertext[4], decrypted[4];
    uint32_t round_keys[32];
    
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
    bool correct = memcmp(plaintext, decrypted, 16) == 0;
    cout << "T-table正确性: " << (correct ? "通过" : "失败") << endl;
}

void performance_test_single() {
    cout << "\n=== 单块性能测试 ===" << endl;
    
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t plaintext[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t ciphertext[4];
    uint32_t round_keys[32];
    
    sm4_ttable_init();
    sm4_ttable_keygen(key, round_keys);
    
    const int iterations = 100000;
    
    // T-table性能测试
    auto start = high_resolution_clock::now();
    for (int i = 0; i < iterations; i++) {
        sm4_ttable_encrypt(plaintext, ciphertext, round_keys);
    }
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(end - start);
    double throughput = (double)(iterations * 16) / (1024 * 1024) / (duration.count() / 1000000.0);
    cout << "T-table: " << iterations << " 次加密耗时 " 
         << duration.count() << " 微秒 (" 
         << fixed << setprecision(2) << throughput << " MB/s)" << endl;
}

void test_gcm_basic() {
    cout << "\n=== SM4-GCM基础测试 ===" << endl;
    
    uint8_t key[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    
    uint8_t iv[12] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b
    };
    
    string message = "Hello SM4-GCM!";
    uint8_t* plaintext = (uint8_t*)message.c_str();
    size_t plaintext_len = message.length();
    
    string aad_str = "AAD";
    uint8_t* aad = (uint8_t*)aad_str.c_str();
    size_t aad_len = aad_str.length();
    
    vector<uint8_t> ciphertext(plaintext_len);
    vector<uint8_t> decrypted(plaintext_len + 1);
    uint8_t tag[16];
    
    sm4_gcm_ctx_t ctx;
    
    cout << "明文: " << message << endl;
    
    // 加密
    if (sm4_gcm_init(&ctx, key, iv) == 0 &&
        sm4_gcm_encrypt(&ctx, plaintext, ciphertext.data(), plaintext_len, 
                       aad, aad_len, tag) == 0) {
        cout << "GCM加密: 成功" << endl;
        
        // 解密
        if (sm4_gcm_init(&ctx, key, iv) == 0 &&
            sm4_gcm_decrypt(&ctx, ciphertext.data(), decrypted.data(), plaintext_len,
                           aad, aad_len, tag) == 0) {
            decrypted[plaintext_len] = '\0';
            cout << "GCM解密: " << (char*)decrypted.data() << endl;
            bool correct = (memcmp(plaintext, decrypted.data(), plaintext_len) == 0);
            cout << "GCM正确性: " << (correct ? "通过" : "失败") << endl;
        } else {
            cout << "GCM解密: 失败" << endl;
        }
    } else {
        cout << "GCM加密: 失败" << endl;
    }
}

void sm4_performance_test() {
    cout << "========== SM4优化实现性能测试 ==========" << endl;
    test_correctness();
    performance_test_single();
    test_gcm_basic();
    cout << "\n========== 测试完成 ==========" << endl;
}

int main() {
    sm4_performance_test();
    return 0;
}
