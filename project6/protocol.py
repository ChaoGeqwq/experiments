import json
import time
from typing import List, Dict, Set, Tuple, Optional
from crypto_utils import CryptoUtils, HashBucket


class PSIMessage:
    """PSI协议消息类"""
    
    def __init__(self, message_type: str, data: Dict):
        self.message_type = message_type
        self.data = data
        self.timestamp = time.time()
    
    def to_json(self) -> str:
        """转换为JSON格式"""
        return json.dumps({
            'type': self.message_type,
            'data': {k: v.hex() if isinstance(v, bytes) else v 
                    for k, v in self.data.items()},
            'timestamp': self.timestamp
        })
    
    @classmethod
    def from_json(cls, json_str: str) -> 'PSIMessage':
        """从JSON格式创建消息"""
        obj = json.loads(json_str)
        data = {}
        for k, v in obj['data'].items():
            if isinstance(v, str) and k.endswith('_bytes'):
                data[k] = bytes.fromhex(v)
            else:
                data[k] = v
        message = cls(obj['type'], data)
        message.timestamp = obj['timestamp']
        return message


class PSIProtocol:
    """PSI协议实现类"""
    
    def __init__(self, role: str, element_set: Set[str] = None):
        """
        初始化PSI协议
        
        Args:
            role: 'server' 或 'client'
            element_set: 参与PSI的元素集合
        """
        self.role = role
        self.element_set = element_set or set()
        self.session_key = CryptoUtils.generate_random_key()
        self.prf_key = CryptoUtils.generate_prf_key()
        
        # 协议状态
        self.state = "initialized"
        self.blinding_factors: Dict[str, bytes] = {}
        self.blinded_elements: Dict[str, bytes] = {}
        
        # 哈希桶（用于优化大规模数据处理）
        self.hash_buckets: Dict[int, HashBucket] = {}
        self.num_buckets = 1000  # 桶的数量
        
        print(f"[{self.role.upper()}] PSI协议初始化完成")
        print(f"[{self.role.upper()}] 元素集合大小: {len(self.element_set)}")
    
    def _hash_to_bucket(self, element: str) -> int:
        """将元素哈希到对应的桶中"""
        element_hash = CryptoUtils.sha256_hash(element.encode())
        return int.from_bytes(element_hash[:4], 'big') % self.num_buckets
    
    def _prepare_elements(self) -> Dict[int, List[bytes]]:
        """准备元素：哈希并分桶"""
        bucketed_elements = {}
        
        for element in self.element_set:
            # 对密码进行哈希处理
            password_hash = CryptoUtils.sha256_hash(element.encode())
            bucket_id = self._hash_to_bucket(element)
            
            if bucket_id not in bucketed_elements:
                bucketed_elements[bucket_id] = []
            
            bucketed_elements[bucket_id].append(password_hash)
        
        return bucketed_elements
    
    def step1_client_blind_elements(self) -> PSIMessage:
        """
        步骤1: 客户端盲化自己的元素
        """
        if self.role != 'client':
            raise ValueError("此步骤只能由客户端执行")
        
        print(f"[CLIENT] 步骤1: 盲化客户端元素")
        
        # 准备元素并分桶
        bucketed_elements = self._prepare_elements()
        blinded_buckets = {}
        
        for bucket_id, elements in bucketed_elements.items():
            blinded_buckets[bucket_id] = []
            
            for element in elements:
                # 为每个元素生成盲化因子
                blinding_factor = CryptoUtils.generate_random_key()
                element_str = element.hex()
                self.blinding_factors[element_str] = blinding_factor
                
                # 盲化元素
                blinded_element = CryptoUtils.blind_element(element, blinding_factor)
                self.blinded_elements[element_str] = blinded_element
                blinded_buckets[bucket_id].append(blinded_element)
        
        self.state = "elements_blinded"
        
        # 转换为可序列化的格式
        serializable_buckets = {}
        for bucket_id, elements in blinded_buckets.items():
            serializable_buckets[f"bucket_{bucket_id}_bytes"] = b''.join(elements)
        
        return PSIMessage("CLIENT_BLINDED_ELEMENTS", {
            "num_buckets": len(blinded_buckets),
            **serializable_buckets
        })
    
    def step2_server_process_blinded_elements(self, client_message: PSIMessage) -> PSIMessage:
        """
        步骤2: 服务器处理客户端的盲化元素
        """
        if self.role != 'server':
            raise ValueError("此步骤只能由服务器执行")
        
        print(f"[SERVER] 步骤2: 处理客户端盲化元素")
        
        # 准备服务器的元素集合
        server_bucketed = self._prepare_elements()
        
        # 处理客户端发送的盲化元素
        processed_buckets = {}
        num_client_buckets = client_message.data["num_buckets"]
        
        for i in range(num_client_buckets):
            bucket_key = f"bucket_{i}_bytes"
            if bucket_key in client_message.data:
                client_elements_data = client_message.data[bucket_key]
                
                # 使用PRF处理盲化元素
                processed_elements = []
                element_size = 32  # SHA256 输出长度
                
                # 将连续的字节数据分割为单个元素
                for j in range(0, len(client_elements_data), element_size):
                    if j + element_size <= len(client_elements_data):
                        element = client_elements_data[j:j+element_size]
                        processed_element = CryptoUtils.prf(self.prf_key, element)
                        processed_elements.append(processed_element)
                
                processed_buckets[f"bucket_{i}_bytes"] = b''.join(processed_elements)
        
        # 同时处理服务器自己的元素
        server_processed = {}
        for bucket_id, elements in server_bucketed.items():
            server_elements = []
            for element in elements:
                # 对服务器元素应用相同的PRF
                processed_element = CryptoUtils.prf(self.prf_key, element)
                server_elements.append(processed_element)
            server_processed[f"server_bucket_{bucket_id}_bytes"] = b''.join(server_elements)
        
        self.state = "server_processed"
        
        return PSIMessage("SERVER_PROCESSED_ELEMENTS", {
            "num_buckets": len(processed_buckets),
            **processed_buckets,
            **server_processed
        })
    
    def step3_client_check_intersection(self, server_message: PSIMessage) -> List[str]:
        """
        步骤3: 客户端检查交集
        """
        if self.role != 'client':
            raise ValueError("此步骤只能由客户端执行")
        
        print(f"[CLIENT] 步骤3: 检查密码是否泄露")
        
        compromised_passwords = []
        
        # 处理服务器返回的数据
        num_buckets = server_message.data["num_buckets"]
        
        for bucket_id in range(num_buckets):
            client_bucket_key = f"bucket_{bucket_id}_bytes"
            server_bucket_key = f"server_bucket_{bucket_id}_bytes"
            
            if (client_bucket_key in server_message.data and 
                server_bucket_key in server_message.data):
                
                # 获取处理后的客户端元素
                client_processed_data = server_message.data[client_bucket_key]
                server_processed_data = server_message.data[server_bucket_key]
                
                # 分割为单个元素
                element_size = 32
                client_elements = []
                server_elements = set()
                
                for i in range(0, len(client_processed_data), element_size):
                    if i + element_size <= len(client_processed_data):
                        client_elements.append(client_processed_data[i:i+element_size])
                
                for i in range(0, len(server_processed_data), element_size):
                    if i + element_size <= len(server_processed_data):
                        server_elements.add(server_processed_data[i:i+element_size])
                
                # 检查交集
                for processed_element in client_elements:
                    if processed_element in server_elements:
                        # 找到匹配的原始密码
                        # 注意：在实际实现中，这里需要更复杂的映射机制
                        print(f"[CLIENT] 发现泄露密码（桶{bucket_id}）")
                        compromised_passwords.append(f"password_in_bucket_{bucket_id}")
        
        self.state = "intersection_computed"
        return compromised_passwords
    
    def get_protocol_stats(self) -> Dict:
        """获取协议统计信息"""
        return {
            "role": self.role,
            "state": self.state,
            "element_count": len(self.element_set),
            "blinding_factors_count": len(self.blinding_factors),
            "blinded_elements_count": len(self.blinded_elements)
        }


