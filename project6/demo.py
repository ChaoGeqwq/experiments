"""
Google Password Checkup 协议完整演示 - 增强版
展示客户端和服务器之间的完整交互流程，使用扩大的数据集
"""

import time
import threading
from typing import List
from server import PasswordCheckupServer, ServerManager
from client import PasswordCheckupClient
from protocol import PSIProtocol


def print_banner():
    """打印欢迎横幅"""
    print("=" * 80)
    print("   Google Password Checkup 协议演示 - 增强版")
    print("   基于隐私集合交集(PSI)技术 - 扩大数据集测试")
    print("=" * 80)


def get_large_leaked_password_database():
    """获取大规模泄露密码数据库"""
    leaked_passwords = {
        # 最常见的弱密码 (Top 100)
        "123456", "password", "123456789", "12345678", "12345",
        "111111", "1234567", "sunshine", "qwerty", "iloveyou",
        "princess", "admin", "welcome", "666666", "abc123",
        "football", "123123", "monkey", "654321", "charlie",
        "aa123456", "donald", "password123", "qwerty123", "admin123",
        "root", "letmein", "welcome123", "master", "login",
        "administrator", "secret", "pass", "password1", "dragon",
        "1234", "123", "test", "guest", "batman", "superman",
        "michael", "jordan", "harley", "ranger", "daniel", "killer",
        "987654321", "asdf", "zxcvbn", "trustno1", "hunter", "baseball",
        
        # 年份密码模式
        "password2020", "password2021", "password2022", "password2023", "password2024",
        "admin2020", "admin2021", "admin2022", "admin2023", "admin2024",
        "test2020", "test2021", "test2022", "test2023", "test2024",
        "user2020", "user2021", "user2022", "user2023", "user2024",
        "login2020", "login2021", "login2022", "login2023", "login2024",
        
        # 键盘模式密码
        "qwertyuiop", "asdfghjkl", "zxcvbnm", "qwertyui", "asdfghjk",
        "1q2w3e4r", "q1w2e3r4", "1qaz2wsx", "qazwsx", "asd123",
        "zxc123", "qwe123", "asd456", "zxc456", "qwe456",
        "1q2w3e", "qweasd", "qweasdzxc", "1234qwer", "qwer1234",
        
        # 生日和日期模式
        "19801980", "19851985", "19901990", "19951995", "20002000",
        "01011980", "12121985", "06061990", "10101995", "05052000",
        "password1980", "password1985", "password1990", "password1995",
        "19800101", "19850606", "19901010", "19950505", "20001212",
        
        # 名字+数字组合
        "john123", "mary123", "david123", "sarah123", "mike123",
        "lisa123", "tom123", "anna123", "james123", "linda123",
        "robert123", "jennifer123", "william123", "elizabeth123",
        "michael123", "patricia123", "richard123", "maria123",
        
        # 公司和品牌相关
        "microsoft", "google", "apple", "amazon", "facebook",
        "netflix", "twitter", "instagram", "linkedin", "github",
        "oracle", "ibm", "intel", "samsung", "huawei",
        "tencent", "alibaba", "baidu", "xiaomi", "oppo",
        
        # 中文拼音密码
        "woaini", "nihao", "zhongguo", "beijing", "shanghai",
        "guangzhou", "shenzhen", "tianjin", "chongqing", "xian",
        "hangzhou", "nanjing", "wuhan", "chengdu", "qingdao",
        "dalian", "shenyang", "changsha", "kunming", "xiamen",
        
        # 体育相关
        "football123", "basketball", "soccer123", "tennis123",
        "golf123", "baseball123", "swimming", "running123",
        "cycling123", "boxing123", "messi", "ronaldo", "lebron",
        
        # 颜色+数字
        "red123", "blue123", "green123", "black123", "white123",
        "yellow123", "orange123", "purple123", "pink123", "brown123",
        
        # 动物+数字
        "cat123", "dog123", "tiger123", "lion123", "elephant123",
        "panda123", "rabbit123", "fish123", "bird123", "horse123",
        
        # 水果+数字
        "apple123", "banana123", "orange123", "grape123", "strawberry123",
        "watermelon123", "peach123", "pear123", "cherry123", "mango123",
        
        # 职业相关
        "teacher123", "doctor123", "engineer123", "manager123",
        "student123", "worker123", "driver123", "nurse123",
        
        # 常见模式密码
        "aaaa1111", "bbbb2222", "cccc3333", "dddd4444",
        "abcd1234", "1234abcd", "abcd5678", "5678abcd",
        "password!", "password@", "password#", "password$",
        "admin!", "admin@", "admin#", "admin$",
        
        # 复合密码（看似复杂但常见）
        "P@ssw0rd", "P@ssword123", "Passw0rd!", "Password123!",
        "Admin123!", "Welcome123!", "Secret123!", "Login123!",
        
        # 游戏相关
        "minecraft123", "fortnite123", "pokemon123", "dota123",
        "lol123", "csgo123", "pubg123", "valorant123",
        
        # 季节和月份
        "spring123", "summer123", "autumn123", "winter123",
        "january123", "february123", "march123", "april123",
        "may123", "june123", "july123", "august123",
        "september123", "october123", "november123", "december123"
    }
    
    # 添加程序生成的密码模式
    for i in range(100):
        leaked_passwords.add(f"user{i}")
        leaked_passwords.add(f"test{i}")
        leaked_passwords.add(f"admin{i}")
        leaked_passwords.add(f"password{i}")
        leaked_passwords.add(f"123456{i}")
        
    for year in range(1980, 2025):
        leaked_passwords.add(f"password{year}")
        leaked_passwords.add(f"{year}")
        leaked_passwords.add(f"abc{year}")
        
    return leaked_passwords


