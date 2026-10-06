#!/bin/bash

# 自动配置环境并构建 Horizon-OC 的脚本
# 基于 DferTune v5.0.0 的构建说明编写

set -e

echo "--- 开始配置 Horizon-OC 构建环境 ---"

# 1. 检查 python3 是否存在
if ! command -v python3 &> /dev/null; then
    echo "错误:未找到 python3,请先安装 Python 3。"
    exit 1
fi

# 2. 克隆所需仓库
echo "正在克隆 Atmosphere..."
git clone https://github.com/Atmosphere-NX/Atmosphere.git --depth 1

echo "正在克隆 Horizon-OC..."
git clone https://github.com/Horizon-OC/Horizon-OC.git --recurse-submodules --depth 1

# 3. 准备构建目录结构
echo "正在准备构建目录..."
mkdir -p Horizon-OC/build

# 4. 将 Atmosphere 文件复制到构建目录
echo "正在整合 Atmosphere 文件..."
cp -r Atmosphere/* Horizon-OC/build/

# 5. 给源码打补丁 (ldr_process_creation.cpp)
# 假定 DferTune 中有包含所需补丁的 source 目录
if [ -d "DferTune/source" ]; then
    echo "正在应用 ldr_process_creation.cpp 补丁..."
    # 说明中的路径:Source/Atmosphere/stratosphere/loader/source/ldr_process_creation.cpp
    # 如果补丁存在,按本仓库的目录结构进行适配
    # 这里只是创建目录结构;如果没有补丁,则输出警告
    PATCH_PATH="DferTune/source/Atmosphere/stratosphere/loader/source/ldr_process_creation.cpp"
    if [ -f "$PATCH_PATH" ]; then
        cp "$PATCH_PATH" Horizon-OC/build/stratosphere/loader/source/ldr_process_creation.cpp
    else
        echo "警告:未找到补丁文件 $PATCH_PATH,跳过打补丁。"
    fi
fi

# 6. 开始构建
echo "正在运行最终构建脚本..."
cd Horizon-OC
if [ -f "./build.sh" ]; then
    chmod +x ./build.sh
    ./build.sh
else
    echo "错误:Horizon-OC 仓库中未找到 build.sh。"
    exit 1
fi

echo "--- 配置与构建已成功完成 ---"
