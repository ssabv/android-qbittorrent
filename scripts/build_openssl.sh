#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=========================================="
echo "编译 OpenSSL for Android"
echo "=========================================="

ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-$HOME/android-sdk/ndk/27.0.12077973}"

if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "错误: NDK 未找到: $ANDROID_NDK_HOME"
    exit 1
fi

OPENSSL_VERSION="${OPENSSL_VERSION:-3.2.1}"
THIRD_PARTY="$PROJECT_ROOT/third_party"

OPENSSL_SRC="$THIRD_PARTY/openssl-${OPENSSL_VERSION}"
OPENSSL_BUILD="$THIRD_PARTY/openssl-build"
OPENSSL_INSTALL="$THIRD_PARTY/openssl-install"

mkdir -p "$THIRD_PARTY"
cd /tmp

# 下载 OpenSSL
if [ ! -d "$OPENSSL_SRC" ]; then
    echo "下载 OpenSSL $OPENSSL_VERSION..."
    if [ ! -f "openssl-${OPENSSL_VERSION}.tar.gz" ]; then
        wget -q https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz
    fi
    tar -xzf openssl-${OPENSSL_VERSION}.tar.gz -C $THIRD_PARTY
fi

mkdir -p "$OPENSSL_BUILD"

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
export CFLAGS="-fPIC -O3"
export LDFLAGS="-fPIC"

cd "$OPENSSL_BUILD"

echo "编译 OpenSSL..."

"$OPENSSL_SRC/Configure" android-arm64 \
    --prefix="$OPENSSL_INSTALL" \
    no-shared \
    no-tests \
    -fPIC

make -j$(nproc)
make install_sw

echo "OpenSSL 编译完成: $OPENSSL_INSTALL"