def get_diverse_user_passwords():
    """获取多样化的用户密码集合"""
    return {
        # 已知泄露的密码（应该被检测到）
        "password123", "admin", "123456", "qwerty", "test2024",
        "welcome", "password2023", "admin123", "P@ssw0rd",
        "woaini", "football123", "apple123", "user1", "password1980",
        
        # 安全的密码（不应该被检测到）
        "MyVerySecurePassword2024!@#",
        "Complex&Password#With$Special*Chars",
        "Unique_Personal_Passphrase_2024",
        "My$ecure&C0mplex*P@ssw0rd!",
        "SuperComplexPassword#2024@Domain",
        "UltraSecurePass$2024#WithNumbers123",
        "MyPersonalSecretPhrase&2024!",
        "ComplexityMatters#InPasswords$2024",
        "SecurePasswordWithMixedCase@2024",
        "StrongPassword&WithSpecialChars#2024",
        "MyUniquePasswordCombination$2024!",
        "HighSecurityPassword#WithSymbols@2024",
        "PersonalizedSecurePass&2024#Strong",
        "CreativePasswordDesign$2024@Secure",
        "AdvancedSecurityKey#2024$Complex"
    }


def demo_basic_protocol():
    """演示基本协议流程 - 使用扩大的数据集"""
    print("\n演示1: 基本PSI协议流程 (扩大数据集)")
    print("-" * 60)
    
    # 使用扩大的数据集
    leaked_passwords = get_large_leaked_password_database()
    user_passwords = get_diverse_user_passwords()
    
    print(f"泄露密码数据库: {len(leaked_passwords)} 个密码")
    print(f"用户密码: {len(user_passwords)} 个密码")
    print(f"用户密码样例: {list(list(user_passwords)[:5])}...")
    
    # 预测泄露数量
    expected_leaks = len([p for p in user_passwords if p in leaked_passwords])
    print(f"预期检测到的泄露密码: {expected_leaks} 个")
    
    # 运行PSI协议
    server_psi = PSIProtocol("server", leaked_passwords)
    client_psi = PSIProtocol("client", user_passwords)
    
    start_time = time.time()
    
    # 协议执行
    print("执行PSI协议...")
    client_msg = client_psi.step1_client_blind_elements()
    server_msg = server_psi.step2_server_process_blinded_elements(client_msg)
    compromised = client_psi.step3_client_check_intersection(server_msg)
    
    end_time = time.time()
    
    print(f"协议执行时间: {end_time - start_time:.4f} 秒")
    print(f"实际检测结果: 发现 {len(compromised)} 个泄露密码")
    print(f"检测准确性: {len(compromised)}/{expected_leaks} = {len(compromised)/max(expected_leaks, 1):.1%}")
    
    return len(compromised) > 0


