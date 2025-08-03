#!/bin/bash

# 清理未使用的文件脚本
echo "=== 清理未使用的文件 ==="

# 检查当前使用的文件
echo "1. 检查当前输入文件..."
if grep -q "particle.input" inputs.3d.static_cylinder; then
    echo "   ✅ particle.input 在当前输入文件中被使用"
else
    echo "   ⚠️  particle.input 在当前输入文件中未使用"
fi

# 备份选项
echo ""
echo "2. 选择操作："
echo "   a) 删除未使用的文件"
echo "   b) 备份到backup目录"
echo "   c) 仅显示文件信息"
echo "   d) 取消操作"
read -p "请选择 (a/b/c/d): " choice

case $choice in
    a)
        echo "删除未使用的文件..."
        if [ -f "cylinder_particles.txt" ]; then
            rm cylinder_particles.txt
            echo "   ✅ 已删除 cylinder_particles.txt"
        fi
        echo "   ℹ️  保留 particle_inputs (作为参考)"
        ;;
    b)
        echo "备份文件到backup目录..."
        mkdir -p backup
        if [ -f "cylinder_particles.txt" ]; then
            mv cylinder_particles.txt backup/
            echo "   ✅ 已备份 cylinder_particles.txt"
        fi
        if [ -f "particle_inputs" ]; then
            cp particle_inputs backup/
            echo "   ✅ 已备份 particle_inputs"
        fi
        ;;
    c)
        echo "显示文件信息..."
        echo ""
        echo "particle_inputs:"
        echo "   - 格式: 键值对"
        echo "   - 用途: Diffused IBM粒子配置"
        echo "   - 状态: 当前未使用，但保留作为参考"
        echo ""
        echo "cylinder_particles.txt:"
        echo "   - 格式: 数组格式"
        echo "   - 用途: 同上，但格式不同"
        echo "   - 状态: 当前未使用，可删除"
        ;;
    d)
        echo "取消操作"
        exit 0
        ;;
    *)
        echo "无效选择，取消操作"
        exit 1
        ;;
esac

echo ""
echo "=== 清理完成 ==="
echo "建议："
echo "1. 保留 particle_inputs 作为参考"
echo "2. 如需使用粒子文件初始化，在输入文件中添加："
echo "   particle.input = particle_inputs"
echo "3. 其他教程都使用这种配置方式" 