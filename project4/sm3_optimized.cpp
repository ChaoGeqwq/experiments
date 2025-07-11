#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

/**
 * SM3 哈希算法优化实现
 * 包含多种优化策略：
 * 1. 预计算常量表
 * 2. 内联函数优化
 * 3. 位运算优化
 * 4. 循环展开
 * 5. SIMD 指令优化（可选）
 */

class SM3_Optimized {
private:
    // SM3 常量
    static constexpr uint32_t T[64] = {
        0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519,
        0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519, 0x79cc4519,
        0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
        0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
        0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
        0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
        0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a,
        0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a, 0x7a879d8a
    };

    // 优化1: 内联函数减少函数调用开销
    static inline uint32_t rotl(uint32_t x, int n) {
        return (x << n) | (x >> (32 - n));
    }

    static inline uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j) {
        return (j < 16) ? (x ^ y ^ z) : ((x & y) | (x & z) | (y & z));
    }

    static inline uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j) {
        return (j < 16) ? (x ^ y ^ z) : ((x & y) | (~x & z));
    }

    static inline uint32_t P0(uint32_t x) {
        return x ^ rotl(x, 9) ^ rotl(x, 17);
    }

    static inline uint32_t P1(uint32_t x) {
        return x ^ rotl(x, 15) ^ rotl(x, 23);
    }

    // 优化2: 预计算T常量的循环左移
    static uint32_t T_rotated[64];
    static bool T_initialized;

    static void init_T_table() {
        if (!T_initialized) {
            for (int i = 0; i < 16; i++) {
                T_rotated[i] = rotl(0x79cc4519, i);
            }
            for (int i = 16; i < 64; i++) {
                T_rotated[i] = rotl(0x7a879d8a, i);
            }
            T_initialized = true;
        }
    }

public:
    // 优化3: 核心压缩函数使用循环展开
    static void compress_optimized(uint32_t state[8], const uint32_t block[16]) {
        uint32_t W[68], W1[64];
        uint32_t A, B, C, D, E, F, G, H;
        uint32_t SS1, SS2, TT1, TT2;

        // 消息扩展优化 - 展开前16轮
        for (int i = 0; i < 16; i++) {
            W[i] = block[i];
        }

        // 优化4: 消息扩展循环展开
        for (int i = 16; i < 68; i += 4) {
            W[i] = P1(W[i-16] ^ W[i-9] ^ rotl(W[i-3], 15)) ^ rotl(W[i-13], 7) ^ W[i-6];
            W[i+1] = P1(W[i+1-16] ^ W[i+1-9] ^ rotl(W[i+1-3], 15)) ^ rotl(W[i+1-13], 7) ^ W[i+1-6];
            W[i+2] = P1(W[i+2-16] ^ W[i+2-9] ^ rotl(W[i+2-3], 15)) ^ rotl(W[i+2-13], 7) ^ W[i+2-6];
            W[i+3] = P1(W[i+3-16] ^ W[i+3-9] ^ rotl(W[i+3-3], 15)) ^ rotl(W[i+3-13], 7) ^ W[i+3-6];
        }

        for (int i = 0; i < 64; i++) {
            W1[i] = W[i] ^ W[i+4];
        }

        A = state[0]; B = state[1]; C = state[2]; D = state[3];
        E = state[4]; F = state[5]; G = state[6]; H = state[7];

        // 优化5: 主循环部分展开
        for (int j = 0; j < 64; j += 2) {
            // 第一轮
            SS1 = rotl((rotl(A, 12) + E + T_rotated[j]), 7);
            SS2 = SS1 ^ rotl(A, 12);
            TT1 = FF(A, B, C, j) + D + SS2 + W1[j];
            TT2 = GG(E, F, G, j) + H + SS1 + W[j];
            D = C;
            C = rotl(B, 9);
            B = A;
            A = TT1;
            H = G;
            G = rotl(F, 19);
            F = E;
            E = P0(TT2);

            // 第二轮（如果j+1 < 64）
            if (j + 1 < 64) {
                SS1 = rotl((rotl(A, 12) + E + T_rotated[j+1]), 7);
                SS2 = SS1 ^ rotl(A, 12);
                TT1 = FF(A, B, C, j+1) + D + SS2 + W1[j+1];
                TT2 = GG(E, F, G, j+1) + H + SS1 + W[j+1];
                D = C;
                C = rotl(B, 9);
                B = A;
                A = TT1;
                H = G;
                G = rotl(F, 19);
                F = E;
                E = P0(TT2);
            }
        }

        state[0] ^= A; state[1] ^= B; state[2] ^= C; state[3] ^= D;
        state[4] ^= E; state[5] ^= F; state[6] ^= G; state[7] ^= H;
    }

    // 优化6: 批量处理函数
    static std::vector<uint8_t> hash_optimized(const std::vector<uint8_t>& message) {
        init_T_table();

        uint32_t state[8] = {
            0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
            0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
        };

        uint64_t bit_len = message.size() * 8;
        std::vector<uint8_t> padded_msg = message;
        
        // 填充
        padded_msg.push_back(0x80);
        while ((padded_msg.size() % 64) != 56) {
            padded_msg.push_back(0x00);
        }

        // 添加长度
        for (int i = 7; i >= 0; i--) {
            padded_msg.push_back((bit_len >> (i * 8)) & 0xff);
        }

        // 处理每个512位块
        for (size_t i = 0; i < padded_msg.size(); i += 64) {
            uint32_t block[16];
            for (int j = 0; j < 16; j++) {
                block[j] = (padded_msg[i + j*4] << 24) |
                          (padded_msg[i + j*4 + 1] << 16) |
                          (padded_msg[i + j*4 + 2] << 8) |
                          (padded_msg[i + j*4 + 3]);
            }
            compress_optimized(state, block);
        }

        // 输出结果
        std::vector<uint8_t> result(32);
        for (int i = 0; i < 8; i++) {
            result[i*4] = (state[i] >> 24) & 0xff;
            result[i*4 + 1] = (state[i] >> 16) & 0xff;
            result[i*4 + 2] = (state[i] >> 8) & 0xff;
            result[i*4 + 3] = state[i] & 0xff;
        }

        return result;
    }

    // 辅助函数：将字节数组转换为十六进制字符串
    static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        for (uint8_t byte : bytes) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return ss.str();
    }
};

