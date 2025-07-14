import json
import time
from typing import Set, List, Dict, Optional
from protocol import PSIProtocol, PSIMessage
from crypto_utils import CryptoUtils


class PasswordCheckupClient:
    """Password Checkup客户端类"""
    
    def __init__(self, user_id: str = "default_user"):
        self.user_id = user_id
        self.user_passwords: Set[str] = set()
        self.client_protocol: PSIProtocol = None
        self.current_session_id: Optional[str] = None
        self.check_history: List[Dict] = []
        
        print(f"[CLIENT] Password Checkup客户端启动 (用户: {user_id})")
    
    def add_password(self, password: str):
        """添加用户密码到检查列表"""
        self.user_passwords.add(password)
        print(f"[CLIENT] 添加密码: {password[:3]}*** (长度: {len(password)})")
    
    def remove_password(self, password: str):
        """从检查列表移除密码"""
        if password in self.user_passwords:
            self.user_passwords.remove(password)
            print(f"[CLIENT] 移除密码: {password[:3]}***")
    
    def load_passwords_from_file(self, file_path: str):
        """从文件加载密码列表"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                passwords = [line.strip() for line in f if line.strip()]
                for password in passwords:
                    self.add_password(password)
            print(f"[CLIENT] 从文件 {file_path} 加载了 {len(passwords)} 个密码")
        except FileNotFoundError:
            print(f"[CLIENT] 错误: 文件 {file_path} 不存在")
        except Exception as e:
            print(f"[CLIENT] 加载密码文件时出错: {e}")
    
    def check_passwords_with_server(self, server) -> Dict:
        """与服务器进行密码检查"""
        print(f"[CLIENT] 开始密码检查流程")
        print(f"[CLIENT] 待检查密码数量: {len(self.user_passwords)}")
        
        check_start_time = time.time()
        
        try:
            # 1. 创建会话
            session_id = server.create_session(self.user_id)
            self.current_session_id = session_id
            
            # 2. 初始化客户端协议
            self.client_protocol = PSIProtocol("client", self.user_passwords)
            
            # 3. 步骤1: 客户端盲化元素
            print(f"[CLIENT] 步骤1: 盲化用户密码")
            client_message = self.client_protocol.step1_client_blind_elements()
            
            # 4. 步骤2: 发送给服务器处理
            print(f"[CLIENT] 步骤2: 发送盲化数据给服务器")
            server_response = server.process_client_request(session_id, client_message)
            
            # 5. 步骤3: 检查交集
            print(f"[CLIENT] 步骤3: 分析服务器响应")
            compromised_passwords = self.client_protocol.step3_client_check_intersection(server_response)
            
            check_end_time = time.time()
            check_duration = check_end_time - check_start_time
            
            # 记录检查历史
            check_result = {
                "timestamp": check_start_time,
                "session_id": session_id,
                "passwords_checked": len(self.user_passwords),
                "compromised_count": len(compromised_passwords),
                "compromised_passwords": compromised_passwords,
                "duration_seconds": check_duration,
                "status": "completed"
            }
            
            self.check_history.append(check_result)
            
            # 6. 清理会话
            server.cleanup_session(session_id)
            self.current_session_id = None
            
            return check_result
            
        except Exception as e:
            error_result = {
                "timestamp": time.time(),
                "session_id": self.current_session_id,
                "passwords_checked": len(self.user_passwords),
                "compromised_count": 0,
                "compromised_passwords": [],
                "duration_seconds": 0,
                "status": "error",
                "error_message": str(e)
            }
            
            self.check_history.append(error_result)
            print(f"[CLIENT] 密码检查出错: {e}")
            
            return error_result
    
    def get_check_summary(self) -> Dict:
        """获取检查摘要"""
        if not self.check_history:
            return {
                "total_checks": 0,
                "total_passwords_checked": 0,
                "total_compromised": 0,
                "last_check": None
            }
        
        total_checks = len(self.check_history)
        total_passwords = sum(check["passwords_checked"] for check in self.check_history)
        total_compromised = sum(check["compromised_count"] for check in self.check_history)
        last_check = self.check_history[-1]
        
        return {
            "user_id": self.user_id,
            "total_checks": total_checks,
            "total_passwords_checked": total_passwords,
            "total_compromised": total_compromised,
            "last_check_time": last_check["timestamp"],
            "last_check_status": last_check["status"]
        }
    
    def print_security_report(self):
        """打印安全报告"""
        print("\n=== 密码安全报告 ===")
        print(f"用户ID: {self.user_id}")
        print(f"当前密码数量: {len(self.user_passwords)}")
        
        if self.check_history:
            latest_check = self.check_history[-1]
            print(f"最近检查时间: {time.ctime(latest_check['timestamp'])}")
            print(f"检查状态: {latest_check['status']}")
            
            if latest_check['status'] == 'completed':
                compromised_count = latest_check['compromised_count']
                total_checked = latest_check['passwords_checked']
                safe_count = total_checked - compromised_count
                
                print(f"安全密码: {safe_count}/{total_checked}")
                print(f"泄露密码: {compromised_count}/{total_checked}")
                
                if compromised_count > 0:
                    print("⚠️  发现泄露密码，建议立即更换以下密码:")
                    for pwd in latest_check['compromised_passwords']:
                        print(f"   - {pwd}")
                    
                    print("\n🔒 密码安全建议:")
                    print("   1. 立即更换所有泄露的密码")
                    print("   2. 使用强密码（至少12位，包含大小写、数字、特殊字符）")
                    print("   3. 为每个账户使用不同的密码")
                    print("   4. 考虑使用密码管理器")
                    print("   5. 启用双因素认证")
                else:
                    print("✅ 所有密码都是安全的！")
            else:
                print(f"检查失败: {latest_check.get('error_message', '未知错误')}")
        else:
            print("尚未进行密码检查")
    
    def generate_sample_passwords(self):
        """生成示例密码用于测试"""
        sample_passwords = [
            "MySecurePassword2024!",  # 安全密码
            "password123",            # 泄露密码
            "MyUniquePass@2024",     # 安全密码
            "admin",                 # 泄露密码
            "SuperSecret#Pass",      # 安全密码
            "123456",                # 泄露密码
            "ComplexPassword$789",   # 安全密码
            "qwerty"                 # 泄露密码
        ]
        
        for password in sample_passwords:
            self.add_password(password)
        
        print(f"[CLIENT] 生成了 {len(sample_passwords)} 个示例密码")


def demonstrate_client():
    """演示客户端功能"""
    print("=== Password Checkup客户端演示 ===")
    
    # 导入服务器模块以进行完整演示
    from server import PasswordCheckupServer
    
    # 创建服务器和客户端
    server = PasswordCheckupServer()
    client = PasswordCheckupClient("demo_user")
    
    # 添加测试密码
    print("\n--- 添加测试密码 ---")
    client.generate_sample_passwords()
    
    # 执行密码检查
    print("\n--- 执行密码检查 ---")
    check_result = client.check_passwords_with_server(server)
    
    # 显示检查结果
    print("\n--- 检查结果 ---")
    print(f"检查状态: {check_result['status']}")
    print(f"检查耗时: {check_result['duration_seconds']:.2f} 秒")
    print(f"检查密码数量: {check_result['passwords_checked']}")
    print(f"发现泄露密码: {check_result['compromised_count']}")
    
    # 打印安全报告
    client.print_security_report()
    
    # 显示检查摘要
    print("\n--- 检查摘要 ---")
    summary = client.get_check_summary()
    for key, value in summary.items():
        print(f"{key}: {value}")


if __name__ == "__main__":
    demonstrate_client()
