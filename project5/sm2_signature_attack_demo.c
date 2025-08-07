#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sm2_signature_attack.h"

// 主演示程序
int main() {
    printf("=================================================\n");
    printf("     SM2签名算法安全性分析与攻击演示\n");
    printf("=================================================\n");
    printf("本程序演示SM2签名算法的常见攻击方法，用于教育目的\n");
    printf("包括：k值重用攻击、弱k值攻击、中本聪签名伪造等\n");
    printf("=================================================\n\n");
    
    // 初始化随机数生成器
    srand(time(NULL));
    
    int choice;
    while (1) {
        printf("\n请选择要演示的攻击类型:\n");
        printf("1. K值重用攻击 (K-Reuse Attack)\n");
        printf("2. 弱K值攻击 (Weak K Attack)\n");
        printf("3. 中本聪签名伪造攻击 (Satoshi Signature Forge)\n");
        printf("4. 显示安全建议\n");
        printf("5. 运行所有攻击演示\n");
        printf("0. 退出\n");
        printf("请输入选择 (0-5): ");
        
        if (scanf("%d", &choice) != 1) {
            printf("无效输入，请输入数字\n");
            while (getchar() != '\n'); // 清空输入缓冲区
            continue;
        }
        
        switch (choice) {
            case 1:
                demonstrate_k_reuse_attack();
                break;
                
            case 2:
                demonstrate_weak_k_attack();
                break;
                
            case 3:
                demonstrate_satoshi_forge_attack();
                break;
                
            case 4:
                print_security_recommendations();
                break;
                
            case 5:
                printf("\n开始运行所有攻击演示...\n");
                demonstrate_k_reuse_attack();
                demonstrate_weak_k_attack();
                demonstrate_satoshi_forge_attack();
                print_security_recommendations();
                break;
                
            case 0:
                printf("感谢使用SM2签名攻击演示程序！\n");
                printf("记住：了解攻击方法是为了更好地防护！\n");
                exit(0);
                
            default:
                printf("无效选择，请重新输入\n");
                break;
        }
        
        printf("\n按Enter键继续...");
        while (getchar() != '\n'); // 清空输入缓冲区
        getchar(); // 等待用户按Enter
    }
    
    return 0;
}
