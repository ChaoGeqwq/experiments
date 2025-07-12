#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "sm2_optimized.h"
#include "sm2_create_key_pair.h"

// 比较原始实现和优化实现的性能
int compare_implementations() {
    printf("=== SM2 实现对比测试 ===\n");
    
    int iterations = 50;
    struct timeval start, end;
    
    // 测试原始实现
    printf("\n测试原始实现 (%d 次迭代):\n", iterations);
    gettimeofday(&start, NULL);
    
    double total_time_original = 0;
    for (int i = 0; i < iterations; i++) {
        struct timeval iter_start, iter_end;
        gettimeofday(&iter_start, NULL);
        
        SM2_KEY_PAIR original_key_pair;
        if (sm2_create_key_pair(&original_key_pair) != 0) {
            printf("原始密钥生成失败\n");
            return -1;
        }
        
        gettimeofday(&iter_end, NULL);
        double iter_time = get_time_diff(iter_start, iter_end);
        total_time_original += iter_time;
        
        if ((i + 1) % 10 == 0) {
            printf("完成 %d/%d 次 (平均: %.3f ms)\n", 
                   i + 1, iterations, (total_time_original / (i + 1)) * 1000);
        }
    }
    
    gettimeofday(&end, NULL);
    double avg_time_original = total_time_original / iterations;
    
    // 测试优化实现
    printf("\n测试优化实现:\n");
    
    SM2_OPTIMIZATION_STRATEGY strategies[] = {
        SM2_STRATEGY_BASIC,
        SM2_STRATEGY_PRECOMPUTE,
        SM2_STRATEGY_MONTGOMERY,
        SM2_STRATEGY_WINDOWING
    };
    int strategy_count = sizeof(strategies) / sizeof(strategies[0]);
    
    double avg_times[4];
    
    for (int s = 0; s < strategy_count; s++) {
        printf("\n策略: %s (%d 次迭代)\n", sm2_strategy_name(strategies[s]), iterations);
        
        double total_time_optimized = 0;
        for (int i = 0; i < iterations; i++) {
            struct timeval iter_start, iter_end;
            gettimeofday(&iter_start, NULL);
            
            SM2_OPTIMIZED_KEY_PAIR optimized_key_pair;
            if (sm2_optimized_create_key_pair(&optimized_key_pair, strategies[s]) != 0) {
                printf("优化密钥生成失败\n");
                return -1;
            }
            
            gettimeofday(&iter_end, NULL);
            double iter_time = get_time_diff(iter_start, iter_end);
            total_time_optimized += iter_time;
            
            sm2_optimized_cleanup(&optimized_key_pair);
            
            if ((i + 1) % 10 == 0) {
                printf("完成 %d/%d 次 (平均: %.3f ms)\n", 
                       i + 1, iterations, (total_time_optimized / (i + 1)) * 1000);
            }
        }
        
        avg_times[s] = total_time_optimized / iterations;
    }
    
    // 打印对比结果
    printf("\n=== 性能对比结果 ===\n");
    printf("%-20s %-15s %-15s\n", "实现方式", "平均时间(ms)", "相对性能");
    printf("%-20s %-15s %-15s\n", "--------", "----------", "--------");
    
    printf("%-20s %-15.3f %-15s\n", "原始实现", avg_time_original * 1000, "基准");
    
    for (int s = 0; s < strategy_count; s++) {
        double speedup = avg_time_original / avg_times[s];
        printf("%-20s %-15.3f %-15.2fx\n", 
               sm2_strategy_name(strategies[s]), avg_times[s] * 1000, speedup);
    }
    
    return 0;
}

// 内存使用分析
void analyze_memory_footprint() {
    printf("\n=== 内存占用分析 ===\n");
    
    // 基础实现内存占用
    SM2_KEY_PAIR basic_key;
    printf("基础密钥对大小: %zu bytes\n", sizeof(basic_key));
    
    // 优化实现内存占用
    SM2_OPTIMIZED_KEY_PAIR opt_key;
    printf("优化密钥对大小: %zu bytes\n", sizeof(opt_key));
    
    // 预计算表内存占用估算
    printf("\n预计算表内存占用估算:\n");
    printf("- 窗口大小 2: ~256 bytes\n");
    printf("- 窗口大小 4: ~1KB\n");
    printf("- 窗口大小 6: ~4KB\n");
    printf("- 窗口大小 8: ~16KB\n");
    
    printf("\n内存效率分析:\n");
    printf("- 基础实现: 内存占用最少，适合资源受限环境\n");
    printf("- 预计算优化: 内存换时间，适合重复使用固定密钥\n");
    printf("- 蒙哥马利阶梯: 临时内存使用适中，抗侧信道攻击\n");
    printf("- 滑动窗口: 动态内存分配，适合批量不同密钥操作\n");
}

