#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

/**
 * SM3 哈希算法 GPU 加速实现
 * 使用 CUDA 并行计算大幅提升性能
 */

#define THREADS_PER_BLOCK 256
#define BLOCKS_PER_GRID 128

// CUDA 错误检查宏
#define CUDA_CHECK(call) \
    do { \
        cudaError_t error = call; \
        if (error != cudaSuccess) { \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ \
                      << " - " << cudaGetErrorString(error) << std::endl; \
            exit(1); \
        } \
    } while(0)

// GPU 设备函数
__device__ uint32_t d_rotl(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

__device__ uint32_t d_FF(uint32_t x, uint32_t y, uint32_t z, int j) {
    return (j < 16) ? (x ^ y ^ z) : ((x & y) | (x & z) | (y & z));
}

__device__ uint32_t d_GG(uint32_t x, uint32_t y, uint32_t z, int j) {
    return (j < 16) ? (x ^ y ^ z) : ((x & y) | (~x & z));
}

__device__ uint32_t d_P0(uint32_t x) {
    return x ^ d_rotl(x, 9) ^ d_rotl(x, 17);
}

__device__ uint32_t d_P1(uint32_t x) {
    return x ^ d_rotl(x, 15) ^ d_rotl(x, 23);
}

// GPU 核心函数：压缩函数
__device__ void d_compress(uint32_t state[8], const uint32_t block[16]) {
    uint32_t W[68], W1[64];
    uint32_t A, B, C, D, E, F, G, H;
    uint32_t SS1, SS2, TT1, TT2;

    // 消息扩展
    for (int i = 0; i < 16; i++) {
        W[i] = block[i];
    }

    for (int i = 16; i < 68; i++) {
        W[i] = d_P1(W[i-16] ^ W[i-9] ^ d_rotl(W[i-3], 15)) ^ d_rotl(W[i-13], 7) ^ W[i-6];
    }

    for (int i = 0; i < 64; i++) {
        W1[i] = W[i] ^ W[i+4];
    }

    A = state[0]; B = state[1]; C = state[2]; D = state[3];
    E = state[4]; F = state[5]; G = state[6]; H = state[7];

    // 主循环
    for (int j = 0; j < 64; j++) {
        uint32_t T = (j < 16) ? 0x79cc4519 : 0x7a879d8a;
        SS1 = d_rotl((d_rotl(A, 12) + E + d_rotl(T, j)), 7);
        SS2 = SS1 ^ d_rotl(A, 12);
        TT1 = d_FF(A, B, C, j) + D + SS2 + W1[j];
        TT2 = d_GG(E, F, G, j) + H + SS1 + W[j];
        D = C;
        C = d_rotl(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = d_rotl(F, 19);
        F = E;
        E = d_P0(TT2);
    }

    state[0] ^= A; state[1] ^= B; state[2] ^= C; state[3] ^= D;
    state[4] ^= E; state[5] ^= F; state[6] ^= G; state[7] ^= H;
}

// GPU 核心：并行处理多个哈希任务
__global__ void sm3_hash_kernel(
    const uint8_t* input_data,
    size_t* input_lengths,
    uint8_t* output_hashes,
    int num_tasks
) {
    int tid = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (tid >= num_tasks) return;

    // 初始化状态
    uint32_t state[8] = {
        0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
        0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
    };

    size_t data_len = input_lengths[tid];
    const uint8_t* data = input_data + tid * 1024;  // 假设每个任务最大1KB

    // 计算需要的块数
    size_t bit_len = data_len * 8;
    size_t padded_len = data_len + 1;
    while ((padded_len % 64) != 56) {
        padded_len++;
    }
    padded_len += 8;

    // 处理每个 512 位块
    for (size_t block_start = 0; block_start < padded_len; block_start += 64) {
        uint32_t block[16] = {0};
        
        // 复制数据到块中
        for (int i = 0; i < 16; i++) {
            for (int j = 0; j < 4; j++) {
                size_t byte_idx = block_start + i * 4 + j;
                uint8_t byte_val = 0;
                
                if (byte_idx < data_len) {
                    byte_val = data[byte_idx];
                } else if (byte_idx == data_len) {
                    byte_val = 0x80;
                } else if (byte_idx >= padded_len - 8) {
                    // 添加长度信息
                    int len_byte_idx = byte_idx - (padded_len - 8);
                    byte_val = (bit_len >> ((7 - len_byte_idx) * 8)) & 0xff;
                }
                
                block[i] |= (byte_val << ((3 - j) * 8));
            }
        }
        
        d_compress(state, block);
    }

    // 输出结果
    uint8_t* output = output_hashes + tid * 32;
    for (int i = 0; i < 8; i++) {
        output[i*4] = (state[i] >> 24) & 0xff;
        output[i*4 + 1] = (state[i] >> 16) & 0xff;
        output[i*4 + 2] = (state[i] >> 8) & 0xff;
        output[i*4 + 3] = state[i] & 0xff;
    }
}

// GPU 加速的批量哈希计算
__global__ void sm3_batch_kernel(
    const uint8_t* input_data,
    uint8_t* output_hashes,
    size_t data_size,
    int num_blocks
) {
    int block_id = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (block_id >= num_blocks) return;

    uint32_t state[8] = {
        0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
        0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
    };

    const uint8_t* block_data = input_data + block_id * 64;
    uint32_t block[16];
    
    // 转换字节序
    for (int i = 0; i < 16; i++) {
        block[i] = (block_data[i*4] << 24) |
                  (block_data[i*4 + 1] << 16) |
                  (block_data[i*4 + 2] << 8) |
                  (block_data[i*4 + 3]);
    }
    
    d_compress(state, block);

    // 输出结果
    uint8_t* output = output_hashes + block_id * 32;
    for (int i = 0; i < 8; i++) {
        output[i*4] = (state[i] >> 24) & 0xff;
        output[i*4 + 1] = (state[i] >> 16) & 0xff;
        output[i*4 + 2] = (state[i] >> 8) & 0xff;
        output[i*4 + 3] = state[i] & 0xff;
    }
}

class SM3_GPU {
private:
    bool gpu_initialized;
    int device_count;
    
public:
    SM3_GPU() : gpu_initialized(false), device_count(0) {
        initialize_gpu();
    }
    
    ~SM3_GPU() {
        if (gpu_initialized) {
            cudaDeviceReset();
        }
    }

    bool initialize_gpu() {
        CUDA_CHECK(cudaGetDeviceCount(&device_count));
        
        if (device_count == 0) {
            std::cerr << "没有找到 CUDA 设备" << std::endl;
            return false;
        }

        // 选择最佳设备
        int best_device = 0;
        size_t max_memory = 0;
        
        for (int i = 0; i < device_count; i++) {
            cudaDeviceProp prop;
            CUDA_CHECK(cudaGetDeviceProperties(&prop, i));
            
            std::cout << "设备 " << i << ": " << prop.name 
                      << " (计算能力: " << prop.major << "." << prop.minor << ")" << std::endl;
            
            if (prop.totalGlobalMem > max_memory) {
                max_memory = prop.totalGlobalMem;
                best_device = i;
            }
        }
        
        CUDA_CHECK(cudaSetDevice(best_device));
        gpu_initialized = true;
        
        std::cout << "使用设备 " << best_device << std::endl;
        return true;
    }

    // 批量哈希计算
    std::vector<std::vector<uint8_t>> batch_hash(const std::vector<std::vector<uint8_t>>& messages) {
        if (!gpu_initialized || messages.empty()) {
            return {};
        }

        int num_messages = messages.size();
        size_t max_msg_size = 1024; // 限制每个消息最大1KB

        // 分配 GPU 内存
        uint8_t* d_input;
        size_t* d_lengths;
        uint8_t* d_output;
        
        CUDA_CHECK(cudaMalloc(&d_input, num_messages * max_msg_size));
        CUDA_CHECK(cudaMalloc(&d_lengths, num_messages * sizeof(size_t)));
        CUDA_CHECK(cudaMalloc(&d_output, num_messages * 32));

        // 准备输入数据
        std::vector<uint8_t> input_buffer(num_messages * max_msg_size, 0);
        std::vector<size_t> lengths(num_messages);
        
        for (int i = 0; i < num_messages; i++) {
            lengths[i] = std::min(messages[i].size(), max_msg_size);
            memcpy(input_buffer.data() + i * max_msg_size, 
                   messages[i].data(), lengths[i]);
        }

        // 复制到 GPU
        CUDA_CHECK(cudaMemcpy(d_input, input_buffer.data(), 
                             num_messages * max_msg_size, cudaMemcpyHostToDevice));
        CUDA_CHECK(cudaMemcpy(d_lengths, lengths.data(), 
                             num_messages * sizeof(size_t), cudaMemcpyHostToDevice));

        // 启动内核
        int blocks = (num_messages + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
        sm3_hash_kernel<<<blocks, THREADS_PER_BLOCK>>>(
            d_input, d_lengths, d_output, num_messages
        );
        
        CUDA_CHECK(cudaDeviceSynchronize());

        // 复制结果回 CPU
        std::vector<uint8_t> output_buffer(num_messages * 32);
        CUDA_CHECK(cudaMemcpy(output_buffer.data(), d_output, 
                             num_messages * 32, cudaMemcpyDeviceToHost));

        // 清理 GPU 内存
        CUDA_CHECK(cudaFree(d_input));
        CUDA_CHECK(cudaFree(d_lengths));
        CUDA_CHECK(cudaFree(d_output));

        // 转换为返回格式
        std::vector<std::vector<uint8_t>> results(num_messages);
        for (int i = 0; i < num_messages; i++) {
            results[i].assign(output_buffer.begin() + i * 32, 
                             output_buffer.begin() + (i + 1) * 32);
        }

        return results;
    }

    // 大数据流哈希处理
    std::vector<uint8_t> stream_hash(const std::vector<uint8_t>& large_data) {
        if (!gpu_initialized) {
            return {};
        }

        // 对于大数据，分块处理
        size_t chunk_size = 64 * 1024; // 64KB 块
        size_t num_chunks = (large_data.size() + chunk_size - 1) / chunk_size;
        
        uint32_t final_state[8] = {
            0x7380166f, 0x4914b2b9, 0x172442d7, 0xda8a0600,
            0xa96f30bc, 0x163138aa, 0xe38dee4d, 0xb0fb0e4e
        };

        for (size_t chunk = 0; chunk < num_chunks; chunk++) {
            size_t start = chunk * chunk_size;
            size_t end = std::min(start + chunk_size, large_data.size());
            size_t current_size = end - start;
            
            // 处理当前块
            std::vector<uint8_t> chunk_data(large_data.begin() + start, 
                                           large_data.begin() + end);
            
            // 如果是最后一块，进行填充
            if (chunk == num_chunks - 1) {
                uint64_t total_bits = large_data.size() * 8;
                chunk_data.push_back(0x80);
                
                while ((chunk_data.size() % 64) != 56) {
                    chunk_data.push_back(0x00);
                }
                
                for (int i = 7; i >= 0; i--) {
                    chunk_data.push_back((total_bits >> (i * 8)) & 0xff);
                }
            }
            
            // GPU 处理
            size_t padded_size = chunk_data.size();
            if (padded_size % 64 != 0) {
                padded_size += 64 - (padded_size % 64);
                chunk_data.resize(padded_size, 0);
            }
            
            int num_blocks = padded_size / 64;
            
            uint8_t* d_input;
            uint8_t* d_output;
            
            CUDA_CHECK(cudaMalloc(&d_input, padded_size));
            CUDA_CHECK(cudaMalloc(&d_output, num_blocks * 32));
            
            CUDA_CHECK(cudaMemcpy(d_input, chunk_data.data(), 
                                 padded_size, cudaMemcpyHostToDevice));
            
            int grid_size = (num_blocks + THREADS_PER_BLOCK - 1) / THREADS_PER_BLOCK;
            sm3_batch_kernel<<<grid_size, THREADS_PER_BLOCK>>>(
                d_input, d_output, padded_size, num_blocks
            );
            
            CUDA_CHECK(cudaDeviceSynchronize());
            
            // 获取最后一个块的结果作为中间状态
            if (num_blocks > 0) {
                std::vector<uint8_t> last_result(32);
                CUDA_CHECK(cudaMemcpy(last_result.data(), 
                                     d_output + (num_blocks - 1) * 32, 
                                     32, cudaMemcpyDeviceToHost));
                
                // 更新状态
                for (int i = 0; i < 8; i++) {
                    final_state[i] = (last_result[i*4] << 24) |
                                    (last_result[i*4 + 1] << 16) |
                                    (last_result[i*4 + 2] << 8) |
                                    (last_result[i*4 + 3]);
                }
            }
            
            CUDA_CHECK(cudaFree(d_input));
            CUDA_CHECK(cudaFree(d_output));
        }

        // 转换最终结果
        std::vector<uint8_t> result(32);
        for (int i = 0; i < 8; i++) {
            result[i*4] = (final_state[i] >> 24) & 0xff;
            result[i*4 + 1] = (final_state[i] >> 16) & 0xff;
            result[i*4 + 2] = (final_state[i] >> 8) & 0xff;
            result[i*4 + 3] = final_state[i] & 0xff;
        }

        return result;
    }

    static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
        std::stringstream ss;
        for (uint8_t byte : bytes) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        return ss.str();
    }
};

// GPU 性能测试
class GPU_Benchmark {
public:
    static void benchmark_gpu_performance() {
        std::cout << "=== SM3 GPU 加速性能测试 ===" << std::endl;
        
        SM3_GPU gpu_hasher;
        
        // 测试批量哈希
        std::cout << "\n批量哈希测试:" << std::endl;
        std::vector<int> batch_sizes = {100, 1000, 10000, 100000};
        
        for (int batch_size : batch_sizes) {
            std::vector<std::vector<uint8_t>> messages(batch_size);
            
            // 生成测试数据
            for (int i = 0; i < batch_size; i++) {
                std::string test_str = "Message " + std::to_string(i) + " for testing";
                messages[i] = std::vector<uint8_t>(test_str.begin(), test_str.end());
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            auto results = gpu_hasher.batch_hash(messages);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            double throughput = (batch_size * 1000000.0) / duration.count();
            
            std::cout << "批量大小: " << batch_size << ", "
                      << "时间: " << duration.count() << " μs, "
                      << "吞吐量: " << std::fixed << std::setprecision(2) 
                      << throughput << " hashes/s" << std::endl;
        }
        
        // 测试大数据流处理
        std::cout << "\n大数据流测试:" << std::endl;
        std::vector<size_t> data_sizes = {1024*1024, 10*1024*1024, 100*1024*1024}; // 1MB, 10MB, 100MB
        
        for (size_t size : data_sizes) {
            std::vector<uint8_t> large_data(size);
            for (size_t i = 0; i < size; i++) {
                large_data[i] = i % 256;
            }
            
            auto start = std::chrono::high_resolution_clock::now();
            auto result = gpu_hasher.stream_hash(large_data);
            auto end = std::chrono::high_resolution_clock::now();
            
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            double throughput = (size * 1000000.0) / duration.count() / 1024 / 1024; // MB/s
            
            std::cout << "数据大小: " << size / (1024*1024) << " MB, "
                      << "时间: " << duration.count() << " μs, "
                      << "吞吐量: " << std::fixed << std::setprecision(2) 
                      << throughput << " MB/s" << std::endl;
                      
            // 显示结果示例
            if (size == 1024*1024) {
                std::cout << "1MB数据哈希值: " << SM3_GPU::bytes_to_hex(result) << std::endl;
            }
        }
    }
};

int main() {
    std::cout << "SM3 哈希算法 GPU 加速实现" << std::endl;
    std::cout << "优化策略:" << std::endl;
    std::cout << "1. CUDA 并行计算" << std::endl;
    std::cout << "2. 批量数据处理" << std::endl;
    std::cout << "3. GPU 内存优化" << std::endl;
    std::cout << "4. 流式数据处理" << std::endl;
    std::cout << "5. 设备自动选择" << std::endl;
    std::cout << std::endl;

    try {
        // GPU 性能测试
        GPU_Benchmark::benchmark_gpu_performance();
        
        // 简单正确性测试
        std::cout << "\n=== 正确性测试 ===" << std::endl;
        SM3_GPU gpu_hasher;
        
        std::string test_str = "Hello, SM3 GPU!";
        std::vector<std::vector<uint8_t>> test_messages = {
            std::vector<uint8_t>(test_str.begin(), test_str.end())
        };
        
        auto results = gpu_hasher.batch_hash(test_messages);
        if (!results.empty()) {
            std::cout << "测试字符串: \"" << test_str << "\"" << std::endl;
            std::cout << "GPU SM3 哈希值: " << SM3_GPU::bytes_to_hex(results[0]) << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
