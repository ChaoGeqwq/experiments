#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <chrono>

using namespace std;

// 常量初始化
const uint32_t IV[8] = {
    0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
    0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
};

const uint32_t T[64] = {
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A
};

// 循环左移
uint32_t rotateLeft(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// 布尔函数FF
uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j) {
    return (j < 16) ? (x ^ y ^ z) : ((x & y) | (x & z) | (y & z));
}

// 布尔函数GG
uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j) {
    return (j < 16) ? (x ^ y ^ z) : ((x & y) | (~x & z));
}

// 压缩函数P0
uint32_t P0(uint32_t x) {
    return x ^ rotateLeft(x, 9) ^ rotateLeft(x, 17);
}

// 压缩函数P1
uint32_t P1(uint32_t x) {
    return x ^ rotateLeft(x, 15) ^ rotateLeft(x, 23);
}

// 填充消息
vector<uint8_t> padding(const vector<uint8_t>& message) {
    vector<uint8_t> paddedMessage = message;
    size_t originalBitLength = message.size() * 8;

    // 添加一个1位
    paddedMessage.push_back(0x80);

    // 填充0直到长度满足条件
    while ((paddedMessage.size() * 8) % 512 != 448) {
        paddedMessage.push_back(0x00);
    }

    // 添加原始消息长度
    for (int i = 7; i >= 0; --i) {
        paddedMessage.push_back((originalBitLength >> (i * 8)) & 0xFF);
    }

    return paddedMessage;
}

// 消息扩展
vector<uint32_t> messageExpansion(const vector<uint8_t>& block, bool verbose = false) {
    vector<uint32_t> W(68);
    vector<uint32_t> W1(64);

    // 将消息分组为16个字
    for (int i = 0; i < 16; ++i) {
        W[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) |
               (block[i * 4 + 2] << 8) | block[i * 4 + 3];
    }

    // 扩展消息
    for (int i = 16; i < 68; ++i) {
        W[i] = P1(W[i - 16] ^ W[i - 9] ^ rotateLeft(W[i - 3], 15)) ^
               rotateLeft(W[i - 13], 7) ^ W[i - 6];
    }

    // 计算W1
    for (int i = 0; i < 64; ++i) {
        W1[i] = W[i] ^ W[i + 4];
    }

    if (verbose) {
        // 打印扩展后的消息
        cout << "扩展后的消息W: ";
        for (int i = 0; i < 68; ++i) {
            cout << hex << setw(8) << setfill('0') << W[i] << " ";
        }
        cout << endl;

        cout << "扩展后的消息W1: ";
        for (int i = 0; i < 64; ++i) {
            cout << hex << setw(8) << setfill('0') << W1[i] << " ";
        }
        cout << endl;
    }

    return W;
}

