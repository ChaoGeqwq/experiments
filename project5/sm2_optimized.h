#ifndef SM2_OPTIMIZED_H
#define SM2_OPTIMIZED_H

#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <time.h>
#include <sys/time.h>

// 性能测试结构
typedef struct {
    double key_generation_time;
    double encryption_time;
    double decryption_time;
    double signature_time;
    double verification_time;
    size_t operations_count;
} SM2_PERFORMANCE_STATS;

// 优化策略枚举
typedef enum {
    SM2_STRATEGY_BASIC,           // 基础实现
    SM2_STRATEGY_PRECOMPUTE,      // 预计算优化
    SM2_STRATEGY_MONTGOMERY,      // 蒙哥马利阶梯
    SM2_STRATEGY_WINDOWING,       // 窗口方法
    SM2_STRATEGY_MIXED_COORDINATES // 混合坐标系
} SM2_OPTIMIZATION_STRATEGY;

// 预计算表结构
typedef struct {
    EC_POINT **table;
    int table_size;
    int window_size;
} SM2_PRECOMPUTE_TABLE;

// 优化版本的密钥对结构
typedef struct {
    unsigned char pri_key[32];
    unsigned char pub_key[65];
    SM2_PRECOMPUTE_TABLE *precompute_table;
    SM2_OPTIMIZATION_STRATEGY strategy;
} SM2_OPTIMIZED_KEY_PAIR;

// 函数声明

// 基础功能
int sm2_optimized_create_key_pair(SM2_OPTIMIZED_KEY_PAIR *key_pair, 
                                  SM2_OPTIMIZATION_STRATEGY strategy);
int sm2_optimized_encrypt(const unsigned char *message, int message_len,
                         const unsigned char *pub_key,
                         unsigned char **ciphertext, int *ciphertext_len,
                         SM2_OPTIMIZATION_STRATEGY strategy);
int sm2_optimized_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                         const unsigned char *pri_key,
                         unsigned char **plaintext, int *plaintext_len,
                         SM2_OPTIMIZATION_STRATEGY strategy);

// 预计算优化
int sm2_precompute_table_create(SM2_PRECOMPUTE_TABLE **table, 
                               const EC_POINT *point, 
                               const EC_GROUP *group, 
                               int window_size);
void sm2_precompute_table_free(SM2_PRECOMPUTE_TABLE *table);
int sm2_point_mul_precompute(const EC_GROUP *group, 
                            EC_POINT *result,
                            const BIGNUM *scalar, 
                            const SM2_PRECOMPUTE_TABLE *table, 
                            BN_CTX *ctx);

// 蒙哥马利阶梯优化
int sm2_montgomery_ladder(const EC_GROUP *group, 
                         EC_POINT *result, 
                         const BIGNUM *scalar, 
                         const EC_POINT *point, 
                         BN_CTX *ctx);

// 窗口方法优化
int sm2_sliding_window_mul(const EC_GROUP *group, 
                          EC_POINT *result, 
                          const BIGNUM *scalar, 
                          const EC_POINT *point, 
                          int window_size, 
                          BN_CTX *ctx);

// 性能测试
int sm2_performance_test(SM2_OPTIMIZATION_STRATEGY strategy, 
                        int iterations, 
                        SM2_PERFORMANCE_STATS *stats);
void sm2_print_performance_comparison(SM2_PERFORMANCE_STATS *stats, 
                                     SM2_OPTIMIZATION_STRATEGY *strategies, 
                                     int strategy_count);

// 时间测量工具
double get_time_diff(struct timeval start, struct timeval end);
void start_timer(struct timeval *start);
double end_timer(struct timeval start);

// 内存和错误处理
void sm2_optimized_cleanup(SM2_OPTIMIZED_KEY_PAIR *key_pair);
const char* sm2_strategy_name(SM2_OPTIMIZATION_STRATEGY strategy);

#endif // SM2_OPTIMIZED_H
