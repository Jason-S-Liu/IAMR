# 圆柱涡激振动 (VIV) 教程 - 3D版本


## 版本信息
- **维度**: 3D
- **编译目标**: main3d.gnu.ex
- **输入文件**: inputs.3d.static_cylinder

## 文件结构

```
CylinderVIV/
├── inputs.2d.static_cylinder      # 静止圆柱绕流验证案例
├── inputs.2d.cylinder_viv         # 涡激振动案例
├── GNUmakefile                    # 编译配置
├── run_static_validation.sh       # 验证运行脚本
├── validate_static_cylinder.py    # 验证脚本
└── README.md                      # 本文件
```

## 验证案例

### 1. 静止圆柱绕流 (Re=100)

**参数设置：**
- 雷诺数: Re = 100
- 圆柱直径: D = 0.02 m
- 来流速度: U = 1.0 m/s
- 运动粘度: ν = 0.00001 m²/s

**理论值：**
- 阻力系数: Cd ≈ 1.4
- Strouhal数: St ≈ 0.165

**运行方法：**
```bash
# 方法1: 使用运行脚本
chmod +x run_static_validation.sh
./run_static_validation.sh

# 方法2: 手动运行
export USE_PARTICLES=TRUE
export USE_EB=TRUE
export USE_CYLINDER_VIV=TRUE
make clean && make -j4
./main3d.gnu.ex inputs.3d.static_cylinder
python3 validate_static_cylinder.py
```

**验证标准：**
- 阻力系数误差 < 20%
- Strouhal数误差 < 15%

### 2. 涡激振动案例

**参数设置：**
- 质量比: m* = 2.0
- 自然频率: fn = 10.0 Hz
- 阻尼比: ζ = 0.01
- 约束条件: 仅Y方向自由振动

**运行方法：**
```bash
export USE_PARTICLES=TRUE
export USE_EB=TRUE
export USE_CYLINDER_VIV=TRUE
make clean && make -j4
./main2d.gnu.MPI.ex inputs.2d.cylinder_viv
```

## 输出文件

### 力数据文件
- `cylinder_force_ibm_0.dat`: 力和力矩时间序列
- 格式: `时间 步数 dt Fx Fy Fz Mx My Mz X Y Z Vx Vy Vz`

### 振动数据文件
- `cylinder_vibration_ibm_0.dat`: 振动响应时间序列
- 格式: `时间 步数 dt X Y Z Vx Vy Vz Ax Ay Az SpringFx SpringFy SpringFz DampFx DampFy DampFz`

### 可视化文件
- `static_cylinder_validation.png`: 验证结果图表
- 包含: 阻力系数、升力系数、频谱分析、相位图

## 参数说明

### 圆柱几何参数
```bash
cylinder_vib.shape = 0                  # 0=圆形, 1=方形
cylinder_vib.radius = 0.01              # 半径
cylinder_vib.center = 0.0 0.0 0.0       # 中心位置
```

### 振动类型
```bash
cylinder_vib.vibration_type = 0         # 0=强迫振动, 1=自由振动, 2=VIV
```

### 约束条件
```bash
cylinder_vib.translation_lock = 0 2 0   # 0=锁定, 1=自由, 2=振动
cylinder_vib.rotation_lock = 0 0 0      # 旋转自由度
```

### VIV参数
```bash
cylinder_vib.mass_ratio = 2.0           # 质量比
cylinder_vib.natural_frequency_x = 10.0 # X方向自然频率
cylinder_vib.natural_frequency_y = 10.0 # Y方向自然频率
cylinder_vib.damping_ratio_x = 0.01     # X方向阻尼比
cylinder_vib.damping_ratio_y = 0.01     # Y方向阻尼比
```

### IBM参数
```bash
cylinder_vib.markers_per_circumference = 64  # 拉格朗日标记点数量
cylinder_vib.delta_function_type = 0         # 0=四点, 1=三点
```

## 故障排除

### 编译问题
1. 确保设置了正确的环境变量
2. 检查AMReX和IAMR的依赖关系
3. 确认USE_PARTICLES=TRUE

### 运行问题
1. 检查输入文件格式
2. 确认网格分辨率足够
3. 验证时间步长稳定性

### 验证问题
1. 确保模拟时间足够长
2. 检查力数据文件格式
3. 验证理论值是否正确

## 扩展功能

### 添加新的振动模式
1. 在`VIBRATION_TYPE`枚举中添加新类型
2. 实现对应的位置计算函数
3. 更新参数读取功能

### 改进力计算模型
1. 修改`ComputeLagrangianForces`函数
2. 添加更复杂的流体-结构相互作用模型
3. 实现自适应时间步长

### 后处理分析
1. 添加频率分析功能
2. 实现相位图分析
3. 计算能量传递效率

## 参考文献

1. Williamson, C. H. K., & Govardhan, R. (2004). Vortex-induced vibrations. Annual Review of Fluid Mechanics, 36, 413-455.
2. Bearman, P. W. (1984). Vortex shedding from oscillating bluff bodies. Annual Review of Fluid Mechanics, 16, 195-222.
3. Sarpkaya, T. (2004). A critical review of the intrinsic nature of vortex-induced vibrations. Journal of Fluids and Structures, 19, 389-447. 