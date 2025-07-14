#include "sm4_optimized.h"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace std;

void print_header(const string& title) {
    cout << "\n" << string(50, '=') << endl;
    cout << title << endl;
    cout << string(50, '=') << endl;
}

void basic_encryption_test() {
    print_header("基础加密解密测试");
    
    // 测试向量
    uint32_t key[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t plaintext[4] = {0x01234567, 0x89abcdef, 0xfedcba98, 0x76543210};
    uint32_t ciphertext[4], decrypted[4];
    uint32_t round_keys[32];
    
    cout << "密钥: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << key[i] << " ";
    }
    cout << dec << endl;
    
    cout << "明文: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << plaintext[i] << " ";
    }
    cout << dec << endl;
    
    // 初始化并生成轮密钥
    sm4_ttable_init();
    sm4_ttable_keygen(key, round_keys);
    
    // T-table实现测试
    cout << "\n--- T-table实现 ---" << endl;
    sm4_ttable_encrypt(plaintext, ciphertext, round_keys);
    cout << "密文: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << ciphertext[i] << " ";
    }
    cout << dec << endl;
    
    sm4_ttable_decrypt(ciphertext, decrypted, round_keys);
    cout << "解密: ";
    for (int i = 0; i < 4; i++) {
        cout << hex << setfill('0') << setw(8) << decrypted[i] << " ";
    }
    cout << dec << endl;
    
    // 验证正确性
    bool correct = true;
    for (int i = 0; i < 4; i++) {
        if (plaintext[i] != decrypted[i]) {
            correct = false;
            break;
        }
    }
    cout << "T-table 正确性: " << (correct ? "✓ 通过" : "✗ 失败") << endl;
    
    // AESNI实现测试
    cout << "\n--- AESNI实现 ---" << endl;
    memset(ciphertext, 0, sizeof(ciphertext));
    memset(decrypted, 0, sizeof(decrypted));
    
    sm4_aesni_encrypt(plaintext, ciphertext, round_keys);
    cout << "密文: ";
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
    cout << "AESNI 正确性: " << (correct ? "✓ 通过" : "✗ 失败") << endl;
    
    // GFNI实现测试
    cout << "\n--- GFNI实现 ---" << endl;
    memset(ciphertext, 0, sizeof(ciphertext));
    memset(decrypted, 0, sizeof(decrypted));
    
    sm4_gfni_encrypt(plaintext, ciphertext, round_keys);
    cout << "密文: ";
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
    cout << "GFNI 正确性: " << (correct ? "✓ 通过" : "✗ 失败") << endl;
}

void gcm_mode_demo() {
    print_header("SM4-GCM模式演示");
    
    // 测试数据
    uint8_t key[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10
    };
    
    uint8_t iv[12] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b
    };
    
    string message = "This is a secret message for SM4-GCM encryption!";
    uint8_t* plaintext = (uint8_t*)message.c_str();
    size_t plaintext_len = message.length();
    
    string aad_str = "Additional Authentication Data";
    uint8_t* aad = (uint8_t*)aad_str.c_str();
    size_t aad_len = aad_str.length();
    
    cout << "原始消息: " << message << endl;
    cout << "AAD: " << aad_str << endl;
    
    // 显示密钥和IV
    cout << "密钥: ";
    for (int i = 0; i < 16; i++) {
        cout << hex << setfill('0') << setw(2) << (int)key[i];
    }
    cout << dec << endl;
    
    cout << "IV: ";
    for (int i = 0; i < 12; i++) {
        cout << hex << setfill('0') << setw(2) << (int)iv[i];
    }
    cout << dec << endl;
    
    // 加密
    vector<uint8_t> ciphertext(plaintext_len);
    uint8_t tag[16];
    sm4_gcm_ctx_t ctx;
    
    if (sm4_gcm_init(&ctx, key, iv) != 0) {
        cout << "❌ GCM初始化失败!" << endl;
        return;
    }
    
    if (sm4_gcm_encrypt(&ctx, plaintext, ciphertext.data(), plaintext_len, 
                       aad, aad_len, tag) != 0) {
        cout << "❌ GCM加密失败!" << endl;
        return;
    }
    
    cout << "\n密文: ";
    for (size_t i = 0; i < plaintext_len; i++) {
        cout << hex << setfill('0') << setw(2) << (int)ciphertext[i];
    }
    cout << dec << endl;
    
    cout << "认证标签: ";
    for (int i = 0; i < 16; i++) {
        cout << hex << setfill('0') << setw(2) << (int)tag[i];
    }
    cout << dec << endl;
    
    // 解密
    vector<uint8_t> decrypted(plaintext_len + 1);  // +1 for null terminator
    
    if (sm4_gcm_init(&ctx, key, iv) != 0) {
        cout << "❌ GCM重新初始化失败!" << endl;
        return;
    }
    
    if (sm4_gcm_decrypt(&ctx, ciphertext.data(), decrypted.data(), plaintext_len,
                       aad, aad_len, tag) != 0) {
        cout << "❌ GCM解密或认证失败!" << endl;
        return;
    }
    
    decrypted[plaintext_len] = '\0';  // Add null terminator
    cout << "\n解密结果: " << (char*)decrypted.data() << endl;
    
    // 验证正确性
    bool correct = (memcmp(plaintext, decrypted.data(), plaintext_len) == 0);
    cout << "解密正确性: " << (correct ? "✓ 通过" : "✗ 失败") << endl;
    
    // 演示认证失败情况
    cout << "\n--- 测试认证失败情况 ---" << endl;
    uint8_t corrupted_tag[16];
    memcpy(corrupted_tag, tag, 16);
    corrupted_tag[0] ^= 0x01;  // 破坏标签
    
    if (sm4_gcm_init(&ctx, key, iv) != 0) {
        cout << "❌ GCM重新初始化失败!" << endl;
        return;
    }
    
    vector<uint8_t> decrypted2(plaintext_len);
    int result = sm4_gcm_decrypt(&ctx, ciphertext.data(), decrypted2.data(), plaintext_len,
                                aad, aad_len, corrupted_tag);
    
    if (result == 0) {
        cout << "❌ 错误！被篡改的标签通过了验证" << endl;
    } else {
        cout << "✓ 正确！被篡改的标签被检测出来" << endl;
    }
}

void instruction_support_check() {
    print_header("指令集支持检查");
    
    // 检查编译时的指令集支持
    cout << "编译时指令集支持:" << endl;
    
#ifdef __AES__
    cout << "✓ AES-NI 支持" << endl;
#else
    cout << "✗ AES-NI 不支持" << endl;
#endif

#ifdef __GFNI__
    cout << "✓ GFNI 支持" << endl;
#else
    cout << "✗ GFNI 不支持" << endl;
#endif

#ifdef __AVX512F__
    cout << "✓ AVX512F 支持" << endl;
#else
    cout << "✗ AVX512F 不支持" << endl;
#endif

#ifdef __AVX512VL__
    cout << "✓ AVX512VL 支持" << endl;
#else
    cout << "✗ AVX512VL 不支持" << endl;
#endif

    cout << "\n注意: 即使编译时支持，运行时也需要CPU支持这些指令集。" << endl;
}

int main() {
    cout << "SM4密码算法优化实现测试程序" << endl;
    cout << "包含T-table、AESNI、GFNI指令集优化和SM4-GCM模式" << endl;
    
    instruction_support_check();
    basic_encryption_test();
    gcm_mode_demo();
    
    cout << "\n\n如需进行详细性能测试，请调用 sm4_performance_test() 函数。" << endl;
    
    // 可选：运行性能测试（取消注释下行）
    // sm4_performance_test();
    
    return 0;
}
