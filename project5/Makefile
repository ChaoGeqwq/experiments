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

# SM2签名攻击相关程序
sm2_signature_attack_demo: sm2_signature_attack.c sm2_signature_attack_demo.c
	$(CC) $(CFLAGS) -o sm2_signature_attack_demo sm2_signature_attack.c sm2_signature_attack_demo.c $(LDFLAGS)

satoshi_forge_poc: sm2_signature_attack.c satoshi_forge_poc.c
	$(CC) $(CFLAGS) -o satoshi_forge_poc sm2_signature_attack.c satoshi_forge_poc.c $(LDFLAGS)

test_signature_attacks: sm2_signature_attack.c test_signature_attacks.c
	$(CC) $(CFLAGS) -o test_signature_attacks sm2_signature_attack.c test_signature_attacks.c $(LDFLAGS)

# 编译所有签名攻击程序
signature_attacks: sm2_signature_attack_demo satoshi_forge_poc test_signature_attacks

# 清理签名攻击程序
clean_attacks:
	rm -f sm2_signature_attack_demo satoshi_forge_poc test_signature_attacks

# 运行所有签名攻击演示
run_attacks: signature_attacks
	@echo "运行签名攻击演示..."
	./test_signature_attacks
	@echo ""
	@echo "运行中本聪签名伪造POC..."
	echo "1" | ./satoshi_forge_poc

.PHONY: signature_attacks clean_attacks run_attacks