def demonstrate_psi_protocol():
    """演示PSI协议的完整流程"""
    print("=== PSI协议演示 ===")
    
    # 模拟泄露密码数据库（服务器端）
    leaked_passwords = {
        "password123", "123456", "qwerty", "admin", "letmein",
        "welcome", "monkey", "dragon", "secret", "password"
    }
    
    # 模拟用户密码（客户端）
    user_passwords = {
        "password123",  # 这个密码已泄露
        "my_secure_pass",  # 这个密码安全
        "admin",  # 这个密码已泄露
        "unique_password_2024"  # 这个密码安全
    }
    
    print(f"泄露密码数据库大小: {len(leaked_passwords)}")
    print(f"用户密码数量: {len(user_passwords)}")
    print(f"用户密码: {list(user_passwords)}")
    
    # 初始化协议参与方
    server = PSIProtocol("server", leaked_passwords)
    client = PSIProtocol("client", user_passwords)
    
    try:
        # 步骤1: 客户端盲化元素
        print("\n--- 步骤1: 客户端盲化 ---")
        client_message = client.step1_client_blind_elements()
        print(f"客户端消息类型: {client_message.message_type}")
        
        # 步骤2: 服务器处理盲化元素
        print("\n--- 步骤2: 服务器处理 ---")
        server_message = server.step2_server_process_blinded_elements(client_message)
        print(f"服务器消息类型: {server_message.message_type}")
        
        # 步骤3: 客户端检查交集
        print("\n--- 步骤3: 客户端检查结果 ---")
        compromised_passwords = client.step3_client_check_intersection(server_message)
        
        if compromised_passwords:
            print(f"⚠️  发现 {len(compromised_passwords)} 个泄露密码！")
            for pwd in compromised_passwords:
                print(f"   - {pwd}")
        else:
            print("✅ 未发现泄露密码")
        
        # 显示协议统计
        print("\n--- 协议统计 ---")
        print("服务器统计:", server.get_protocol_stats())
        print("客户端统计:", client.get_protocol_stats())
        
    except Exception as e:
        print(f"协议执行出错: {e}")


if __name__ == "__main__":
    demonstrate_psi_protocol()