// CPU使用率测试
void cpu_utilization_test() {
    printf("\n=== CPU 使用率测试 ===\n");
    
    int test_duration = 5; // 秒
    printf("运行 %d 秒的密集计算测试...\n", test_duration);
    
    SM2_OPTIMIZATION_STRATEGY strategies[] = {
        SM2_STRATEGY_BASIC,
        SM2_STRATEGY_PRECOMPUTE,
        SM2_STRATEGY_MONTGOMERY,
        SM2_STRATEGY_WINDOWING
    };
    
    for (int s = 0; s < 4; s++) {
        printf("\n测试策略: %s\n", sm2_strategy_name(strategies[s]));
        
        time_t start_time = time(NULL);
        int operations = 0;
        
        while (time(NULL) - start_time < test_duration) {
            SM2_OPTIMIZED_KEY_PAIR key_pair;
            if (sm2_optimized_create_key_pair(&key_pair, strategies[s]) == 0) {
                operations++;
                sm2_optimized_cleanup(&key_pair);
            }
        }
        
        double ops_per_sec = (double)operations / test_duration;
        printf("完成 %d 次操作，平均 %.2f 操作/秒\n", operations, ops_per_sec);
    }
}

// 并发性能测试（简化版）
void concurrency_simulation() {
    printf("\n=== 并发性能模拟 ===\n");
    printf("模拟不同并发级别下的性能表现\n");
    
    int concurrent_levels[] = {1, 2, 4, 8, 16};
    int level_count = sizeof(concurrent_levels) / sizeof(concurrent_levels[0]);
    
    for (int l = 0; l < level_count; l++) {
        int concurrent = concurrent_levels[l];
        printf("\n模拟 %d 个并发连接:\n", concurrent);
        
        // 简单模拟：每个连接需要一次密钥生成
        struct timeval start;
        gettimeofday(&start, NULL);
        
        for (int i = 0; i < concurrent; i++) {
            SM2_OPTIMIZED_KEY_PAIR key_pair;
            sm2_optimized_create_key_pair(&key_pair, SM2_STRATEGY_BASIC);
            sm2_optimized_cleanup(&key_pair);
        }
        
        double total_time = end_timer(start);
        double avg_latency = total_time / concurrent;
        double throughput = concurrent / total_time;
        
        printf("总时间: %.3f 秒\n", total_time);
        printf("平均延迟: %.3f 秒\n", avg_latency);
        printf("吞吐量: %.2f 操作/秒\n", throughput);
    }
}

int main() {
    printf("=== SM2 椭圆曲线算法详细性能分析 ===\n");
    printf("基于OpenSSL %s\n", OpenSSL_version(OPENSSL_VERSION));
    printf("编译时间: %s %s\n", __DATE__, __TIME__);
    
    // 初始化OpenSSL
    OpenSSL_add_all_algorithms();
    
    // 运行各项测试
    if (compare_implementations() != 0) {
        printf("实现对比测试失败\n");
        return -1;
    }
    
    analyze_memory_footprint();
    cpu_utilization_test();
    concurrency_simulation();
    
    printf("\n=== 性能优化建议 ===\n");
    printf("1. 小规模应用: 使用基础实现或蒙哥马利阶梯\n");
    printf("2. 大规模应用: 考虑预计算优化或滑动窗口\n");
    printf("3. 安全敏感场景: 推荐蒙哥马利阶梯实现\n");
    printf("4. 内存受限环境: 使用基础实现\n");
    printf("5. 高并发场景: 考虑硬件加速或专用芯片\n");
    
    printf("\n详细性能分析完成!\n");
    return 0;
}
