import cv2
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle
import os
import sys

class WatermarkVisualizer:
    def __init__(self):
        # 设置中文字体
        plt.rcParams['font.sans-serif'] = ['Noto Sans CJK SC', 'WenQuanYi Zen Hei', 'SimHei', 'Arial Unicode MS']
        plt.rcParams['axes.unicode_minus'] = False
        
    def visualize_watermark_process(self, original_path, watermarked_path, extracted_path):
        """
        可视化水印嵌入和提取过程
        
        Args:
            original_path: 原始图像路径
            watermarked_path: 水印图像路径
            extracted_path: 提取水印路径
        """
        # 读取图像
        original = cv2.imread(original_path)
        watermarked = cv2.imread(watermarked_path)
        
        if original is None or watermarked is None:
            print("无法读取图像文件")
            return
        
        # 转换颜色空间
        original_rgb = cv2.cvtColor(original, cv2.COLOR_BGR2RGB)
        watermarked_rgb = cv2.cvtColor(watermarked, cv2.COLOR_BGR2RGB)
        
        # 计算差异图像
        diff_img = np.abs(watermarked.astype(np.float32) - original.astype(np.float32))
        diff_img = (diff_img / diff_img.max() * 255).astype(np.uint8)
        diff_rgb = cv2.cvtColor(diff_img, cv2.COLOR_BGR2RGB)
        
        # 创建子图
        fig, axes = plt.subplots(2, 3, figsize=(15, 10))
        fig.suptitle('水印嵌入和提取过程可视化', fontsize=16, fontweight='bold')
        
        # 原始图像
        axes[0, 0].imshow(original_rgb)
        axes[0, 0].set_title('原始图像')
        axes[0, 0].axis('off')
        
        # 水印图像
        axes[0, 1].imshow(watermarked_rgb)
        axes[0, 1].set_title('水印图像')
        axes[0, 1].axis('off')
        
        # 差异图像
        axes[0, 2].imshow(diff_rgb)
        axes[0, 2].set_title('差异图像 (增强显示)')
        axes[0, 2].axis('off')
        
        # 提取的水印
        if os.path.exists(extracted_path):
            extracted = cv2.imread(extracted_path, cv2.IMREAD_GRAYSCALE)
            axes[1, 0].imshow(extracted, cmap='gray')
            axes[1, 0].set_title('提取的水印')
            axes[1, 0].axis('off')
        else:
            axes[1, 0].text(0.5, 0.5, '水印提取失败', ha='center', va='center', transform=axes[1, 0].transAxes)
            axes[1, 0].set_title('提取的水印')
            axes[1, 0].axis('off')
        
        # 直方图对比
        # 原始图像直方图
        original_gray = cv2.cvtColor(original, cv2.COLOR_BGR2GRAY)
        watermarked_gray = cv2.cvtColor(watermarked, cv2.COLOR_BGR2GRAY)
        
        hist_original = cv2.calcHist([original_gray], [0], None, [256], [0, 256])
        hist_watermarked = cv2.calcHist([watermarked_gray], [0], None, [256], [0, 256])
        
        axes[1, 1].plot(hist_original, color='blue', alpha=0.7, label='原始图像')
        axes[1, 1].plot(hist_watermarked, color='red', alpha=0.7, label='水印图像')
        axes[1, 1].set_title('灰度直方图对比')
        axes[1, 1].set_xlabel('像素值')
        axes[1, 1].set_ylabel('频数')
        axes[1, 1].legend()
        axes[1, 1].grid(True, alpha=0.3)
        
        # 质量指标
        psnr = self.calculate_psnr(original.astype(np.float64), watermarked.astype(np.float64))
        ssim = self.calculate_ssim(original, watermarked)
        
        axes[1, 2].axis('off')
        axes[1, 2].text(0.1, 0.8, f'质量指标:', fontsize=14, fontweight='bold', transform=axes[1, 2].transAxes)
        axes[1, 2].text(0.1, 0.6, f'PSNR: {psnr:.2f} dB', fontsize=12, transform=axes[1, 2].transAxes)
        axes[1, 2].text(0.1, 0.4, f'SSIM: {ssim:.4f}', fontsize=12, transform=axes[1, 2].transAxes)
        
        # 图像尺寸信息
        axes[1, 2].text(0.1, 0.2, f'图像尺寸: {original.shape[1]}×{original.shape[0]}', fontsize=10, transform=axes[1, 2].transAxes)
        
        plt.tight_layout()
        plt.savefig('watermark_process_visualization.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        print("可视化结果已保存到: watermark_process_visualization.png")
    
    def visualize_robustness_results(self, results):
        """
        可视化鲁棒性测试结果
        
        Args:
            results: 鲁棒性测试结果列表
        """
        # 提取数据
        attacks = [r['attack'] for r in results]
        psnr_values = [r['psnr'] for r in results]
        ssim_values = [r['ssim'] for r in results]
        success_flags = [r['extraction_success'] for r in results]
        
        # 创建子图
        fig, axes = plt.subplots(2, 2, figsize=(15, 10))
        fig.suptitle('水印鲁棒性测试结果', fontsize=16, fontweight='bold')
        
        # PSNR柱状图
        colors = ['green' if s else 'red' for s in success_flags]
        axes[0, 0].bar(range(len(attacks)), psnr_values, color=colors, alpha=0.7)
        axes[0, 0].set_title('PSNR值 (dB)')
        axes[0, 0].set_ylabel('PSNR (dB)')
        axes[0, 0].set_xticks(range(len(attacks)))
        axes[0, 0].set_xticklabels(attacks, rotation=45, ha='right')
        axes[0, 0].grid(True, alpha=0.3)
        
        # SSIM柱状图
        axes[0, 1].bar(range(len(attacks)), ssim_values, color=colors, alpha=0.7)
        axes[0, 1].set_title('SSIM值')
        axes[0, 1].set_ylabel('SSIM')
        axes[0, 1].set_xticks(range(len(attacks)))
        axes[0, 1].set_xticklabels(attacks, rotation=45, ha='right')
        axes[0, 1].grid(True, alpha=0.3)
        
        # 成功率饼图
        success_count = sum(success_flags)
        fail_count = len(success_flags) - success_count
        
        axes[1, 0].pie([success_count, fail_count], 
                      labels=[f'成功 ({success_count})', f'失败 ({fail_count})'],
                      colors=['green', 'red'], 
                      autopct='%1.1f%%',
                      startangle=90)
        axes[1, 0].set_title('水印提取成功率')
        
        # 质量指标散点图
        axes[1, 1].scatter(psnr_values, ssim_values, c=colors, alpha=0.7, s=100)
        axes[1, 1].set_xlabel('PSNR (dB)')
        axes[1, 1].set_ylabel('SSIM')
        axes[1, 1].set_title('PSNR vs SSIM')
        axes[1, 1].grid(True, alpha=0.3)
        
        # 添加图例
        from matplotlib.patches import Patch
        legend_elements = [Patch(facecolor='green', alpha=0.7, label='提取成功'),
                          Patch(facecolor='red', alpha=0.7, label='提取失败')]
        axes[1, 1].legend(handles=legend_elements, loc='upper right')
        
        plt.tight_layout()
        plt.savefig('robustness_results_visualization.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        print("鲁棒性测试可视化结果已保存到: robustness_results_visualization.png")
    
    def show_attack_effects(self, attacked_images, original_path):
        """
        显示各种攻击对图像的影响
        
        Args:
            attacked_images: 攻击后的图像列表
            original_path: 原始图像路径
        """
        # 读取原始图像
        original = cv2.imread(original_path)
        if original is None:
            print(f"无法读取原始图像: {original_path}")
            return
        
        original_rgb = cv2.cvtColor(original, cv2.COLOR_BGR2RGB)
        
        # 计算网格布局
        n_images = len(attacked_images) + 1  # +1 for original
        n_cols = 4
        n_rows = (n_images + n_cols - 1) // n_cols
        
        # 创建大图
        fig, axes = plt.subplots(n_rows, n_cols, figsize=(16, 4 * n_rows))
        fig.suptitle('各种攻击对图像的影响', fontsize=16, fontweight='bold')
        
        # 展平axes数组以便索引
        if n_rows == 1:
            axes = [axes]
        axes = np.array(axes).flatten()
        
        # 显示原始图像
        axes[0].imshow(original_rgb)
        axes[0].set_title('原始图像')
        axes[0].axis('off')
        
        # 显示攻击后的图像
        for i, (attack_name, attacked_path) in enumerate(attacked_images, 1):
            if i < len(axes):
                attacked = cv2.imread(attacked_path)
                if attacked is not None:
                    attacked_rgb = cv2.cvtColor(attacked, cv2.COLOR_BGR2RGB)
                    axes[i].imshow(attacked_rgb)
                    axes[i].set_title(attack_name)
                    axes[i].axis('off')
                else:
                    axes[i].text(0.5, 0.5, f'无法读取\n{attack_name}', 
                               ha='center', va='center', transform=axes[i].transAxes)
                    axes[i].set_title(attack_name)
                    axes[i].axis('off')
        
        # 隐藏多余的子图
        for i in range(n_images, len(axes)):
            axes[i].axis('off')
        
        plt.tight_layout()
        plt.savefig('attack_effects_visualization.png', dpi=300, bbox_inches='tight')
        plt.show()
        
        print("攻击效果可视化结果已保存到: attack_effects_visualization.png")
    
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

def main():
    """主函数"""
    visualizer = WatermarkVisualizer()
    
    # 检查必要的文件
    required_files = ['picture.png', 'watermarked_picture.png', 'extracted_watermark.png']
    for file in required_files:
        if not os.path.exists(file):
            print(f"错误: 文件 {file} 不存在")
            print("请先运行 watermark_system.py 生成必要的文件")
            return
    
    print("正在生成可视化...")
    
    # 1. 可视化水印嵌入和提取过程
    visualizer.visualize_watermark_process('picture.png', 'watermarked_picture.png', 'extracted_watermark.png')
    
    # 2. 显示攻击效果
    attack_dir = "attack_tests"
    if os.path.exists(attack_dir):
        attacked_images = []
        for file in os.listdir(attack_dir):
            if file.endswith('.png') and not file.endswith('_extracted.png'):
                attack_name = file.replace('.png', '').replace('_', ' ')
                attacked_images.append((attack_name, os.path.join(attack_dir, file)))
        
        if attacked_images:
            visualizer.show_attack_effects(attacked_images, 'watermarked_picture.png')
    
    print("可视化完成！")

if __name__ == "__main__":
    main()
