#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <openssl/rand.h>
#include "sm2_signature_attack.h"

// 模拟著名的比特币创世区块消息
const char* GENESIS_BLOCK_MSG = "The Times 03/Jan/2009 Chancellor on brink of second bailout for banks";

// 模拟一些著名的比特币交易消息
const char* FAMOUS_BITCOIN_MSGS[] = {
    "Hal Finney transaction - first Bitcoin transaction",
    "Pizza transaction - 10000 BTC for two pizzas",
    "Satoshi's last known transaction",
    "Genesis block coinbase transaction"
};

// 中本聪签名伪造专门的POC验证
void satoshi_signature_forge_poc() {
    printf("=======================================================\n");
    printf("    中本聪数字签名伪造POC验证程序\n");
    printf("=======================================================\n");
    printf("警告：此程序仅用于学术研究和安全教育目的\n");
    printf("实际伪造Satoshi签名在密码学上是不可行的\n");
    printf("=======================================================\n\n");
    
    // 模拟中本聪的公钥（实际中本聪公钥是已知的）
    // 这里使用模拟的公钥进行演示
    unsigned char satoshi_mock_pubkey[65];
    RAND_bytes(satoshi_mock_pubkey, 65);
    satoshi_mock_pubkey[0] = 0x04; // 未压缩公钥标识
    
    printf("步骤1: 加载模拟的中本聪公钥\n");
    print_hex(satoshi_mock_pubkey, 65, "模拟的Satoshi公钥");
    
    printf("\n步骤2: 准备要伪造签名的消息\n");
    printf("选择要伪造签名的消息:\n");
    printf("1. %s\n", GENESIS_BLOCK_MSG);
    for (int i = 0; i < 4; i++) {
        printf("%d. %s\n", i + 2, FAMOUS_BITCOIN_MSGS[i]);
    }
    
    int msg_choice;
    printf("请选择消息 (1-5): ");
    scanf("%d", &msg_choice);
    
    const char* target_message;
    if (msg_choice == 1) {
        target_message = GENESIS_BLOCK_MSG;
    } else if (msg_choice >= 2 && msg_choice <= 5) {
        target_message = FAMOUS_BITCOIN_MSGS[msg_choice - 2];
    } else {
        printf("无效选择，使用默认消息\n");
        target_message = GENESIS_BLOCK_MSG;
    }
    
    printf("目标消息: %s\n", target_message);
    
    // 准备伪造数据
    SATOSHI_FORGE_DATA forge_data;
    memcpy(forge_data.satoshi_pubkey, satoshi_mock_pubkey, 65);
    strncpy((char*)forge_data.target_message, target_message, sizeof(forge_data.target_message) - 1);
    forge_data.message_len = strlen(target_message);
    
    printf("\n步骤3: 尝试伪造签名\n");
    printf("开始伪造攻击，这可能需要一些时间...\n");
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    SM2_ATTACK_RESULT result;
    int attack_success = sm2_forge_satoshi_signature(&forge_data, &result);
    
    gettimeofday(&end, NULL);
    double total_time = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
    
    printf("\n步骤4: 攻击结果分析\n");
    print_attack_result(&result, ATTACK_SATOSHI_SIGNATURE);
    
    printf("步骤5: 安全性分析\n");
    printf("----------------------------------------\n");
    printf("总攻击时间: %.6f 秒\n", total_time);
    printf("攻击成功率: %.2f%%\n", result.success ? 100.0 : 0.0);
    printf("理论分析:\n");
    printf("- SM2签名基于椭圆曲线离散对数问题(ECDLP)\n");
    printf("- 当前最好的ECDLP求解算法需要指数时间\n");
    printf("- 256位椭圆曲线的安全强度约为128位\n");
    printf("- 伪造一个有效签名需要约2^128次运算\n");
    printf("- 即使使用全球最快的超级计算机，也需要数十亿年\n");
    
    if (!result.success) {
        printf("\n✓ 攻击失败证明了SM2算法的安全性\n");
        printf("✓ 实际中伪造Satoshi签名是不可行的\n");
    } else {
        printf("\n⚠ 注意：如果攻击显示成功，这只是演示性的\n");
        printf("⚠ 实际的密码学攻击不会如此简单成功\n");
    }
    
    printf("\n步骤6: 防护建议\n");
    printf("----------------------------------------\n");
    printf("1. 密钥生成安全性:\n");
    printf("   - 使用密码学安全的随机数生成器\n");
    printf("   - 确保私钥具有足够的熵(256位)\n");
    printf("   - 私钥生成过程要防止侧信道攻击\n\n");
    
    printf("2. 签名过程安全性:\n");
    printf("   - 每次签名使用新的随机数k\n");
    printf("   - k值必须是密码学安全的随机数\n");
    printf("   - 避免k值的任何模式或偏置\n\n");
    
    printf("3. 实现安全性:\n");
    printf("   - 使用经过验证的密码学库\n");
    printf("   - 避免时间、功耗侧信道泄露\n");
    printf("   - 定期进行安全审计\n\n");
}

