/*
 * SM3 Length Extension Attack 验证实现
 * 
 * Length Extension Attack 是针对 Merkle-Damgård 结构哈希函数的一种攻击方式
 * SM3 采用 Merkle-Damgård 结构，因此理论上存在此类攻击
 * 
 * 攻击原理：
 * 1. 已知 H(M) 和 |M|，攻击者可以计算 H(M || padding || M')
 * 2. 不需要知道原始消息 M 的内容
 * 3. 只需要知道 M 的长度和哈希值
 */

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cassert>

using namespace std;

// SM3算法的初始值
const uint32_t IV[8] = {
    0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
    0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
};

// 基础函数声明（从原sm3.cpp中复用）
uint32_t rotateLeft(uint32_t x, int n);
uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j);
uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j);
uint32_t P0(uint32_t x);
uint32_t P1(uint32_t x);
uint32_t T(int j);
void messageExpansion(const vector<uint8_t>& block, uint32_t W[68], uint32_t W1[64], bool verbose = false);
void CF(uint32_t V[8], const uint32_t W[68], const uint32_t W1[64], bool verbose = false);

// 循环左移
uint32_t rotateLeft(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// 布尔函数FF
uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j >= 0 && j <= 15) {
        return x ^ y ^ z;
    } else if (j >= 16 && j <= 63) {
        return (x & y) | (x & z) | (y & z);
    }
    return 0;
}

// 布尔函数GG
uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j >= 0 && j <= 15) {
        return x ^ y ^ z;
    } else if (j >= 16 && j <= 63) {
        return (x & y) | (~x & z);
    }
    return 0;
}

// 置换函数P0
uint32_t P0(uint32_t x) {
    return x ^ rotateLeft(x, 9) ^ rotateLeft(x, 17);
}

// 置换函数P1
uint32_t P1(uint32_t x) {
    return x ^ rotateLeft(x, 15) ^ rotateLeft(x, 23);
}

// 常量T
uint32_t T(int j) {
    if (j >= 0 && j <= 15) {
        return 0x79CC4519;
    } else if (j >= 16 && j <= 63) {
        return 0x7A879D8A;
    }
    return 0;
}

// 填充消息（标准SM3填充）
vector<uint8_t> padding(const vector<uint8_t>& message) {
    vector<uint8_t> paddedMessage = message;
    uint64_t originalBitLength = message.size() * 8;

    // 添加一个1位
    paddedMessage.push_back(0x80);

    // 填充0直到长度满足条件 (l + 1 + k) ≡ 448 (mod 512)
    while ((paddedMessage.size() * 8) % 512 != 448) {
        paddedMessage.push_back(0x00);
    }

    // 添加原始消息长度（64位，大端序）
    for (int i = 7; i >= 0; --i) {
        paddedMessage.push_back((originalBitLength >> (i * 8)) & 0xFF);
    }

    return paddedMessage;
}