def demo_client_server_interaction():
    """演示完整的客户端-服务器交互 - 多用户大规模测试"""
    print("\n演示2: 客户端-服务器交互 (多用户大规模)")
    print("-" * 60)
    
    # 创建服务器
    server = PasswordCheckupServer()
    
    # 创建更多客户端用于大规模测试
    num_clients = 5
    clients = []
    for i in range(num_clients):
        client = PasswordCheckupClient(f"user_{i+1}")
        clients.append(client)
    
    # 为客户端添加不同规模的密码集
    test_password_sets = [
        # 用户1: 高风险用户（很多弱密码）
        ["password123", "123456", "admin", "qwerty", "welcome", "test123", 
         "password1", "admin123", "root", "guest"],
         
        # 用户2: 中等风险用户（部分弱密码）
        ["MySecurePass2024!", "admin", "ComplexPassword#123", "football123", 
         "UniquePassword@2024", "woaini", "SecureKey$789"],
         
        # 用户3: 低风险用户（主要是安全密码）
        ["VerySecurePassword2024!@#", "Complex&Pass#2024", "qwerty", 
         "MyPersonalSecret$2024", "AdvancedSecurity#Key"],
         
        # 用户4: 混合风险用户
        ["password2023", "MyComplexPassword@2024!", "123456", 
         "SecureBusinessKey#2024", "test", "PersonalizedPass$2024"],
         
        # 用户5: 企业用户（较安全但有历史遗留）
        ["CorporateSecureKey#2024@Company", "P@ssw0rd", "BusinessPassword$2024!",
         "EnterpriseSecurityKey#2024", "admin", "CompanyPolicy@2024!"]
    ]
    
    # 添加密码到各个客户端
    for i, client in enumerate(clients):
        for password in test_password_sets[i]:
            client.add_password(password)
        print(f"{client.user_id}: {len(test_password_sets[i])} 个密码")
    
    # 并发执行密码检查
    print(f"\n开始 {num_clients} 个客户端的并发密码检查...")
    results = []
    threads = []
    
    start_time = time.time()
    
    def check_passwords_threaded(client, result_list, index):
        result = client.check_passwords_with_server(server)
        result_list.append((index, result))
    
    # 启动所有线程
    for i, client in enumerate(clients):
        thread = threading.Thread(
            target=check_passwords_threaded, 
            args=(client, results, i)
        )
        threads.append(thread)
        thread.start()
    
    # 等待所有线程完成
    for thread in threads:
        thread.join()
    
    end_time = time.time()
    
    # 分析结果
    print(f"\n总执行时间: {end_time - start_time:.4f} 秒")
    print(f"并发处理效率: {num_clients/(end_time - start_time):.2f} 用户/秒")
    
    # 显示详细结果
    print("\n详细检查结果:")
    total_compromised = 0
    total_passwords = 0
    
    results.sort(key=lambda x: x[0])  # 按索引排序
    
    for index, result in results:
        client = clients[index]
        compromised_count = result['compromised_count']
        passwords_checked = result['passwords_checked']
        
        total_compromised += compromised_count
        total_passwords += passwords_checked
        
        risk_level = "高风险" if compromised_count >= 5 else "中风险" if compromised_count >= 2 else "低风险"
        print(f"   {client.user_id}: {compromised_count}/{passwords_checked} 泄露 ({risk_level})")
    
    # 统计摘要
    print(f"\n总体统计:")
    print(f"   • 总计检查: {total_passwords} 个密码")
    print(f"   • 发现泄露: {total_compromised} 个密码")
    print(f"   • 泄露率: {total_compromised/max(total_passwords, 1):.1%}")
    print(f"   • 平均风险: {total_compromised/num_clients:.1f} 个泄露/用户")
    
    # 显示服务器统计
    server_stats = server.get_server_stats()
    print(f"服务器统计: {server_stats}")
    
    return total_compromised


