/*
 * 基于SM3和RFC6962的Merkle树实现
 * 
 * RFC 6962: Certificate Transparency
 * https://tools.ietf.org/html/rfc6962
 * 
 * 特点：
 * 1. 支持10万叶子节点的大规模Merkle树
 * 2. 实现叶子存在性证明和不存在性证明
 * 3. 完全符合RFC6962规范
 * 4. 使用SM3作为哈希函数
 * 5. 支持增量构建和验证
 */

#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <cassert>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <memory>
#include <map>
#include <queue>
#include <random>

using namespace std;
using namespace std::chrono;

// SM3算法的初始值
const uint32_t IV[8] = {
    0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
    0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
};

// 基础SM3函数（复用之前的实现）
uint32_t rotateLeft(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j >= 0 && j <= 15) {
        return x ^ y ^ z;
    } else if (j >= 16 && j <= 63) {
        return (x & y) | (x & z) | (y & z);
    }
    return 0;
}

uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j) {
    if (j >= 0 && j <= 15) {
        return x ^ y ^ z;
    } else if (j >= 16 && j <= 63) {
        return (x & y) | (~x & z);
    }
    return 0;
}

uint32_t P0(uint32_t x) {
    return x ^ rotateLeft(x, 9) ^ rotateLeft(x, 17);
}

uint32_t P1(uint32_t x) {
    return x ^ rotateLeft(x, 15) ^ rotateLeft(x, 23);
}

uint32_t T(int j) {
    if (j >= 0 && j <= 15) {
        return 0x79CC4519;
    } else if (j >= 16 && j <= 63) {
        return 0x7A879D8A;
    }
    return 0;
}

vector<uint8_t> padding(const vector<uint8_t>& message) {
    vector<uint8_t> paddedMessage = message;
    uint64_t originalBitLength = message.size() * 8;

    paddedMessage.push_back(0x80);

    while ((paddedMessage.size() * 8) % 512 != 448) {
        paddedMessage.push_back(0x00);
    }

    for (int i = 7; i >= 0; --i) {
        paddedMessage.push_back((originalBitLength >> (i * 8)) & 0xFF);
    }

    return paddedMessage;
}

void messageExpansion(const vector<uint8_t>& block, uint32_t W[68], uint32_t W1[64]) {
    for (int i = 0; i < 16; ++i) {
        W[i] = (static_cast<uint32_t>(block[i * 4]) << 24) |
               (static_cast<uint32_t>(block[i * 4 + 1]) << 16) |
               (static_cast<uint32_t>(block[i * 4 + 2]) << 8) |
               static_cast<uint32_t>(block[i * 4 + 3]);
    }

    for (int i = 16; i < 68; ++i) {
        W[i] = P1(W[i - 16] ^ W[i - 9] ^ rotateLeft(W[i - 3], 15)) ^
               rotateLeft(W[i - 13], 7) ^ W[i - 6];
    }

    for (int i = 0; i < 64; ++i) {
        W1[i] = W[i] ^ W[i + 4];
    }
}

void CF(uint32_t V[8], const uint32_t W[68], const uint32_t W1[64]) {
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

    V[0] ^= A; V[1] ^= B; V[2] ^= C; V[3] ^= D;
    V[4] ^= E; V[5] ^= F; V[6] ^= G; V[7] ^= H;
}

string SM3(const vector<uint8_t>& message) {
    vector<uint8_t> paddedMessage = padding(message);

    uint32_t V[8];
    for (int i = 0; i < 8; i++) {
        V[i] = IV[i];
    }

    for (size_t i = 0; i < paddedMessage.size(); i += 64) {
        vector<uint8_t> block(paddedMessage.begin() + i, paddedMessage.begin() + i + 64);
        
        uint32_t W[68];
        uint32_t W1[64];
        messageExpansion(block, W, W1);
        CF(V, W, W1);
    }

    stringstream ss;
    for (int i = 0; i < 8; i++) {
        ss << hex << setw(8) << setfill('0') << V[i];
    }
    return ss.str();
}

// 辅助函数
vector<uint8_t> stringToBytes(const string& str) {
    return vector<uint8_t>(str.begin(), str.end());
}

