# SM3项目Makefile
# 支持编译所有C++文件和CUDA文件

# 编译器设置
CXX = g++
NVCC = nvcc

# 编译选项
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native
NVCCFLAGS = -O3 -arch=sm_50
INCLUDES = -I.

# 链接选项
LDFLAGS = -pthread
CUDA_LDFLAGS = -lcuda -lcudart

# 目标文件
CPP_TARGETS = sm3 sm3_optimized sm3_simd sm3_length_extension_attack sm3_merkle_tree_rfc6962
CUDA_TARGETS = sm3_gpu
ALL_TARGETS = $(CPP_TARGETS) $(CUDA_TARGETS)

# 源文件
SM3_SOURCES = sm3.cpp
SM3_OPTIMIZED_SOURCES = sm3_optimized.cpp
SM3_SIMD_SOURCES = sm3_simd.cpp
SM3_LENGTH_EXTENSION_SOURCES = sm3_length_extension_attack.cpp
SM3_MERKLE_TREE_SOURCES = sm3_merkle_tree_rfc6962.cpp
SM3_GPU_SOURCES = sm3_gpu.cu

# 默认目标
all: $(ALL_TARGETS)

# C++程序编译规则
sm3: $(SM3_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SM3_SOURCES) $(LDFLAGS)

sm3_optimized: $(SM3_OPTIMIZED_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SM3_OPTIMIZED_SOURCES) $(LDFLAGS)