// 压缩函数
void CF(vector<uint32_t>& V, const vector<uint32_t>& W, bool verbose = false) {
    uint32_t A = V[0], B = V[1], C = V[2], D = V[3];
    uint32_t E = V[4], F = V[5], G = V[6], H = V[7];

    vector<uint32_t> W1(64);
    // 计算W1
    for (int i = 0; i < 64; ++i) {
        W1[i] = W[i] ^ W[i + 4];
    }

    for (int j = 0; j < 64; ++j) {
        uint32_t SS1 = rotateLeft((rotateLeft(A, 12) + E + rotateLeft(T[j], j % 32)), 7);
        uint32_t SS2 = SS1 ^ rotateLeft(A, 12);
        uint32_t TT1 = FF(A, B, C, j) + D + SS2 + W1[j];
        uint32_t TT2 = GG(E, F, G, j) + H + SS1 + W[j];
        D = C;
        C = rotateLeft(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotateLeft(F, 19);
        F = E;
        E = P0(TT2);

        if (verbose) {
            // 输出迭代压缩的中间值
            cout << "迭代压缩中间值 j=" << dec << j << ": ";
            cout << hex << setw(8) << setfill('0') << A << " "
                 << hex << setw(8) << setfill('0') << B << " "
                 << hex << setw(8) << setfill('0') << C << " "
                 << hex << setw(8) << setfill('0') << D << " "
                 << hex << setw(8) << setfill('0') << E << " "
                 << hex << setw(8) << setfill('0') << F << " "
                 << hex << setw(8) << setfill('0') << G << " "
                 << hex << setw(8) << setfill('0') << H << endl;
        }
    }

    V[0] ^= A;
    V[1] ^= B;
    V[2] ^= C;
    V[3] ^= D;
    V[4] ^= E;
    V[5] ^= F;
    V[6] ^= G;
    V[7] ^= H;
}

// SM3哈希函数
string SM3(const vector<uint8_t>& message, bool verbose = false) {
    vector<uint8_t> paddedMessage = padding(message);

    if (verbose) {
        // 打印填充后的消息
        cout << "填充后的消息: ";
        for (uint8_t byte : paddedMessage) {
            cout << hex << setw(2) << setfill('0') << (int)byte;
        }
        cout << endl;
    }

    vector<uint32_t> V(IV, IV + 8);

    // 分组处理
    for (size_t i = 0; i < paddedMessage.size(); i += 64) {
        vector<uint8_t> block(paddedMessage.begin() + i, paddedMessage.begin() + i + 64);
        vector<uint32_t> W = messageExpansion(block, verbose);
        CF(V, W, verbose);
    }

    // 输出最终的杂凑值
    stringstream ss;
    for (uint32_t v : V) {
        ss << hex << setw(8) << setfill('0') << v;
    }
    return ss.str();
}

// 辅助函数：将十六进制字符串转换为字节向量
vector<uint8_t> hex_to_bytes(const string& hex) {
    vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

// 性能测试类
class SM3_BasicBenchmark {
public:
    static void benchmark_performance() {
        cout << "\n=== SM3 基础版本性能测试 ===" << endl;
        
        // 测试不同大小的数据
        vector<size_t> test_sizes = {64, 1024, 10240, 102400, 1048576}; // 64B to 1MB
        
        for (size_t size : test_sizes) {
            vector<uint8_t> test_data(size);
            // 填充测试数据
            for (size_t i = 0; i < size; i++) {
                test_data[i] = i % 256;
            }
            
            auto start = chrono::high_resolution_clock::now();
            
            // 执行多次计算平均时间
            const int iterations = (size > 10240) ? 10 : 100;
            string last_result;
            for (int i = 0; i < iterations; i++) {
                last_result = SM3(test_data, false);
            }
            
            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(end - start);
            
            double avg_time = duration.count() / (double)iterations;
            double throughput = (size * iterations * 1000000.0) / duration.count() / 1024 / 1024; // MB/s
            
            cout << "数据大小: " << size << " bytes, "
                 << "平均时间: " << fixed << setprecision(2) << avg_time << " μs, "
                 << "吞吐量: " << fixed << setprecision(2) << throughput << " MB/s";
            
            // 对于较小的数据，显示结果的前16个字符
            if (size <= 1024) {
                cout << ", 哈希值: " << last_result.substr(0, 16) << "...";
            }
            cout << endl;
        }
    }

    static void test_correctness() {
        cout << "\n=== SM3 正确性测试 ===" << endl;
        
        // 测试用例1: 空字符串
        vector<uint8_t> empty_msg;
        string result1 = SM3(empty_msg);
        cout << "空字符串 hash: " << result1 << endl;
        
        // 测试用例2: "abc"
        string test_str = "abc";
        vector<uint8_t> msg2(test_str.begin(), test_str.end());
        string result2 = SM3(msg2);
        cout << "\"abc\" hash: " << result2 << endl;
        
        // 测试用例3: 长字符串
        string long_str = "abcdefghijklmnopqrstuvwxyz0123456789";
        vector<uint8_t> msg3(long_str.begin(), long_str.end());
        string result3 = SM3(msg3);
        cout << "长字符串 hash: " << result3 << endl;
        
        // 测试用例4: 重复字符
        string repeat_str(1000, 'a');
        vector<uint8_t> msg4(repeat_str.begin(), repeat_str.end());
        string result4 = SM3(msg4);
        cout << "1000个'a' hash: " << result4 << endl;
    }

    static void memory_usage_test() {
        cout << "\n=== SM3 内存使用测试 ===" << endl;
        
        // 测试大数据块的内存使用
        vector<size_t> memory_test_sizes = {1024*1024, 10*1024*1024}; // 1MB, 10MB
        
        for (size_t size : memory_test_sizes) {
            cout << "测试 " << size / (1024*1024) << "MB 数据块..." << endl;
            
            auto start = chrono::high_resolution_clock::now();
            
            // 创建测试数据
            vector<uint8_t> large_data(size);
            for (size_t i = 0; i < size; i++) {
                large_data[i] = (i * 7) % 256; // 使用简单的伪随机模式
            }
            
            auto data_gen_end = chrono::high_resolution_clock::now();
            
            // 计算哈希
            string result = SM3(large_data);
            
            auto end = chrono::high_resolution_clock::now();
            
            auto data_gen_time = chrono::duration_cast<chrono::milliseconds>(data_gen_end - start);
            auto hash_time = chrono::duration_cast<chrono::milliseconds>(end - data_gen_end);
            auto total_time = chrono::duration_cast<chrono::milliseconds>(end - start);
            
            double throughput = (size * 1000.0) / hash_time.count() / 1024 / 1024; // MB/s
            
            cout << "  数据生成时间: " << data_gen_time.count() << " ms" << endl;
            cout << "  哈希计算时间: " << hash_time.count() << " ms" << endl;
            cout << "  总时间: " << total_time.count() << " ms" << endl;
            cout << "  吞吐量: " << fixed << setprecision(2) << throughput << " MB/s" << endl;
            cout << "  结果: " << result.substr(0, 32) << "..." << endl;
            
            // 清理内存
            large_data.clear();
            large_data.shrink_to_fit();
        }
    }

    static void comparative_test() {
        cout << "\n=== SM3 与标准测试向量对比 ===" << endl;
        
        // 一些已知的测试向量（这里是示例，实际项目中应该使用官方测试向量）
        struct TestVector {
            string input;
            string expected_output;
        };
        
        vector<TestVector> test_vectors = {
            {"", "1ab21d8355cfa17f8e61194831e81a8f22bec8c728fefb747ed035eb5082aa2b"},
            {"a", "623476ac18f65a2909e43c7fec61b49c7e764a91a18ccb82f1917a29c86c5e88"},
            {"abc", "66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0"}
        };
        
        for (const auto& test : test_vectors) {
            vector<uint8_t> input_data(test.input.begin(), test.input.end());
            string result = SM3(input_data);
            
            bool match = (result == test.expected_output);
            cout << "输入: \"" << test.input << "\"" << endl;
            cout << "期望: " << test.expected_output << endl;
            cout << "实际: " << result << endl;
            cout << "结果: " << (match ? "✓ 匹配" : "✗ 不匹配") << endl << endl;
        }
    }
};

int main() {
    cout << "SM3 哈希算法基础实现 - 性能测试版本" << endl;
    cout << "======================================" << endl;
    
    // 选择测试模式
    cout << "请选择测试模式:" << endl;
    cout << "1. 详细模式 (显示中间过程)" << endl;
    cout << "2. 性能测试模式" << endl;
    cout << "3. 正确性测试" << endl;
    cout << "4. 内存使用测试" << endl;
    cout << "5. 对比测试" << endl;
    cout << "6. 完整测试套件" << endl;
    cout << "请输入选择 (1-6): ";
    
    int choice;
    cin >> choice;
    
    switch (choice) {
        case 1: {
            // 详细模式 - 原始功能
            cout << "\n=== 详细模式测试 ===" << endl;
            string input = "abc";
            vector<uint8_t> message(input.begin(), input.end());
            string hash = SM3(message, true);
            cout << "最终的杂凑值: " << hash << endl;
            break;
        }
        case 2:
            SM3_BasicBenchmark::benchmark_performance();
            break;
        case 3:
            SM3_BasicBenchmark::test_correctness();
            break;
        case 4:
            SM3_BasicBenchmark::memory_usage_test();
            break;
        case 5:
            SM3_BasicBenchmark::comparative_test();
            break;
        case 6:
            // 完整测试套件
            SM3_BasicBenchmark::test_correctness();
            SM3_BasicBenchmark::comparative_test();
            SM3_BasicBenchmark::benchmark_performance();
            SM3_BasicBenchmark::memory_usage_test();
            break;
        default:
            cout << "无效选择，执行默认测试..." << endl;
            string input = "abc";
            vector<uint8_t> message(input.begin(), input.end());
            string hash = SM3(message);
            cout << "输入: \"" << input << "\"" << endl;
            cout << "最终的杂凑值: " << hash << endl;
    }

    return 0;
}