import cv2
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import os
import logging
from typing import Tuple, Optional
import argparse

# 配置日志
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class DCTWatermark:
    """基于DCT变换的数字水印系统"""
    
    def __init__(self, block_size: int = 8, alpha: float = 0.1):
        """
        初始化水印系统
        
        Args:
            block_size: DCT变换的块大小
            alpha: 水印嵌入强度
        """
        self.block_size = block_size
        self.alpha = alpha
        
    def _pad_image(self, img: np.ndarray) -> np.ndarray:
        """填充图像使其尺寸能被block_size整除"""
        h, w = img.shape[:2]
        new_h = ((h + self.block_size - 1) // self.block_size) * self.block_size
        new_w = ((w + self.block_size - 1) // self.block_size) * self.block_size
        
        if len(img.shape) == 3:
            padded = np.zeros((new_h, new_w, img.shape[2]), dtype=img.dtype)
            padded[:h, :w] = img
        else:
            padded = np.zeros((new_h, new_w), dtype=img.dtype)
            padded[:h, :w] = img
            
        return padded
    
    def _dct_2d(self, block: np.ndarray) -> np.ndarray:
        """2D DCT变换"""
        return cv2.dct(block.astype(np.float32))
    
    def _idct_2d(self, block: np.ndarray) -> np.ndarray:
        """2D IDCT变换"""
        return cv2.idct(block)
    
    def _generate_watermark_sequence(self, size: int, seed: int = 42) -> np.ndarray:
        """生成伪随机水印序列"""
        np.random.seed(seed)
        return np.random.choice([-1, 1], size=size)
    
    def _select_embedding_positions(self, block_shape: Tuple[int, int]) -> list:
        """选择DCT系数嵌入位置（中频区域）"""
        positions = []
        for i in range(1, min(4, block_shape[0])):
            for j in range(1, min(4, block_shape[1])):
                if i + j <= 4:  # 选择中频系数
                    positions.append((i, j))
        return positions
    
    def embed_watermark(self, host_image: np.ndarray, watermark_data: str) -> np.ndarray:
        """
        在宿主图像中嵌入水印
        
        Args:
            host_image: 宿主图像
            watermark_data: 水印数据（字符串）
            
        Returns:
            嵌入水印后的图像
        """
        logger.info(f"开始嵌入水印: {watermark_data}")
        
        # 转换为灰度图像
        if len(host_image.shape) == 3:
            gray_image = cv2.cvtColor(host_image, cv2.COLOR_BGR2GRAY)
        else:
            gray_image = host_image.copy()
        
        # 填充图像
        padded_image = self._pad_image(gray_image)
        
        # 将水印数据转换为二进制
        watermark_bits = ''.join(format(ord(c), '08b') for c in watermark_data)
        watermark_bits += '1111111111111111'  # 结束标记
        
        logger.info(f"水印比特长度: {len(watermark_bits)}")
        
        # 生成伪随机序列
        watermark_sequence = self._generate_watermark_sequence(len(watermark_bits))
        
        # 获取嵌入位置
        positions = self._select_embedding_positions((self.block_size, self.block_size))
        
        watermarked_image = padded_image.astype(np.float32)
        bit_index = 0
        
        # 对每个8x8块进行DCT变换并嵌入水印
        for i in range(0, padded_image.shape[0], self.block_size):
            for j in range(0, padded_image.shape[1], self.block_size):
                if bit_index >= len(watermark_bits):
                    break
                
                # 提取8x8块
                block = watermarked_image[i:i+self.block_size, j:j+self.block_size]
                
                # DCT变换
                dct_block = self._dct_2d(block)
                
                # 嵌入水印位
                for pos_idx, (x, y) in enumerate(positions):
                    if bit_index >= len(watermark_bits):
                        break
                    
                    # 获取当前水印位
                    bit_value = int(watermark_bits[bit_index])
                    sequence_value = watermark_sequence[bit_index]
                    
                    # 修改DCT系数
                    if bit_value == 1:
                        dct_block[x, y] += self.alpha * abs(dct_block[x, y]) * sequence_value
                    else:
                        dct_block[x, y] -= self.alpha * abs(dct_block[x, y]) * sequence_value
                    
                    bit_index += 1
                
                # IDCT变换
                watermarked_image[i:i+self.block_size, j:j+self.block_size] = self._idct_2d(dct_block)
        
        # 裁剪回原始尺寸
        result = watermarked_image[:host_image.shape[0], :host_image.shape[1]]
        
        # 如果原图是彩色的，保持其他通道不变
        if len(host_image.shape) == 3:
            result_color = host_image.copy().astype(np.float32)
            result_color[:, :, 0] = result  # 假设是BGR格式，修改B通道
            return np.clip(result_color, 0, 255).astype(np.uint8)
        else:
            return np.clip(result, 0, 255).astype(np.uint8)
    
    def extract_watermark(self, watermarked_image: np.ndarray, max_length: int = 100) -> str:
        """
        从水印图像中提取水印
        
        Args:
            watermarked_image: 含水印的图像
            max_length: 水印数据的最大长度
            
        Returns:
            提取的水印字符串
        """
        logger.info("开始提取水印")
        
        # 转换为灰度图像
        if len(watermarked_image.shape) == 3:
            gray_image = cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2GRAY)
        else:
            gray_image = watermarked_image.copy()
        
        # 填充图像
        padded_image = self._pad_image(gray_image)
        
        # 获取嵌入位置
        positions = self._select_embedding_positions((self.block_size, self.block_size))
        
        # 预估最大比特数
        max_bits = max_length * 8 + 16  # 包括结束标记
        watermark_sequence = self._generate_watermark_sequence(max_bits)
        
        extracted_bits = []
        bit_index = 0
        
        # 对每个8x8块进行DCT变换并提取水印
        for i in range(0, padded_image.shape[0], self.block_size):
            for j in range(0, padded_image.shape[1], self.block_size):
                if bit_index >= max_bits:
                    break
                
                # 提取8x8块
                block = padded_image[i:i+self.block_size, j:j+self.block_size].astype(np.float32)
                
                # DCT变换
                dct_block = self._dct_2d(block)
                
                # 提取水印位
                for pos_idx, (x, y) in enumerate(positions):
                    if bit_index >= max_bits:
                        break
                    
                    # 获取DCT系数
                    coeff = dct_block[x, y]
                    sequence_value = watermark_sequence[bit_index]
                    
                    # 根据系数符号和序列值判断水印位
                    if coeff * sequence_value > 0:
                        extracted_bits.append('1')
                    else:
                        extracted_bits.append('0')
                    
                    bit_index += 1
        
        # 查找结束标记
        bit_string = ''.join(extracted_bits)
        end_marker = '1111111111111111'
        end_pos = bit_string.find(end_marker)
        
        if end_pos != -1:
            bit_string = bit_string[:end_pos]
        
        # 转换为字符串
        try:
            # 确保比特串长度是8的倍数
            if len(bit_string) % 8 != 0:
                bit_string = bit_string[:-(len(bit_string) % 8)]
            
            watermark_chars = []
            for i in range(0, len(bit_string), 8):
                byte = bit_string[i:i+8]
                if len(byte) == 8:
                    char_code = int(byte, 2)
                    if 32 <= char_code <= 126:  # 可打印ASCII字符
                        watermark_chars.append(chr(char_code))
                    else:
                        break
            
            watermark = ''.join(watermark_chars)
            logger.info(f"提取的水印: {watermark}")
            return watermark
            
        except Exception as e:
            logger.error(f"水印提取失败: {e}")
            return ""

class WatermarkTester:
    """水印鲁棒性测试器"""
    
    def __init__(self, watermark_system: DCTWatermark):
        self.watermark_system = watermark_system
    
    def apply_rotation(self, image: np.ndarray, angle: float) -> np.ndarray:
        """旋转变换"""
        h, w = image.shape[:2]
        center = (w//2, h//2)
        rotation_matrix = cv2.getRotationMatrix2D(center, angle, 1.0)
        return cv2.warpAffine(image, rotation_matrix, (w, h))
    
    def apply_flip(self, image: np.ndarray, flip_code: int) -> np.ndarray:
        """翻转变换"""
        return cv2.flip(image, flip_code)
    
    def apply_translation(self, image: np.ndarray, dx: int, dy: int) -> np.ndarray:
        """平移变换"""
        h, w = image.shape[:2]
        translation_matrix = np.float32([[1, 0, dx], [0, 1, dy]])
        return cv2.warpAffine(image, translation_matrix, (w, h))
    
    def apply_cropping(self, image: np.ndarray, crop_ratio: float = 0.8) -> np.ndarray:
        """裁剪变换"""
        h, w = image.shape[:2]
        new_h, new_w = int(h * crop_ratio), int(w * crop_ratio)
        start_h, start_w = (h - new_h) // 2, (w - new_w) // 2
        cropped = image[start_h:start_h+new_h, start_w:start_w+new_w]
        
        # 调整回原始尺寸
        return cv2.resize(cropped, (w, h))
    
    def apply_contrast_adjustment(self, image: np.ndarray, alpha: float = 1.2, beta: int = 10) -> np.ndarray:
        """对比度调整"""
        return cv2.convertScaleAbs(image, alpha=alpha, beta=beta)
    
    def apply_noise(self, image: np.ndarray, noise_level: float = 0.1) -> np.ndarray:
        """添加高斯噪声"""
        noise = np.random.normal(0, noise_level * 255, image.shape)
        noisy_image = image.astype(np.float32) + noise
        return np.clip(noisy_image, 0, 255).astype(np.uint8)
    
    def apply_jpeg_compression(self, image: np.ndarray, quality: int = 50) -> np.ndarray:
        """JPEG压缩"""
        # 保存为临时JPEG文件
        temp_path = "temp_compressed.jpg"
        cv2.imwrite(temp_path, image, [cv2.IMWRITE_JPEG_QUALITY, quality])
        compressed_image = cv2.imread(temp_path)
        
        # 清理临时文件
        if os.path.exists(temp_path):
            os.remove(temp_path)
        
        return compressed_image
    
    def run_robustness_test(self, original_image: np.ndarray, watermarked_image: np.ndarray, 
                           watermark_text: str) -> dict:
        """运行鲁棒性测试"""
        logger.info("开始鲁棒性测试")
        
        test_results = {}
        
        # 测试项目列表
        tests = [
            ("Original", lambda x: x),
            ("Horizontal Flip", lambda x: self.apply_flip(x, 1)),
            ("Vertical Flip", lambda x: self.apply_flip(x, 0)),
            ("Rotation 10°", lambda x: self.apply_rotation(x, 10)),
            ("Rotation -10°", lambda x: self.apply_rotation(x, -10)),
            ("Translation (10,10)", lambda x: self.apply_translation(x, 10, 10)),
            ("Translation (-10,-10)", lambda x: self.apply_translation(x, -10, -10)),
            ("Cropping 80%", lambda x: self.apply_cropping(x, 0.8)),
            ("Cropping 90%", lambda x: self.apply_cropping(x, 0.9)),
            ("Contrast +20%", lambda x: self.apply_contrast_adjustment(x, 1.2, 10)),
            ("Contrast -20%", lambda x: self.apply_contrast_adjustment(x, 0.8, -10)),
            ("Gaussian Noise 5%", lambda x: self.apply_noise(x, 0.05)),
            ("Gaussian Noise 10%", lambda x: self.apply_noise(x, 0.10)),
            ("JPEG Quality 90", lambda x: self.apply_jpeg_compression(x, 90)),
            ("JPEG Quality 70", lambda x: self.apply_jpeg_compression(x, 70)),
            ("JPEG Quality 50", lambda x: self.apply_jpeg_compression(x, 50)),
        ]
        
        for test_name, transform_func in tests:
            try:
                # 应用变换
                transformed_image = transform_func(watermarked_image)
                
                # 提取水印
                extracted_watermark = self.watermark_system.extract_watermark(transformed_image)
                
                # 计算相似度
                similarity = self._calculate_similarity(watermark_text, extracted_watermark)
                
                test_results[test_name] = {
                    'extracted': extracted_watermark,
                    'similarity': similarity,
                    'success': similarity > 0.7  # 70%以上认为成功
                }
                
                logger.info(f"{test_name}: {extracted_watermark} (相似度: {similarity:.2f})")
                
            except Exception as e:
                logger.error(f"{test_name} 测试失败: {e}")
                test_results[test_name] = {
                    'extracted': "",
                    'similarity': 0.0,
                    'success': False
                }
        
        return test_results
    
    def _calculate_similarity(self, original: str, extracted: str) -> float:
        """计算字符串相似度"""
        if not original or not extracted:
            return 0.0
        
        # 简单的编辑距离相似度
        max_len = max(len(original), len(extracted))
        if max_len == 0:
            return 1.0
        
        # 计算公共子序列长度
        common_chars = 0
        min_len = min(len(original), len(extracted))
        
        for i in range(min_len):
            if original[i] == extracted[i]:
                common_chars += 1
        
        return common_chars / max_len

def create_test_images():
    """创建测试图像"""
    # 创建一个简单的测试图像
    test_image = np.zeros((512, 512, 3), dtype=np.uint8)
    
    # 添加一些图案
    cv2.rectangle(test_image, (50, 50), (200, 200), (255, 0, 0), -1)
    cv2.circle(test_image, (300, 300), 80, (0, 255, 0), -1)
    cv2.rectangle(test_image, (400, 100), (500, 400), (0, 0, 255), -1)
    
    # 添加一些文字
    cv2.putText(test_image, "TEST IMAGE", (150, 450), 
                cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
    
    return test_image

def visualize_results(original_image: np.ndarray, watermarked_image: np.ndarray, 
                     test_results: dict, watermark_text: str):
    """可视化结果"""
    plt.figure(figsize=(15, 10))
    
    # 显示原图和水印图
    plt.subplot(2, 3, 1)
    plt.imshow(cv2.cvtColor(original_image, cv2.COLOR_BGR2RGB))
    plt.title("Original Image")
    plt.axis('off')
    
    plt.subplot(2, 3, 2)
    plt.imshow(cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2RGB))
    plt.title(f"Watermarked Image\nWatermark: {watermark_text}")
    plt.axis('off')
    
    # 统计测试结果
    success_count = sum(1 for result in test_results.values() if result['success'])
    total_count = len(test_results)
    
    plt.subplot(2, 3, 3)
    plt.bar(['Success', 'Failed'], [success_count, total_count - success_count], 
            color=['green', 'red'])
    plt.title(f"Test Results\n{success_count}/{total_count} Passed")
    plt.ylabel("Count")
    
    # 显示相似度分布
    plt.subplot(2, 3, 4)
    similarities = [result['similarity'] for result in test_results.values()]
    test_names = list(test_results.keys())
    
    colors = ['green' if sim > 0.7 else 'red' for sim in similarities]
    plt.barh(range(len(similarities)), similarities, color=colors)
    plt.yticks(range(len(similarities)), test_names)
    plt.xlabel("Similarity")
    plt.title("Similarity Scores")
    plt.xlim(0, 1)
    
    # 显示详细结果
    plt.subplot(2, 3, 5)
    plt.text(0.1, 0.9, "Detailed Results:", fontsize=12, fontweight='bold')
    
    y_pos = 0.8
    for test_name, result in test_results.items():
        status = "✓" if result['success'] else "✗"
        text = f"{status} {test_name}: {result['extracted'][:10]}..."
        plt.text(0.1, y_pos, text, fontsize=8)
        y_pos -= 0.05
        if y_pos < 0.1:
            break
    
    plt.xlim(0, 1)
    plt.ylim(0, 1)
    plt.axis('off')
    
    plt.tight_layout()
    plt.savefig('watermark_test_results.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    """主函数"""
    # 创建水印系统
    watermark_system = DCTWatermark(block_size=8, alpha=0.1)
    
    # 创建测试器
    tester = WatermarkTester(watermark_system)
    
    # 创建或加载测试图像
    test_image = create_test_images()
    
    # 设置水印文本
    watermark_text = "gzc2025"
    
    # 嵌入水印
    logger.info("嵌入水印...")
    watermarked_image = watermark_system.embed_watermark(test_image, watermark_text)
    
    # 验证水印嵌入
    logger.info("验证水印嵌入...")
    extracted_watermark = watermark_system.extract_watermark(watermarked_image)
    logger.info(f"原始水印: {watermark_text}")
    logger.info(f"提取水印: {extracted_watermark}")
    
    # 保存图像
    cv2.imwrite('original_image.png', test_image)
    cv2.imwrite('watermarked_image.png', watermarked_image)
    
    # 运行鲁棒性测试
    test_results = tester.run_robustness_test(test_image, watermarked_image, watermark_text)
    
    # 显示结果
    logger.info("\n=== 鲁棒性测试结果 ===")
    success_count = 0
    for test_name, result in test_results.items():
        status = "通过" if result['success'] else "失败"
        logger.info(f"{test_name}: {status} (相似度: {result['similarity']:.2f}) - {result['extracted']}")
        if result['success']:
            success_count += 1
    
    logger.info(f"\n总体结果: {success_count}/{len(test_results)} 个测试通过")
    logger.info(f"成功率: {success_count/len(test_results)*100:.1f}%")
    
    # 可视化结果
    visualize_results(test_image, watermarked_image, test_results, watermark_text)

if __name__ == "__main__":
    main()