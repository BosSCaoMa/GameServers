#!/bin/bash

set -e

# 检查 libsodium 是否已安装
if ! pkg-config --exists libsodium; then
    echo "libsodium 未安装，准备安装..."
    sudo apt-get update
    sudo apt-get install -y libsodium-dev
else
    echo "libsodium 已安装"
fi