import time
import threading
from typing import List
from server import PasswordCheckupServer, ServerManager
from client import PasswordCheckupClient
from protocol import PSIProtocol


def print_banner():
    """打印欢迎横幅"""
    print("=" * 60)
    print("   Google Password Checkup 协议演示")
    print("   基于隐私集合交集(PSI)技术")
    print("=" * 60)


def demo_basic_protocol():
    """演示基本协议流程"""
    print("\n🔍 演示1: 基本PSI协议流程")
    print("-" * 40)
    
    # 创建测试数据
    leaked_passwords = {
        "password123", "123456", "qwerty", "admin", "letmein",
        "welcome", "monkey", "dragon", "secret", "password"
    }
    
    user_passwords = {
        "password123",  # 泄露
        "my_secure_pass",  # 安全
        "admin",  # 泄露
        "unique_password_2024"  # 安全
    }
    
    print(f"📊 泄露密码数据库: {len(leaked_passwords)} 个密码")
    print(f"👤 用户密码: {len(user_passwords)} 个密码")
    print(f"   用户密码列表: {list(user_passwords)}")
    
    # 运行PSI协议
    server_psi = PSIProtocol("server", leaked_passwords)
    client_psi = PSIProtocol("client", user_passwords)
    
    start_time = time.time()
    
    # 协议执行
    client_msg = client_psi.step1_client_blind_elements()
    server_msg = server_psi.step2_server_process_blinded_elements(client_msg)
    compromised = client_psi.step3_client_check_intersection(server_msg)
    
    end_time = time.time()
    
    print(f"⏱️  协议执行时间: {end_time - start_time:.4f} 秒")
    print(f"🔍 检查结果: 发现 {len(compromised)} 个泄露密码")
    
    return len(compromised) > 0


def demo_client_server_interaction():
    """演示完整的客户端-服务器交互"""
    print("\n🌐 演示2: 客户端-服务器交互")
    print("-" * 40)
    
    # 创建服务器
    server = PasswordCheckupServer()
    
    # 创建多个客户端
    clients = []
    for i in range(3):
        client = PasswordCheckupClient(f"user_{i+1}")
        clients.append(client)
    
    # 为客户端添加不同的密码
    test_passwords = [
        ["password123", "secure_pass_1", "admin"],  # 用户1：2个泄露
        ["123456", "my_secret_2024", "qwerty"],     # 用户2：2个泄露  
        ["unique_password", "another_safe_one"]      # 用户3：0个泄露
    ]
    
    for i, client in enumerate(clients):
        for password in test_passwords[i]:
            client.add_password(password)
    
    # 并发执行密码检查
    def check_passwords(client):
        return client.check_passwords_with_server(server)
    
    print("🔄 开始并发密码检查...")
    threads = []
    results = [None] * len(clients)
    
    start_time = time.time()
    
    for i, client in enumerate(clients):
        def check_func(idx=i, cli=client):
            results[idx] = check_func_inner(cli)
        
        def check_func_inner(cli):
            return cli.check_passwords_with_server(server)
        
        thread = threading.Thread(target=lambda: setattr(threading.current_thread(), 'result', check_passwords(client)))
        threads.append(thread)
        thread.start()
    
    # 等待所有线程完成
    for thread in threads:
        thread.join()
    
    end_time = time.time()
    
    print(f"⏱️  总执行时间: {end_time - start_time:.4f} 秒")
    
    # 显示结果
    print("\n📊 检查结果摘要:")
    total_compromised = 0
    for i, client in enumerate(clients):
        summary = client.get_check_summary()
        print(f"   {client.user_id}: {summary['total_compromised']} 个泄露密码")
        total_compromised += summary['total_compromised']
    
    print(f"🔍 总计发现: {total_compromised} 个泄露密码")
    
    # 显示服务器统计
    server_stats = server.get_server_stats()
    print(f"🖥️  服务器统计: {server_stats}")
    
    return total_compromised


def demo_security_features():
    """演示安全特性"""
    print("\n🛡️ 演示3: 安全特性展示")
    print("-" * 40)
    
    # 创建服务器和客户端
    server = PasswordCheckupServer()
    client = PasswordCheckupClient("security_demo_user")
    
    # 添加测试密码
    test_passwords = [
        "VerySecurePassword2024!",  # 安全密码
        "AnotherSafeOne#123",       # 安全密码
        "password123",              # 已知泄露
        "ComplexPass@2024",         # 安全密码
        "admin"                     # 已知泄露
    ]
    
    for password in test_passwords:
        client.add_password(password)
    
    print("🔐 密码隐私保护演示:")
    print("   - 客户端密码经过盲化处理")
    print("   - 服务器无法获知具体密码内容")
    print("   - 客户端只能了解自己的密码是否泄露")
    
    # 执行检查
    result = client.check_passwords_with_server(server)
    
    # 生成安全报告
    client.print_security_report()
    
    return result['compromised_count']


