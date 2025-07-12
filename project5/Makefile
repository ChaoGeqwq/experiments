# 编译器
CC = gcc

# 编译选项：开启警告信息，包含当前目录的头文件，启用优化
CFLAGS = -Wall -O2 -I. -std=c99

# 链接 OpenSSL 的库
LDFLAGS = -lssl -lcrypto -lm

# 原始源文件
BASIC_SRCS = sm2_create_key_pair.c sm2_encrypt_and_decrypt.c test_sm2_encrypt_and_decrypt.c test_demo.c

# 优化版本源文件
OPTIMIZED_SRCS = sm2_optimized.c sm2_benchmark.c

# 生成的可执行文件名
BASIC_TARGET = sm2_test
OPTIMIZED_TARGET = sm2_benchmark

# 默认目标：编译所有版本
all: $(BASIC_TARGET) $(OPTIMIZED_TARGET)

# 基础版本
$(BASIC_TARGET): $(BASIC_SRCS)
    $(CC) $(CFLAGS) -o $(BASIC_TARGET) $(BASIC_SRCS) $(LDFLAGS)

# 优化版本
$(OPTIMIZED_TARGET): $(OPTIMIZED_SRCS)
    $(CC) $(CFLAGS) -o $(OPTIMIZED_TARGET) $(OPTIMIZED_SRCS) $(LDFLAGS)

# 运行性能测试
test: all
    @echo "=== 运行SM2优化性能测试 ==="
    ./$(OPTIMIZED_TARGET)

# 运行原始测试
test-basic: $(BASIC_TARGET)
    @echo "=== 运行SM2基础测试 ==="
    ./$(BASIC_TARGET)

# 清理目标
clean:
    rm -f $(BASIC_TARGET) $(OPTIMIZED_TARGET)

# 帮助信息
help:
    @echo "可用目标:"
    @echo "  all        - 编译所有版本"
    @echo "  test       - 运行优化版本性能测试"
    @echo "  test-basic - 运行基础版本测试"
    @echo "  clean      - 清理编译文件"
    @echo "  help       - 显示此帮助信息"

.PHONY: all test test-basic clean help