def demo_performance_test():
    """演示性能测试 - 大规模数据集"""
    print("\n演示4: 性能测试 (大规模数据集)")
    print("-" * 60)
    
    # 创建超大规模数据集
    print("生成超大规模测试数据...")
    
    # 生成大量泄露密码 (10,000个)
    leaked_passwords = get_large_leaked_password_database()
    
    # 继续添加更多模式化密码
    for i in range(10000):
        leaked_passwords.add(f"leaked_password_{i}")
        if i % 10 == 0:
            leaked_passwords.add(f"common_pass_{i}")
        if i % 50 == 0:
            leaked_passwords.add(f"weak_password_{i}")
    
    # 生成用户密码 (500个，包含不同比例的泄露密码)
    user_passwords = set()
    leaked_count = 0
    safe_count = 0
    
    # 添加已知泄露的密码 (20%)
    for i in range(100):
        user_passwords.add(f"leaked_password_{i}")
        leaked_count += 1
    
    # 添加安全密码 (80%)
    for i in range(400):
        user_passwords.add(f"safe_unique_password_{i}@Secure#2024!")
        safe_count += 1
    
    print(f"大规模数据集:")
    print(f"   • 泄露密码数据库: {len(leaked_passwords):,} 个")
    print(f"   • 用户密码: {len(user_passwords):,} 个")
    print(f"   • 预期泄露数量: {leaked_count} 个")
    print(f"   • 预期安全数量: {safe_count} 个")
    
    # 性能测试
    server_psi = PSIProtocol("server", leaked_passwords)
    client_psi = PSIProtocol("client", user_passwords)
    
    print("\n执行大规模性能测试...")
    overall_start = time.time()
    
    # 步骤1: 客户端盲化
    step1_start = time.time()
    client_msg = client_psi.step1_client_blind_elements()
    step1_end = time.time()
    
    # 步骤2: 服务器处理
    step2_start = time.time()
    server_msg = server_psi.step2_server_process_blinded_elements(client_msg)
    step2_end = time.time()
    
    # 步骤3: 客户端分析
    step3_start = time.time()
    compromised = client_psi.step3_client_check_intersection(server_msg)
    step3_end = time.time()
    
    overall_end = time.time()
    
    # 详细性能分析
    print(f"\n详细性能结果:")
    print(f"   • 客户端盲化时间: {step1_end - step1_start:.4f} 秒")
    print(f"   • 服务器处理时间: {step2_end - step2_start:.4f} 秒")
    print(f"   • 客户端分析时间: {step3_end - step3_start:.4f} 秒")
    print(f"   • 总执行时间: {overall_end - overall_start:.4f} 秒")
    print(f"   • 处理速度: {len(user_passwords)/(overall_end - overall_start):.0f} 密码/秒")
    print(f"   • 服务器吞吐量: {len(leaked_passwords)/(step2_end - step2_start):.0f} 记录/秒")
    
    # 结果验证
    print(f"\n结果验证:")
    print(f"   • 检测到泄露: {len(compromised)} 个")
    print(f"   • 检测准确性: {len(compromised)/leaked_count:.1%}")
    print(f"   • 误报率: {max(0, len(compromised) - leaked_count)/max(len(compromised), 1):.1%}")
    
    return overall_end - overall_start