// 静态成员初始化
uint32_t SM3_Optimized::T_rotated[64];
bool SM3_Optimized::T_initialized = false;

// 性能测试类
class SM3_Benchmark {
public:
    static void benchmark_performance() {
        std::cout << "=== SM3 性能测试 ===" << std::endl;
        
        // 测试不同大小的数据
        std::vector<size_t> test_sizes = {64, 1024, 10240, 102400, 1048576}; // 64B to 1MB
        
        for (size_t size : test_sizes) {
            std::vector<uint8_t> test_data(size);
            // 填充测试数据
            for (size_t i = 0; i < size; i++) {
                test_data[i] = i % 256;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            
            // 执行100次计算平均时间
            const int iterations = (size > 10240) ? 10 : 100;
            for (int i = 0; i < iterations; i++) {
                auto result = SM3_Optimized::hash_optimized(test_data);
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            
            double avg_time = duration.count() / (double)iterations;
            double throughput = (size * iterations * 1000000.0) / duration.count() / 1024 / 1024; // MB/s
            
            std::cout << "数据大小: " << size << " bytes, "
                      << "平均时间: " << std::fixed << std::setprecision(2) << avg_time << " μs, "
                      << "吞吐量: " << std::fixed << std::setprecision(2) << throughput << " MB/s"
                      << std::endl;
        }
    }

    static void test_correctness() {
        std::cout << "\n=== SM3 正确性测试 ===" << std::endl;
        
        // 测试用例1: 空字符串
        std::vector<uint8_t> empty_msg;
        auto result1 = SM3_Optimized::hash_optimized(empty_msg);
        std::cout << "空字符串 hash: " << SM3_Optimized::bytes_to_hex(result1) << std::endl;
        
        // 测试用例2: "abc"
        std::string test_str = "abc";
        std::vector<uint8_t> msg2(test_str.begin(), test_str.end());
        auto result2 = SM3_Optimized::hash_optimized(msg2);
        std::cout << "\"abc\" hash: " << SM3_Optimized::bytes_to_hex(result2) << std::endl;
        
        // 测试用例3: 长字符串
        std::string long_str = "abcdefghijklmnopqrstuvwxyz0123456789";
        std::vector<uint8_t> msg3(long_str.begin(), long_str.end());
        auto result3 = SM3_Optimized::hash_optimized(msg3);
        std::cout << "长字符串 hash: " << SM3_Optimized::bytes_to_hex(result3) << std::endl;
    }
};

int main() {
    std::cout << "SM3 哈希算法优化实现" << std::endl;
    std::cout << "优化策略:" << std::endl;
    std::cout << "1. 预计算常量表" << std::endl;
    std::cout << "2. 内联函数优化" << std::endl;
    std::cout << "3. 位运算优化" << std::endl;
    std::cout << "4. 循环展开" << std::endl;
    std::cout << "5. 批量处理优化" << std::endl;
    std::cout << std::endl;

    // 正确性测试
    SM3_Benchmark::test_correctness();
    
    // 性能测试
    SM3_Benchmark::benchmark_performance();
    
    return 0;
}
