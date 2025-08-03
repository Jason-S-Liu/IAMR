# 3D静止圆柱绕流Diffused IBM快速参考

## 🚀 快速启动

### 1. 环境设置
```bash
export AMREX_HOME=/Users/jason/CQU/IAMRex/amrex
export AMREX_HYDRO_HOME=/Users/jason/CQU/IAMRex/AMReX-Hydro
export TOP=/Users/jason/CQU/IAMRex/IAMR
cd Tutorials/CylinderVIV
```

### 2. 编译运行
```bash
make clean && make -j4
./main3d.gnu.ex inputs.3d.static_cylinder
```

### 3. 验证结果
```bash
python3 validate_results.py
```

## 📊 关键参数

### 几何参数
- **计算域**: 0.2 × 0.1 × 0.1 m³
- **圆柱半径**: 0.01 m
- **圆柱中心**: (0.05, 0.05, 0.05) m
- **网格**: 64 × 32 × 32

### 物理参数
- **运动粘度**: 1.0×10⁻⁵ m²/s
- **雷诺数**: ~100
- **模拟时间**: 5.0 秒
- **时间步数**: 839步

### 输入文件关键设置
```ini
# 问题类型
prob.probtype = 97

# 圆柱参数
cylinder_vib.vibration_type = 0  # 静止
cylinder_vib.radius = 0.01
cylinder_vib.center = 0.05 0.05 0.05

# 输出设置
amr.plot_int = -1        # 禁用plot输出
amr.check_int = 1000     # checkpoint输出
```

## 🔧 编译配置

### GNUmakefile关键设置
```makefile
DIM = 3                    # 3D模拟
USE_MPI = FALSE           # 单进程
USE_PARTICLES = TRUE      # 启用粒子
# USE_EB = TRUE           # 禁用EB
CXXFLAGS += -DUSE_CYLINDER_VIV
```

## 📁 重要文件

### 输入文件
- `inputs.3d.static_cylinder` - 主输入文件

### 输出文件
- `chk00839/` - 最终checkpoint数据
- `validate_results.py` - 验证脚本

### 配置文件
- `GNUmakefile` - 编译配置
- `setup_env.sh` - 环境设置

## ⚠️ 常见问题

### 1. 环境变量错误
```bash
# 错误: make: *** No rule to make target '/Tools/GNUMake/Make.rules'. Stop.
# 解决: 设置环境变量
export AMREX_HOME=/Users/jason/CQU/IAMRex/amrex
export AMREX_HYDRO_HOME=/Users/jason/CQU/IAMRex/AMReX-Hydro
export TOP=/Users/jason/CQU/IAMRex/IAMR
```

### 2. EB参数冲突
```bash
# 错误: ParmParse::getval(): eb2.geom_type not found
# 解决: 确保USE_EB = FALSE，去除所有eb2.*参数
```

### 3. probtype缺失
```bash
# 错误: unknown probtype !!!
# 解决: 添加 prob.probtype = 97
```

### 4. Plot文件段错误
```bash
# 现象: 模拟完成但plot文件生成时崩溃
# 解决: 设置 amr.plot_int = -1 禁用plot输出
# 影响: 不影响checkpoint数据
```

## 📈 结果验证

### 成功指标
- ✅ 模拟运行到t=5.0秒
- ✅ 时间步数839步
- ✅ 生成checkpoint文件
- ✅ 数值稳定，无发散

### 物理量范围
- **速度分量**: ~0.04-0.06 m/s
- **压力**: 0.001-0.009 Pa
- **时间步长**: ~0.001-0.025秒

## 🎯 下一步

### 立即行动
1. 读取checkpoint数据
2. 计算阻力系数
3. 与文献对比

### 功能扩展
1. 修复plot输出
2. 实现强迫振动
3. 添加自由振动