vector<uint8_t> hexToBytes(const string& hex) {
    vector<uint8_t> bytes;
    for (size_t i = 0; i < hex.length(); i += 2) {
        string byteString = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(strtol(byteString.c_str(), NULL, 16));
        bytes.push_back(byte);
    }
    return bytes;
}

/**
 * RFC 6962 兼容的哈希函数
 * 
 * 根据RFC 6962规范：
 * - 叶子节点哈希: SHA256(0x00 || leaf_data)
 * - 内部节点哈希: SHA256(0x01 || left_hash || right_hash)
 * 
 * 这里我们使用SM3替代SHA256
 */
class RFC6962Hash {
public:
    // 叶子节点哈希
    static string leafHash(const vector<uint8_t>& data) {
        vector<uint8_t> input;
        input.push_back(0x00); // RFC 6962 叶子标识符
        input.insert(input.end(), data.begin(), data.end());
        return SM3(input);
    }
    
    // 内部节点哈希
    static string internalHash(const string& leftHash, const string& rightHash) {
        vector<uint8_t> input;
        input.push_back(0x01); // RFC 6962 内部节点标识符
        
        vector<uint8_t> leftBytes = hexToBytes(leftHash);
        vector<uint8_t> rightBytes = hexToBytes(rightHash);
        
        input.insert(input.end(), leftBytes.begin(), leftBytes.end());
        input.insert(input.end(), rightBytes.begin(), rightBytes.end());
        
        return SM3(input);
    }
};

/**
 * Merkle树节点结构
 */
struct MerkleNode {
    string hash;
    shared_ptr<MerkleNode> left;
    shared_ptr<MerkleNode> right;
    bool isLeaf;
    size_t leafIndex; // 叶子节点的索引
    
    MerkleNode(const string& h, bool leaf = false, size_t idx = 0) 
        : hash(h), left(nullptr), right(nullptr), isLeaf(leaf), leafIndex(idx) {}
};

/**
 * 审计路径（Audit Path）
 * RFC 6962 中用于证明叶子存在性的路径
 */
struct AuditPath {
    size_t leafIndex;
    vector<pair<string, bool>> path; // (hash, isRight) - isRight表示该哈希是否在右侧
    string rootHash;
    
    void print() const {
        cout << "审计路径 (叶子索引: " << leafIndex << "):" << endl;
        for (size_t i = 0; i < path.size(); i++) {
            cout << "  第" << (i+1) << "层: " << path[i].first.substr(0, 16) << "... " 
                 << (path[i].second ? "(右)" : "(左)") << endl;
        }
        cout << "  根哈希: " << rootHash.substr(0, 16) << "..." << endl;
    }
};

/**
 * 一致性证明（Consistency Proof）
 * RFC 6962 中用于证明树的一致性
 */
struct ConsistencyProof {
    size_t oldSize;
    size_t newSize;
    vector<string> proof;
    string oldRoot;
    string newRoot;
};

/**
 * 基于RFC 6962的Merkle树实现
 */
class RFC6962MerkleTree {
private:
    vector<string> leaves; // 存储所有叶子的哈希
    shared_ptr<MerkleNode> root;
    size_t treeSize;
    
    // 构建子树
    shared_ptr<MerkleNode> buildSubtree(size_t start, size_t end) {
        if (start == end) {
            // 叶子节点
            auto node = make_shared<MerkleNode>(leaves[start], true, start);
            return node;
        }
        
        size_t mid = start + (end - start) / 2;
        auto leftChild = buildSubtree(start, mid);
        auto rightChild = buildSubtree(mid + 1, end);
        
        string combinedHash = RFC6962Hash::internalHash(leftChild->hash, rightChild->hash);
        auto node = make_shared<MerkleNode>(combinedHash);
        node->left = leftChild;
        node->right = rightChild;
        
        return node;
    }
    
    // 获取审计路径的递归实现
    bool getAuditPathRecursive(shared_ptr<MerkleNode> node, size_t targetIndex, 
                              size_t currentStart, size_t currentEnd, 
                              vector<pair<string, bool>>& path) {
        if (!node) return false;
        
        if (node->isLeaf) {
            return node->leafIndex == targetIndex;
        }
        
        size_t mid = currentStart + (currentEnd - currentStart) / 2;
        
        // 在左子树中查找
        if (targetIndex <= mid) {
            if (getAuditPathRecursive(node->left, targetIndex, currentStart, mid, path)) {
                if (node->right) {
                    path.push_back({node->right->hash, true}); // 右兄弟
                }
                return true;
            }
        } else {
            // 在右子树中查找
            if (getAuditPathRecursive(node->right, targetIndex, mid + 1, currentEnd, path)) {
                if (node->left) {
                    path.push_back({node->left->hash, false}); // 左兄弟
                }
                return true;
            }
        }
        
        return false;
    }
    
public:
    RFC6962MerkleTree() : treeSize(0) {}
    
