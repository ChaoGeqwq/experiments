# SM4优化实现Makefile

CXX = g++
CXXFLAGS = -std=c++11 -O3 -Wall -Wextra

# 指令集优化选项
ARCH_FLAGS = -march=native -mtune=native
AES_FLAGS = -maes -msse4.1
GFNI_FLAGS = -mgfni -mavx512f -mavx512vl
AVX512_FLAGS = -mavx512f -mavx512vl -mavx512bw

# 根据目标选择不同的编译选项
BASIC_FLAGS = $(CXXFLAGS) $(ARCH_FLAGS)
AESNI_FLAGS = $(CXXFLAGS) $(ARCH_FLAGS) $(AES_FLAGS)
GFNI_FLAGS_FULL = $(CXXFLAGS) $(ARCH_FLAGS) $(AES_FLAGS) $(GFNI_FLAGS)
AVX512_FLAGS_FULL = $(CXXFLAGS) $(ARCH_FLAGS) $(AES_FLAGS) $(AVX512_FLAGS)

# 源文件
TTABLE_SRC = sm4_ttable.cpp
AESNI_SRC = sm4_aesni.cpp
GFNI_SRC = sm4_gfni.cpp
GCM_SRC = sm4_gcm.cpp
PERF_SRC = sm4_performance_test.cpp
MAIN_SRC = main_optimized.cpp

# 目标文件
TTABLE_OBJ = sm4_ttable.o
AESNI_OBJ = sm4_aesni.o
GFNI_OBJ = sm4_gfni.o
GCM_OBJ = sm4_gcm.o
PERF_OBJ = sm4_performance_test.o
MAIN_OBJ = main_optimized.o

# 可执行文件
TARGET = sm4_optimized
PERF_TARGET = sm4_performance

.PHONY: all clean basic aesni gfni avx512 performance

# 默认目标：根据CPU能力自动选择最优编译选项
all: $(TARGET)

# 基础版本（仅T-table优化）
basic: COMPILE_FLAGS = $(BASIC_FLAGS)
basic: $(TARGET)

# AES-NI优化版本
aesni: COMPILE_FLAGS = $(AESNI_FLAGS)
aesni: $(TARGET)

# GFNI优化版本
gfni: COMPILE_FLAGS = $(GFNI_FLAGS_FULL)
gfni: $(TARGET)

# AVX512优化版本
avx512: COMPILE_FLAGS = $(AVX512_FLAGS_FULL)
avx512: $(TARGET)

# 性能测试版本
performance: COMPILE_FLAGS = $(GFNI_FLAGS_FULL)
performance: $(PERF_TARGET)

# 编译目标文件
$(TTABLE_OBJ): $(TTABLE_SRC) sm4_optimized.h
$(CXX) $(COMPILE_FLAGS) -c $(TTABLE_SRC) -o $(TTABLE_OBJ)

$(AESNI_OBJ): $(AESNI_SRC) sm4_optimized.h
$(CXX) $(COMPILE_FLAGS) -c $(AESNI_SRC) -o $(AESNI_OBJ)

$(GFNI_OBJ): $(GFNI_SRC) sm4_optimized.h
$(CXX) $(COMPILE_FLAGS) -c $(GFNI_SRC) -o $(GFNI_OBJ)

$(GCM_OBJ): $(GCM_SRC) sm4_optimized.h
$(CXX) $(COMPILE_FLAGS) -c $(GCM_SRC) -o $(GCM_OBJ)

$(MAIN_OBJ): $(MAIN_SRC) sm4_optimized.h
$(CXX) $(COMPILE_FLAGS) -c $(MAIN_SRC) -o $(MAIN_OBJ)

$(PERF_OBJ): $(PERF_SRC) sm4_optimized.h
$(CXX) $(COMPILE_FLAGS) -c $(PERF_SRC) -o $(PERF_OBJ)

# 链接主程序
$(TARGET): $(TTABLE_OBJ) $(AESNI_OBJ) $(GFNI_OBJ) $(GCM_OBJ) $(MAIN_OBJ)
$(CXX) $(COMPILE_FLAGS) -o $(TARGET) $^

# 链接性能测试程序
$(PERF_TARGET): $(TTABLE_OBJ) $(AESNI_OBJ) $(GFNI_OBJ) $(GCM_OBJ) $(PERF_OBJ)
$(CXX) $(COMPILE_FLAGS) -o $(PERF_TARGET) $^
@echo "运行性能测试: ./$(PERF_TARGET)"

# 清理
clean:
rm -f *.o $(TARGET) $(PERF_TARGET)

# 安装依赖和检查CPU能力
check-cpu:
@echo "检查CPU指令集支持..."
@if grep -q "aes" /proc/cpuinfo; then echo "✓ AES-NI 支持"; else echo "✗ AES-NI 不支持"; fi
@if grep -q "avx512f" /proc/cpuinfo; then echo "✓ AVX512F 支持"; else echo "✗ AVX512F 不支持"; fi
@if grep -q "gfni" /proc/cpuinfo; then echo "✓ GFNI 支持"; else echo "✗ GFNI 不支持"; fi

# 测试所有版本
test-all: clean
@echo "编译并测试基础版本..."
@make basic COMPILE_FLAGS="$(BASIC_FLAGS)"
@./$(TARGET) > test_basic.log 2>&1 || true
@echo "基础版本测试完成"

@echo "编译并测试AES-NI版本..."
@make clean
@make aesni COMPILE_FLAGS="$(AESNI_FLAGS)"
@./$(TARGET) > test_aesni.log 2>&1 || true
@echo "AES-NI版本测试完成"

@echo "编译并测试GFNI版本..."
@make clean
@make gfni COMPILE_FLAGS="$(GFNI_FLAGS_FULL)"
@./$(TARGET) > test_gfni.log 2>&1 || true
@echo "GFNI版本测试完成"

@echo "所有版本测试完成，查看 test_*.log 文件"

# 帮助信息
help:
@echo "SM4优化实现编译选项:"
@echo "  make all        - 自动选择最优编译选项"
@echo "  make basic      - 基础版本(仅T-table)"
@echo "  make aesni      - AES-NI优化版本"
@echo "  make gfni       - GFNI指令集优化版本"
@echo "  make avx512     - AVX512优化版本"
@echo "  make performance- 编译性能测试程序"
@echo "  make check-cpu  - 检查CPU指令集支持"
@echo "  make test-all   - 测试所有版本"
@echo "  make clean      - 清理编译文件"
@echo "  make help       - 显示此帮助信息"