sm3_simd: $(SM3_SIMD_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -mavx2 -msse4.1 -o $@ $(SM3_SIMD_SOURCES) $(LDFLAGS)

sm3_length_extension_attack: $(SM3_LENGTH_EXTENSION_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SM3_LENGTH_EXTENSION_SOURCES) $(LDFLAGS)

sm3_merkle_tree_rfc6962: $(SM3_MERKLE_TREE_SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $(SM3_MERKLE_TREE_SOURCES) $(LDFLAGS)

# CUDA程序编译规则
sm3_gpu: $(SM3_GPU_SOURCES)
	$(NVCC) $(NVCCFLAGS) $(INCLUDES) -o $@ $(SM3_GPU_SOURCES) $(CUDA_LDFLAGS)

# 只编译C++程序
cpp: $(CPP_TARGETS)

# 只编译CUDA程序
cuda: $(CUDA_TARGETS)
	@echo "注意: 编译CUDA程序需要安装NVIDIA CUDA工具包"

# 清理
clean:
	rm -f $(ALL_TARGETS) *.o *.out

# 深度清理（包括备份文件）
clean-all: clean
	rm -f *~ *.bak *.tmp

# 运行测试
test: all
	@echo "=== 运行SM3基础测试 ==="
	./sm3
	@echo ""
	@echo "=== 运行SM3优化版本测试 ==="
	./sm3_optimized
	@echo ""
	@echo "=== 运行Length Extension Attack测试 ==="
	echo "1" | ./sm3_length_extension_attack
	@echo ""
	@echo "=== 运行Merkle Tree基本测试 ==="
	echo "1" | ./sm3_merkle_tree_rfc6962

# 运行性能测试
benchmark: all
	@echo "=== SM3性能基准测试 ==="
	@echo "基础版本:"
	time ./sm3
	@echo ""
	@echo "优化版本:"
	time ./sm3_optimized
	@echo ""
	@echo "SIMD版本:"
	time ./sm3_simd
	@if command -v nvidia-smi > /dev/null 2>&1; then \
		echo ""; \
		echo "GPU版本:"; \
		time ./sm3_gpu; \
	else \
		echo ""; \
		echo "跳过GPU测试 (未检测到NVIDIA GPU)"; \
	fi

# 运行大规模测试
test-large: sm3_merkle_tree_rfc6962
	@echo "=== 运行10万叶子节点大规模Merkle树测试 ==="
	echo "2" | ./sm3_merkle_tree_rfc6962

# 运行完整的Length Extension Attack测试套件
test-attack: sm3_length_extension_attack
	@echo "=== 运行完整Length Extension Attack测试套件 ==="
	echo "4" | ./sm3_length_extension_attack

# 运行完整的Merkle树测试套件
test-merkle: sm3_merkle_tree_rfc6962
	@echo "=== 运行完整Merkle树测试套件 ==="
	echo "5" | ./sm3_merkle_tree_rfc6962

# 检查系统环境
check-env:
	@echo "=== 系统环境检查 ==="
	@echo "C++编译器: $(shell $(CXX) --version | head -n1)"
	@echo "C++标准库支持: $(shell echo '#include<iostream>' | $(CXX) -x c++ -std=c++17 -E - >/dev/null 2>&	1 && echo "C++17支持" || echo "C++17不支持")"
	@if command -v $(NVCC) > /dev/null 2>&1; then \
		echo "CUDA编译器: $$($(NVCC) --version | grep release)"; \
		echo "GPU设备: $$(nvidia-smi --query-gpu=name --format=csv,noheader,nounits 2>/dev/null || echo '未检测	到NVIDIA GPU')"; \
	else \
		echo "CUDA编译器: 未安装"; \
	fi
	@echo "处理器特性: $(shell lscpu | grep -E '(avx|sse)' || echo '无特殊指令集支持')"

# 安装依赖（Ubuntu/Debian系统）
install-deps:
	@echo "=== 安装编译依赖 ==="
	sudo apt-get update
	sudo apt-get install -y build-essential g++ make
	@echo "如需GPU支持，请手动安装NVIDIA CUDA工具包"

# 代码格式化（如果安装了clang-format）
format:
	@if command -v clang-format > /dev/null 2>&1; 
	then \
		echo "格式化C++代码..."; \
		clang-format -i *.cpp *.cu; \
		echo "代码格式化完成"; \
	else \
		echo "请安装clang-format来格式化代码: sudo apt-get install clang-format"; \
	fi

# 代码静态分析（如果安装了cppcheck）
analyze:
	@if command -v cppcheck > /dev/null 2>&1; then \
		echo "运行静态代码分析..."; \
		cppcheck --enable=all --std=c++17 *.cpp; \
	else \
		echo "请安装cppcheck来进行静态分析: sudo apt-get install cppcheck"; \
	fi

# 生成编译数据库（用于IDE支持）
compile-db: clean
	@if command -v bear > /dev/null 2>&1; then \
		echo "生成编译数据库..."; \
		bear -- make all; \
		echo "编译数据库已生成: compile_commands.json"; \
	else \
		echo "请安装bear来生成编译数据库: sudo apt-get install bear"; \
	fi

# 打包源代码
package:
	@echo "打包项目源代码..."
	tar -czf sm3_project_$(shell date +%Y%m%d_%H%M%S).tar.gz *.cpp *.cu *.h Makefile *.md
	@echo "源代码包已创建"

# 显示帮助信息
help:
	@echo "SM3项目Makefile帮助"
	@echo "===================="
	@echo ""
	@echo "编译目标:"
	@echo "  all                    - 编译所有程序（默认）"
	@echo "  cpp                    - 只编译C++程序"
	@echo "  cuda                   - 只编译CUDA程序"
	@echo "  sm3                    - 编译基础SM3实现"
	@echo "  sm3_optimized          - 编译优化版SM3实现"
	@echo "  sm3_simd              - 编译SIMD优化版SM3实现"
	@echo "  sm3_gpu               - 编译GPU版SM3实现（需要CUDA）"
	@echo "  sm3_length_extension_attack - 编译长度扩展攻击验证程序"
	@echo "  sm3_merkle_tree_rfc6962     - 编译RFC6962 Merkle树实现"
	@echo ""
	@echo "测试目标:"
	@echo "  test                   - 运行基本测试"
	@echo "  test-large            - 运行大规模测试"
	@echo "  test-attack           - 运行完整长度扩展攻击测试"
	@echo "  test-merkle           - 运行完整Merkle树测试"
	@echo "  benchmark             - 运行性能基准测试"
	@echo ""
	@echo "实用工具:"
	@echo "  clean                 - 清理编译文件"
	@echo "  clean-all             - 深度清理"
	@echo "  check-env             - 检查系统环境"
	@echo "  install-deps          - 安装编译依赖"
	@echo "  format                - 格式化代码"
	@echo "  analyze               - 静态代码分析"
	@echo "  compile-db            - 生成编译数据库"
	@echo "  package               - 打包源代码"
	@echo "  help                  - 显示此帮助信息"
	@echo ""
	@echo "示例用法:"
	@echo "  make                  - 编译所有程序"
	@echo "  make cpp              - 只编译C++程序"
	@echo "  make test             - 编译并运行基本测试"
	@echo "  make benchmark        - 运行性能对比测试"
	@echo "  make check-env        - 检查编译环境"

# 声明伪目标
.PHONY: all cpp cuda clean clean-all test benchmark test-large test-attack test-merkle check-env install-deps format analyze compile-db package help

# 默认目标
.DEFAULT_GOAL := all