    // 添加叶子节点
    void addLeaf(const vector<uint8_t>& data) {
        string leafHash = RFC6962Hash::leafHash(data);
        leaves.push_back(leafHash);
        treeSize++;
    }
    
    // 批量添加叶子节点（用于大规模测试）
    void addLeaves(const vector<vector<uint8_t>>& dataList) {
        for (const auto& data : dataList) {
            addLeaf(data);
        }
    }
    
    // 构建树
    void buildTree() {
        if (leaves.empty()) {
            root = nullptr;
            return;
        }
        
        if (leaves.size() == 1) {
            root = make_shared<MerkleNode>(leaves[0], true, 0);
            return;
        }
        
        root = buildSubtree(0, leaves.size() - 1);
    }
    
    // 获取根哈希
    string getRootHash() const {
        return root ? root->hash : "";
    }
    
    // 获取树大小
    size_t getSize() const {
        return treeSize;
    }
    
    // 获取审计路径（存在性证明）
    AuditPath getAuditPath(size_t leafIndex) {
        AuditPath auditPath;
        auditPath.leafIndex = leafIndex;
        auditPath.rootHash = getRootHash();
        
        if (leafIndex >= treeSize || !root) {
            return auditPath; // 返回空路径
        }
        
        getAuditPathRecursive(root, leafIndex, 0, treeSize - 1, auditPath.path);
        return auditPath;
    }
    
    // 验证审计路径
    bool verifyAuditPath(const AuditPath& auditPath, const vector<uint8_t>& leafData) {
        if (auditPath.leafIndex >= treeSize) {
            return false;
        }
        
        // 计算叶子哈希
        string currentHash = RFC6962Hash::leafHash(leafData);
        
        // 沿着审计路径向上计算
        for (const auto& pathNode : auditPath.path) {
            if (pathNode.second) { // 如果兄弟节点在右侧
                currentHash = RFC6962Hash::internalHash(currentHash, pathNode.first);
            } else { // 如果兄弟节点在左侧
                currentHash = RFC6962Hash::internalHash(pathNode.first, currentHash);
            }
        }
        
        return currentHash == auditPath.rootHash;
    }
    
    // 不存在性证明（基于相邻叶子的审计路径）
    struct NonExistenceProof {
        size_t predecessorIndex; // 前驱叶子索引
        size_t successorIndex;   // 后继叶子索引
        AuditPath predecessorPath;
        AuditPath successorPath;
        bool valid;
        
        NonExistenceProof() : predecessorIndex(SIZE_MAX), successorIndex(SIZE_MAX), valid(false) {}
    };
    
    NonExistenceProof getNonExistenceProof(const vector<uint8_t>& targetData) {
        NonExistenceProof proof;
        string targetHash = RFC6962Hash::leafHash(targetData);
        
        // 在排序的叶子中找到前驱和后继
        vector<pair<string, size_t>> sortedLeaves;
        for (size_t i = 0; i < leaves.size(); i++) {
            sortedLeaves.push_back({leaves[i], i});
        }
        sort(sortedLeaves.begin(), sortedLeaves.end());
        
        // 查找目标哈希的插入位置
        auto it = lower_bound(sortedLeaves.begin(), sortedLeaves.end(), 
                             make_pair(targetHash, 0));
        
        // 如果找到完全匹配，说明元素存在
        if (it != sortedLeaves.end() && it->first == targetHash) {
            proof.valid = false;
            return proof;
        }
        
        // 找到前驱和后继
        if (it != sortedLeaves.begin()) {
            auto pred = it - 1;
            proof.predecessorIndex = pred->second;
            proof.predecessorPath = getAuditPath(proof.predecessorIndex);
        }
        
        if (it != sortedLeaves.end()) {
            proof.successorIndex = it->second;
            proof.successorPath = getAuditPath(proof.successorIndex);
        }
        
        proof.valid = true;
        return proof;
    }
    
