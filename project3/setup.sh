#!/bin/bash

# Poseidon2 零知识证明环境安装脚本
echo "? 设置 Poseidon2 零知识证明环境"
echo "=================================="

# 检查操作系统
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    OS="macos"
else
    echo "? 不支持的操作系统: $OSTYPE"
    exit 1
fi

echo "? 检测到系统: $OS"

# 创建项目目录
echo "? 创建项目目录..."
mkdir -p build keys proofs inputs witnesses

# 检查并安装 Node.js
echo "? 检查 Node.js..."
if ! command -v node &> /dev/null; then
    echo "? 安装 Node.js..."
    if [[ "$OS" == "linux" ]]; then
        curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
        sudo apt-get install -y nodejs
    elif [[ "$OS" == "macos" ]]; then
        if command -v brew &> /dev/null; then
            brew install node
        else
            echo "? 请先安装 Homebrew: https://brew.sh/"
            exit 1
        fi
    fi
else
    echo "? Node.js 已安装: $(node --version)"
fi

# 检查并安装 Rust (编译 circom 需要)
echo "? 检查 Rust..."
if ! command -v cargo &> /dev/null; then
    echo "? 安装 Rust..."
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
    source ~/.cargo/env
else
    echo "? Rust 已安装: $(cargo --version)"
fi

# 检查并安装 circom
echo "? 检查 circom..."
if ! command -v circom &> /dev/null; then
    echo "? 编译安装 circom..."
    git clone https://github.com/iden3/circom.git /tmp/circom
    cd /tmp/circom
    cargo build --release
    sudo cp target/release/circom /usr/local/bin/
    cd - > /dev/null
    rm -rf /tmp/circom
else
    echo "? circom 已安装: $(circom --version)"
fi

# 安装 snarkjs
echo "? 安装 snarkjs..."
if ! command -v snarkjs &> /dev/null; then
    npm install -g snarkjs
else
    echo "? snarkjs 已安装: $(snarkjs --version)"
fi

# 安装项目依赖
echo "? 安装项目依赖..."
if [ -f "package.json" ]; then
    npm install
else
    echo "??  package.json 不存在，跳过 npm install"
fi

# 安装 Python 依赖
echo "? 检查 Python 依赖..."
python3 -c "import json, subprocess, pathlib, logging" 2>/dev/null || {
    echo "??  Python 标准库模块正常"
}

# 验证安装
echo ""
echo "? 验证安装..."
echo "=================================="

tools=("node" "npm" "circom" "snarkjs" "python3")
all_good=true

for tool in "${tools[@]}"; do
    if command -v "$tool" &> /dev/null; then
        version=$($tool --version 2>/dev/null || echo "已安装")
        echo "? $tool: $version"
    else
        echo "? $tool: 未安装"
        all_good=false
    fi
done

echo ""
if [ "$all_good" = true ]; then
    echo "? 环境设置完成!"
    echo ""
    echo "? 接下来的步骤:"
    echo "1. 运行完整测试: python3 poseidon2_test.py"
    echo "2. 或者手动编译: circom poseidon2.circom --r1cs --wasm --sym -o build/"
    echo "3. 查看帮助: python3 poseidon2_test.py --help"
    echo ""
    echo "? 项目结构:"
    echo "├── poseidon2.circom       # Circom 电路"
    echo "├── poseidon2_test.py      # 测试脚本"
    echo "├── package.json           # Node.js 配置"
    echo "├── setup.sh               # 安装脚本"
    echo "└── build/                 # 编译输出"
else
    echo "? 环境设置失败，请检查上述错误"
    exit 1
fi
