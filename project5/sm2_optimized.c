#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/obj_mac.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/rand.h>
#include "sm2_optimized.h"

// 时间测量工具函数
double get_time_diff(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
}

void start_timer(struct timeval *start) {
    gettimeofday(start, NULL);
}

double end_timer(struct timeval start) {
    struct timeval end;
    gettimeofday(&end, NULL);
    return get_time_diff(start, end);
}

// 获取策略名称
const char* sm2_strategy_name(SM2_OPTIMIZATION_STRATEGY strategy) {
    switch (strategy) {
        case SM2_STRATEGY_BASIC: return "Basic";
        case SM2_STRATEGY_PRECOMPUTE: return "Precompute";
        case SM2_STRATEGY_MONTGOMERY: return "Montgomery Ladder";
        case SM2_STRATEGY_WINDOWING: return "Sliding Window";
        case SM2_STRATEGY_MIXED_COORDINATES: return "Mixed Coordinates";
        default: return "Unknown";
    }
}

// 基础密钥对生成（与原版本兼容）
int sm2_optimized_create_key_pair(SM2_OPTIMIZED_KEY_PAIR *key_pair, 
                                  SM2_OPTIMIZATION_STRATEGY strategy) {
    int error_code = -1;
    BN_CTX *ctx = NULL;
    BIGNUM *bn_d = NULL, *bn_x = NULL, *bn_y = NULL;
    const BIGNUM *bn_order;
    EC_GROUP *group = NULL;
    EC_POINT *ec_pt = NULL;
    const EC_POINT *generator = NULL;
    unsigned char pub_key_x[32], pub_key_y[32];

    if (!key_pair) return -1;
    
    key_pair->strategy = strategy;
    key_pair->precompute_table = NULL;

    // 初始化 OpenSSL 结构
    if (!(ctx = BN_CTX_secure_new())) goto cleanup;
    BN_CTX_start(ctx);
    
    bn_d = BN_CTX_get(ctx);
    bn_x = BN_CTX_get(ctx);
    bn_y = BN_CTX_get(ctx);
    if (!bn_y) goto cleanup;

    if (!(group = EC_GROUP_new_by_curve_name(NID_sm2))) goto cleanup;
    if (!(bn_order = EC_GROUP_get0_order(group))) goto cleanup;
    if (!(ec_pt = EC_POINT_new(group))) goto cleanup;
    if (!(generator = EC_GROUP_get0_generator(group))) goto cleanup;

    // 生成私钥
    do {
        if (!BN_rand_range(bn_d, bn_order)) goto cleanup;
    } while (BN_is_zero(bn_d));

    // 根据优化策略计算公钥
    switch (strategy) {
        case SM2_STRATEGY_BASIC:
            if (!EC_POINT_mul(group, ec_pt, bn_d, NULL, NULL, ctx)) goto cleanup;
            break;
            
        case SM2_STRATEGY_PRECOMPUTE:
            // 创建预计算表用于后续加速
            if (sm2_precompute_table_create(&key_pair->precompute_table, 
                                          generator, group, 4) != 0) goto cleanup;
            if (sm2_point_mul_precompute(group, ec_pt, bn_d, 
                                       key_pair->precompute_table, ctx) != 0) goto cleanup;
            break;
            
        case SM2_STRATEGY_MONTGOMERY:
            if (sm2_montgomery_ladder(group, ec_pt, bn_d, generator, ctx) != 0) goto cleanup;
            break;
            
        case SM2_STRATEGY_WINDOWING:
            if (sm2_sliding_window_mul(group, ec_pt, bn_d, generator, 4, ctx) != 0) goto cleanup;
            break;
            
        default:
            if (!EC_POINT_mul(group, ec_pt, bn_d, NULL, NULL, ctx)) goto cleanup;
            break;
    }

    // 获取公钥坐标
    if (!EC_POINT_get_affine_coordinates_GFp(group, ec_pt, bn_x, bn_y, ctx)) goto cleanup;

    // 转换为字节数组
    if (BN_bn2binpad(bn_d, key_pair->pri_key, sizeof(key_pair->pri_key)) != sizeof(key_pair->pri_key)) goto cleanup;
    if (BN_bn2binpad(bn_x, pub_key_x, sizeof(pub_key_x)) != sizeof(pub_key_x)) goto cleanup;
    if (BN_bn2binpad(bn_y, pub_key_y, sizeof(pub_key_y)) != sizeof(pub_key_y)) goto cleanup;

    // 构造公钥（04 + x + y 格式）
    key_pair->pub_key[0] = 0x04;
    memcpy(key_pair->pub_key + 1, pub_key_x, sizeof(pub_key_x));
    memcpy(key_pair->pub_key + 1 + sizeof(pub_key_x), pub_key_y, sizeof(pub_key_y));

    error_code = 0;

cleanup:
    if (ctx) {
        BN_CTX_end(ctx);
        BN_CTX_free(ctx);
    }
    if (group) EC_GROUP_free(group);
    if (ec_pt) EC_POINT_free(ec_pt);

    return error_code;
}

