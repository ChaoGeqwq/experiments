import sys
import os
import time
from watermark_system import WatermarkSystem
from watermark_visualizer import WatermarkVisualizer

def print_banner():
    """打印横幅"""
    print("=" * 70)
    print("               图片水印嵌入和提取系统演示")
    print("               基于DCT变换的数字水印技术")
    print("=" * 70)
    print()

def wait_for_input():
    """等待用户输入"""
    input("按Enter键继续...")
    print()

def main():
    """主演示函数"""
    print_banner()
    
    # 检查输入文件
    if not os.path.exists('picture.png'):
        print("错误: 找不到输入图像 'picture.png'")
        print("请确保在project2目录中有这个文件")
        return
    
    print("输入图像: picture.png")
    print("水印文本: WATERMARK")
    print("算法: DCT变换 + 中频系数嵌入")
    print()
    
    wait_for_input()
    
    # 创建水印系统实例
    print("正在初始化水印系统...")
    watermark_system = WatermarkSystem(block_size=8, alpha=0.1)
    print("水印系统初始化完成")
    print()
    
    # 步骤1: 嵌入水印
    print("步骤1: 嵌入水印")
    print("-" * 30)
    
    start_time = time.time()
    success = watermark_system.embed_watermark(
        'picture.png', 
        'WATERMARK', 
        'watermarked_picture.png'
    )
    end_time = time.time()
    
    if success:
        print(f"水印嵌入成功！耗时: {end_time - start_time:.2f} 秒")
        print("输出文件: watermarked_picture.png")
    else:
        print("水印嵌入失败")
        return
    
    print()
    wait_for_input()
    
    # 步骤2: 提取水印
    print("步骤2: 提取水印")
    print("-" * 30)
    
    start_time = time.time()
    success = watermark_system.extract_watermark(
        'watermarked_picture.png',
        'picture.png',
        'extracted_watermark.png'
    )
    end_time = time.time()
    
    if success:
        print(f"水印提取成功！耗时: {end_time - start_time:.2f} 秒")
        print("输出文件: extracted_watermark.png")
    else:
        print("水印提取失败")
        return
    
    print()
    wait_for_input()
    
    # 步骤3: 鲁棒性测试
    print("步骤3: 鲁棒性测试")
    print("-" * 30)
    print("正在生成各种攻击测试...")
    
    start_time = time.time()
    attacked_images = watermark_system.apply_attacks('watermarked_picture.png', 'attack_tests')
    end_time = time.time()
    
    print(f"✅生成了 {len(attacked_images)} 种攻击测试，耗时: {end_time - start_time:.2f} 秒")
    
    # 显示攻击类型
    print("\n攻击类型包括:")
    for i, (attack_name, _) in enumerate(attacked_images, 1):
        print(f"  {i:2d}. {attack_name}")
    
    print()
    wait_for_input()
    
    # 步骤4: 评估鲁棒性
    print("步骤4: 评估鲁棒性")
    print("-" * 30)
    
    start_time = time.time()
    results = watermark_system.evaluate_robustness(
        'picture.png',
        'watermarked_picture.png',
        attacked_images
    )
    end_time = time.time()
    
    print(f"鲁棒性评估完成，耗时: {end_time - start_time:.2f} 秒")
    
    # 统计结果
    success_count = sum(1 for r in results if r['extraction_success'])
    total_count = len(results)
    success_rate = success_count / total_count * 100
    
    print(f"\n测试结果统计:")
    print(f"  总测试数: {total_count}")
    print(f"  成功提取: {success_count}")
    print(f"  失败提取: {total_count - success_count}")
    print(f"  成功率: {success_rate:.1f}%")
    
    # 显示各攻击的详细结果
    print(f"\n详细结果:")
    print(f"{'攻击类型':<12} {'PSNR(dB)':<10} {'SSIM':<8} {'提取状态':<8}")
    print("-" * 45)
    
    for result in results:
        status = "✅成功" if result['extraction_success'] else "❌ 失败"
        print(f"{result['attack']:<12} {result['psnr']:<10.2f} {result['ssim']:<8.4f} {status}")
    
    print()
    wait_for_input()
    
    # 步骤5: 生成可视化
    print("步骤5: 生成可视化")
    print("-" * 30)
    
    try:
        visualizer = WatermarkVisualizer()
        
        print("正在生成水印过程可视化...")
        visualizer.visualize_watermark_process(
            'picture.png',
            'watermarked_picture.png',
            'extracted_watermark.png'
        )
        
        print("正在生成攻击效果可视化...")
        visualizer.show_attack_effects(attacked_images, 'watermarked_picture.png')
        
        print("正在生成鲁棒性结果可视化...")
        visualizer.visualize_robustness_results(results)
        
        print("✅ 可视化生成完成！")
        
    except Exception as e:
        print(f"可视化生成失败: {str(e)}")
        print("这可能是因为matplotlib显示问题，但不影响主要功能")
    
    print()
    
    # 步骤6: 生成报告
    print("步骤6: 生成测试报告")
    print("-" * 30)
    
    report_path = "watermark_demo_report.txt"
    with open(report_path, 'w', encoding='utf-8') as f:
        f.write("图片水印系统演示报告\n")
        f.write("=" * 50 + "\n\n")
        
        f.write("系统配置:\n")
        f.write(f"  - 算法: DCT变换 + 中频系数嵌入\n")
        f.write(f"  - 块大小: {watermark_system.block_size}x{watermark_system.block_size}\n")
        f.write(f"  - 嵌入强度: {watermark_system.alpha}\n")
        f.write(f"  - 水印文本: WATERMARK\n\n")
        
        f.write("文件列表:\n")
        f.write(f"  - 原始图像: picture.png\n")
        f.write(f"  - 水印图像: watermarked_picture.png\n")
        f.write(f"  - 提取水印: extracted_watermark.png\n")
        f.write(f"  - 攻击测试: attack_tests/\n\n")
        
        f.write("鲁棒性测试结果:\n")
        f.write(f"  - 总测试数: {total_count}\n")
        f.write(f"  - 成功提取: {success_count}\n")
        f.write(f"  - 成功率: {success_rate:.1f}%\n\n")
        
        f.write("详细结果:\n")
        for result in results:
            f.write(f"  {result['attack']}: ")
            f.write(f"PSNR={result['psnr']:.2f}dB, ")
            f.write(f"SSIM={result['ssim']:.4f}, ")
            f.write(f"{'成功' if result['extraction_success'] else '失败'}\n")
        
        f.write(f"\n生成时间: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
    
    print(f"✅报告已保存到: {report_path}")
    
    print("\n演示完成！")
    print("\n生成的文件包括:")
    print("  - watermarked_picture.png (水印图像)")
    print("  - extracted_watermark.png (提取的水印)")
    print("  - attack_tests/ (攻击测试目录)")
    print("  - watermark_demo_report.txt (测试报告)")
    print("  - 各种可视化PNG文件")
    
    print(f"\n水印系统性能总结:")
    print(f"  - 嵌入成功率: 100%")
    print(f"  - 提取成功率: 100%")
    print(f"  - 鲁棒性测试成功率: {success_rate:.1f}%")
    
    # 鲁棒性评级
    if success_rate >= 80:
        rating = "优秀"
    elif success_rate >= 60:
        rating = " 良好"
    elif success_rate >= 40:
        rating = "中等"
    else:
        rating = "较差"
    
    print(f"  - 鲁棒性评级: {rating}")

if __name__ == "__main__":
    main()
