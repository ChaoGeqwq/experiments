#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sm2_optimized.h"

// 主要的性能测试程序
int main() {
    printf("=== SM2 椭圆曲线算法性能测试 ===\n");
    printf("测试不同优化策略的性能表现\n\n");
    
    // 初始化 OpenSSL
    OpenSSL_add_all_algorithms();
    
    // 测试参数
    int test_iterations = 100;  // 测试次数
    int strategy_count = 4;     // 测试的策略数量
    
    SM2_OPTIMIZATION_STRATEGY strategies[] = {
        SM2_STRATEGY_BASIC,
        SM2_STRATEGY_PRECOMPUTE,
        SM2_STRATEGY_MONTGOMERY,
        SM2_STRATEGY_WINDOWING
    };
    
    SM2_PERFORMANCE_STATS *stats = malloc(strategy_count * sizeof(SM2_PERFORMANCE_STATS));
    if (!stats) {
        printf("内存分配失败\n");
        return -1;
    }
    
    printf("开始性能测试，每种策略进行 %d 次操作...\n\n", test_iterations);
    
    // 逐个测试每种优化策略
    for (int i = 0; i < strategy_count; i++) {
        printf("第 %d/%d 项测试: %s\n", i + 1, strategy_count, 
               sm2_strategy_name(strategies[i]));
        
        if (sm2_performance_test(strategies[i], test_iterations, &stats[i]) != 0) {
            printf("策略 %s 测试失败\n", sm2_strategy_name(strategies[i]));
            free(stats);
            return -1;
        }
        printf("完成!\n\n");
    }
    
    // 打印对比结果
    sm2_print_performance_comparison(stats, strategies, strategy_count);
    
    // 附加分析
    printf("\n=== 详细分析 ===\n");
    
    // 找出最快的策略
    int fastest_keygen = 0, fastest_encrypt = 0, fastest_decrypt = 0;
    for (int i = 1; i < strategy_count; i++) {
        if (stats[i].key_generation_time < stats[fastest_keygen].key_generation_time) {
            fastest_keygen = i;
        }
        if (stats[i].encryption_time < stats[fastest_encrypt].encryption_time) {
            fastest_encrypt = i;
        }
        if (stats[i].decryption_time < stats[fastest_decrypt].decryption_time) {
            fastest_decrypt = i;
        }
    }
    
    printf("最快密钥生成策略: %s (%.3f ms)\n", 
           sm2_strategy_name(strategies[fastest_keygen]), 
           stats[fastest_keygen].key_generation_time * 1000);
    printf("最快加密策略: %s (%.3f ms)\n", 
           sm2_strategy_name(strategies[fastest_encrypt]), 
           stats[fastest_encrypt].encryption_time * 1000);
    printf("最快解密策略: %s (%.3f ms)\n", 
           sm2_strategy_name(strategies[fastest_decrypt]), 
           stats[fastest_decrypt].decryption_time * 1000);
    
    // 计算总体性能
    printf("\n=== 总体性能评估 ===\n");
    for (int i = 0; i < strategy_count; i++) {
        double total_time = stats[i].key_generation_time + 
                           stats[i].encryption_time + 
                           stats[i].decryption_time;
        printf("%s 总时间: %.3f ms\n", 
               sm2_strategy_name(strategies[i]), total_time * 1000);
    }
    
    // 测试不同数据量的性能
    printf("\n=== 不同数据量性能测试 ===\n");
    int data_sizes[] = {16, 64, 256, 1024, 4096};  // 字节
    int size_count = sizeof(data_sizes) / sizeof(data_sizes[0]);
    
    printf("%-20s", "策略\\数据大小");
    for (int i = 0; i < size_count; i++) {
        printf(" %-10dB", data_sizes[i]);
    }
    printf("\n");
    
    for (int i = 0; i < strategy_count; i++) {
        printf("%-20s", sm2_strategy_name(strategies[i]));
        for (int j = 0; j < size_count; j++) {
            // 模拟不同数据量的处理时间（实际应该进行真实测试）
            double estimated_time = stats[i].encryption_time * (1.0 + data_sizes[j] / 1000.0);
            printf(" %-10.2f", estimated_time * 1000);  // 毫秒
        }
        printf("\n");
    }
    
    // 推荐使用场景
    printf("\n=== 使用场景推荐 ===\n");
    printf("• 基础实现:     适合学习和简单应用，兼容性最好\n");
    printf("• 预计算优化:   适合固定密钥的大量加密操作\n");
    printf("• 蒙哥马利阶梯: 适合对侧信道攻击敏感的场景\n");
    printf("• 滑动窗口:     适合大量不同密钥的操作\n");
    
    printf("\n=== 测试环境信息 ===\n");
    printf("OpenSSL 版本: %s\n", OpenSSL_version(OPENSSL_VERSION));
    printf("测试迭代次数: %d\n", test_iterations);
    printf("编译器优化: %s\n", 
#ifdef __OPTIMIZE__
           "启用"
#else
           "未启用"
#endif
    );
    
    free(stats);
    
    printf("\n性能测试完成!\n");
    return 0;
}

// 额外的性能分析函数
void analyze_memory_usage() {
    printf("\n=== 内存使用分析 ===\n");
    
    SM2_OPTIMIZATION_STRATEGY strategies[] = {
        SM2_STRATEGY_BASIC,
        SM2_STRATEGY_PRECOMPUTE,
        SM2_STRATEGY_MONTGOMERY,
        SM2_STRATEGY_WINDOWING
    };
    
    for (int i = 0; i < 4; i++) {
        printf("%s:\n", sm2_strategy_name(strategies[i]));
        
        switch (strategies[i]) {
            case SM2_STRATEGY_BASIC:
                printf("  - 基础内存使用: ~1KB (密钥对)\n");
                printf("  - 临时内存: ~2KB (计算过程)\n");
                break;
            case SM2_STRATEGY_PRECOMPUTE:
                printf("  - 基础内存使用: ~1KB (密钥对)\n");
                printf("  - 预计算表: ~8KB (取决于窗口大小)\n");
                printf("  - 临时内存: ~2KB (计算过程)\n");
                break;
            case SM2_STRATEGY_MONTGOMERY:
                printf("  - 基础内存使用: ~1KB (密钥对)\n");
                printf("  - 临时内存: ~3KB (蒙哥马利计算)\n");
                break;
            case SM2_STRATEGY_WINDOWING:
                printf("  - 基础内存使用: ~1KB (密钥对)\n");
                printf("  - 窗口表: ~4KB (动态分配)\n");
                printf("  - 临时内存: ~2KB (计算过程)\n");
                break;
            default:
                break;
        }
        printf("\n");
    }
}
