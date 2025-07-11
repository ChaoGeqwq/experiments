#include <iostream>
#include <vector>
#include <iomanip>
#include <sstream>
#include <cstring>

using namespace std;

// 常量初始化
const uint32_t IV[8] = {
    0x7380166F, 0x4914B2B9, 0x172442D7, 0xDA8A0600,
    0xA96F30BC, 0x163138AA, 0xE38DEE4D, 0xB0FB0E4E
};

const uint32_t T[64] = {
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x79CC4519, 0x79CC4519, 0x79CC4519, 0x79CC4519,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A,
    0x7A879D8A, 0x7A879D8A, 0x7A879D8A, 0x7A879D8A
};

// 循环左移
uint32_t rotateLeft(uint32_t x, int n) {
    return (x << n) | (x >> (32 - n));
}

// 布尔函数FF
uint32_t FF(uint32_t x, uint32_t y, uint32_t z, int j) {
    return (j < 16) ? (x ^ y ^ z) : ((x & y) | (x & z) | (y & z));
}

// 布尔函数GG
uint32_t GG(uint32_t x, uint32_t y, uint32_t z, int j) {
    return (j < 16) ? (x ^ y ^ z) : ((x & y) | (~x & z));
}

// 压缩函数P0
uint32_t P0(uint32_t x) {
    return x ^ rotateLeft(x, 9) ^ rotateLeft(x, 17);
}

// 压缩函数P1
uint32_t P1(uint32_t x) {
    return x ^ rotateLeft(x, 15) ^ rotateLeft(x, 23);
}

// 填充消息
vector<uint8_t> padding(const vector<uint8_t>& message) {
    vector<uint8_t> paddedMessage = message;
    size_t originalBitLength = message.size() * 8;

    // 添加一个1位
    paddedMessage.push_back(0x80);

    // 填充0直到长度满足条件
    while ((paddedMessage.size() * 8) % 512 != 448) {
        paddedMessage.push_back(0x00);
    }

    // 添加原始消息长度
    for (int i = 7; i >= 0; --i) {
        paddedMessage.push_back((originalBitLength >> (i * 8)) & 0xFF);
    }

    return paddedMessage;
}

// 消息扩展
vector<uint32_t> messageExpansion(const vector<uint8_t>& block) {
    vector<uint32_t> W(68);
    vector<uint32_t> W1(64);

    // 将消息分组为16个字
    for (int i = 0; i < 16; ++i) {
        W[i] = (block[i * 4] << 24) | (block[i * 4 + 1] << 16) |
               (block[i * 4 + 2] << 8) | block[i * 4 + 3];
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

    // 打印扩展后的消息
    cout << "扩展后的消息W: ";
    for (int i = 0; i < 68; ++i) {
        cout << hex << setw(8) << setfill('0') << W[i] << " ";
    }
    cout << endl;

    cout << "扩展后的消息W1: ";
    for (int i = 0; i < 64; ++i) {
        cout << hex << setw(8) << setfill('0') << W1[i] << " ";
    }
    cout << endl;

    return W;
}

// 压缩函数
void CF(vector<uint32_t>& V, const vector<uint32_t>& W) {
    uint32_t A = V[0], B = V[1], C = V[2], D = V[3];
    uint32_t E = V[4], F = V[5], G = V[6], H = V[7];

    vector<uint32_t> W1(64);
    // 计算W1
    for (int i = 0; i < 64; ++i) {
        W1[i] = W[i] ^ W[i + 4];
    }

    for (int j = 0; j < 64; ++j) {
        uint32_t SS1 = rotateLeft((rotateLeft(A, 12) + E + rotateLeft(T[j], j % 32)), 7);
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

        // 输出迭代压缩的中间值
        cout << "迭代压缩中间值 j=" << dec << j << ": ";
        cout << hex << setw(8) << setfill('0') << A << " "
             << hex << setw(8) << setfill('0') << B << " "
             << hex << setw(8) << setfill('0') << C << " "
             << hex << setw(8) << setfill('0') << D << " "
             << hex << setw(8) << setfill('0') << E << " "
             << hex << setw(8) << setfill('0') << F << " "
             << hex << setw(8) << setfill('0') << G << " "
             << hex << setw(8) << setfill('0') << H << endl;
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

// SM3哈希函数
string SM3(const vector<uint8_t>& message) {
    vector<uint8_t> paddedMessage = padding(message);

    // 打印填充后的消息
    cout << "填充后的消息: ";
    for (uint8_t byte : paddedMessage) {
        cout << hex << setw(2) << setfill('0') << (int)byte;
    }
    cout << endl;

    vector<uint32_t> V(IV, IV + 8);

    // 分组处理
    for (size_t i = 0; i < paddedMessage.size(); i += 64) {
        vector<uint8_t> block(paddedMessage.begin() + i, paddedMessage.begin() + i + 64);
        vector<uint32_t> W = messageExpansion(block);
        CF(V, W);
    }

    // 输出最终的杂凑值
    stringstream ss;
    for (uint32_t v : V) {
        ss << hex << setw(8) << setfill('0') << v;
    }
    return ss.str();
}

int main() {
    // 输入消息
    string input = "abc";
    vector<uint8_t> message(input.begin(), input.end());

    // 计算SM3哈希值
    string hash = SM3(message);

    // 输出最终的哈希值
    cout << "最终的杂凑值: " << hash << endl;

    return 0;
}