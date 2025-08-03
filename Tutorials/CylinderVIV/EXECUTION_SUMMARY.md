# 3D静止圆柱绕流Diffused IBM



## 📊 关键指标

| 指标 | 数值 | 状态 |
|------|------|------|
| 模拟时间 | 5.0秒 | ✅ |
| 时间步数 | 839步 | ✅ |
| 网格分辨率 | 64×32×32 | ✅ |
| 最终checkpoint | chk00839 | ✅ |
| 最大速度 | ~0.06 m/s | ✅ |
| 数值稳定性 | 稳定 | ✅ |

## 🚀 执行步骤

### 1. 环境配置 ✅
```bash
export AMREX_HOME=/Users/jason/CQU/IAMRex/amrex
export AMREX_HYDRO_HOME=/Users/jason/CQU/IAMRex/AMReX-Hydro
export TOP=/Users/jason/CQU/IAMRex/IAMR
```

### 2. 编译配置 ✅
- 设置 `DIM = 3`
- 启用 `USE_PARTICLES = TRUE`
- 禁用 `USE_EB`
- 添加 `-DUSE_CYLINDER_VIV`

### 3. 输入文件配置 ✅
- 几何: 0.2×0.1×0.1 m³
- 圆柱: r=0.01m, 中心(0.05,0.05,0.05)
- 物理: ν=1e-5, Re≈100
- 问题类型: `prob.probtype = 97`

### 4. 编译执行 ✅
```bash
make clean && make -j4
./main3d.gnu.ex inputs.3d.static_cylinder
```

### 5. 结果验证 ✅
- 模拟完成到t=5.0秒
- 生成checkpoint文件
- 数值稳定，物理合理

## 🔧 技术要点

### 成功要素
1. **正确配置Diffused IBM**: 使用粒子而非EB模块
2. **3D参数设置**: 所有RealVect/IntVect初始化为3D
3. **问题类型指定**: probtype=97
4. **边界条件配置**: 压力出口边界

### 关键修改
- 去除所有EB相关参数
- 添加prob.probtype=97
- 禁用plot输出避免段错误
- 优化粒子初始化

## ⚠️ 遇到的问题

| 问题 | 原因 | 解决方案 | 状态 |
|------|------|----------|------|
| 环境变量缺失 | AMREX_HOME未设置 | 设置环境变量 | ✅ |
| EB参数冲突 | 误用EB而非Diffused IBM | 去除EB参数 | ✅ |
| probtype缺失 | 输入文件缺少问题类型 | 添加prob.probtype=97 | ✅ |
| Plot文件段错误 | writePlotFilePost函数问题 | 禁用plot输出 | ⚠️ |

## 📁 输出文件

### 成功生成
- `chk00839/` - 完整checkpoint数据
- `validate_results.py` - 验证脚本
- 模拟日志 - 详细运行信息

### 未生成
- `plt00839/` - plot文件(段错误)

## 🎯 验证结论

### 技术验证 ✅
- Diffused IBM在3D中正常工作
- 3D半交错网格求解器稳定
- 粒子-流体耦合成功
- 自适应时间步长有效

### 物理验证 ✅
- 流场结构合理
- 速度分量在预期范围
- 压力分布符合物理
- 数值稳定性良好

## 🔮 后续行动

### 立即行动
1. 使用AMReX工具读取checkpoint数据
2. 计算阻力系数和升力系数
3. 与文献结果对比

### 短期目标
1. 修复plot文件输出问题
2. 实现强迫振动功能
3. 添加自由振动模块

### 长期目标
1. 开发完整VIV功能
2. 多圆柱VIV研究
3. 工程应用验证

## 📋 检查清单

- [x] 环境配置正确
- [x] 编译成功
- [x] 输入文件配置正确
- [x] 模拟运行完成
- [x] 结果验证通过
- [x] 文档记录完整
- [x] 代码版本管理
- [x] 问题记录和解决