// 消息扩展
void messageExpansion(const vector<uint8_t>& block, uint32_t W[68], uint32_t W1[64], bool verbose) {
    // 将消息分组为16个字（大端序）
    for (int i = 0; i < 16; ++i) {
        W[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
               (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
               static_cast<uint32_t>(block[i * 4 + 3]);
    }

    // 扩展消息
    for (int i = 16; i < 68; ++i) {
        W[i] = P1(W[i - 16] ^ W[i - 9] ^ rotateLeft(W[i - 3], 15)) ^
               rotateLeft(W[i - 13], 7) ^ W[i - 6];
    }

    // 计算W1
    for (int i = 0; i < 64; ++i) {
        W1[i] = W[i] ^ W[i + 4];
    }
}

// 压缩函数
void CF(uint32_t V[8], const uint32_t W[68], const uint32_t W1[64], bool verbose) {
    uint32_t A = V[0], B = V[1], C = V[2], D = V[3];
    uint32_t E = V[4], F = V[5], G = V[6], H = V[7];

    for (int j = 0; j < 64; ++j) {
        uint32_t SS1 = rotateLeft((rotateLeft(A, 12) + E + rotateLeft(T(j), j % 32)), 7);
        uint32_t SS2 = SS1 ^ rotateLeft(A, 12);
        uint32_t TT1 = FF(A, B, C, j) + D + SS2 + W1[j];
        uint32_t TT2 = GG(E, F, G, j) + H + SS1 + W[j];
        
        D = C;
        C = rotateLeft(B, 9);
        B = A;
        A = TT1;
        H = G;
        G = rotateLeft(F, 19);
        F = E;
        E = P0(TT2);
    }

    V[0] ^= A;
    V[1] ^= B;
    V[2] ^= C;
    V[3] ^= D;
    V[4] ^= E;
    V[5] ^= F;
    V[6] ^= G;
    V[7] ^= H;
}

// 标准SM3哈希函数
string SM3(const vector<uint8_t>& message, bool verbose = false) {
    vector<uint8_t> paddedMessage = padding(message);

    // 初始化杂凑值
    uint32_t V[8];
    for (int i = 0; i < 8; i++) {
        V[i] = IV[i];
    }

    // 分组处理
    for (size_t i = 0; i < paddedMessage.size(); i += 64) {
        vector<uint8_t> block(paddedMessage.begin() + i, paddedMessage.begin() + i + 64);
        
        uint32_t W[68];
        uint32_t W1[64];
        messageExpansion(block, W, W1, verbose);
        CF(V, W, W1, verbose);
    }

    // 输出最终的杂凑值
    stringstream ss;
    for (int i = 0; i < 8; i++) {
        ss << hex << setw(8) << setfill('0') << V[i];
    }
    return ss.str();
}

// Length Extension Attack 相关函数

/**
 * 获取消息填充后的长度
 * 用于计算攻击时需要的填充
 */
size_t getPaddedLength(size_t originalLength) {
    size_t bitLength = originalLength * 8;
    size_t paddedBitLength = bitLength + 1; // 添加'1'位
    
    // 填充到满足 (l + 1 + k) ≡ 448 (mod 512)
    while (paddedBitLength % 512 != 448) {
        paddedBitLength++;
    }
    
    // 加上64位长度字段
    paddedBitLength += 64;
    
    return paddedBitLength / 8; // 转换为字节
}

/**
 * 构造填充数据
 * 用于Length Extension Attack中的填充构造
 */
vector<uint8_t> constructPadding(size_t originalLength) {
    vector<uint8_t> padding_data;
    uint64_t originalBitLength = originalLength * 8;

    // 添加一个1位
    padding_data.push_back(0x80);

    // 填充0直到长度满足条件
    while ((originalLength + padding_data.size()) * 8 % 512 != 448) {
        padding_data.push_back(0x00);
    }

    // 添加原始消息长度（64位，大端序）
    for (int i = 7; i >= 0; --i) {
        padding_data.push_back((originalBitLength >> (i * 8)) & 0xFF);
    }

    return padding_data;
}

/**
 * 从十六进制字符串解析出SM3状态值
 */
vector<uint32_t> parseHashToState(const string& hash) {
    vector<uint32_t> state(8);
    for (int i = 0; i < 8; i++) {
        string substr = hash.substr(i * 8, 8);
        state[i] = stoul(substr, nullptr, 16);
    }
    return state;
}

/**
 * 使用给定的初始状态计算SM3哈希
 * 这是Length Extension Attack的核心函数
 */
string SM3_WithState(const vector<uint8_t>& message, const vector<uint32_t>& initialState) {
    vector<uint8_t> paddedMessage = padding(message);

    // 使用给定的初始状态而不是标准IV
    uint32_t V[8];
    for (int i = 0; i < 8; i++) {
        V[i] = initialState[i];
    }

    // 分组处理
    for (size_t i = 0; i < paddedMessage.size(); i += 64) {
        vector<uint8_t> block(paddedMessage.begin() + i, paddedMessage.begin() + i + 64);
        
        uint32_t W[68];
        uint32_t W1[64];
        messageExpansion(block, W, W1, false);
        CF(V, W, W1, false);
    }

    // 输出最终的杂凑值
    stringstream ss;
    for (int i = 0; i < 8; i++) {
        ss << hex << setw(8) << setfill('0') << V[i];
    }
    return ss.str();
}

/**
 * 执行Length Extension Attack
 * 
 * @param originalHash 原始消息的哈希值（十六进制字符串）
 * @param originalLength 原始消息的长度
 * @param appendMessage 要追加的消息
 * @return 扩展后消息的哈希值
 */
string performLengthExtensionAttack(const string& originalHash, size_t originalLength, 
                                  const vector<uint8_t>& appendMessage) {
    // 解析原始哈希值为状态
    vector<uint32_t> state = parseHashToState(originalHash);
    
    // 计算扩展消息的总长度（包括原始消息、填充和追加消息）
    vector<uint8_t> padding_data = constructPadding(originalLength);
    size_t extendedLength = originalLength + padding_data.size() + appendMessage.size();
    
    // 构造用于哈希计算的消息（只包含追加的消息部分）
    // 长度需要调整为总长度
    vector<uint8_t> messageForHash = appendMessage;
    
    // 创建一个临时消息来正确计算填充
    vector<uint8_t> totalMessage;
    // 模拟原始消息长度（用零填充）
    totalMessage.resize(originalLength, 0x00);
    totalMessage.insert(totalMessage.end(), padding_data.begin(), padding_data.end());
    totalMessage.insert(totalMessage.end(), appendMessage.begin(), appendMessage.end());
    
    // 使用修正的填充计算
    vector<uint8_t> finalPaddedMessage = padding(totalMessage);
    
    // 提取追加消息部分（从填充后开始）
    size_t startPos = originalLength + padding_data.size();
    vector<uint8_t> extractedMessage(finalPaddedMessage.begin() + startPos, finalPaddedMessage.end());
    
    return SM3_WithState(extractedMessage, state);
}

// 辅助函数：将字符串转换为字节向量
vector<uint8_t> stringToBytes(const string& str) {
    return vector<uint8_t>(str.begin(), str.end());
}

// 辅助函数：将字节向量转换为十六进制字符串
string bytesToHex(const vector<uint8_t>& bytes) {
    stringstream ss;
    for (uint8_t byte : bytes) {
        ss << hex << setw(2) << setfill('0') << static_cast<int>(byte);
    }
    return ss.str();
}

/**
 * Length Extension Attack 验证测试
 */
class LengthExtensionAttackVerification {
public:
    static void demonstrateAttack() {
        cout << "\n=== SM3 Length Extension Attack 演示 ===" << endl;
        
        // 步骤1：设置原始消息
        string originalMessage = "secret_key||user_data";
        vector<uint8_t> originalBytes = stringToBytes(originalMessage);
        string originalHash = SM3(originalBytes);
        
        cout << "1. 原始消息设置:" << endl;
        cout << "   消息: \"" << originalMessage << "\"" << endl;
        cout << "   长度: " << originalBytes.size() << " bytes" << endl;
        cout << "   哈希: " << originalHash << endl << endl;
        
        // 步骤2：攻击者信息（攻击者不知道消息内容，只知道长度和哈希）
        cout << "2. 攻击者已知信息:" << endl;
        cout << "   - 原始消息长度: " << originalBytes.size() << " bytes" << endl;
        cout << "   - 原始消息哈希: " << originalHash << endl;
        cout << "   - 不知道原始消息内容！" << endl << endl;
        
        // 步骤3：构造攻击
        string appendData = "||admin=true";
        vector<uint8_t> appendBytes = stringToBytes(appendData);
        
        cout << "3. 攻击过程:" << endl;
        cout << "   要追加的数据: \"" << appendData << "\"" << endl;
        
        // 计算填充
        vector<uint8_t> padding_data = constructPadding(originalBytes.size());
        cout << "   计算出的填充长度: " << padding_data.size() << " bytes" << endl;
        cout << "   填充数据: " << bytesToHex(padding_data).substr(0, 20) << "..." << endl;
        
        // 执行Length Extension Attack
        string attackHash = performLengthExtensionAttack(originalHash, originalBytes.size(), appendBytes);
        cout << "   攻击计算的哈希: " << attackHash << endl << endl;
        
        // 步骤4：验证攻击是否成功
        cout << "4. 验证攻击结果:" << endl;
        
        // 构造实际的扩展消息进行验证
        vector<uint8_t> fullExtendedMessage = originalBytes;
        fullExtendedMessage.insert(fullExtendedMessage.end(), padding_data.begin(), padding_data.end());
        fullExtendedMessage.insert(fullExtendedMessage.end(), appendBytes.begin(), appendBytes.end());
        
        string verificationHash = SM3(fullExtendedMessage);
        cout << "   实际扩展消息的哈希: " << verificationHash << endl;
        
        bool attackSuccess = (attackHash == verificationHash);
        cout << "   攻击结果: " << (attackSuccess ? "✓ 成功" : "✗ 失败") << endl;
        
        if (attackSuccess) {
            cout << "\n   ⚠️  Length Extension Attack 验证成功!" << endl;
            cout << "   攻击者在不知道原始消息内容的情况下，" << endl;
            cout << "   成功计算出了扩展消息的哈希值。" << endl;
        }
        
        // 步骤5：显示完整的攻击场景
        cout << "\n5. 攻击场景分析:" << endl;
        cout << "   原始完整消息: \"" << originalMessage << "\"" << endl;
        cout << "   扩展后的消息: \"" << originalMessage << "[填充数据]" << appendData << "\"" << endl;
        cout << "   扩展消息长度: " << fullExtendedMessage.size() << " bytes" << endl;
    }
    
    static void testMultipleCases() {
        cout << "\n=== 多种场景下的 Length Extension Attack 测试 ===" << endl;
        
        vector<pair<string, string>> testCases = {
            {"key1", "||admin=true"},
            {"short", "||role=admin"},
            {"a_longer_secret_key_for_testing", "||permission=all"},
            {"", "data"},  // 空消息测试
            {"single_byte", "x"}  // 单字节追加
        };
        
        int caseNum = 1;
        int successCount = 0;
        
        for (const auto& testCase : testCases) {
            cout << "\n--- 测试案例 " << caseNum++ << " ---" << endl;
            
            vector<uint8_t> originalBytes = stringToBytes(testCase.first);
            string originalHash = SM3(originalBytes);
            vector<uint8_t> appendBytes = stringToBytes(testCase.second);
            
            cout << "原始: \"" << testCase.first << "\" (长度: " << originalBytes.size() << ")" << endl;
            cout << "追加: \"" << testCase.second << "\"" << endl;
            
            // 执行攻击
            string attackHash = performLengthExtensionAttack(originalHash, originalBytes.size(), appendBytes);
            
            // 验证
            vector<uint8_t> padding_data = constructPadding(originalBytes.size());
            vector<uint8_t> fullMessage = originalBytes;
            fullMessage.insert(fullMessage.end(), padding_data.begin(), padding_data.end());
            fullMessage.insert(fullMessage.end(), appendBytes.begin(), appendBytes.end());
            
            string verificationHash = SM3(fullMessage);
            bool success = (attackHash == verificationHash);
            
            cout << "结果: " << (success ? "✓ 成功" : "✗ 失败") << endl;
            if (success) successCount++;
        }
        
        cout << "\n总结: " << successCount << "/" << testCases.size() 
             << " 个测试案例攻击成功 (" 
             << (successCount * 100 / testCases.size()) << "%)" << endl;
    }
    
    static void analyzeSecurityImplications() {
        cout << "\n=== SM3 Length Extension Attack 安全影响分析 ===" << endl;
        
        cout << "\n1. 攻击原理:" << endl;
        cout << "   - SM3采用Merkle-Damgård结构" << endl;
        cout << "   - 哈希计算是迭代的：H(m) = f(IV, m1) -> f(H1, m2) -> ..." << endl;
        cout << "   - 最终哈希值实际上是最后一轮的内部状态" << endl;
        cout << "   - 攻击者可以利用这个状态继续哈希计算" << endl;
        
        cout << "\n2. 攻击条件:" << endl;
        cout << "   - 已知消息长度" << endl;
        cout << "   - 已知消息哈希值" << endl;
        cout << "   - 不需要知道消息内容" << endl;
        
        cout << "\n3. 实际威胁场景:" << endl;
        cout << "   - MAC验证绕过: H(key||message) -> H(key||message||padding||evil_data)" << endl;
        cout << "   - 身份验证绕过: H(token||data) -> H(token||data||padding||admin=true)" << endl;
        cout << "   - 完整性校验绕过: 在不知道密钥的情况下伪造有效签名" << endl;
        
        cout << "\n4. 防御措施:" << endl;
        cout << "   - 使用HMAC而不是简单的H(key||message)" << endl;
        cout << "   - 采用更安全的哈希函数（如SHA-3/Keccak，使用海绵结构）" << endl;
        cout << "   - 在消息中添加长度字段进行验证" << endl;
        cout << "   - 使用专门的认证加密算法" << endl;
        
        cout << "\n5. SM3特有考虑:" << endl;
        cout << "   - SM3作为中国密码标准，在国产化环境中广泛使用" << endl;
        cout << "   - 在设计基于SM3的认证系统时必须考虑此攻击" << endl;
        cout << "   - 建议配合SM4等对称加密算法使用" << endl;
    }
};

int main() {
    cout << "SM3 Length Extension Attack 验证程序" << endl;
    cout << "=====================================" << endl;
    
    cout << "\n选择测试模式:" << endl;
    cout << "1. 演示基本的Length Extension Attack" << endl;
    cout << "2. 多场景测试" << endl;
    cout << "3. 安全影响分析" << endl;
    cout << "4. 完整测试套件" << endl;
    cout << "请输入选择 (1-4): ";
    
    int choice;
    cin >> choice;
    
    switch (choice) {
        case 1:
            LengthExtensionAttackVerification::demonstrateAttack();
            break;
        case 2:
            LengthExtensionAttackVerification::testMultipleCases();
            break;
        case 3:
            LengthExtensionAttackVerification::analyzeSecurityImplications();
            break;
        case 4:
            LengthExtensionAttackVerification::demonstrateAttack();
            LengthExtensionAttackVerification::testMultipleCases();
            LengthExtensionAttackVerification::analyzeSecurityImplications();
            break;
        default:
            cout << "无效选择，执行基本演示..." << endl;
            LengthExtensionAttackVerification::demonstrateAttack();
    }
    
    return 0;
}
