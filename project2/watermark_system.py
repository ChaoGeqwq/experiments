import cv2
import numpy as np
from PIL import Image
import matplotlib.pyplot as plt
from scipy.fft import dct, idct
import os
import warnings
warnings.filterwarnings('ignore')

class WatermarkSystem:
    def __init__(self, block_size=8, alpha=0.1):
        """
        初始化水印系统
        
        Args:
            block_size: DCT块大小，通常为8x8
            alpha: 水印强度系数，控制水印的嵌入强度
        """
        self.block_size = block_size
        self.alpha = alpha
        self.watermark_positions = []  # 存储水印嵌入位置
        
    def dct2(self, block):
        """2D DCT变换"""
        return dct(dct(block.T, norm='ortho').T, norm='ortho')
    
    def idct2(self, block):
        """2D IDCT逆变换"""
        return idct(idct(block.T, norm='ortho').T, norm='ortho')
    
    def generate_watermark(self, text, size):
        """
        生成文本水印
        
        Args:
            text: 水印文本
            size: 水印大小 (width, height)
            
        Returns:
            watermark: 二值化水印图像
        """
        # 创建白色背景
        img = Image.new('RGB', size, 'white')
        
        # 使用PIL画文字
        from PIL import ImageDraw, ImageFont
        draw = ImageDraw.Draw(img)
        
        # 尝试使用系统字体
        try:
            font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 36)
        except:
            font = ImageFont.load_default()
        
        # 计算文字位置使其居中
        text_bbox = draw.textbbox((0, 0), text, font=font)
        text_width = text_bbox[2] - text_bbox[0]
        text_height = text_bbox[3] - text_bbox[1]
        
        x = (size[0] - text_width) // 2
        y = (size[1] - text_height) // 2
        
        # 绘制黑色文字
        draw.text((x, y), text, fill='black', font=font)
        
        # 转换为灰度图像
        watermark = cv2.cvtColor(np.array(img), cv2.COLOR_RGB2GRAY)
        
        # 二值化
        _, watermark = cv2.threshold(watermark, 127, 1, cv2.THRESH_BINARY_INV)
        
        return watermark.astype(np.float32)
    
    def embed_watermark(self, image_path, watermark_text, output_path):
        """
        在图像中嵌入水印
        
        Args:
            image_path: 原始图像路径
            watermark_text: 水印文本
            output_path: 输出图像路径
            
        Returns:
            success: 是否成功嵌入
        """
        try:
            # 读取原始图像
            img = cv2.imread(image_path)
            if img is None:
                print(f"无法读取图像: {image_path}")
                return False
            
            # 转换为YUV颜色空间，在Y通道嵌入水印
            yuv = cv2.cvtColor(img, cv2.COLOR_BGR2YUV)
            y_channel = yuv[:,:,0].astype(np.float32)
            
            # 生成水印
            watermark_size = (y_channel.shape[1]//4, y_channel.shape[0]//4)
            watermark = self.generate_watermark(watermark_text, watermark_size)
            
            # 调整水印大小以匹配图像
            watermark = cv2.resize(watermark, (y_channel.shape[1], y_channel.shape[0]))
            
            # 将图像和水印分割为8x8块
            h, w = y_channel.shape
            h_blocks = h // self.block_size
            w_blocks = w // self.block_size
            
            # 裁剪到块大小的整数倍
            y_channel = y_channel[:h_blocks*self.block_size, :w_blocks*self.block_size]
            watermark = watermark[:h_blocks*self.block_size, :w_blocks*self.block_size]
            
            watermarked_y = y_channel.copy()
            self.watermark_positions = []
            
            # 对每个8x8块进行DCT变换和水印嵌入
            for i in range(h_blocks):
                for j in range(w_blocks):
                    # 提取8x8块
                    img_block = y_channel[i*self.block_size:(i+1)*self.block_size, 
                                        j*self.block_size:(j+1)*self.block_size]
                    
                    watermark_block = watermark[i*self.block_size:(i+1)*self.block_size,
                                              j*self.block_size:(j+1)*self.block_size]
                    
                    # DCT变换
                    dct_block = self.dct2(img_block)
                    
                    # 在中频系数位置嵌入水印
                    # 选择(3,3)到(5,5)的中频系数
                    for u in range(3, 6):
                        for v in range(3, 6):
                            if u < self.block_size and v < self.block_size:
                                # 获取水印位值
                                watermark_bit = watermark_block[u, v]
                                
                                # 嵌入水印
                                if watermark_bit > 0.5:
                                    dct_block[u, v] = abs(dct_block[u, v]) + self.alpha * abs(dct_block[u, v])
                                else:
                                    dct_block[u, v] = abs(dct_block[u, v]) - self.alpha * abs(dct_block[u, v])
                                
                                # 记录嵌入位置
                                self.watermark_positions.append((i, j, u, v, watermark_bit))
                    
                    # IDCT逆变换
                    watermarked_block = self.idct2(dct_block)
                    
                    # 放回原图
                    watermarked_y[i*self.block_size:(i+1)*self.block_size,
                                j*self.block_size:(j+1)*self.block_size] = watermarked_block
            
            # 重新组合YUV图像
            yuv_watermarked = yuv.copy()
            yuv_watermarked[:watermarked_y.shape[0], :watermarked_y.shape[1], 0] = watermarked_y.astype(np.uint8)
            
            # 转换回BGR
            result = cv2.cvtColor(yuv_watermarked, cv2.COLOR_YUV2BGR)
            
            # 保存结果
            cv2.imwrite(output_path, result)
            print(f"水印嵌入成功，保存到: {output_path}")
            
            return True
            
        except Exception as e:
            print(f"嵌入水印时发生错误: {str(e)}")
            return False
    
    def extract_watermark(self, watermarked_image_path, original_image_path, output_path):
        """
        从水印图像中提取水印
        
        Args:
            watermarked_image_path: 含水印图像路径
            original_image_path: 原始图像路径
            output_path: 提取的水印保存路径
            
        Returns:
            success: 是否成功提取
        """
        try:
            # 读取图像
            watermarked_img = cv2.imread(watermarked_image_path)
            original_img = cv2.imread(original_image_path)
            
            if watermarked_img is None or original_img is None:
                print("无法读取图像文件")
                return False
            
            # 转换为YUV颜色空间
            watermarked_yuv = cv2.cvtColor(watermarked_img, cv2.COLOR_BGR2YUV)
            original_yuv = cv2.cvtColor(original_img, cv2.COLOR_BGR2YUV)
            
            watermarked_y = watermarked_yuv[:,:,0].astype(np.float32)
            original_y = original_yuv[:,:,0].astype(np.float32)
            
            # 确保两个图像大小相同
            min_h = min(watermarked_y.shape[0], original_y.shape[0])
            min_w = min(watermarked_y.shape[1], original_y.shape[1])
            
            watermarked_y = watermarked_y[:min_h, :min_w]
            original_y = original_y[:min_h, :min_w]
            
            # 计算块数
            h_blocks = min_h // self.block_size
            w_blocks = min_w // self.block_size
            
            # 裁剪到块大小的整数倍
            watermarked_y = watermarked_y[:h_blocks*self.block_size, :w_blocks*self.block_size]
            original_y = original_y[:h_blocks*self.block_size, :w_blocks*self.block_size]
            
            # 创建提取的水印图像
            extracted_watermark = np.zeros_like(watermarked_y)
            
            # 对每个8x8块进行DCT变换和水印提取
            for i in range(h_blocks):
                for j in range(w_blocks):
                    # 提取8x8块
                    watermarked_block = watermarked_y[i*self.block_size:(i+1)*self.block_size,
                                                    j*self.block_size:(j+1)*self.block_size]
                    
                    original_block = original_y[i*self.block_size:(i+1)*self.block_size,
                                              j*self.block_size:(j+1)*self.block_size]
                    
                    # DCT变换
                    watermarked_dct = self.dct2(watermarked_block)
                    original_dct = self.dct2(original_block)
                    
                    # 提取水印
                    extracted_block = np.zeros((self.block_size, self.block_size))
                    
                    for u in range(3, 6):
                        for v in range(3, 6):
                            if u < self.block_size and v < self.block_size:
                                # 计算差值
                                diff = watermarked_dct[u, v] - original_dct[u, v]
                                
                                # 根据差值判断水印位
                                if diff > 0:
                                    extracted_block[u, v] = 255
                                else:
                                    extracted_block[u, v] = 0
                    
                    # 放回提取的水印
                    extracted_watermark[i*self.block_size:(i+1)*self.block_size,
                                      j*self.block_size:(j+1)*self.block_size] = extracted_block
            
            # 保存提取的水印
            cv2.imwrite(output_path, extracted_watermark)
            print(f"水印提取成功，保存到: {output_path}")
            
            return True
            
        except Exception as e:
            print(f"提取水印时发生错误: {str(e)}")
            return False
    
    def apply_attacks(self, image_path, output_dir):
        """
        应用各种攻击测试鲁棒性
        
        Args:
            image_path: 输入图像路径
            output_dir: 输出目录
            
        Returns:
            attacked_images: 攻击后的图像路径列表
        """
        if not os.path.exists(output_dir):
            os.makedirs(output_dir)
        
        img = cv2.imread(image_path)
        if img is None:
            print(f"无法读取图像: {image_path}")
            return []
        
        attacked_images = []
        
        # 1. 水平翻转
        flipped_h = cv2.flip(img, 1)
        flipped_h_path = os.path.join(output_dir, "flipped_horizontal.png")
        cv2.imwrite(flipped_h_path, flipped_h)
        attacked_images.append(("水平翻转", flipped_h_path))
        
        # 2. 垂直翻转
        flipped_v = cv2.flip(img, 0)
        flipped_v_path = os.path.join(output_dir, "flipped_vertical.png")
        cv2.imwrite(flipped_v_path, flipped_v)
        attacked_images.append(("垂直翻转", flipped_v_path))
        
        # 3. 平移
        rows, cols = img.shape[:2]
        M = np.float32([[1, 0, 50], [0, 1, 30]])  # 向右50像素，向下30像素
        translated = cv2.warpAffine(img, M, (cols, rows))
        translated_path = os.path.join(output_dir, "translated.png")
        cv2.imwrite(translated_path, translated)
        attacked_images.append(("平移", translated_path))
        
        # 4. 截取 (裁剪中心80%区域)
        h, w = img.shape[:2]
        crop_h, crop_w = int(h * 0.8), int(w * 0.8)
        start_h, start_w = (h - crop_h) // 2, (w - crop_w) // 2
        cropped = img[start_h:start_h+crop_h, start_w:start_w+crop_w]
        cropped_path = os.path.join(output_dir, "cropped.png")
        cv2.imwrite(cropped_path, cropped)
        attacked_images.append(("截取", cropped_path))
        
        # 5. 调整对比度
        # 增加对比度
        alpha = 1.5  # 对比度控制
        beta = 10    # 亮度控制
        contrast_high = cv2.convertScaleAbs(img, alpha=alpha, beta=beta)
        contrast_high_path = os.path.join(output_dir, "contrast_high.png")
        cv2.imwrite(contrast_high_path, contrast_high)
        attacked_images.append(("高对比度", contrast_high_path))
        
        # 降低对比度
        alpha = 0.7
        beta = -10
        contrast_low = cv2.convertScaleAbs(img, alpha=alpha, beta=beta)
        contrast_low_path = os.path.join(output_dir, "contrast_low.png")
        cv2.imwrite(contrast_low_path, contrast_low)
        attacked_images.append(("低对比度", contrast_low_path))
        
        # 6. 旋转
        center = (cols // 2, rows // 2)
        M = cv2.getRotationMatrix2D(center, 15, 1.0)  # 旋转15度
        rotated = cv2.warpAffine(img, M, (cols, rows))
        rotated_path = os.path.join(output_dir, "rotated.png")
        cv2.imwrite(rotated_path, rotated)
        attacked_images.append(("旋转", rotated_path))
        
        # 7. 缩放
        # 缩小再放大
        scaled_down = cv2.resize(img, (cols//2, rows//2))
        scaled_up = cv2.resize(scaled_down, (cols, rows))
        scaled_path = os.path.join(output_dir, "scaled.png")
        cv2.imwrite(scaled_path, scaled_up)
        attacked_images.append(("缩放", scaled_path))
        
        # 8. 高斯噪声
        noise = np.random.normal(0, 25, img.shape).astype(np.uint8)
        noisy = cv2.add(img, noise)
        noisy_path = os.path.join(output_dir, "noisy.png")
        cv2.imwrite(noisy_path, noisy)
        attacked_images.append(("高斯噪声", noisy_path))
        
        # 9. 高斯模糊
        blurred = cv2.GaussianBlur(img, (5, 5), 0)
        blurred_path = os.path.join(output_dir, "blurred.png")
        cv2.imwrite(blurred_path, blurred)
        attacked_images.append(("高斯模糊", blurred_path))
        
        # 10. JPEG压缩
        jpeg_path = os.path.join(output_dir, "jpeg_compressed.jpg")
        cv2.imwrite(jpeg_path, img, [cv2.IMWRITE_JPEG_QUALITY, 50])
        jpeg_png_path = os.path.join(output_dir, "jpeg_compressed.png")
        jpeg_img = cv2.imread(jpeg_path)
        cv2.imwrite(jpeg_png_path, jpeg_img)
        attacked_images.append(("JPEG压缩", jpeg_png_path))
        
        print(f"生成了 {len(attacked_images)} 种攻击测试图像")
        return attacked_images
    
    def calculate_psnr(self, img1, img2):
        """计算PSNR"""
        mse = np.mean((img1 - img2) ** 2)
        if mse == 0:
            return float('inf')
        return 20 * np.log10(255.0 / np.sqrt(mse))
    
    def calculate_ssim(self, img1, img2):
        """简单的SSIM实现"""
        # 转换为灰度图
        if len(img1.shape) == 3:
            img1 = cv2.cvtColor(img1, cv2.COLOR_BGR2GRAY)
        if len(img2.shape) == 3:
            img2 = cv2.cvtColor(img2, cv2.COLOR_BGR2GRAY)
        
        # 确保数据类型为float
        img1 = img1.astype(np.float64)
        img2 = img2.astype(np.float64)
        
        # 常数
        c1 = (0.01 * 255) ** 2
        c2 = (0.03 * 255) ** 2
        
        # 计算均值
        mu1 = cv2.GaussianBlur(img1, (11, 11), 1.5)
        mu2 = cv2.GaussianBlur(img2, (11, 11), 1.5)
        
        mu1_sq = mu1 * mu1
        mu2_sq = mu2 * mu2
        mu1_mu2 = mu1 * mu2
        
        # 计算方差和协方差
        sigma1_sq = cv2.GaussianBlur(img1 * img1, (11, 11), 1.5) - mu1_sq
        sigma2_sq = cv2.GaussianBlur(img2 * img2, (11, 11), 1.5) - mu2_sq
        sigma12 = cv2.GaussianBlur(img1 * img2, (11, 11), 1.5) - mu1_mu2
        
        # 计算SSIM
        ssim_map = ((2 * mu1_mu2 + c1) * (2 * sigma12 + c2)) / ((mu1_sq + mu2_sq + c1) * (sigma1_sq + sigma2_sq + c2))
        
        return ssim_map.mean()
    
    def evaluate_robustness(self, original_path, watermarked_path, attacked_images):
        """
        评估水印的鲁棒性
        
        Args:
            original_path: 原始图像路径
            watermarked_path: 水印图像路径
            attacked_images: 攻击后的图像列表
            
        Returns:
            results: 评估结果
        """
        results = []
        
        # 读取原始图像和水印图像
        original = cv2.imread(original_path)
        watermarked = cv2.imread(watermarked_path)
        
        if original is None or watermarked is None:
            print("无法读取原始图像或水印图像")
            return results
        
        # 计算原始图像和水印图像的质量指标
        original_psnr = self.calculate_psnr(original.astype(np.float64), watermarked.astype(np.float64))
        original_ssim = self.calculate_ssim(original, watermarked)
        
        print(f"原始图像 vs 水印图像:")
        print(f"  PSNR: {original_psnr:.2f} dB")
        print(f"  SSIM: {original_ssim:.4f}")
        print("-" * 50)
        
        # 评估每种攻击
        for attack_name, attacked_path in attacked_images:
            attacked = cv2.imread(attacked_path)
            if attacked is None:
                continue
            
            # 调整图像大小以便比较
            if attacked.shape != watermarked.shape:
                attacked = cv2.resize(attacked, (watermarked.shape[1], watermarked.shape[0]))
            
            # 计算质量指标
            psnr = self.calculate_psnr(watermarked.astype(np.float64), attacked.astype(np.float64))
            ssim = self.calculate_ssim(watermarked, attacked)
            
            # 尝试提取水印
            extract_path = attacked_path.replace('.png', '_extracted.png')
            extraction_success = self.extract_watermark(attacked_path, original_path, extract_path)
            
            result = {
                'attack': attack_name,
                'psnr': psnr,
                'ssim': ssim,
                'extraction_success': extraction_success,
                'extract_path': extract_path if extraction_success else None
            }
            
            results.append(result)
            
            print(f"{attack_name}:")
            print(f"  PSNR: {psnr:.2f} dB")
            print(f"  SSIM: {ssim:.4f}")
            print(f"  水印提取: {'成功' if extraction_success else '失败'}")
            print("-" * 30)
        
        return results

def main():
    """主函数"""
    print("=" * 60)
    print("图片水印嵌入和提取系统")
    print("基于DCT变换的数字水印技术")
    print("=" * 60)
    
    # 创建水印系统实例
    watermark_system = WatermarkSystem(block_size=8, alpha=0.1)
    
    # 设置路径
    input_image = "picture.png"
    watermarked_image = "watermarked_picture.png"
    watermark_text = "WATERMARK"
    
    # 检查输入图像是否存在
    if not os.path.exists(input_image):
        print(f"错误: 输入图像 {input_image} 不存在")
        return
    
    print(f"输入图像: {input_image}")
    print(f"水印文本: {watermark_text}")
    print(f"输出图像: {watermarked_image}")
    print()
    
    # 1. 嵌入水印
    print("1. 嵌入水印...")
    success = watermark_system.embed_watermark(input_image, watermark_text, watermarked_image)
    if not success:
        print("水印嵌入失败")
        return
    
    # 2. 提取水印
    print("\n2. 提取水印...")
    extracted_watermark = "extracted_watermark.png"
    success = watermark_system.extract_watermark(watermarked_image, input_image, extracted_watermark)
    if not success:
        print("水印提取失败")
        return
    
    # 3. 生成攻击测试图像
    print("\n3. 生成攻击测试图像...")
    attack_dir = "attack_tests"
    attacked_images = watermark_system.apply_attacks(watermarked_image, attack_dir)
    
    # 4. 评估鲁棒性
    print("\n4. 评估鲁棒性...")
    results = watermark_system.evaluate_robustness(input_image, watermarked_image, attacked_images)
    
    # 5. 生成报告
    print("\n5. 生成测试报告...")
    report_path = "robustness_report.txt"
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write("图片水印鲁棒性测试报告\n")
        f.write("=" * 50 + "\n\n")
        
        f.write(f"测试图像: {input_image}\n")
        f.write(f"水印文本: {watermark_text}\n")
        f.write(f"水印图像: {watermarked_image}\n\n")
        
        f.write("鲁棒性测试结果:\n")
        f.write("-" * 30 + "\n")
        
        success_count = 0
        for result in results:
            f.write(f"攻击类型: {result['attack']}\n")
            f.write(f"PSNR: {result['psnr']:.2f} dB\n")
            f.write(f"SSIM: {result['ssim']:.4f}\n")
            f.write(f"水印提取: {'成功' if result['extraction_success'] else '失败'}\n")
            f.write("-" * 30 + "\n")
            
            if result['extraction_success']:
                success_count += 1
        
        f.write(f"\n总结:\n")
        f.write(f"总测试数: {len(results)}\n")
        f.write(f"成功提取: {success_count}\n")
        f.write(f"成功率: {success_count/len(results)*100:.1f}%\n")
    
    print(f"测试完成！报告已保存到: {report_path}")
    
    # 6. 显示结果统计
    print("\n6. 结果统计:")
    print(f"  总测试数: {len(results)}")
    success_count = sum(1 for r in results if r['extraction_success'])
    print(f"  成功提取: {success_count}")
    print(f"  成功率: {success_count/len(results)*100:.1f}%")

if __name__ == "__main__":
    main()