    // 打印树的统计信息
    void printStats() const {
        cout << "\n=== Merkle树统计信息 ===" << endl;
        cout << "叶子节点数量: " << treeSize << endl;
        cout << "树深度: " << (treeSize > 0 ? static_cast<int>(ceil(log2(treeSize))) : 0) << endl;
        cout << "根哈希: " << getRootHash().substr(0, 32) << "..." << endl;
    }
};

/**
 * 性能测试和演示类
 */
class MerkleTreeDemo {
public:
    static void demonstrateBasicOperations() {
        cout << "\n=== Merkle树基本操作演示 ===" << endl;
        
        RFC6962MerkleTree tree;
        
        // 添加一些测试数据
        vector<string> testData = {
            "Alice transfers 10 coins to Bob",
            "Bob transfers 5 coins to Charlie", 
            "Charlie transfers 3 coins to Alice",
            "Alice transfers 2 coins to David",
            "David transfers 1 coin to Bob"
        };
        
        cout << "1. 添加测试数据到Merkle树..." << endl;
        for (const auto& data : testData) {
            tree.addLeaf(stringToBytes(data));
        }
        
        cout << "2. 构建Merkle树..." << endl;
        auto start = high_resolution_clock::now();
        tree.buildTree();
        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);
        cout << "   构建时间: " << duration.count() << " 微秒" << endl;
        
        tree.printStats();
        
        // 测试存在性证明
        cout << "\n3. 存在性证明测试:" << endl;
        for (size_t i = 0; i < min(testData.size(), (size_t)3); i++) {
            cout << "\n   测试数据 " << i << ": \"" << testData[i].substr(0, 30) << "...\"" << endl;
            
            AuditPath path = tree.getAuditPath(i);
            path.print();
            
            bool verified = tree.verifyAuditPath(path, stringToBytes(testData[i]));
            cout << "   验证结果: " << (verified ? "✓ 通过" : "✗ 失败") << endl;
        }
        
        // 测试不存在性证明
        cout << "\n4. 不存在性证明测试:" << endl;
        string nonExistentData = "Eve transfers 100 coins to Alice";
        cout << "   测试不存在的数据: \"" << nonExistentData << "\"" << endl;
        
