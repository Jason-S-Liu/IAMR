#!/bin/bash

# 静止圆柱绕流验证运行脚本

echo "=== 静止圆柱绕流验证 (Re=100) ==="
echo "开始编译和运行..."

# 设置环境变量
export USE_PARTICLES=TRUE
export USE_EB=TRUE
export USE_CYLINDER_VIV=TRUE

# 编译
echo "编译中..."
make clean
make -j4

if [ $? -ne 0 ]; then
    echo "编译失败！"
    exit 1
fi

echo "编译成功！"

# 运行模拟
echo "运行模拟..."
./main3d.gnu.MPI.ex inputs.3d.static_cylinder

if [ $? -ne 0 ]; then
    echo "模拟运行失败！"
    exit 1
fi

echo "模拟完成！"

# 运行验证脚本
echo "运行验证脚本..."
python3 validate_static_cylinder.py

if [ $? -ne 0 ]; then
    echo "验证脚本运行失败！"
    exit 1
fi

echo "验证完成！"

# 显示结果文件
echo "生成的文件："
ls -la cylinder_force_ibm_*.dat
ls -la static_cylinder_validation.png

echo "=== 验证完成 ===" 