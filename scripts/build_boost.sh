#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=========================================="
echo "编译 Boost for Android"
echo "=========================================="

ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-$HOME/android-sdk/ndk/27.0.12077973}"

if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "错误: NDK 未找到: $ANDROID_NDK_HOME"
    exit 1
fi

BOOST_VERSION="${BOOST_VERSION:-1.85.0}"
THIRD_PARTY="$PROJECT_ROOT/third_party"

BOOST_SRC="$THIRD_PARTY/boost_${BOOST_VERSION//./_}"
BOOST_BUILD="$THIRD_PARTY/boost-build"
BOOST_INSTALL="$THIRD_PARTY/boost-install"

mkdir -p "$THIRD_PARTY"
cd /tmp

# 下载 Boost
if [ ! -d "$BOOST_SRC" ]; then
    echo "下载 Boost $BOOST_VERSION..."
    if [ ! -f "boost_${BOOST_VERSION//./_}.tar.gz" ]; then
        wget -q https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz
    fi
    tar -xzf boost_${BOOST_VERSION//./_}.tar.gz -C $THIRD_PARTY
fi

mkdir -p "$BOOST_BUILD"

# 创建 toolchain
TOOLCHAIN="$THIRD_PARTY/toolchain"
rm -rf "$TOOLCHAIN"

$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh \
    --platform=android-29 \
    --arch=arm64 \
    --install-dir="$TOOLCHAIN" \
    --toolchain=llvm \
    --stl=libc++

export PATH="$TOOLCHAIN/bin:$PATH"
export CC=clang
export CXX=clang++

cd "$BOOST_SRC"

echo "编译 Boost 组件..."

./bootstrap.sh --with-toolset=clang

./b2 install \
    toolset=clang-arm64 \
    variant=release \
    link=static \
    runtime-link=static \
    threading=multi \
    --prefix="$BOOST_INSTALL" \
    -j$(nproc) \
    --disable-headers \
    system \
    filesystem \
    thread \
    chrono \
    date_time \
    atomic

echo "Boost 编译完成: $BOOST_INSTALL"