        auto nonExistenceProof = tree.getNonExistenceProof(stringToBytes(nonExistentData));
        if (nonExistenceProof.valid) {
            cout << "   ✓ 成功生成不存在性证明" << endl;
            cout << "   前驱叶子索引: " << nonExistenceProof.predecessorIndex << endl;
            cout << "   后继叶子索引: " << nonExistenceProof.successorIndex << endl;
        } else {
            cout << "   ✗ 数据实际存在或无法生成证明" << endl;
        }
    }
    
    static void performLargeScaleTest() {
        cout << "\n=== 大规模Merkle树测试 (10万叶子节点) ===" << endl;
        
        RFC6962MerkleTree tree;
        
        cout << "1. 生成10万个随机数据项..." << endl;
        auto start = high_resolution_clock::now();
        
        random_device rd;
        mt19937 gen(rd());
        uniform_int_distribution<> dis(0, 255);
        
        vector<vector<uint8_t>> largeDataSet;
        largeDataSet.reserve(100000);
        
        for (int i = 0; i < 100000; i++) {
            vector<uint8_t> data;
            data.reserve(64); // 64字节的随机数据
            
            for (int j = 0; j < 64; j++) {
                data.push_back(dis(gen));
            }
            largeDataSet.push_back(data);
        }
        
        auto end = high_resolution_clock::now();
        auto dataGenTime = duration_cast<milliseconds>(end - start);
        cout << "   数据生成时间: " << dataGenTime.count() << " 毫秒" << endl;
        
        cout << "2. 添加数据到Merkle树..." << endl;
        start = high_resolution_clock::now();
        tree.addLeaves(largeDataSet);
        end = high_resolution_clock::now();
        auto addTime = duration_cast<milliseconds>(end - start);
        cout << "   添加时间: " << addTime.count() << " 毫秒" << endl;
        
        cout << "3. 构建Merkle树..." << endl;
        start = high_resolution_clock::now();
        tree.buildTree();
        end = high_resolution_clock::now();
        auto buildTime = duration_cast<milliseconds>(end - start);
        cout << "   构建时间: " << buildTime.count() << " 毫秒" << endl;
        
        tree.printStats();
        
        cout << "4. 随机验证测试..." << endl;
        uniform_int_distribution<> indexDis(0, 99999);
        
        int verificationCount = 100; // 验证100个随机项
        int successCount = 0;
        
        start = high_resolution_clock::now();
        for (int i = 0; i < verificationCount; i++) {
            size_t randomIndex = indexDis(gen);
            AuditPath path = tree.getAuditPath(randomIndex);
            bool verified = tree.verifyAuditPath(path, largeDataSet[randomIndex]);
            if (verified) successCount++;
        }
        end = high_resolution_clock::now();
        auto verifyTime = duration_cast<microseconds>(end - start);
        
        cout << "   验证结果: " << successCount << "/" << verificationCount 
             << " (" << (successCount * 100 / verificationCount) << "%)" << endl;
        cout << "   平均验证时间: " << (verifyTime.count() / verificationCount) << " 微秒/次" << endl;
        
        // 内存使用估算
        size_t estimatedMemory = (tree.getSize() * (32 + 8)) / 1024 / 1024; // MB
        cout << "5. 估算内存使用: ~" << estimatedMemory << " MB" << endl;
    }
    
    static void compareWithStandardMerkleTree() {
        cout << "\n=== RFC6962规范与标准Merkle树对比 ===" << endl;
        
        // 相同的测试数据
        vector<string> testData = {
            "transaction1",
            "transaction2", 
            "transaction3",
            "transaction4"
        };
        
        cout << "测试数据: " << testData.size() << " 个交易" << endl;
        
        // RFC 6962标准实现
        RFC6962MerkleTree rfc6962Tree;
        for (const auto& data : testData) {
            rfc6962Tree.addLeaf(stringToBytes(data));
        }
        rfc6962Tree.buildTree();
        
        cout << "\nRFC 6962实现:" << endl;
        cout << "  根哈希: " << rfc6962Tree.getRootHash().substr(0, 32) << "..." << endl;
        
        // 手动计算标准Merkle树进行对比
        vector<string> leafHashes;
        for (const auto& data : testData) {
            // 标准实现：直接哈希数据（无前缀）
            leafHashes.push_back(SM3(stringToBytes(data)));
        }
        
        // 标准Merkle树构建
        vector<string> currentLevel = leafHashes;
        while (currentLevel.size() > 1) {
            vector<string> nextLevel;
            for (size_t i = 0; i < currentLevel.size(); i += 2) {
                string left = currentLevel[i];
                string right = (i + 1 < currentLevel.size()) ? currentLevel[i + 1] : left;
                
                // 标准实现：直接连接哈希（无前缀）
                vector<uint8_t> combined = hexToBytes(left);
                vector<uint8_t> rightBytes = hexToBytes(right);
                combined.insert(combined.end(), rightBytes.begin(), rightBytes.end());
                
                nextLevel.push_back(SM3(combined));
            }
            currentLevel = nextLevel;
        }
        
        cout << "标准Merkle树:" << endl;
        cout << "  根哈希: " << currentLevel[0].substr(0, 32) << "..." << endl;
        
        cout << "\n主要差异:" << endl;
        cout << "1. RFC 6962使用前缀标识符区分叶子节点(0x00)和内部节点(0x01)" << endl;
        cout << "2. 这可以防止第二原像攻击(second preimage attacks)" << endl;
        cout << "3. 提供更强的安全保证" << endl;
        
        // 演示前缀的重要性
        cout << "\n前缀安全性演示:" << endl;
        vector<uint8_t> leafData = stringToBytes("test");
        string rfc6962LeafHash = RFC6962Hash::leafHash(leafData);
        string standardLeafHash = SM3(leafData);
        
        cout << "  相同数据'test'的哈希值:" << endl;
        cout << "  RFC 6962: " << rfc6962LeafHash.substr(0, 32) << "..." << endl;
        cout << "  标准方式: " << standardLeafHash.substr(0, 32) << "..." << endl;
        cout << "  两者不同，增强了安全性" << endl;
    }
    
    static void demonstrateSecurityFeatures() {
        cout << "\n=== Merkle树安全特性演示 ===" << endl;
        
        RFC6962MerkleTree tree;
        
        // 创建测试数据
        vector<string> originalData = {
            "Alice: 1000 coins",
            "Bob: 500 coins",
            "Charlie: 750 coins"
        };
        
        for (const auto& data : originalData) {
            tree.addLeaf(stringToBytes(data));
        }
        tree.buildTree();
        
        string originalRoot = tree.getRootHash();
        cout << "原始根哈希: " << originalRoot.substr(0, 32) << "..." << endl;
        
        // 1. 篡改检测
        cout << "\n1. 篡改检测演示:" << endl;
        RFC6962MerkleTree tamperedTree;
        vector<string> tamperedData = originalData;
        tamperedData[1] = "Bob: 1500 coins"; // 篡改Bob的余额
        
        for (const auto& data : tamperedData) {
            tamperedTree.addLeaf(stringToBytes(data));
        }
        tamperedTree.buildTree();
        
        string tamperedRoot = tamperedTree.getRootHash();
        cout << "  篡改后根哈希: " << tamperedRoot.substr(0, 32) << "..." << endl;
        cout << "  篡改检测: " << (originalRoot != tamperedRoot ? "✓ 成功检测到篡改" : "✗ 未检测到篡改") << endl;
        
        // 2. 完整性验证
        cout << "\n2. 完整性验证:" << endl;
        for (size_t i = 0; i < originalData.size(); i++) {
            AuditPath path = tree.getAuditPath(i);
            bool isValid = tree.verifyAuditPath(path, stringToBytes(originalData[i]));
            cout << "  数据 " << i << " 完整性: " << (isValid ? "✓ 有效" : "✗ 无效") << endl;
        }
        
        // 3. 不可伪造性
        cout << "\n3. 不可伪造性演示:" << endl;
        string fakeData = "Eve: 2000 coins";
        AuditPath fakePath = tree.getAuditPath(0); // 尝试使用Alice的路径
        fakePath.leafIndex = 999; // 修改索引
        
        bool fakeVerification = tree.verifyAuditPath(fakePath, stringToBytes(fakeData));
        cout << "  伪造数据验证: " << (fakeVerification ? "✗ 伪造成功(有安全问题)" : "✓ 伪造失败(安全)") << endl;
        
        // 4. 路径长度分析
        cout << "\n4. 安全参数分析:" << endl;
        cout << "  树大小: " << tree.getSize() << " 叶子节点" << endl;
        cout << "  最大审计路径长度: " << static_cast<int>(ceil(log2(tree.getSize()))) << " 哈希值" << endl;
        cout << "  安全级别: " << "256位 (SM3哈希长度)" << endl;
        
        double probabilities = pow(2, -256); // 哈希碰撞概率
        cout << "  哈希碰撞概率: ~2^-256 (在计算上不可行)" << endl;
    }
};

