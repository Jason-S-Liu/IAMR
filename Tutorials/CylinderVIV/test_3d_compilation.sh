#!/bin/bash

# 3D编译测试脚本

echo "=== 3D圆柱涡激振动编译测试 ==="

# 设置环境变量
export AMREX_HOME=/Users/jason/CQU/IAMRex/amrex
export AMREX_HYDRO_HOME=/Users/jason/CQU/IAMRex/AMReX-Hydro
export TOP=/Users/jason/CQU/IAMRex/IAMR

echo "环境变量设置："
echo "AMREX_HOME = $AMREX_HOME"
echo "AMREX_HYDRO_HOME = $AMREX_HYDRO_HOME"
echo "TOP = $TOP"

# 检查文件是否存在
if [ ! -f "$AMREX_HOME/Tools/GNUMake/Make.rules" ]; then
    echo "错误：找不到 AMReX Make.rules 文件"
    exit 1
fi

if [ ! -f "$AMREX_HYDRO_HOME/Utils/Make.package" ]; then
    echo "错误：找不到 AMReX-Hydro 文件"
    exit 1
fi

echo "环境检查通过！"

# 清理
echo "清理旧文件..."
make clean

# 编译
echo "开始编译3D版本..."
make -j4

if [ $? -eq 0 ]; then
    echo "✓ 3D编译成功！"
    
    # 检查生成的可执行文件
    if [ -f "main3d.gnu.ex" ]; then
        echo "✓ 3D可执行文件生成成功"
        ls -la main3d.gnu.ex
        file main3d.gnu.ex
    else
        echo "✗ 3D可执行文件未找到"
        exit 1
    fi
    
    # 检查输入文件
    if [ -f "inputs.3d.static_cylinder" ]; then
        echo "✓ 3D输入文件存在"
    else
        echo "✗ 3D输入文件不存在"
        exit 1
    fi
    
    echo "3D编译测试通过！"
    echo ""
    echo "下一步："
    echo "1. 运行验证: ./main3d.gnu.ex inputs.3d.static_cylinder"
    echo "2. 或使用脚本: ./run_static_validation.sh"
else
    echo "✗ 3D编译失败！"
    exit 1
fi 