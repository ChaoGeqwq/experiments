pragma circom 2.0.0;

// Poseidon2 哈希算法的 Circom 实现
// 参数: (n,t,d) = (256,3,5) - 256位字段, 3个状态元素, 5次方S-box
// 基于论文: https://eprint.iacr.org/2023/323.pdf

// 5次方S-box组件
template SBox5() {
    signal input in;
    signal output out;
    
    signal t2 <== in * in;
    signal t4 <== t2 * t2;
    out <== t4 * in;
}

// MDS矩阵乘法组件 (3x3)
template MDS3x3() {
    signal input state[3];
    signal output newState[3];
    
    // 简化的MDS矩阵 (生产环境需要使用优化的矩阵)
    // 这里使用Cauchy矩阵的简化版本
    newState[0] <== 2 * state[0] + 1 * state[1] + 1 * state[2];
    newState[1] <== 1 * state[0] + 2 * state[1] + 1 * state[2];
    newState[2] <== 1 * state[0] + 1 * state[1] + 2 * state[2];
}

// MDS矩阵乘法组件 (2x2)
template MDS2x2() {
    signal input state[2];
    signal output newState[2];
    
    newState[0] <== 2 * state[0] + 1 * state[1];
    newState[1] <== 1 * state[0] + 2 * state[1];
}

// 轮常数加法组件
template AddRoundConstants(t) {
    signal input state[t];
    signal input roundConstants[t];
    signal output newState[t];
    
    for (var i = 0; i < t; i++) {
        newState[i] <== state[i] + roundConstants[i];
    }
}

// Poseidon2 哈希函数 (t=3, d=5)
template Poseidon2_T3_D5() {
    // 参数定义
    var t = 3;          // 状态宽度
    var d = 5;          // S-box度数
    var rounds_f = 8;   // 完整轮数 (前半部分 + 后半部分)
    var rounds_p = 56;  // 部分轮数 (根据论文Table 1)
    var total_rounds = rounds_f + rounds_p;
    
    // 输入输出信号
    signal input inputs[2];  // 输入: 2个字段元素
    signal output out;       // 输出: 哈希值
    
    // 内部状态
    signal state[total_rounds + 1][t];
    
    // 轮常数 (简化版本, 实际需要从论文规范生成)
    var round_constants[total_rounds][t];
    
    // 初始化轮常数 (这里使用简化版本)
    for (var round = 0; round < total_rounds; round++) {
        for (var i = 0; i < t; i++) {
            round_constants[round][i] = (round * t + i + 1) * 123456789;
        }
    }
    
    // 初始化状态
    state[0][0] <== inputs[0];
    state[0][1] <== inputs[1];
    state[0][2] <== 0;  // 填充0
    
    // 组件实例化
    component addRC[total_rounds];
    component sbox[total_rounds][t];
    component mds[total_rounds];
    
    // 主循环
    for (var round = 0; round < total_rounds; round++) {
        // 步骤1: 添加轮常数
        addRC[round] = AddRoundConstants(t);
        for (var i = 0; i < t; i++) {
            addRC[round].state[i] <== state[round][i];
            addRC[round].roundConstants[i] <== round_constants[round][i];
        }
        
        // 步骤2: S-box层
        if (round < rounds_f / 2 || round >= rounds_f / 2 + rounds_p) {
            // 完整轮: 对所有状态元素应用S-box
            for (var i = 0; i < t; i++) {
                sbox[round][i] = SBox5();
                sbox[round][i].in <== addRC[round].newState[i];
            }
        } else {
            // 部分轮: 只对第一个状态元素应用S-box
            sbox[round][0] = SBox5();
            sbox[round][0].in <== addRC[round].newState[0];
            
            // 其他元素保持不变
            for (var i = 1; i < t; i++) {
                sbox[round][i] = SBox5();
                sbox[round][i].in <== addRC[round].newState[i];
                sbox[round][i].out <== addRC[round].newState[i];
            }
        }
        
        // 步骤3: MDS矩阵乘法
        mds[round] = MDS3x3();
        for (var i = 0; i < t; i++) {
            mds[round].state[i] <== sbox[round][i].out;
        }
        
        // 更新状态
        for (var i = 0; i < t; i++) {
            state[round + 1][i] <== mds[round].newState[i];
        }
    }
    
    // 输出最终状态的第一个元素
    out <== state[total_rounds][0];
}

// Poseidon2 哈希函数 (t=2, d=5)
template Poseidon2_T2_D5() {
    var t = 2;
    var d = 5;
    var rounds_f = 8;
    var rounds_p = 56;
    var total_rounds = rounds_f + rounds_p;
    
    signal input inputs[1];
    signal output out;
    
    signal state[total_rounds + 1][t];
    
    var round_constants[total_rounds][t];
    for (var round = 0; round < total_rounds; round++) {
        for (var i = 0; i < t; i++) {
            round_constants[round][i] = (round * t + i + 1) * 123456789;
        }
    }
    
    // 初始化状态
    state[0][0] <== inputs[0];
    state[0][1] <== 0;
    
    component addRC[total_rounds];
    component sbox[total_rounds][t];
    component mds[total_rounds];
    
    for (var round = 0; round < total_rounds; round++) {
        addRC[round] = AddRoundConstants(t);
        for (var i = 0; i < t; i++) {
            addRC[round].state[i] <== state[round][i];
            addRC[round].roundConstants[i] <== round_constants[round][i];
        }
        
        if (round < rounds_f / 2 || round >= rounds_f / 2 + rounds_p) {
            for (var i = 0; i < t; i++) {
                sbox[round][i] = SBox5();
                sbox[round][i].in <== addRC[round].newState[i];
            }
        } else {
            sbox[round][0] = SBox5();
            sbox[round][0].in <== addRC[round].newState[0];
            
            for (var i = 1; i < t; i++) {
                sbox[round][i] = SBox5();
                sbox[round][i].in <== addRC[round].newState[i];
                sbox[round][i].out <== addRC[round].newState[i];
            }
        }
        
        mds[round] = MDS2x2();
        for (var i = 0; i < t; i++) {
            mds[round].state[i] <== sbox[round][i].out;
        }
        
        for (var i = 0; i < t; i++) {
            state[round + 1][i] <== mds[round].newState[i];
        }
    }
    
    out <== state[total_rounds][0];
}

// 主电路: 验证Poseidon2哈希原象的零知识证明
template Poseidon2ProofT3() {
    // 公开输入: 哈希值
    signal input hash;
    
    // 私有输入: 哈希原象
    signal private input preimage[2];
    
    // 计算哈希值
    component hasher = Poseidon2_T3_D5();
    hasher.inputs[0] <== preimage[0];
    hasher.inputs[1] <== preimage[1];
    
    // 验证哈希值匹配
    hash === hasher.out;
}

// 主电路: 验证Poseidon2哈希原象的零知识证明 (t=2版本)
template Poseidon2ProofT2() {
    signal input hash;
    signal private input preimage[1];
    
    component hasher = Poseidon2_T2_D5();
    hasher.inputs[0] <== preimage[0];
    
    hash === hasher.out;
}

// 实例化主电路 (默认使用t=3版本)
component main = Poseidon2ProofT3();
