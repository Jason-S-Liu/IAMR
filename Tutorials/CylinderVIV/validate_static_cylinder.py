#!/usr/bin/env python3
"""
静止圆柱绕流验证脚本
验证Re=100时的阻力系数和升力系数
"""

import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
from scipy import stats
import os
import sys

def read_force_data(filename):
    """读取力数据文件"""
    try:
        data = pd.read_csv(filename, delim_whitespace=True, comment='#')
        return data
    except FileNotFoundError:
        print(f"警告: 找不到文件 {filename}")
        return None

def compute_drag_coefficient(force_data, rho=1.0, U=1.0, D=0.02):
    """计算阻力系数"""
    if force_data is None:
        return None
    
    # 提取阻力 (Fx) - 3D情况下可能有不同的列索引
    if len(force_data.columns) >= 4:
        Fx = force_data.iloc[:, 3].values  # 第4列是Fx
    else:
        print("警告：力数据列数不足")
        return None
    
    # 计算阻力系数 Cd = 2*Fx/(ρ*U²*D)
    Cd = 2.0 * Fx / (rho * U * U * D)
    
    return Cd

def compute_lift_coefficient(force_data, rho=1.0, U=1.0, D=0.02):
    """计算升力系数"""
    if force_data is None:
        return None
    
    # 提取升力 (Fy)
    Fy = force_data.iloc[:, 4].values  # 第5列是Fy
    
    # 计算升力系数 Cl = 2*Fy/(ρ*U²*D)
    Cl = 2.0 * Fy / (rho * U * U * D)
    
    return Cl

def compute_strouhal_number(force_data, U=1.0, D=0.02):
    """计算Strouhal数"""
    if force_data is None:
        return None
    
    # 提取升力时间序列
    Fy = force_data.iloc[:, 4].values
    time = force_data.iloc[:, 1].values
    
    # 使用FFT计算频率
    if len(Fy) > 100:  # 确保有足够的数据点
        # 去除线性趋势
        Fy_detrend = Fy - np.polyfit(time, Fy, 1)[0] * time - np.polyfit(time, Fy, 1)[1]
        
        # FFT
        fft = np.fft.fft(Fy_detrend)
        freqs = np.fft.fftfreq(len(Fy_detrend), time[1] - time[0])
        
        # 找到主要频率
        power = np.abs(fft)**2
        main_freq_idx = np.argmax(power[1:len(power)//2]) + 1
        main_freq = np.abs(freqs[main_freq_idx])
        
        # 计算Strouhal数
        St = main_freq * D / U
        
        return St
    
    return None

def validate_results():
    """验证结果与理论值比较"""
    print("=== 静止圆柱绕流验证 (Re=100) ===")
    
    # 理论值 (Re=100)
    theoretical_Cd = 1.4  # 阻力系数理论值
    theoretical_St = 0.165  # Strouhal数理论值
    
    # 读取模拟结果
    force_file = "cylinder_force_ibm_0.dat"
    force_data = read_force_data(force_file)
    
    if force_data is None:
        print("错误: 无法读取力数据文件")
        return False
    
    # 计算系数
    Cd = compute_drag_coefficient(force_data)
    Cl = compute_lift_coefficient(force_data)
    St = compute_strouhal_number(force_data)
    
    if Cd is not None:
        # 计算稳态阻力系数 (使用后半段数据)
        n_half = len(Cd) // 2
        Cd_steady = np.mean(Cd[n_half:])
        Cd_std = np.std(Cd[n_half:])
        
        print(f"阻力系数 Cd = {Cd_steady:.3f} ± {Cd_std:.3f}")
        print(f"理论值 Cd = {theoretical_Cd:.3f}")
        print(f"相对误差 = {abs(Cd_steady - theoretical_Cd) / theoretical_Cd * 100:.1f}%")
        
        # 判断是否在可接受范围内 (误差 < 20%)
        if abs(Cd_steady - theoretical_Cd) / theoretical_Cd < 0.2:
            print("✓ 阻力系数验证通过")
        else:
            print("✗ 阻力系数验证失败")
    
    if St is not None:
        print(f"Strouhal数 St = {St:.3f}")
        print(f"理论值 St = {theoretical_St:.3f}")
        print(f"相对误差 = {abs(St - theoretical_St) / theoretical_St * 100:.1f}%")
        
        # 判断是否在可接受范围内 (误差 < 15%)
        if abs(St - theoretical_St) / theoretical_St < 0.15:
            print("✓ Strouhal数验证通过")
        else:
            print("✗ Strouhal数验证失败")
    
    return True

def plot_results():
    """绘制结果"""
    force_file = "cylinder_force_ibm_0.dat"
    force_data = read_force_data(force_file)
    
    if force_data is None:
        return
    
    # 创建图形
    fig, axes = plt.subplots(2, 2, figsize=(12, 8))
    
    # 时间序列
    time = force_data.iloc[:, 1].values
    
    # 1. 阻力系数时间序列
    Cd = compute_drag_coefficient(force_data)
    axes[0, 0].plot(time, Cd, 'b-', linewidth=1)
    axes[0, 0].set_xlabel('时间 (s)')
    axes[0, 0].set_ylabel('阻力系数 Cd')
    axes[0, 0].set_title('阻力系数时间序列')
    axes[0, 0].grid(True)
    
    # 2. 升力系数时间序列
    Cl = compute_lift_coefficient(force_data)
    axes[0, 1].plot(time, Cl, 'r-', linewidth=1)
    axes[0, 1].set_xlabel('时间 (s)')
    axes[0, 1].set_ylabel('升力系数 Cl')
    axes[0, 1].set_title('升力系数时间序列')
    axes[0, 1].grid(True)
    
    # 3. 升力系数频谱
    if len(Cl) > 100:
        Fy = force_data.iloc[:, 4].values
        Fy_detrend = Fy - np.polyfit(time, Fy, 1)[0] * time - np.polyfit(time, Fy, 1)[1]
        fft = np.fft.fft(Fy_detrend)
        freqs = np.fft.fftfreq(len(Fy_detrend), time[1] - time[0])
        power = np.abs(fft)**2
        
        # 只显示正频率部分
        pos_freqs = freqs[:len(freqs)//2]
        pos_power = power[:len(power)//2]
        
        axes[1, 0].plot(pos_freqs, pos_power, 'g-', linewidth=1)
        axes[1, 0].set_xlabel('频率 (Hz)')
        axes[1, 0].set_ylabel('功率谱密度')
        axes[1, 0].set_title('升力频谱')
        axes[1, 0].grid(True)
        axes[1, 0].set_xlim(0, 50)  # 限制频率范围
    
    # 4. 相位图 (升力 vs 阻力)
    axes[1, 1].plot(Cd, Cl, 'k-', linewidth=0.5, alpha=0.7)
    axes[1, 1].set_xlabel('阻力系数 Cd')
    axes[1, 1].set_ylabel('升力系数 Cl')
    axes[1, 1].set_title('升力-阻力相位图')
    axes[1, 1].grid(True)
    axes[1, 1].axis('equal')
    
    plt.tight_layout()
    plt.savefig('static_cylinder_validation.png', dpi=300, bbox_inches='tight')
    plt.show()

def main():
    """主函数"""
    print("开始静止圆柱绕流验证...")
    
    # 验证结果
    success = validate_results()
    
    # 绘制结果
    plot_results()
    
    if success:
        print("\n验证完成！")
    else:
        print("\n验证失败！")
        sys.exit(1)

if __name__ == "__main__":
    main() 