// 具体的中本聪签名分析
void analyze_satoshi_signature_security() {
    printf("\n=== 中本聪签名安全性深度分析 ===\n");
    
    printf("1. 历史背景:\n");
    printf("   - 中本聪在比特币早期进行了多次交易\n");
    printf("   - 这些交易的签名使用ECDSA算法(secp256k1曲线)\n");
    printf("   - 中本聪的公钥是公开的，但私钥未知\n\n");
    
    printf("2. 攻击可能性分析:\n");
    printf("   a) 暴力攻击:\n");
    printf("      - 需要尝试2^256个私钥\n");
    printf("      - 时间复杂度: O(2^128) (生日攻击)\n");
    printf("      - 当前技术下不可行\n\n");
    
    printf("   b) 量子计算攻击:\n");
    printf("      - Shor算法可以在多项式时间内解决ECDLP\n");
    printf("      - 需要约2330个逻辑量子比特\n");
    printf("      - 当前量子计算机还远未达到此能力\n\n");
    
    printf("   c) 密码学弱点攻击:\n");
    printf("      - 如果随机数生成器有缺陷\n");
    printf("      - 如果签名过程有侧信道泄露\n");
    printf("      - 目前没有发现此类问题\n\n");
    
    printf("3. SM2 vs ECDSA比较:\n");
    printf("   - SM2使用256位素数域曲线\n");
    printf("   - ECDSA(secp256k1)使用特殊的Koblitz曲线\n");
    printf("   - 两者都基于ECDLP困难性假设\n");
    printf("   - 安全强度相当(约128位)\n\n");
    
    printf("4. 现实中的攻击向量:\n");
    printf("   - 社会工程学攻击\n");
    printf("   - 物理攻击获取私钥\n");
    printf("   - 软件漏洞利用\n");
    printf("   - 这些都不涉及密码学破解\n\n");
    
    printf("结论: 从纯密码学角度，伪造中本聪签名在当前技术下是不可行的。\n");
}

// 演示不同攻击策略的理论分析
void demonstrate_attack_strategies() {
    printf("\n=== 签名伪造攻击策略分析 ===\n");
    
    printf("策略1: 存在性伪造攻击\n");
    printf("- 目标：为任意消息生成有效签名\n");
    printf("- 方法：随机选择r,s，计算对应的消息\n");
    printf("- 限制：无法为指定消息生成签名\n");
    printf("- 实用性：很低\n\n");
    
    printf("策略2: 选择性伪造攻击\n");
    printf("- 目标：为指定消息生成有效签名\n");
    printf("- 方法：解决椭圆曲线离散对数问题\n");
    printf("- 难度：指数级时间复杂度\n");
    printf("- 实用性：当前不可行\n\n");
    
    printf("策略3: 通用性伪造攻击\n");
    printf("- 目标：恢复私钥，为任意消息签名\n");
    printf("- 方法：暴力搜索私钥空间\n");
    printf("- 难度：2^256次尝试\n");
    printf("- 实用性：完全不可行\n\n");
    
    printf("策略4: 实现层面攻击\n");
    printf("- 目标：利用实现缺陷\n");
    printf("- 方法：侧信道分析、故障注入等\n");
    printf("- 效果：可能有效\n");
    printf("- 防护：安全实现、硬件保护\n\n");
}

int main() {
    printf("中本聪数字签名伪造POC验证程序\n");
    printf("====================================\n\n");
    
    int choice;
    while (1) {
        printf("请选择操作:\n");
        printf("1. 执行中本聪签名伪造POC\n");
        printf("2. 中本聪签名安全性深度分析\n");
        printf("3. 攻击策略理论分析\n");
        printf("4. 运行完整分析\n");
        printf("0. 退出\n");
        printf("请选择 (0-4): ");
        
        if (scanf("%d", &choice) != 1) {
            printf("无效输入\n");
            while (getchar() != '\n');
            continue;
        }
        
        switch (choice) {
            case 1:
                satoshi_signature_forge_poc();
                break;
            case 2:
                analyze_satoshi_signature_security();
                break;
            case 3:
                demonstrate_attack_strategies();
                break;
            case 4:
                satoshi_signature_forge_poc();
                analyze_satoshi_signature_security();
                demonstrate_attack_strategies();
                break;
            case 0:
                printf("程序结束\n");
                return 0;
            default:
                printf("无效选择\n");
        }
        
        printf("\n按Enter继续...");
        while (getchar() != '\n');
        getchar();
    }
    
    return 0;
}
