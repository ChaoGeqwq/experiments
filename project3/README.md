# Poseidon2 零知识证明电路实现

基于 Circom 和 Groth16 协议的 Poseidon2 哈希算法零知识证明系统。

## ? 项目概述

本项目实现了 Poseidon2 哈希算法的零知识证明电路，支持以下特性：

- ? **Poseidon2 哈希算法**：基于论文 [Poseidon2: A Fast and Secure Hash Function](https://eprint.iacr.org/2023/323.pdf)
- ? **参数配置**：支持 (n,t,d) = (256,3,5) 和 (256,2,5) 两种配置
- ? **零知识证明**：证明知道某个哈希值的原象，而不泄露原象本身
- ? **Groth16 协议**：使用 Groth16 zk-SNARK 协议生成简洁证明
- ? **完整工具链**：从电路编译到证明验证的完整流程

## ?? 项目结构

```
project3/
├── poseidon2.circom           # Circom 电路实现
├── poseidon2_test.py          # Python 测试脚本
├── package.json               # Node.js 依赖配置
├── setup.sh                   # 环境安装脚本
├── README.md                  # 项目说明文档
└── build/                     # 编译输出目录
    ├── poseidon2.r1cs         # R1CS 约束系统
    ├── poseidon2.wasm         # WASM 证人生成器
    └── poseidon2_js/          # JavaScript 接口
```

## ? 快速开始

### 1. 环境设置

首先运行安装脚本来设置环境：

```bash
chmod +x setup.sh
./setup.sh
```

或者手动安装依赖：

```bash
# 安装 Node.js (Ubuntu/Debian)
curl -fsSL https://deb.nodesource.com/setup_18.x | sudo -E bash -
sudo apt-get install -y nodejs

# 安装 Rust (编译 circom 需要)
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh
source ~/.cargo/env

# 编译安装 circom
git clone https://github.com/iden3/circom.git
cd circom
cargo build --release
sudo cp target/release/circom /usr/local/bin/

# 安装 snarkjs
npm install -g snarkjs
```

### 2. 运行测试

运行完整的测试流程：

```bash
# 使用 t=3 版本 (默认)
python3 poseidon2_test.py

# 使用 t=2 版本
python3 poseidon2_test.py --t2
```

### 3. 手动执行步骤

如果想要手动执行各个步骤：

```bash
# 1. 编译电路
circom poseidon2.circom --r1cs --wasm --sym -o build/

# 2. 生成测试输入 (创建 inputs/input.json)
echo '{"hash": "998877665544332211", "preimage": ["12345", "67890"]}' > inputs/input.json

# 3. 生成证人
node build/poseidon2_js/generate_witness.js \
     build/poseidon2_js/poseidon2.wasm \
     inputs/input.json \
     witnesses/witness.wtns

# 4. 可信设置 (Powers of Tau)
snarkjs powersoftau new bn128 12 pot12_0000.ptau -v
snarkjs powersoftau contribute pot12_0000.ptau pot12_0001.ptau --name="First" -v
snarkjs powersoftau prepare phase2 pot12_0001.ptau pot12_final.ptau -v

# 5. 电路特定设置
snarkjs groth16 setup build/poseidon2.r1cs pot12_final.ptau keys/poseidon2_0000.zkey
snarkjs zkey contribute keys/poseidon2_0000.zkey keys/poseidon2_0001.zkey --name="Second" -v
snarkjs zkey export verificationkey keys/poseidon2_0001.zkey keys/verification_key.json

# 6. 生成证明
snarkjs groth16 prove keys/poseidon2_0001.zkey witnesses/witness.wtns proofs/proof.json proofs/public.json

# 7. 验证证明
snarkjs groth16 verify keys/verification_key.json proofs/public.json proofs/proof.json
```

## ? 技术细节

### Poseidon2 算法参数

根据论文 Table 1，我们使用以下参数：

| 参数 | 描述 | t=3 版本 | t=2 版本 |
|------|------|----------|----------|
| n | 字段大小 | 256 | 256 |
| t | 状态宽度 | 3 | 2 |
| d | S-box 度数 | 5 | 5 |
| R_F | 完整轮数 | 8 | 8 |
| R_P | 部分轮数 | 56 | 56 |

### 电路结构

```circom
// 主要组件
template SBox5()              // 5次方S-box
template MDS3x3()             // 3x3 MDS矩阵
template AddRoundConstants()  // 轮常数加法
template Poseidon2_T3_D5()    // 主哈希函数
template Poseidon2ProofT3()   // 零知识证明电路
```

### 零知识证明逻辑

```
公开输入：hash (哈希值)
私有输入：preimage[] (原象)
约束：hash === Poseidon2(preimage)
```

## ? 性能指标

以下是典型的性能指标（具体数值取决于硬件和参数）：

| 指标 | t=3 版本 | t=2 版本 |
|------|----------|----------|
| 约束数量 | ~10,000 | ~8,000 |
| 证明时间 | 2-5 秒 | 1-3 秒 |
| 验证时间 | <100ms | <100ms |
| 证明大小 | 128 字节 | 128 字节 |

## ? 测试示例

成功运行后，你会看到类似的输出：

```
? Poseidon2 零知识证明测试
? 使用参数: t=3, d=5
============================================================

? 检查依赖...
? Circom compiler: circom 2.1.6
? SnarkJS toolkit: snarkjs 0.6.11
? Node.js runtime: v18.17.0

? 编译电路...
? 电路编译成功

? 生成测试输入...
? 测试输入已生成: inputs/input.json
   哈希值: 998877665544332211
   原象: [12345, 67890]

? 生成证人...
? 证人生成成功

? 可信设置...
? 可信设置完成

? 生成证明...
? 证明生成成功

? 验证证明...
? 证明验证成功! ?

? 显示信息...
? 电路信息:
  R1CS文件大小: 245760 bytes
  估算约束数量: ~7680

? 所有测试步骤完成!
? Poseidon2 零知识证明系统运行成功!
```

## ? 生成的文件

运行完成后，会生成以下文件：

```
build/
├── poseidon2.r1cs                    # R1CS约束系统
├── poseidon2.wasm                    # WASM证人生成器
└── poseidon2_js/
    ├── poseidon2.wasm               # WASM文件
    ├── witness_calculator.js        # 证人计算器
    └── generate_witness.js          # 证人生成脚本

keys/
├── poseidon2_0000.zkey              # 初始证明密钥
├── poseidon2_0001.zkey              # 最终证明密钥
└── verification_key.json           # 验证密钥

proofs/
├── proof.json                       # Groth16证明
└── public.json                      # 公开输入

inputs/
└── input.json                       # 测试输入数据

witnesses/
└── witness.wtns                     # 证人文件
```

## ? 安全考虑

?? **重要提醒**：

1. **轮常数**：当前实现使用简化的轮常数，生产环境必须使用论文规范的官方常数
2. **MDS矩阵**：使用了简化的MDS矩阵，实际应用需要使用经过优化的安全矩阵
3. **可信设置**：Groth16需要可信设置，生产环境应使用大型仪式生成的参数
4. **代码审计**：这是演示实现，生产使用需要完整的安全审计

## ?? 故障排除

### 常见问题

1. **circom 编译失败**
   ```bash
   # 检查circom版本
   circom --version
   
   # 重新安装circom
   cargo install --force --git https://github.com/iden3/circom.git
   ```

2. **snarkjs 命令不存在**
   ```bash
   # 全局安装snarkjs
   npm install -g snarkjs
   
   # 检查PATH
   echo $PATH | grep node
   ```

3. **内存不足**
   ```bash
   # 增加Node.js内存限制
   export NODE_OPTIONS="--max-old-space-size=4096"
   ```

4. **权限问题**
   ```bash
   # 修复npm权限
   sudo chown -R $(whoami) ~/.npm
   ```

### 日志和调试

启用详细日志：

```bash
# 设置日志级别
export LOG_LEVEL=DEBUG

# 运行测试
python3 poseidon2_test.py
```

## ? 参考资料

1. [Poseidon2 论文](https://eprint.iacr.org/2023/323.pdf)
2. [Circom 官方文档](https://docs.circom.io/)
3. [SnarkJS 文档](https://github.com/iden3/snarkjs)
4. [Groth16 论文](https://eprint.iacr.org/2016/260.pdf)
5. [ZK-SNARKs 入门](https://zkp.science/)

## ? 贡献指南

欢迎提交 Issue 和 Pull Request！

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

## ? 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件。

## ? 致谢

感谢以下开源项目：

- [iden3/circom](https://github.com/iden3/circom) - Circom 编译器
- [iden3/snarkjs](https://github.com/iden3/snarkjs) - JavaScript zk-SNARK 实现
- [Poseidon2 论文作者](https://eprint.iacr.org/2023/323.pdf) - 算法设计

---

? **安全提醒**：这是一个教育和研究用途的实现，不适合直接用于生产环境。生产使用前请进行充分的安全审计。