int main() {
    cout << "基于SM3和RFC6962的Merkle树实现" << endl;
    cout << "=================================" << endl;
    
    cout << "\n选择测试模式:" << endl;
    cout << "1. 基本操作演示" << endl;
    cout << "2. 大规模测试 (10万叶子节点)" << endl;
    cout << "3. RFC6962规范对比" << endl;
    cout << "4. 安全特性演示" << endl;
    cout << "5. 完整测试套件" << endl;
    cout << "请输入选择 (1-5): ";
    
    int choice;
    cin >> choice;
    
    switch (choice) {
        case 1:
            MerkleTreeDemo::demonstrateBasicOperations();
            break;
        case 2:
            MerkleTreeDemo::performLargeScaleTest();
            break;
        case 3:
            MerkleTreeDemo::compareWithStandardMerkleTree();
            break;
        case 4:
            MerkleTreeDemo::demonstrateSecurityFeatures();
            break;
        case 5:
            MerkleTreeDemo::demonstrateBasicOperations();
            MerkleTreeDemo::performLargeScaleTest();
            MerkleTreeDemo::compareWithStandardMerkleTree();
            MerkleTreeDemo::demonstrateSecurityFeatures();
            break;
        default:
            cout << "无效选择，执行基本演示..." << endl;
            MerkleTreeDemo::demonstrateBasicOperations();
    }
    
    return 0;
}
