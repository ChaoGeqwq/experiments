import cv2
import numpy as np
import matplotlib.pyplot as plt
from PIL import Image
import os
import logging
from typing import Tuple, Optional
import argparse


# 配置matplotlib中文字体
plt.rcParams['font.sans-serif'] = ['SimHei', 'DejaVu Sans', 'Arial Unicode MS', 'sans-serif']
plt.rcParams['axes.unicode_minus'] = False

# 配置日志
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)

class DCTWatermark:
    """基于DCT变换的数字水印系统 - 修复版本"""
    
    def __init__(self, block_size: int = 8, alpha: float = 0.8):
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
        # 选择更稳定的中频系数位置
        stable_positions = [(1, 1), (1, 2), (2, 1), (2, 2), (1, 3), (3, 1)]
        for pos in stable_positions:
            if pos[0] < block_shape[0] and pos[1] < block_shape[1]:
                positions.append(pos)
        return positions[:4]  # 只使用前4个位置
    
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
        
        # 将字符串转换为二进制序列
        watermark_bits = self._string_to_bits(watermark_data)
        logger.info(f"水印二进制序列长度: {len(watermark_bits)}")
        
        # 计算可用的嵌入位置数量
        h, w = padded_image.shape
        num_blocks = (h // self.block_size) * (w // self.block_size)
        embed_positions = self._select_embedding_positions((self.block_size, self.block_size))
        max_bits = num_blocks * len(embed_positions)
        
        logger.info(f"最大可嵌入位数: {max_bits}, 实际需要嵌入: {len(watermark_bits)}")
        
        if len(watermark_bits) > max_bits:
            raise ValueError(f"水印数据过长，最大支持{max_bits}位，实际{len(watermark_bits)}位")
        
        # 嵌入水印
        watermarked_image = padded_image.copy().astype(np.float32)
        bit_index = 0
        
        for i in range(0, h, self.block_size):
            for j in range(0, w, self.block_size):
                if bit_index >= len(watermark_bits):
                    break
                    
                block = watermarked_image[i:i+self.block_size, j:j+self.block_size]
                dct_block = self._dct_2d(block)
                
                # 在每个块中嵌入多个位
                for pos in embed_positions:
                    if bit_index >= len(watermark_bits):
                        break
                    
                    # 使用量化水印嵌入方法
                    coeff = dct_block[pos[0], pos[1]]
                    bit_value = watermark_bits[bit_index]
                    
                    # 量化嵌入：修改系数使其末位与水印位匹配
                    if bit_value == 1:
                        # 使系数为正且较大
                        if coeff < 0:
                            dct_block[pos[0], pos[1]] = -coeff + self.alpha
                        else:
                            dct_block[pos[0], pos[1]] = coeff + self.alpha
                    else:
                        # 使系数为负且较小
                        if coeff > 0:
                            dct_block[pos[0], pos[1]] = -coeff - self.alpha
                        else:
                            dct_block[pos[0], pos[1]] = coeff - self.alpha
                    
                    bit_index += 1
                
                # 逆DCT变换
                idct_block = self._idct_2d(dct_block)
                watermarked_image[i:i+self.block_size, j:j+self.block_size] = idct_block
            
            if bit_index >= len(watermark_bits):
                break
        
        # 裁剪回原始尺寸
        watermarked_image = watermarked_image[:host_image.shape[0], :host_image.shape[1]]
        
        # 转换回原始格式
        if len(host_image.shape) == 3:
            watermarked_bgr = cv2.cvtColor(watermarked_image.astype(np.uint8), cv2.COLOR_GRAY2BGR)
            return watermarked_bgr
        else:
            return watermarked_image.astype(np.uint8)
    
    def extract_watermark(self, watermarked_image: np.ndarray, watermark_length: int) -> str:
        """
        从水印图像中提取水印
        
        Args:
            watermarked_image: 含水印的图像
            watermark_length: 期望的水印长度
            
        Returns:
            提取的水印字符串
        """
        logger.info(f"开始提取水印，期望长度: {watermark_length}")
        
        # 转换为灰度图像
        if len(watermarked_image.shape) == 3:
            gray_image = cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2GRAY)
        else:
            gray_image = watermarked_image.copy()
        
        # 填充图像
        padded_image = self._pad_image(gray_image)
        
        # 计算需要提取的位数
        watermark_bits_length = watermark_length * 8
        
        # 提取水印位
        extracted_bits = []
        embed_positions = self._select_embedding_positions((self.block_size, self.block_size))
        
        h, w = padded_image.shape
        bit_index = 0
        
        for i in range(0, h, self.block_size):
            for j in range(0, w, self.block_size):
                if bit_index >= watermark_bits_length:
                    break
                    
                block = padded_image[i:i+self.block_size, j:j+self.block_size].astype(np.float32)
                dct_block = self._dct_2d(block)
                
                # 从每个块中提取多个位
                for pos in embed_positions:
                    if bit_index >= watermark_bits_length:
                        break
                    
                    coeff = dct_block[pos[0], pos[1]]
                    # 根据系数符号判断水印位
                    if coeff >= 0:
                        extracted_bits.append(1)
                    else:
                        extracted_bits.append(0)
                    
                    bit_index += 1
            
            if bit_index >= watermark_bits_length:
                break
        
        logger.info(f"提取到的位数: {len(extracted_bits)}")
        
        # 将位序列转换为字符串
        extracted_watermark = self._bits_to_string(extracted_bits)
        logger.info(f"提取的水印: {extracted_watermark}")
        
        return extracted_watermark
    
    def _string_to_bits(self, text: str) -> list:
        """将字符串转换为位序列"""
        bits = []
        for char in text:
            # 每个字符转换为8位二进制
            ascii_val = ord(char)
            char_bits = [(ascii_val >> i) & 1 for i in range(8)]
            bits.extend(char_bits)
        return bits
    
    def _bits_to_string(self, bits: list) -> str:
        """将位序列转换为字符串"""
        if len(bits) % 8 != 0:
            # 补齐到8的倍数
            bits.extend([0] * (8 - len(bits) % 8))
        
        text = ""
        for i in range(0, len(bits), 8):
            byte_bits = bits[i:i+8]
            ascii_val = sum(bit * (2 ** j) for j, bit in enumerate(byte_bits))
            if 32 <= ascii_val <= 126:  # 可打印字符范围
                text += chr(ascii_val)
            else:
                text += '?'  # 无效字符用?代替
        
        return text
    
    def calculate_psnr(self, original: np.ndarray, watermarked: np.ndarray) -> float:
        """计算PSNR"""
        mse = np.mean((original.astype(np.float64) - watermarked.astype(np.float64)) ** 2)
        if mse == 0:
            return float('inf')
        return 20 * np.log10(255.0 / np.sqrt(mse))

