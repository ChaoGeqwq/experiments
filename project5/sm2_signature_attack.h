#ifndef SM2_SIGNATURE_ATTACK_H
#define SM2_SIGNATURE_ATTACK_H

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/sha.h>
#include <time.h>
#include <sys/time.h>

// 攻击类型枚举
typedef enum {
    ATTACK_K_REUSE,              // k值重用攻击
    ATTACK_WEAK_K,               // 弱k值攻击
    ATTACK_NONCE_BIAS,           // 随机数偏置攻击
    ATTACK_SATOSHI_SIGNATURE,    // 中本聪签名伪造
    ATTACK_INVALID_CURVE         // 无效曲线攻击
} SM2_ATTACK_TYPE;

// 签名结构
typedef struct {
    unsigned char r[32];
    unsigned char s[32];
    unsigned char message_hash[32];
    unsigned char public_key[65];
} SM2_SIGNATURE;

// 攻击结果结构
typedef struct {
    int success;
    char description[256];
    double attack_time;
    unsigned char recovered_private_key[32];
    SM2_SIGNATURE forged_signature;
} SM2_ATTACK_RESULT;

// 中本聪签名伪造相关结构
typedef struct {
    unsigned char satoshi_pubkey[65];    // 中本聪的公钥
    unsigned char target_message[256];   // 要伪造签名的消息
    size_t message_len;
    SM2_SIGNATURE forged_sig;           // 伪造的签名
} SATOSHI_FORGE_DATA;

// 函数声明

// 基础签名功能
int sm2_sign_message(const unsigned char *message, size_t message_len,
                     const unsigned char *private_key,
                     SM2_SIGNATURE *signature);

int sm2_verify_signature(const unsigned char *message, size_t message_len,
                        const unsigned char *public_key,
                        const SM2_SIGNATURE *signature);

// k值重用攻击
int sm2_attack_k_reuse(const SM2_SIGNATURE *sig1, const SM2_SIGNATURE *sig2,
                       const unsigned char *msg1_hash, const unsigned char *msg2_hash,
                       SM2_ATTACK_RESULT *result);

// 弱k值攻击
int sm2_attack_weak_k(const SM2_SIGNATURE *signature,
                      const unsigned char *message_hash,
                      SM2_ATTACK_RESULT *result);

// 中本聪签名伪造攻击
int sm2_forge_satoshi_signature(const SATOSHI_FORGE_DATA *forge_data,
                               SM2_ATTACK_RESULT *result);

// 无效曲线攻击
int sm2_attack_invalid_curve(const unsigned char *public_key,
                            SM2_ATTACK_RESULT *result);

// 随机数偏置攻击
int sm2_attack_nonce_bias(const SM2_SIGNATURE *signatures, int sig_count,
                         const unsigned char *message_hashes,
                         SM2_ATTACK_RESULT *result);

// 辅助功能
int generate_weak_k(BIGNUM *k, const EC_GROUP *group);
int simulate_vulnerable_signature(const unsigned char *message, size_t message_len,
                                 const unsigned char *private_key,
                                 SM2_SIGNATURE *signature,
                                 SM2_ATTACK_TYPE vulnerability);

// 演示和测试
void demonstrate_k_reuse_attack(void);
void demonstrate_satoshi_forge_attack(void);
void demonstrate_weak_k_attack(void);
void demonstrate_invalid_curve_attack(void);

// 工具函数
void print_attack_result(const SM2_ATTACK_RESULT *result, SM2_ATTACK_TYPE attack_type);
void print_signature(const SM2_SIGNATURE *signature);
void print_hex(const unsigned char *data, size_t len, const char *label);

// 安全建议
void print_security_recommendations(void);

#endif // SM2_SIGNATURE_ATTACK_H
