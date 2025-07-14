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

# 源文件
SOURCES = sm4_ttable.cpp sm4_aesni.cpp sm4_gfni.cpp sm4_gcm.cpp main_optimized.cpp
OBJECTS = $(SOURCES:.cpp=.o)

# 目标文件
TARGET = sm4_optimized

.PHONY: all clean test

# 默认目标：基础版本
all: COMPILE_FLAGS = $(BASIC_FLAGS)
all: $(TARGET)

# 基础版本（仅T-table优化）
basic: COMPILE_FLAGS = $(BASIC_FLAGS)
basic: $(TARGET)

# AES-NI优化版本  
aesni: COMPILE_FLAGS = $(AESNI_FLAGS)
aesni: $(TARGET)

# 编译目标
$(TARGET): $(OBJECTS)
	$(CXX) $(COMPILE_FLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.cpp sm4_optimized.h
	$(CXX) $(COMPILE_FLAGS) -c $< -o $@

# 测试
test: $(TARGET)
	./$(TARGET)

# 清理
clean:
	rm -f *.o $(TARGET)

# 帮助信息
help:
	@echo "SM4优化实现编译选项:"
	@echo "  make all     - 基础版本"
	@echo "  make basic   - 基础版本(仅T-table)"
	@echo "  make aesni   - AES-NI优化版本"
	@echo "  make test    - 编译并运行测试"
	@echo "  make clean   - 清理编译文件"
