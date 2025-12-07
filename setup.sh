#!/bin/bash

set -e
# 检查 pkg-config 是否已安装
if ! command -v pkg-config >/dev/null 2>&1; then
    echo "pkg-config 未安装，准备安装..."
    sudo apt-get update
    sudo apt-get install -y pkg-config
else
    echo "pkg-config 已安装"
fi

# 检查 libsodium 是否已安装
if ! pkg-config --exists libsodium; then
    echo "libsodium 未安装，准备安装..."
    sudo apt-get update
    sudo apt-get install -y libsodium-dev
else
    echo "libsodium 已安装"
fi

mkdir -p build && cd build
cmake ..
make -j$(nproc) #用全部CPU核心编译
