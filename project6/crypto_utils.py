import hashlib
import hmac
import os
import secrets
from typing import List, Tuple, Dict
from Crypto.Cipher import AES
from Crypto.Protocol.KDF import PBKDF2
from Crypto.Random import get_random_bytes
from Crypto.Util.Padding import pad, unpad


class CryptoUtils:
    """密码学工具类"""
    
    @staticmethod
    def generate_random_key(length: int = 32) -> bytes:
        """生成随机密钥"""
        return secrets.token_bytes(length)
    
    @staticmethod
    def hash_password(password: str, salt: bytes = None) -> Tuple[bytes, bytes]:
        """
        对密码进行哈希处理
        返回: (哈希值, 盐值)
        """
        if salt is None:
            salt = get_random_bytes(16)
        
        # 使用PBKDF2进行密码哈希
        hashed = PBKDF2(password, salt, 32, count=100000)
        return hashed, salt
    
    @staticmethod
    def sha256_hash(data: bytes) -> bytes:
        """SHA256哈希函数"""
        return hashlib.sha256(data).digest()
    
    @staticmethod
    def hmac_sha256(key: bytes, message: bytes) -> bytes:
        """HMAC-SHA256"""
        return hmac.new(key, message, hashlib.sha256).digest()
    
    @staticmethod
    def aes_encrypt(key: bytes, plaintext: bytes) -> Tuple[bytes, bytes]:
        """
        AES加密
        返回: (密文, IV)
        """
        iv = get_random_bytes(16)
        cipher = AES.new(key, AES.MODE_CBC, iv)
        padded_plaintext = pad(plaintext, AES.block_size)
        ciphertext = cipher.encrypt(padded_plaintext)
        return ciphertext, iv
    
    @staticmethod
    def aes_decrypt(key: bytes, ciphertext: bytes, iv: bytes) -> bytes:
        """AES解密"""
        cipher = AES.new(key, AES.MODE_CBC, iv)
        padded_plaintext = cipher.decrypt(ciphertext)
        return unpad(padded_plaintext, AES.block_size)
    
    @staticmethod
    def blind_element(element: bytes, blinding_factor: bytes) -> bytes:
        """
        盲化元素 - 简化版本的盲化函数
        在实际实现中，这里应该使用更复杂的盲化技术（如椭圆曲线盲化）
        """
        # 这是一个简化的盲化实现，仅用于演示
        # 实际应用中应使用更安全的盲化技术
        return CryptoUtils.hmac_sha256(blinding_factor, element)
    
    @staticmethod
    def unblind_element(blinded_element: bytes, blinding_factor: bytes, 
                       original_element: bytes) -> bytes:
        """
        去盲化元素
        验证盲化元素是否对应原始元素
        """
        expected_blinded = CryptoUtils.blind_element(original_element, blinding_factor)
        return blinded_element == expected_blinded
    
    @staticmethod
    def generate_prf_key() -> bytes:
        """生成伪随机函数密钥"""
        return CryptoUtils.generate_random_key(32)
    
    @staticmethod
    def prf(key: bytes, input_data: bytes) -> bytes:
        """
        伪随机函数 (Pseudo-Random Function)
        使用HMAC-SHA256实现
        """
        return CryptoUtils.hmac_sha256(key, input_data)


class HashBucket:
    """哈希桶类 - 用于分布式哈希表"""
    
    def __init__(self, bucket_id: int, bucket_size: int = 1000):
        self.bucket_id = bucket_id
        self.bucket_size = bucket_size
        self.elements: List[bytes] = []
    
    def add_element(self, element: bytes):
        """向桶中添加元素"""
        if len(self.elements) < self.bucket_size:
            self.elements.append(element)
        else:
            raise ValueError(f"Bucket {self.bucket_id} is full")
    
    def contains(self, element: bytes) -> bool:
        """检查桶中是否包含指定元素"""
        return element in self.elements
    
    def get_size(self) -> int:
        """获取桶中元素数量"""
        return len(self.elements)


def demonstrate_crypto_functions():
    """演示加密函数的使用"""
    print("=== 密码学工具函数演示 ===")
    
    # 1. 密码哈希
    password = "my_secret_password"
    hashed_pw, salt = CryptoUtils.hash_password(password)
    print(f"原始密码: {password}")
    print(f"哈希值: {hashed_pw.hex()}")
    print(f"盐值: {salt.hex()}")
    
    # 2. 对称加密
    key = CryptoUtils.generate_random_key()
    plaintext = b"This is a secret message"
    ciphertext, iv = CryptoUtils.aes_encrypt(key, plaintext)
    decrypted = CryptoUtils.aes_decrypt(key, ciphertext, iv)
    print(f"原文: {plaintext}")
    print(f"密文: {ciphertext.hex()}")
    print(f"解密: {decrypted}")
    
    # 3. 盲化演示
    element = b"password123"
    blinding_factor = CryptoUtils.generate_random_key()
    blinded = CryptoUtils.blind_element(element, blinding_factor)
    print(f"原始元素: {element}")
    print(f"盲化后: {blinded.hex()}")


if __name__ == "__main__":
    demonstrate_crypto_functions()