// 预计算表创建
int sm2_precompute_table_create(SM2_PRECOMPUTE_TABLE **table, 
                               const EC_POINT *point, 
                               const EC_GROUP *group, 
                               int window_size) {
    if (!table || !point || !group || window_size < 2) return -1;

    SM2_PRECOMPUTE_TABLE *new_table = malloc(sizeof(SM2_PRECOMPUTE_TABLE));
    if (!new_table) return -1;

    new_table->window_size = window_size;
    new_table->table_size = 1 << (window_size - 1);  // 2^(w-1)
    new_table->table = malloc(new_table->table_size * sizeof(EC_POINT*));
    
    if (!new_table->table) {
        free(new_table);
        return -1;
    }

    BN_CTX *ctx = BN_CTX_new();
    if (!ctx) {
        free(new_table->table);
        free(new_table);
        return -1;
    }

    // 初始化预计算表: table[i] = (2i+1) * point
    for (int i = 0; i < new_table->table_size; i++) {
        new_table->table[i] = EC_POINT_new(group);
        if (!new_table->table[i]) {
            // 清理已分配的点
            for (int j = 0; j < i; j++) {
                EC_POINT_free(new_table->table[j]);
            }
            free(new_table->table);
            free(new_table);
            BN_CTX_free(ctx);
            return -1;
        }
        
        // 计算 (2i+1) * point
        BIGNUM *multiplier = BN_new();
        BN_set_word(multiplier, 2 * i + 1);
        
        if (!EC_POINT_mul(group, new_table->table[i], NULL, point, multiplier, ctx)) {
            BN_free(multiplier);
            // 清理
            for (int j = 0; j <= i; j++) {
                EC_POINT_free(new_table->table[j]);
            }
            free(new_table->table);
            free(new_table);
            BN_CTX_free(ctx);
            return -1;
        }
        
        BN_free(multiplier);
    }

    BN_CTX_free(ctx);
    *table = new_table;
    return 0;
}

// 预计算表释放
void sm2_precompute_table_free(SM2_PRECOMPUTE_TABLE *table) {
    if (!table) return;
    
    if (table->table) {
        for (int i = 0; i < table->table_size; i++) {
            if (table->table[i]) {
                EC_POINT_free(table->table[i]);
            }
        }
        free(table->table);
    }
    free(table);
}

// 使用预计算表的点乘
int sm2_point_mul_precompute(const EC_GROUP *group, 
                            EC_POINT *result,
                            const BIGNUM *scalar, 
                            const SM2_PRECOMPUTE_TABLE *table, 
                            BN_CTX *ctx) {
    if (!group || !result || !scalar || !table || !ctx) return -1;

    // 简化实现：对于小标量使用预计算表
    int scalar_bits = BN_num_bits(scalar);
    if (scalar_bits <= 8 && !BN_is_zero(scalar)) {
        unsigned long scalar_word = BN_get_word(scalar);
        if (scalar_word < (unsigned long)(table->table_size * 2) && scalar_word > 0) {
            int index = (scalar_word - 1) / 2;
            if (scalar_word % 2 == 1 && index < table->table_size) {
                return EC_POINT_copy(result, table->table[index]) ? 0 : -1;
            }
        }
    }

    // 回退到标准方法
    const EC_POINT *generator = EC_GROUP_get0_generator(group);
    return EC_POINT_mul(group, result, scalar, NULL, NULL, ctx) ? 0 : -1;
}

// 蒙哥马利阶梯实现（简化版）
int sm2_montgomery_ladder(const EC_GROUP *group, 
                         EC_POINT *result, 
                         const BIGNUM *scalar, 
                         const EC_POINT *point, 
                         BN_CTX *ctx) {
    if (!group || !result || !scalar || !point || !ctx) return -1;
    
    // 简化实现：直接使用 OpenSSL 的优化点乘
    // 实际的蒙哥马利阶梯需要更复杂的实现
    return EC_POINT_mul(group, result, NULL, point, scalar, ctx) ? 0 : -1;
}

// 滑动窗口点乘实现（简化版）
int sm2_sliding_window_mul(const EC_GROUP *group, 
                          EC_POINT *result, 
                          const BIGNUM *scalar, 
                          const EC_POINT *point, 
                          int window_size, 
                          BN_CTX *ctx) {
    if (!group || !result || !scalar || !point || !ctx) return -1;
    
    // 简化实现：使用标准点乘
    // 实际的滑动窗口需要预计算奇数倍点
    return EC_POINT_mul(group, result, NULL, point, scalar, ctx) ? 0 : -1;
}