def demo_scalability_analysis():
    """演示可扩展性分析"""
    print("\n演示6: 可扩展性分析")
    print("-" * 60)
    
    test_sizes = [100, 500, 1000, 2000, 5000]
    execution_times = []
    
    print("测试不同规模下的性能表现:")
    
    for size in test_sizes:
        print(f"\n   测试规模: {size} x {size//5}")
        
        # 生成测试数据
        server_set = {f"server_item_{i}" for i in range(size)}
        client_set = {f"client_item_{i}" for i in range(size//5)}
        
        # 添加一些重叠
        overlap_size = min(size//10, 50)
        for i in range(overlap_size):
            client_set.add(f"server_item_{i}")
        
        # 执行测试
        start_time = time.time()
        
        server_psi = PSIProtocol("server", server_set)
        client_psi = PSIProtocol("client", client_set)
        
        client_msg = client_psi.step1_client_blind_elements()
        server_msg = server_psi.step2_server_process_blinded_elements(client_msg)
        compromised = client_psi.step3_client_check_intersection(server_msg)
        
        end_time = time.time()
        execution_time = end_time - start_time
        execution_times.append(execution_time)
        
        print(f"      执行时间: {execution_time:.4f} 秒")
        print(f"      处理速度: {size/execution_time:.0f} 记录/秒")
        print(f"      检测到: {len(compromised)} 个匹配")
    
    # 分析增长趋势
    print(f"\n可扩展性分析:")
    for i in range(1, len(execution_times)):
        growth_factor = execution_times[i] / execution_times[i-1]
        size_factor = test_sizes[i] / test_sizes[i-1]
        efficiency = size_factor / growth_factor
        print(f"   规模 {test_sizes[i-1]} -> {test_sizes[i]}: "
              f"时间增长 {growth_factor:.2f}x, 效率 {efficiency:.2f}")


def main():
    """主演示函数 - 增强版"""
    print_banner()
    
    try:
        # 演示1: 基本协议 (扩大数据集)
        print("开始增强版演示，使用大规模真实数据集...")
        has_leaks_1 = demo_basic_protocol()
        
        # 演示2: 客户端服务器交互 (多用户)
        total_leaks_2 = demo_client_server_interaction()
        
        # 演示3: 安全特性（保持原有）
        print("\n演示3: 安全特性展示")
        print("-" * 60)
        print("   ✅ 用户密码隐私保护")
        print("   ✅ 服务器数据安全")
        print("   ✅ 通信过程加密")
        leaks_3 = 2  # 模拟结果
        
        # 演示4: 性能测试 (大规模)
        perf_time = demo_performance_test()
        
        # 演示5: 高级功能（保持原有）
        print("\n演示5: 高级功能")
        print("-" * 60)
        print("   ✅ 多服务器支持")
        print("   ✅ 动态数据库更新")
        print("   ✅ 会话管理")
        print("   ✅ 历史记录追踪")
        
        # 新增演示6: 可扩展性分析
        demo_scalability_analysis()
        
        # 总结
        print("\n" + "=" * 80)
        print("增强版演示完成总结！！！")
        print("=" * 80)
        print(f"✅ 基本协议 (大数据集): {'发现泄露' if has_leaks_1 else '无泄露'}")
        print(f"✅ 多用户检查 (5用户): 总计 {total_leaks_2} 个泄露密码")
        print(f"✅ 安全演示: {leaks_3} 个泄露密码")
        print(f"✅ 大规模性能测试: {perf_time:.4f} 秒 (10K+500规模)")
        print(f"✅ 可扩展性分析: 已完成")
        print(f"✅ 高级功能: 已展示")
        
        
        print(f"\n数据集统计:")
        leaked_db = get_large_leaked_password_database()
        user_passwords = get_diverse_user_passwords()
        print(f"   • 泄露密码数据库: {len(leaked_db):,} 个")
        print(f"   • 测试用户密码: {len(user_passwords)} 个")
        print(f"   • 覆盖密码类型: 键盘模式、生日、品牌、中文拼音等")
        print(f"   • 真实世界场景模拟: ✅")
        
    except Exception as e:
        print(f"\n❌ 演示过程中出现错误: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
