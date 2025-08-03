#!/bin/bash

# 设置环境变量
export AMREX_HOME=/Users/jason/CQU/IAMRex/amrex
export AMREX_HYDRO_HOME=/Users/jason/CQU/IAMRex/AMReX-Hydro
export TOP=/Users/jason/CQU/IAMRex/IAMR

echo "环境变量设置完成："
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