class WatermarkTester:
    """水印鲁棒性测试器"""
    
    def __init__(self, watermark_system: DCTWatermark):
        self.watermark_system = watermark_system
        
    def flip_horizontal(self, image: np.ndarray) -> np.ndarray:
        """水平翻转"""
        return cv2.flip(image, 1)
    
    def flip_vertical(self, image: np.ndarray) -> np.ndarray:
        """垂直翻转"""
        return cv2.flip(image, 0)
    
    def rotate(self, image: np.ndarray, angle: float) -> np.ndarray:
        """旋转图像"""
        h, w = image.shape[:2]
        center = (w // 2, h // 2)
        matrix = cv2.getRotationMatrix2D(center, angle, 1.0)
        return cv2.warpAffine(image, matrix, (w, h))
    
    def translate(self, image: np.ndarray, dx: int, dy: int) -> np.ndarray:
        """平移图像"""
        h, w = image.shape[:2]
        matrix = np.float32([[1, 0, dx], [0, 1, dy]])
        return cv2.warpAffine(image, matrix, (w, h))
    
    def crop_center(self, image: np.ndarray, crop_ratio: float = 0.8) -> np.ndarray:
        """中心裁剪"""
        h, w = image.shape[:2]
        new_h, new_w = int(h * crop_ratio), int(w * crop_ratio)
        start_h, start_w = (h - new_h) // 2, (w - new_w) // 2
        cropped = image[start_h:start_h+new_h, start_w:start_w+new_w]
        # 调整回原尺寸
        return cv2.resize(cropped, (w, h))
    
    def adjust_contrast(self, image: np.ndarray, alpha: float = 1.2, beta: int = 10) -> np.ndarray:
        """调整对比度和亮度"""
        return cv2.convertScaleAbs(image, alpha=alpha, beta=beta)
    
    def add_gaussian_noise(self, image: np.ndarray, mean: float = 0, std: float = 25) -> np.ndarray:
        """添加高斯噪声"""
        noise = np.random.normal(mean, std, image.shape).astype(np.float32)
        noisy_image = image.astype(np.float32) + noise
        return np.clip(noisy_image, 0, 255).astype(np.uint8)
    
    def jpeg_compression(self, image: np.ndarray, quality: int = 70) -> np.ndarray:
        """JPEG压缩"""
        encode_param = [int(cv2.IMWRITE_JPEG_QUALITY), quality]
        _, encoded = cv2.imencode('.jpg', image, encode_param)
        return cv2.imdecode(encoded, cv2.IMREAD_COLOR if len(image.shape) == 3 else cv2.IMREAD_GRAYSCALE)
    
    def calculate_similarity(self, original: str, extracted: str) -> float:
        """计算字符串相似度"""
        if len(original) == 0 and len(extracted) == 0:
            return 1.0
        if len(original) == 0 or len(extracted) == 0:
            return 0.0
        
        # 使用编辑距离计算相似度
        def edit_distance(s1, s2):
            m, n = len(s1), len(s2)
            dp = [[0] * (n + 1) for _ in range(m + 1)]
            
            for i in range(m + 1):
                dp[i][0] = i
            for j in range(n + 1):
                dp[0][j] = j
            
            for i in range(1, m + 1):
                for j in range(1, n + 1):
                    if s1[i-1] == s2[j-1]:
                        dp[i][j] = dp[i-1][j-1]
                    else:
                        dp[i][j] = min(dp[i-1][j], dp[i][j-1], dp[i-1][j-1]) + 1
            
            return dp[m][n]
        
        distance = edit_distance(original, extracted)
        max_len = max(len(original), len(extracted))
        return 1.0 - distance / max_len
    
    def run_robustness_test(self, original_image: np.ndarray, watermark_text: str) -> dict:
        """运行鲁棒性测试"""
        logger.info("开始鲁棒性测试...")
        
        # 首先嵌入水印
        watermarked_image = self.watermark_system.embed_watermark(original_image, watermark_text)
        
        # 计算PSNR
        psnr = self.watermark_system.calculate_psnr(original_image, watermarked_image)
        logger.info(f"原始图像PSNR: {psnr:.2f}dB")
        
        # 定义测试攻击
        attacks = {
            "无攻击": lambda img: img,
            "水平翻转": self.flip_horizontal,
            "垂直翻转": self.flip_vertical,
            "旋转15度": lambda img: self.rotate(img, 15),
            "旋转-15度": lambda img: self.rotate(img, -15),
            "平移(10,10)": lambda img: self.translate(img, 10, 10),
            "平移(-10,-10)": lambda img: self.translate(img, -10, -10),
            "中心裁剪80%": lambda img: self.crop_center(img, 0.8),
            "中心裁剪90%": lambda img: self.crop_center(img, 0.9),
            "对比度增强": lambda img: self.adjust_contrast(img, 1.3, 10),
            "对比度减弱": lambda img: self.adjust_contrast(img, 0.7, -10),
            "高斯噪声": lambda img: self.add_gaussian_noise(img, 0, 20),
            "JPEG压缩70%": lambda img: self.jpeg_compression(img, 70),
            "JPEG压缩50%": lambda img: self.jpeg_compression(img, 50)
        }
        
        results = {}
        similarities = []
        
        for attack_name, attack_func in attacks.items():
            logger.info(f"测试攻击: {attack_name}")
            
            # 应用攻击
            attacked_image = attack_func(watermarked_image)
            
            # 提取水印
            extracted_watermark = self.watermark_system.extract_watermark(
                attacked_image, len(watermark_text)
            )
            
            # 计算相似度
            similarity = self.calculate_similarity(watermark_text, extracted_watermark)
            similarities.append(similarity)
            
            results[attack_name] = {
                "extracted": extracted_watermark,
                "similarity": similarity,
                "success": similarity > 0.5  # 相似度阈值
            }
            
            logger.info(f"  原始水印: {watermark_text}")
            logger.info(f"  提取水印: {extracted_watermark}")
            logger.info(f"  相似度: {similarity:.2f}")
        
        # 计算总体统计
        total_tests = len(attacks)
        successful_tests = sum(1 for r in results.values() if r["success"])
        average_similarity = np.mean(similarities)
        
        logger.info(f"\n=== 测试结果统计 ===")
        logger.info(f"总测试数: {total_tests}")
        logger.info(f"成功测试数: {successful_tests}")
        logger.info(f"成功率: {successful_tests/total_tests*100:.1f}%")
        logger.info(f"平均相似度: {average_similarity:.3f}")
        logger.info(f"PSNR: {psnr:.2f}dB")
        
        return {
            "results": results,
            "statistics": {
                "total_tests": total_tests,
                "successful_tests": successful_tests,
                "success_rate": successful_tests / total_tests,
                "average_similarity": average_similarity,
                "psnr": psnr
            },
            "watermarked_image": watermarked_image
        }

def create_test_images():
    """创建测试图像"""
    # 创建一个复杂的测试图像
    img = np.zeros((256, 256, 3), dtype=np.uint8)
    
    # 添加渐变背景
    for i in range(256):
        for j in range(256):
            img[i, j] = [i, j, (i+j)//2]
    
    # 添加一些几何形状增加纹理
    cv2.circle(img, (64, 64), 30, (255, 255, 255), -1)
    cv2.rectangle(img, (150, 150), (200, 200), (128, 128, 128), -1)
    cv2.line(img, (0, 128), (255, 128), (255, 0, 0), 2)
    cv2.line(img, (128, 0), (128, 255), (0, 255, 0), 2)
    
    # 添加一些文字
    cv2.putText(img, 'TEST', (80, 120), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
    
    return img

def visualize_results(results: dict, original_image: np.ndarray):
    """可视化测试结果"""
    fig, axes = plt.subplots(2, 2, figsize=(15, 12))
    
    # 显示原始图像
    if len(original_image.shape) == 3:
        axes[0, 0].imshow(cv2.cvtColor(original_image, cv2.COLOR_BGR2RGB))
    else:
        axes[0, 0].imshow(original_image, cmap='gray')
    axes[0, 0].set_title('原始图像')
    axes[0, 0].axis('off')
    
    # 显示水印图像
    watermarked_image = results["watermarked_image"]
    if len(watermarked_image.shape) == 3:
        axes[0, 1].imshow(cv2.cvtColor(watermarked_image, cv2.COLOR_BGR2RGB))
    else:
        axes[0, 1].imshow(watermarked_image, cmap='gray')
    axes[0, 1].set_title(f'水印图像 (PSNR: {results["statistics"]["psnr"]:.2f}dB)')
    axes[0, 1].axis('off')
    
    # 显示鲁棒性测试结果
    attack_names = list(results["results"].keys())
    similarities = [results["results"][name]["similarity"] for name in attack_names]
    
    axes[1, 0].bar(range(len(attack_names)), similarities)
    axes[1, 0].set_xlabel('攻击类型')
    axes[1, 0].set_ylabel('相似度')
    axes[1, 0].set_title('各种攻击下的水印相似度')
    axes[1, 0].set_xticks(range(len(attack_names)))
    axes[1, 0].set_xticklabels(attack_names, rotation=45, ha='right')
    axes[1, 0].grid(True, alpha=0.3)
    
    # 显示统计信息
    stats = results["statistics"]
    stats_text = f"""
    总测试数: {stats['total_tests']}
    成功测试数: {stats['successful_tests']}
    成功率: {stats['success_rate']*100:.1f}%
    平均相似度: {stats['average_similarity']:.3f}
    PSNR: {stats['psnr']:.2f}dB
    """
    
    axes[1, 1].text(0.1, 0.5, stats_text, transform=axes[1, 1].transAxes, 
                   fontsize=12, verticalalignment='center')
    axes[1, 1].set_title('测试统计信息')
    axes[1, 1].axis('off')
    
    plt.tight_layout()
    plt.savefig('watermark_test_results.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    """主函数"""
    # 创建水印系统，增加嵌入强度
    watermark_system = DCTWatermark(block_size=8, alpha=0.8)
    
    # 创建测试器
    tester = WatermarkTester(watermark_system)
    
    # 创建或加载测试图像
    test_image = create_test_images()
    
    # 保存原始图像
    cv2.imwrite('original_image.png', test_image)
    logger.info("原始图像已保存为 original_image.png")
    
    # 设置水印文本（使用较短的文本进行测试）
    watermark_text = "TEST"
    
    # 先进行简单测试
    logger.info("=" * 50)
    logger.info("开始简单水印测试...")
    
    # 嵌入水印
    logger.info("嵌入水印...")
    watermarked_image = watermark_system.embed_watermark(test_image, watermark_text)
    
    # 保存水印图像
    cv2.imwrite('watermarked_image.png', watermarked_image)
    logger.info("水印图像已保存为 watermarked_image.png")
    
    # 计算PSNR
    psnr = watermark_system.calculate_psnr(test_image, watermarked_image)
    logger.info(f"PSNR: {psnr:.2f}dB")
    
    # 提取水印
    logger.info("提取水印...")
    extracted_watermark = watermark_system.extract_watermark(watermarked_image, len(watermark_text))
    
    # 计算相似度
    similarity = tester.calculate_similarity(watermark_text, extracted_watermark)
    
    logger.info(f"原始水印: '{watermark_text}'")
    logger.info(f"提取水印: '{extracted_watermark}'")
    logger.info(f"相似度: {similarity:.3f}")
    
    if similarity > 0.5:
        logger.info("✓ 基本水印测试通过！")
        
        # 运行完整鲁棒性测试
        logger.info("=" * 50)
        logger.info("开始完整鲁棒性测试...")
        results = tester.run_robustness_test(test_image, watermark_text)
        
        # 可视化结果
        visualize_results(results, test_image)
        
        logger.info("测试完成！结果已保存为 watermark_test_results.png")
    else:
        logger.error("✗ 基本水印测试失败，请检查算法实现")

if __name__ == "__main__":
    main()