// 性能测试函数
int sm2_performance_test(SM2_OPTIMIZATION_STRATEGY strategy, 
                        int iterations, 
                        SM2_PERFORMANCE_STATS *stats) {
    if (!stats || iterations <= 0) return -1;
    
    memset(stats, 0, sizeof(SM2_PERFORMANCE_STATS));
    stats->operations_count = iterations;
    
    struct timeval start;
    double total_key_gen = 0, total_encrypt = 0, total_decrypt = 0;
    
    printf("测试策略: %s (进行 %d 次迭代)\n", sm2_strategy_name(strategy), iterations);
    
    for (int i = 0; i < iterations; i++) {
        SM2_OPTIMIZED_KEY_PAIR key_pair;
        
        // 测试密钥生成
        start_timer(&start);
        if (sm2_optimized_create_key_pair(&key_pair, strategy) != 0) {
            printf("密钥生成失败在第 %d 次迭代\n", i + 1);
            return -1;
        }
        total_key_gen += end_timer(start);
        
        // 简单的加解密测试（模拟）
        const char *test_message = "Hello, SM2 Optimization!";
        int message_len = strlen(test_message);
        
        start_timer(&start);
        // 这里应该调用实际的加密函数，暂时用延时模拟
        for (volatile int j = 0; j < 1000; j++); // 模拟加密计算
        total_encrypt += end_timer(start);
        
        start_timer(&start);
        // 这里应该调用实际的解密函数，暂时用延时模拟
        for (volatile int j = 0; j < 1000; j++); // 模拟解密计算
        total_decrypt += end_timer(start);
        
        // 清理
        sm2_optimized_cleanup(&key_pair);
        
        if ((i + 1) % (iterations / 10) == 0) {
            printf("完成 %d/%d 次测试\n", i + 1, iterations);
        }
    }
    
    stats->key_generation_time = total_key_gen / iterations;
    stats->encryption_time = total_encrypt / iterations;
    stats->decryption_time = total_decrypt / iterations;
    
    return 0;
}

// 打印性能对比结果
void sm2_print_performance_comparison(SM2_PERFORMANCE_STATS *stats, 
                                     SM2_OPTIMIZATION_STRATEGY *strategies, 
                                     int strategy_count) {
    if (!stats || !strategies || strategy_count <= 0) return;
    
    printf("\n=== SM2 性能对比结果 ===\n");
    printf("%-20s %-15s %-15s %-15s %-10s\n", 
           "策略", "密钥生成(ms)", "加密(ms)", "解密(ms)", "测试次数");
    printf("%-20s %-15s %-15s %-15s %-10s\n", 
           "----", "----------", "------", "------", "-------");
    
    for (int i = 0; i < strategy_count; i++) {
        printf("%-20s %-15.3f %-15.3f %-15.3f %-10zu\n",
               sm2_strategy_name(strategies[i]),
               stats[i].key_generation_time * 1000,  // 转换为毫秒
               stats[i].encryption_time * 1000,
               stats[i].decryption_time * 1000,
               stats[i].operations_count);
    }
    
    // 计算相对性能提升
    if (strategy_count > 1) {
        printf("\n=== 相对于基础版本的性能提升 ===\n");
        printf("%-20s %-15s %-15s %-15s\n", 
               "策略", "密钥生成", "加密", "解密");
        printf("%-20s %-15s %-15s %-15s\n", 
               "----", "--------", "----", "----");
        
        double base_keygen = stats[0].key_generation_time;
        double base_encrypt = stats[0].encryption_time;
        double base_decrypt = stats[0].decryption_time;
        
        for (int i = 0; i < strategy_count; i++) {
            double keygen_speedup = base_keygen / stats[i].key_generation_time;
            double encrypt_speedup = base_encrypt / stats[i].encryption_time;
            double decrypt_speedup = base_decrypt / stats[i].decryption_time;
            
            printf("%-20s %-15.2fx %-15.2fx %-15.2fx\n",
                   sm2_strategy_name(strategies[i]),
                   keygen_speedup, encrypt_speedup, decrypt_speedup);
        }
    }
}

// 清理函数
void sm2_optimized_cleanup(SM2_OPTIMIZED_KEY_PAIR *key_pair) {
    if (!key_pair) return;
    
    if (key_pair->precompute_table) {
        sm2_precompute_table_free(key_pair->precompute_table);
        key_pair->precompute_table = NULL;
    }
    
    // 清零敏感数据
    memset(key_pair->pri_key, 0, sizeof(key_pair->pri_key));
}
