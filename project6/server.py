import json
import threading
import time
from typing import Set, Dict, List
from protocol import PSIProtocol, PSIMessage
from crypto_utils import CryptoUtils


class PasswordCheckupServer:
    """Password Checkup服务器类"""
    
    def __init__(self):
        self.leaked_passwords: Set[str] = set()
        self.server_protocol: PSIProtocol = None
        self.active_sessions: Dict[str, PSIProtocol] = {}
        self.session_counter = 0
        self.lock = threading.Lock()
        
        print("[SERVER] Password Checkup服务器启动")
        self._load_leaked_passwords()
    
    def _load_leaked_passwords(self):
        """加载泄露密码数据库"""
        # 模拟从数据库或文件加载已泄露的密码
        # 在实际应用中，这里会从真实的泄露密码数据库加载
        sample_leaked_passwords = [
            "123456", "password", "123456789", "12345678", "12345",
            "111111", "1234567", "sunshine", "qwerty", "iloveyou",
            "princess", "admin", "welcome", "666666", "abc123",
            "football", "123123", "monkey", "654321", "!@#$%^&*",
            "charlie", "aa123456", "donald", "password123", "qwerty123",
            "admin123", "root", "letmein", "welcome123", "master",
            "login", "administrator", "secret", "pass", "password1",
            "dragon", "1234", "123", "test", "guest", "batman",
            "superman", "michael", "jordan", "harley", "ranger",
            "daniel", "killer", "987654321", "asdf", "zxcvbn"
        ]
        
        for password in sample_leaked_passwords:
            self.leaked_passwords.add(password)
        
        print(f"[SERVER] 已加载 {len(self.leaked_passwords)} 个泄露密码")
    
    def create_session(self, client_id: str = None) -> str:
        """为客户端创建新的PSI会话"""
        with self.lock:
            session_id = f"session_{self.session_counter}_{int(time.time())}"
            self.session_counter += 1
            
            # 为每个会话创建独立的PSI协议实例
            session_protocol = PSIProtocol("server", self.leaked_passwords)
            self.active_sessions[session_id] = session_protocol
            
            print(f"[SERVER] 为客户端 {client_id} 创建会话 {session_id}")
            return session_id
    
    def process_client_request(self, session_id: str, client_message: PSIMessage) -> PSIMessage:
        """处理客户端请求"""
        if session_id not in self.active_sessions:
            raise ValueError(f"无效的会话ID: {session_id}")
        
        session_protocol = self.active_sessions[session_id]
        
        if client_message.message_type == "CLIENT_BLINDED_ELEMENTS":
            print(f"[SERVER] 处理会话 {session_id} 的盲化元素")
            return session_protocol.step2_server_process_blinded_elements(client_message)
        else:
            raise ValueError(f"未知的消息类型: {client_message.message_type}")
    
    def cleanup_session(self, session_id: str):
        """清理会话"""
        with self.lock:
            if session_id in self.active_sessions:
                del self.active_sessions[session_id]
                print(f"[SERVER] 会话 {session_id} 已清理")
    
    def get_server_stats(self) -> Dict:
        """获取服务器统计信息"""
        return {
            "leaked_passwords_count": len(self.leaked_passwords),
            "active_sessions": len(self.active_sessions),
            "total_sessions_created": self.session_counter
        }
    
    def add_leaked_password(self, password: str):
        """向泄露密码数据库添加新密码"""
        with self.lock:
            self.leaked_passwords.add(password)
            print(f"[SERVER] 添加新的泄露密码: {password[:3]}***")
    
    def remove_leaked_password(self, password: str):
        """从泄露密码数据库移除密码"""
        with self.lock:
            if password in self.leaked_passwords:
                self.leaked_passwords.remove(password)
                print(f"[SERVER] 移除泄露密码: {password[:3]}***")
    
    def simulate_database_update(self):
        """模拟数据库更新（添加新的泄露密码）"""
        new_passwords = [
            "newleak2024", "compromised123", "breach2024", 
            "hacked_password", "leaked_pass"
        ]
        
        for password in new_passwords:
            time.sleep(1)  # 模拟间隔
            self.add_leaked_password(password)


class ServerManager:
    """服务器管理器 - 用于管理多个服务器实例"""
    
    def __init__(self):
        self.servers: Dict[str, PasswordCheckupServer] = {}
        self.default_server = None
    
    def create_server(self, server_id: str = "default") -> PasswordCheckupServer:
        """创建新的服务器实例"""
        server = PasswordCheckupServer()
        self.servers[server_id] = server
        
        if self.default_server is None:
            self.default_server = server
        
        return server
    
    def get_server(self, server_id: str = "default") -> PasswordCheckupServer:
        """获取服务器实例"""
        return self.servers.get(server_id, self.default_server)
    
    def list_servers(self) -> List[str]:
        """列出所有服务器ID"""
        return list(self.servers.keys())


def demonstrate_server():
    """演示服务器功能"""
    print("=== Password Checkup服务器演示 ===")
    
    # 创建服务器管理器
    manager = ServerManager()
    server = manager.create_server("demo_server")
    
    # 显示服务器统计
    print("\n--- 服务器初始状态 ---")
    stats = server.get_server_stats()
    for key, value in stats.items():
        print(f"{key}: {value}")
    
    # 模拟客户端会话
    print("\n--- 模拟客户端会话 ---")
    session_id = server.create_session("demo_client")
    print(f"创建会话: {session_id}")
    
    # 模拟添加新的泄露密码
    print("\n--- 模拟数据库更新 ---")
    server.add_leaked_password("demo_leaked_password")
    server.add_leaked_password("another_compromised_pass")
    
    # 显示更新后的统计
    print("\n--- 更新后的服务器状态 ---")
    stats = server.get_server_stats()
    for key, value in stats.items():
        print(f"{key}: {value}")
    
    # 清理会话
    server.cleanup_session(session_id)
    
    print("\n--- 服务器演示完成 ---")


if __name__ == "__main__":
    demonstrate_server()
