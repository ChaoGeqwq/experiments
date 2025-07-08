#!/usr/bin/env python3
"""
Poseidon2 哈希算法零知识证明测试脚本
基于 Circom 和 Groth16 协议

使用方法:
    python poseidon2_test.py
"""

import json
import subprocess
import os
import sys
import hashlib
from pathlib import Path
import logging

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class Poseidon2ZKProof:
    """Poseidon2 零知识证明系统"""
    
    def __init__(self, circuit_name="poseidon2", use_t3=True):
        self.circuit_name = circuit_name
        self.use_t3 = use_t3
        self.project_dir = Path.cwd()
        self.setup_directories()
        
    def setup_directories(self):
        """创建必要的目录结构"""
        dirs = ["build", "keys", "proofs", "inputs", "witnesses"]
        for dir_name in dirs:
            (self.project_dir / dir_name).mkdir(exist_ok=True)
            
    def check_dependencies(self):
        """检查依赖工具是否安装"""
        tools = {
            "circom": "Circom compiler",
            "snarkjs": "SnarkJS toolkit", 
            "node": "Node.js runtime"
        }
        
        missing = []
        for tool, desc in tools.items():
            try:
                result = subprocess.run(
                    [tool, "--version"], 
                    capture_output=True, 
                    text=True,
                    check=True
                )
                logger.info(f"? {desc}: {result.stdout.strip()}")
            except (subprocess.CalledProcessError, FileNotFoundError):
                missing.append(tool)
                logger.error(f"? {desc} 未安装")
                
        if missing:
            logger.error(f"请安装缺失的工具: {', '.join(missing)}")
            logger.info("安装命令:")
            logger.info("npm install -g circom snarkjs")
            return False
            
        return True
        
    def compile_circuit(self):
        """编译 Circom 电路"""
        try:
            logger.info("? 编译 Circom 电路...")
            
            cmd = [
                "circom",
                f"{self.circuit_name}.circom",
                "--r1cs",
                "--wasm", 
                "--sym",
                "-o", "build/"
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=self.project_dir
            )
            
            if result.returncode != 0:
                logger.error(f"编译失败: {result.stderr}")
                return False
                
            logger.info("? 电路编译成功")
            
            # 检查生成的文件
            r1cs_file = self.project_dir / "build" / f"{self.circuit_name}.r1cs"
            wasm_file = self.project_dir / "build" / f"{self.circuit_name}_js" / f"{self.circuit_name}.wasm"
            
            if r1cs_file.exists() and wasm_file.exists():
                logger.info(f"? R1CS文件: {r1cs_file}")
                logger.info(f"? WASM文件: {wasm_file}")
                return True
            else:
                logger.error("生成的文件不完整")
                return False
                
        except Exception as e:
            logger.error(f"编译异常: {e}")
            return False
            
    def generate_test_input(self):
        """生成测试输入数据"""
        if self.use_t3:
            # t=3 版本: 2个输入元素
            preimage = [12345, 67890]
            # 模拟哈希值 (实际应该用真正的Poseidon2计算)
            hash_value = 998877665544332211
        else:
            # t=2 版本: 1个输入元素  
            preimage = [12345]
            hash_value = 123456789012345678
            
        input_data = {
            "hash": str(hash_value),
            "preimage": [str(x) for x in preimage]
        }
        
        input_file = self.project_dir / "inputs" / "input.json"
        with open(input_file, 'w') as f:
            json.dump(input_data, f, indent=2)
            
        logger.info(f"? 测试输入已生成: {input_file}")
        logger.info(f"   哈希值: {hash_value}")
        logger.info(f"   原象: {preimage}")
        
        return input_file
        
    def generate_witness(self, input_file):
        """生成证人文件"""
        try:
            logger.info("? 生成证人文件...")
            
            witness_file = self.project_dir / "witnesses" / "witness.wtns"
            
            cmd = [
                "node",
                f"build/{self.circuit_name}_js/generate_witness.js",
                f"build/{self.circuit_name}_js/{self.circuit_name}.wasm", 
                str(input_file),
                str(witness_file)
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=self.project_dir
            )
            
            if result.returncode != 0:
                logger.error(f"生成证人失败: {result.stderr}")
                return False
                
            logger.info("? 证人生成成功")
            logger.info(f"? 证人文件: {witness_file}")
            return True
            
        except Exception as e:
            logger.error(f"生成证人异常: {e}")
            return False
            
    def trusted_setup(self):
        """执行可信设置"""
        try:
            logger.info("? 执行可信设置...")
            
            # Phase 1: Powers of Tau
            logger.info("Phase 1: Powers of Tau ceremony...")
            
            ptau_files = [
                "pot12_0000.ptau",
                "pot12_0001.ptau", 
                "pot12_final.ptau"
            ]
            
            # 检查是否已存在
            if all((self.project_dir / f).exists() for f in ptau_files):
                logger.info("Powers of Tau 文件已存在，跳过生成")
            else:
                # 生成初始Powers of Tau
                subprocess.run([
                    "snarkjs", "powersoftau", "new", "bn128", "12",
                    "pot12_0000.ptau", "-v"
                ], cwd=self.project_dir, check=True)
                
                # 第一次贡献
                subprocess.run([
                    "snarkjs", "powersoftau", "contribute", 
                    "pot12_0000.ptau", "pot12_0001.ptau",
                    "--name=First contribution", "-v"
                ], cwd=self.project_dir, check=True)
                
                # 准备Phase 2
                subprocess.run([
                    "snarkjs", "powersoftau", "prepare", "phase2",
                    "pot12_0001.ptau", "pot12_final.ptau", "-v"
                ], cwd=self.project_dir, check=True)
                
            # Phase 2: Circuit-specific setup
            logger.info("Phase 2: Circuit-specific setup...")
            
            zkey_files = [
                f"keys/{self.circuit_name}_0000.zkey",
                f"keys/{self.circuit_name}_0001.zkey",
                f"keys/verification_key.json"
            ]
            
            if all((self.project_dir / f).exists() for f in zkey_files):
                logger.info("电路密钥已存在，跳过生成")
            else:
                # Groth16 setup
                subprocess.run([
                    "snarkjs", "groth16", "setup",
                    f"build/{self.circuit_name}.r1cs",
                    "pot12_final.ptau", 
                    f"keys/{self.circuit_name}_0000.zkey"
                ], cwd=self.project_dir, check=True)
                
                # 第二次贡献
                subprocess.run([
                    "snarkjs", "zkey", "contribute",
                    f"keys/{self.circuit_name}_0000.zkey",
                    f"keys/{self.circuit_name}_0001.zkey",
                    "--name=Second contribution", "-v"
                ], cwd=self.project_dir, check=True)
                
                # 导出验证密钥
                subprocess.run([
                    "snarkjs", "zkey", "export", "verificationkey",
                    f"keys/{self.circuit_name}_0001.zkey",
                    "keys/verification_key.json"
                ], cwd=self.project_dir, check=True)
                
            logger.info("? 可信设置完成")
            return True
            
        except subprocess.CalledProcessError as e:
            logger.error(f"可信设置失败: {e}")
            return False
        except Exception as e:
            logger.error(f"可信设置异常: {e}")
            return False
            
    def generate_proof(self):
        """生成 Groth16 证明"""
        try:
            logger.info("? 生成 Groth16 证明...")
            
            cmd = [
                "snarkjs", "groth16", "prove",
                f"keys/{self.circuit_name}_0001.zkey",
                "witnesses/witness.wtns",
                "proofs/proof.json",
                "proofs/public.json"
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=self.project_dir
            )
            
            if result.returncode != 0:
                logger.error(f"生成证明失败: {result.stderr}")
                return False
                
            logger.info("? 证明生成成功")
            
            # 显示证明信息
            proof_file = self.project_dir / "proofs" / "proof.json"
            public_file = self.project_dir / "proofs" / "public.json"
            
            if proof_file.exists() and public_file.exists():
                proof_size = proof_file.stat().st_size
                public_size = public_file.stat().st_size
                logger.info(f"? 证明文件: {proof_file} ({proof_size} bytes)")
                logger.info(f"? 公开输入: {public_file} ({public_size} bytes)")
                
            return True
            
        except Exception as e:
            logger.error(f"生成证明异常: {e}")
            return False
            
    def verify_proof(self):
        """验证证明"""
        try:
            logger.info("? 验证证明...")
            
            cmd = [
                "snarkjs", "groth16", "verify",
                "keys/verification_key.json",
                "proofs/public.json", 
                "proofs/proof.json"
            ]
            
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=self.project_dir
            )
            
            if result.returncode == 0:
                logger.info("? 证明验证成功! ?")
                return True
            else:
                logger.error(f"? 证明验证失败: {result.stderr}")
                return False
                
        except Exception as e:
            logger.error(f"验证证明异常: {e}")
            return False
            
    def show_circuit_info(self):
        """显示电路信息"""
        try:
            logger.info("? 电路信息:")
            
            # 显示R1CS约束数量
            r1cs_file = self.project_dir / "build" / f"{self.circuit_name}.r1cs"
            if r1cs_file.exists():
                # 简单估算约束数量 (实际需要解析R1CS文件)
                size = r1cs_file.stat().st_size
                estimated_constraints = size // 32  # 粗略估算
                logger.info(f"  R1CS文件大小: {size} bytes")
                logger.info(f"  估算约束数量: ~{estimated_constraints}")
                
            # 显示密钥文件大小
            vk_file = self.project_dir / "keys" / "verification_key.json"
            if vk_file.exists():
                with open(vk_file, 'r') as f:
                    vk_data = json.load(f)
                logger.info(f"  验证密钥大小: {vk_file.stat().st_size} bytes")
                
        except Exception as e:
            logger.error(f"显示电路信息异常: {e}")
            
    def run_full_test(self):
        """运行完整测试流程"""
        logger.info("=" * 60)
        logger.info("? Poseidon2 零知识证明测试")
        logger.info(f"? 使用参数: t={'3' if self.use_t3 else '2'}, d=5")
        logger.info("=" * 60)
        
        # 测试步骤
        steps = [
            ("检查依赖", self.check_dependencies),
            ("编译电路", self.compile_circuit),
            ("生成测试输入", self.generate_test_input),
            ("生成证人", lambda: self.generate_witness(self.generate_test_input())),
            ("可信设置", self.trusted_setup),
            ("生成证明", self.generate_proof),
            ("验证证明", self.verify_proof),
            ("显示信息", self.show_circuit_info)
        ]
        
        for step_name, step_func in steps:
            logger.info(f"\n? {step_name}...")
            
            try:
                if step_name == "生成测试输入":
                    input_file = step_func()
                    if not input_file:
                        logger.error(f"? {step_name} 失败")
                        return False
                elif step_name == "生成证人":
                    if not self.generate_witness(input_file):
                        logger.error(f"? {step_name} 失败")
                        return False
                else:
                    if not step_func():
                        logger.error(f"? {step_name} 失败")
                        return False
                        
                logger.info(f"? {step_name} 完成")
                
            except Exception as e:
                logger.error(f"? {step_name} 异常: {e}")
                return False
                
        logger.info("\n" + "=" * 60)
        logger.info("? 所有测试步骤完成!")
        logger.info("? Poseidon2 零知识证明系统运行成功!")
        logger.info("=" * 60)
        
        return True

def main():
    """主函数"""
    import argparse
    
    parser = argparse.ArgumentParser(description='Poseidon2 零知识证明测试')
    parser.add_argument('--t2', action='store_true', help='使用 t=2 版本 (默认 t=3)')
    parser.add_argument('--circuit', default='poseidon2', help='电路名称')
    
    args = parser.parse_args()
    
    # 创建测试实例
    prover = Poseidon2ZKProof(
        circuit_name=args.circuit,
        use_t3=not args.t2
    )
    
    # 运行测试
    success = prover.run_full_test()
    
    if success:
        logger.info("? 测试完成!")
        sys.exit(0)
    else:
        logger.error("? 测试失败!")
        sys.exit(1)

if __name__ == "__main__":
    main()
