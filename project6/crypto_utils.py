"""
Cryptographic utilities for Google Password Checkup Protocol
Based on the protocol described in Section 3.1 of https://eprint.iacr.org/2019/723.pdf
"""

import hashlib
import hmac
import secrets
from typing import List, Tuple, Dict
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa, padding
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
import os

class CryptoUtils:
    """Cryptographic utilities for the password checkup protocol"""
    
    @staticmethod
    def hash_password(password: str, salt: bytes = None) -> bytes:
        """
        Hash a password using SHA-256 with optional salt
        Args:
            password: The password to hash
            salt: Optional salt bytes
        Returns:
            Hash bytes
        """
        if salt is None:
            salt = secrets.token_bytes(32)
        
        # Use PBKDF2 for password hashing (more secure than simple SHA-256)
        password_bytes = password.encode('utf-8')
        hash_result = hashlib.pbkdf2_hmac('sha256', password_bytes, salt, 100000)
        return salt + hash_result
    
    @staticmethod
    def generate_random_element() -> bytes:
        """Generate a random element for blinding"""
        return secrets.token_bytes(32)
    
    @staticmethod
    def blind_element(element: bytes, blinding_factor: bytes) -> bytes:
        """
        Blind an element using XOR operation (simplified blinding)
        In practice, this would use more sophisticated elliptic curve operations
        """
        return bytes(a ^ b for a, b in zip(element, blinding_factor))
    
    @staticmethod
    def unblind_element(blinded_element: bytes, blinding_factor: bytes) -> bytes:
        """Unblind an element (reverse of blinding operation)"""
        return bytes(a ^ b for a, b in zip(blinded_element, blinding_factor))
    
    @staticmethod
    def prf(key: bytes, input_data: bytes) -> bytes:
        """
        Pseudorandom function using HMAC-SHA256
        Args:
            key: PRF key
            input_data: Input data
        Returns:
            PRF output
        """
        return hmac.new(key, input_data, hashlib.sha256).digest()
    
    @staticmethod
    def generate_key_pair() -> Tuple[bytes, bytes]:
        """
        Generate RSA key pair for the protocol
        Returns:
            Tuple of (private_key, public_key) in PEM format
        """
        private_key = rsa.generate_private_key(
            public_exponent=65537,
            key_size=2048,
            backend=default_backend()
        )
        
        private_pem = private_key.private_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PrivateFormat.PKCS8,
            encryption_algorithm=serialization.NoEncryption()
        )
        
        public_key = private_key.public_key()
        public_pem = public_key.public_bytes(
            encoding=serialization.Encoding.PEM,
            format=serialization.PublicFormat.SubjectPublicKeyInfo
        )
        
        return private_pem, public_pem
    
    @staticmethod
    def encrypt_with_public_key(public_key_pem: bytes, plaintext: bytes) -> bytes:
        """Encrypt data with RSA public key"""
        public_key = serialization.load_pem_public_key(public_key_pem, backend=default_backend())
        ciphertext = public_key.encrypt(
            plaintext,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
        return ciphertext
    
    @staticmethod
    def decrypt_with_private_key(private_key_pem: bytes, ciphertext: bytes) -> bytes:
        """Decrypt data with RSA private key"""
        private_key = serialization.load_pem_private_key(private_key_pem, password=None, backend=default_backend())
        plaintext = private_key.decrypt(
            ciphertext,
            padding.OAEP(
                mgf=padding.MGF1(algorithm=hashes.SHA256()),
                algorithm=hashes.SHA256(),
                label=None
            )
        )
        return plaintext

class ProtocolConstants:
    """Constants used in the protocol"""
    HASH_LENGTH = 32  # SHA-256 output length
    BLINDING_FACTOR_LENGTH = 32
    PRF_KEY_LENGTH = 32
    USERNAME_PREFIX = b"username:"
    PASSWORD_PREFIX = b"password:"
