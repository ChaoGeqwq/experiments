CC = gcc
CFLAGS = -Wall -O2 -I. -std=c99
LDFLAGS = -lssl -lcrypto -lm

BASIC_SRCS = sm2_create_key_pair.c sm2_encrypt_and_decrypt.c test_sm2_encrypt_and_decrypt.c test_demo.c
OPTIMIZED_SRCS = sm2_optimized.c sm2_benchmark.c

BASIC_TARGET = sm2_test
OPTIMIZED_TARGET = sm2_benchmark

all: $(BASIC_TARGET) $(OPTIMIZED_TARGET)

$(BASIC_TARGET): $(BASIC_SRCS)
	$(CC) $(CFLAGS) -o $(BASIC_TARGET) $(BASIC_SRCS) $(LDFLAGS)

$(OPTIMIZED_TARGET): $(OPTIMIZED_SRCS)
	$(CC) $(CFLAGS) -o $(OPTIMIZED_TARGET) $(OPTIMIZED_SRCS) $(LDFLAGS)

test: all
	@echo "=== 运行SM2优化性能测试 ==="
	./$(OPTIMIZED_TARGET)

test-basic: $(BASIC_TARGET)
	@echo "=== 运行SM2基础测试 ==="
	./$(BASIC_TARGET)

clean:
	rm -f $(BASIC_TARGET) $(OPTIMIZED_TARGET)

help:
	@echo "可用目标:"
	@echo "  all        - 编译所有版本"
	@echo "  test       - 运行优化版本性能测试"
	@echo "  test-basic - 运行基础版本测试"
	@echo "  clean      - 清理编译文件"

.PHONY: all test test-basic clean help
