# 编译器
CC = gcc

# 编译选项：开启警告信息，包含当前目录的头文件
CFLAGS = -Wall -I.

# 链接 OpenSSL 的库
LDFLAGS = -lssl -lcrypto

# 所有的源文件（手动指定或者自动收集）
SRCS = sm2_create_key_pair.c sm2_encrypt_and_decrypt.c test_sm2_encrypt_and_decrypt.c test_demo.c

# 生成的可执行文件名
TARGET = sm2_test

# 默认目标：编译生成
all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

# 清理目标
clean:
	rm -f $(TARGET)