def demo_performance_test():
    """演示性能测试"""
    print("\n⚡ 演示4: 性能测试")
    print("-" * 40)
    
    # 创建大规模数据集
    print("📈 生成大规模测试数据...")
    
    # 生成大量泄露密码
    leaked_passwords = set()
    for i in range(1000):
        leaked_passwords.add(f"leaked_password_{i}")
    
    # 生成用户密码
    user_passwords = set()
    for i in range(100):
        if i < 10:  # 前10个是泄露的
            user_passwords.add(f"leaked_password_{i}")
        else:
            user_passwords.add(f"safe_password_{i}")
    
    print(f"📊 数据规模:")
    print(f"   - 泄露密码数据库: {len(leaked_passwords)} 个")
    print(f"   - 用户密码: {len(user_passwords)} 个")
    print(f"   - 预期泄露数量: 10 个")
    
    # 性能测试
    server_psi = PSIProtocol("server", leaked_passwords)
    client_psi = PSIProtocol("client", user_passwords)
    
    print("🔄 执行性能测试...")
    start_time = time.time()
    
    # 执行协议
    client_msg = client_psi.step1_client_blind_elements()
    msg_time = time.time()
    
    server_msg = server_psi.step2_server_process_blinded_elements(client_msg)
    server_time = time.time()
    
    compromised = client_psi.step3_client_check_intersection(server_msg)
    end_time = time.time()
    
    # 显示性能结果
    print(f"📈 性能结果:")
    print(f"   - 客户端盲化时间: {msg_time - start_time:.4f} 秒")
    print(f"   - 服务器处理时间: {server_time - msg_time:.4f} 秒")
    print(f"   - 客户端分析时间: {end_time - server_time:.4f} 秒")
    print(f"   - 总执行时间: {end_time - start_time:.4f} 秒")
    print(f"   - 检测到泄露: {len(compromised)} 个")
    
    return end_time - start_time


def demo_advanced_features():
    """演示高级功能"""
    print("\n🚀 演示5: 高级功能")
    print("-" * 40)
    
    # 服务器管理器演示
    manager = ServerManager()
    
    # 创建多个服务器实例
    server1 = manager.create_server("primary_server")
    server2 = manager.create_server("backup_server")
    
    print(f"🖥️  创建了 {len(manager.list_servers())} 个服务器实例")
    
    # 动态密码数据库更新
    print("🔄 模拟动态数据库更新...")
    original_count = len(server1.leaked_passwords)
    
    # 添加新的泄露密码
    new_leaked = ["new_breach_2024", "latest_hack", "fresh_leak"]
    for password in new_leaked:
        server1.add_leaked_password(password)
    
    updated_count = len(server1.leaked_passwords)
    print(f"   数据库更新: {original_count} → {updated_count} 个密码")
    
    # 客户端历史记录演示
    client = PasswordCheckupClient("advanced_user")
    client.add_password("new_breach_2024")  # 新泄露的密码
    client.add_password("safe_password_123")
    
    # 多次检查以建立历史记录
    print("📊 建立检查历史记录...")
    for i in range(3):
        print(f"   执行第 {i+1} 次检查...")
        client.check_passwords_with_server(server1)
        time.sleep(0.1)  # 短暂延迟
    
    # 显示历史记录
    summary = client.get_check_summary()
    print(f"📈 用户检查历史:")
    for key, value in summary.items():
        print(f"   {key}: {value}")


def main():
    """主演示函数"""
    print_banner()
    
    try:
        # 演示1: 基本协议
        has_leaks_1 = demo_basic_protocol()
        
        # 演示2: 客户端服务器交互
        total_leaks_2 = demo_client_server_interaction()
        
        # 演示3: 安全特性
        leaks_3 = demo_security_features()
        
        # 演示4: 性能测试
        perf_time = demo_performance_test()
        
        # 演示5: 高级功能
        demo_advanced_features()
        
        # 总结
        print("\n" + "=" * 60)
        print("🎉 演示完成总结")
        print("=" * 60)
        print(f"✅ 基本协议: {'发现泄露' if has_leaks_1 else '无泄露'}")
        print(f"✅ 多用户检查: 总计 {total_leaks_2} 个泄露密码")
        print(f"✅ 安全演示: {leaks_3} 个泄露密码")
        print(f"✅ 性能测试: {perf_time:.4f} 秒 (1000+100规模)")
        print(f"✅ 高级功能: 已展示")
        
        print("\n🔒 协议特点:")
        print("   • 保护用户密码隐私")
        print("   • 高效的大规模处理能力")
        print("   • 支持动态数据库更新")
        print("   • 完整的会话管理")
        print("   • 详细的安全报告")
        
    except Exception as e:
        print(f"\n❌ 演示过程中出现错误: {e}")
        import traceback
        traceback.print_exc()


if __name__ == "__main__":
    main()
