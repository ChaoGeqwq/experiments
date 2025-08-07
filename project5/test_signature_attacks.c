#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <openssl/rand.h>
#include "sm2_signature_attack.h"

// 测试基础签名功能
void test_basic_signature() {
    printf("测试基础SM2签名功能...\n");
    
    // 生成测试密钥
    unsigned char private_key[32];
    unsigned char public_key[65];
    RAND_bytes(private_key, 32);
    
    // 这里应该从私钥生成公钥，简化为随机生成
    RAND_bytes(public_key, 65);
    public_key[0] = 0x04;
    
    // 测试消息
    const char *test_message = "Hello SM2 Signature";
    SM2_SIGNATURE signature;
    
    // 测试签名
    int sign_result = sm2_sign_message((unsigned char*)test_message, strlen(test_message), private_key, &signature);
    if (sign_result == 1) {
        printf("✓ 签名生成成功\n");
        print_signature(&signature);
    } else {
        printf("✗ 签名生成失败\n");
    }
    
    // 注意：由于公钥是随机生成的，验证会失败，这是预期的
    printf("基础签名测试完成\n\n");
}

// 测试k值重用攻击
void test_k_reuse_attack() {
    printf("测试K值重用攻击...\n");
    
    unsigned char private_key[32];
    RAND_bytes(private_key, 32);
    
    const char *msg1 = "First message";
    const char *msg2 = "Second message";
    
    SM2_SIGNATURE sig1, sig2;
    
    // 生成使用相同k值的两个签名
    simulate_vulnerable_signature((unsigned char*)msg1, strlen(msg1), private_key, &sig1, ATTACK_K_REUSE);
    simulate_vulnerable_signature((unsigned char*)msg2, strlen(msg2), private_key, &sig2, ATTACK_K_REUSE);
    
    SM2_ATTACK_RESULT result;
    int attack_result = sm2_attack_k_reuse(&sig1, &sig2, sig1.message_hash, sig2.message_hash, &result);
    
    if (attack_result) {
        printf("✓ K值重用攻击测试完成\n");
        printf("攻击成功: %s\n", result.success ? "是" : "否");
        printf("描述: %s\n", result.description);
    } else {
        printf("✗ K值重用攻击测试失败\n");
    }
    printf("\n");
}

// 测试弱k值攻击
void test_weak_k_attack() {
    printf("测试弱K值攻击...\n");
    
    unsigned char private_key[32];
    RAND_bytes(private_key, 32);
    
    const char *message = "Message with weak k";
    SM2_SIGNATURE signature;
    
    // 生成使用弱k值的签名
    simulate_vulnerable_signature((unsigned char*)message, strlen(message), private_key, &signature, ATTACK_WEAK_K);
    
    SM2_ATTACK_RESULT result;
    int attack_result = sm2_attack_weak_k(&signature, signature.message_hash, &result);
    
    if (attack_result) {
        printf("✓ 弱K值攻击测试完成\n");
        printf("攻击成功: %s\n", result.success ? "是" : "否");
        printf("描述: %s\n", result.description);
    } else {
        printf("✗ 弱K值攻击测试失败\n");
    }
    printf("\n");
}

// 测试中本聪签名伪造
void test_satoshi_forge() {
    printf("测试中本聪签名伪造...\n");
    
    SATOSHI_FORGE_DATA forge_data;
    RAND_bytes(forge_data.satoshi_pubkey, 65);
    forge_data.satoshi_pubkey[0] = 0x04;
    
    const char *target_msg = "Test message for Satoshi forge";
    strcpy((char*)forge_data.target_message, target_msg);
    forge_data.message_len = strlen(target_msg);
    
    SM2_ATTACK_RESULT result;
    int attack_result = sm2_forge_satoshi_signature(&forge_data, &result);
    
    if (attack_result) {
        printf("✓ 中本聪签名伪造测试完成\n");
        printf("攻击成功: %s\n", result.success ? "是" : "否");
        printf("描述: %s\n", result.description);
    } else {
        printf("✗ 中本聪签名伪造测试失败\n");
    }
    printf("\n");
}

// 性能测试
void performance_test() {
    printf("性能测试...\n");
    
    int num_tests = 10;
    double total_sign_time = 0;
    double total_verify_time = 0;
    
    unsigned char private_key[32];
    unsigned char public_key[65];
    RAND_bytes(private_key, 32);
    RAND_bytes(public_key, 65);
    public_key[0] = 0x04;
    
    for (int i = 0; i < num_tests; i++) {
        char test_msg[100];
        sprintf(test_msg, "Performance test message %d", i);
        
        SM2_SIGNATURE signature;
        
        clock_t start = clock();
        sm2_sign_message((unsigned char*)test_msg, strlen(test_msg), private_key, &signature);
        clock_t end = clock();
        total_sign_time += ((double)(end - start)) / CLOCKS_PER_SEC;
        
        start = clock();
        sm2_verify_signature((unsigned char*)test_msg, strlen(test_msg), public_key, &signature);
        end = clock();
        total_verify_time += ((double)(end - start)) / CLOCKS_PER_SEC;
    }
    
    printf("平均签名时间: %.6f 秒\n", total_sign_time / num_tests);
    printf("平均验证时间: %.6f 秒\n", total_verify_time / num_tests);
    printf("\n");
}

int main() {
    printf("SM2签名攻击测试程序\n");
    printf("===================\n\n");
    
    // 运行所有测试
    test_basic_signature();
    test_k_reuse_attack();
    test_weak_k_attack();
    test_satoshi_forge();
    performance_test();
    
    printf("所有测试完成！\n");
    printf("\n重要提醒：\n");
    printf("1. 这些攻击演示仅用于教育目的\n");
    printf("2. 在实际应用中，请使用安全的实现\n");
    printf("3. 定期更新密码学库和最佳实践\n");
    
    return 0;
}
