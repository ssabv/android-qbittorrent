#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
THIRD_PARTY="$PROJECT_ROOT/third_party"

echo "=========================================="
echo "qBittorrent Android - 完整编译脚本"
echo "=========================================="

# 环境检测
if [ -z "$ANDROID_HOME" ]; then
    ANDROID_HOME=${HOME}/android-sdk
fi

if [ -z "$ANDROID_NDK_HOME" ]; then
    ANDROID_NDK_HOME=${ANDROID_HOME}/ndk/27.0.12077973
fi

# 配置
API_LEVEL=29
ABI="arm64-v8a"
ARCH="arm64"

echo "ANDROID_HOME: $ANDROID_HOME"
echo "ANDROID_NDK_HOME: $ANDROID_NDK_HOME"
echo "API: $API_LEVEL, ABI: $ABI"

# 创建第三方目录
mkdir -p "$THIRD_PARTY"

# ========================================
# 第1步: 编译 OpenSSL
# ========================================
echo ""
echo "=========================================="
echo "步骤 1: 编译 OpenSSL for Android"
echo "=========================================="

OPENSSL_VERSION="3.2.1"
OPENSSL_SRC="$THIRD_PARTY/openssl-${OPENSSL_VERSION}"
OPENSSL_BUILD="$THIRD_PARTY/openssl-build"
OPENSSL_INSTALL="$THIRD_PARTY/openssl-install"

if [ ! -f "$OPENSSL_INSTALL/lib/libcrypto.a" ]; then
    cd /tmp

    # 下载 OpenSSL
    if [ ! -f "openssl-${OPENSSL_VERSION}.tar.gz" ]; then
        wget -q https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz
    fi

    if [ ! -d "$OPENSSL_SRC" ]; then
        tar -xzf openssl-${OPENSSL_VERSION}.tar.gz -C $THIRD_PARTY
    fi

    mkdir -p "$OPENSSL_BUILD"

    # 创建 Android toolchain
    TOOLCHAIN="$THIRD_PARTY/toolchain-openssl"
    rm -rf "$TOOLCHAIN"

    $ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh \
        --platform=android-$API_LEVEL \
        --arch=$ARCH \
        --install-dir="$TOOLCHAIN" \
        --toolchain=llvm \
        --stl=libc++

    export PATH="$TOOLCHAIN/bin:$PATH"
    export CC=clang
    export CXX=clang++
    export CFLAGS="-fPIC -O3"
    export LDFLAGS="-fPIC"

    cd "$OPENSSL_BUILD"
    "$OPENSSL_SRC/Configure" android-arm64 \
        --prefix="$OPENSSL_INSTALL" \
        no-shared \
        no-tests \
        -fPIC

    make -j$(nproc)
    make install_sw

    echo "OpenSSL 编译完成: $OPENSSL_INSTALL"
else
    echo "OpenSSL 已编译"
fi

# ========================================
# 第2步: 编译 Boost
# ========================================
echo ""
echo "=========================================="
echo "步骤 2: 编译 Boost for Android"
echo "=========================================="

BOOST_VERSION="1.85.0"
BOOST_SRC="$THIRD_PARTY/boost_${BOOST_VERSION//./_}"
BOOST_BUILD="$THIRD_PARTY/boost-build"
BOOST_INSTALL="$THIRD_PARTY/boost-install"

if [ ! -f "$BOOST_INSTALL/lib/libboost_system.a" ]; then
    cd /tmp

    # 下载 Boost
    if [ ! -f "boost_${BOOST_VERSION//./_}.tar.gz" ]; then
        wget -q https://boostorg.jfrog.io/artifactory/main/release/${BOOST_VERSION}/source/boost_${BOOST_VERSION//./_}.tar.gz
    fi

    if [ ! -d "$BOOST_SRC" ]; then
        tar -xzf boost_${BOOST_VERSION//./_}.tar.gz -C $THIRD_PARTY
    fi

    mkdir -p "$BOOST_BUILD"

    # 创建 toolchain
    TOOLCHAIN="$THIRD_PARTY/toolchain-boost"
    rm -rf "$TOOLCHAIN"

    $ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh \
        --platform=android-$API_LEVEL \
        --arch=$ARCH \
        --install-dir="$TOOLCHAIN" \
        --toolchain=llvm \
        --stl=libc++

    export PATH="$TOOLCHAIN/bin:$PATH"
    export CC=clang
    export CXX=clang++

    cd "$BOOST_SRC"

    # 配置 Boost
    ./bootstrap.sh --with-toolset=clang

    # 编译 Boost 组件
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
else
    echo "Boost 已编译"
fi

# ========================================
# 第3步: 编译 libtorrent
# ========================================
echo ""
echo "=========================================="
echo "步骤 3: 编译 libtorrent 2.0.9"
echo "=========================================="

LIBTORRENT_VERSION="2.0.9"
LIBTORRENT_SRC="$THIRD_PARTY/libtorrent"
LIBTORRENT_BUILD="$THIRD_PARTY/libtorrent-build"
LIBTORRENT_INSTALL="$THIRD_PARTY/libtorrent-install"

# 克隆 libtorrent
if [ ! -d "$LIBTORRENT_SRC" ]; then
    git clone --depth 1 --branch "v$LIBTORRENT_VERSION" \
        https://github.com/arvidn/libtorrent.git \
        "$LIBTORRENT_SRC"
fi

mkdir -p "$LIBTORRENT_BUILD"
mkdir -p "$LIBTORRENT_INSTALL"

# 创建 toolchain
TOOLCHAIN="$THIRD_PARTY/toolchain-libtorrent"
rm -rf "$TOOLCHAIN"

$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh \
    --platform=android-$API_LEVEL \
    --arch=$ARCH \
    --install-dir="$TOOLCHAIN" \
    --toolchain=llvm \
    --stl=libc++

export PATH="$TOOLCHAIN/bin:$PATH"
export CC=clang
export CXX=clang++

export CFLAGS="-O3 -DNDEBUG -fPIC -fvisibility=hidden -fno-exceptions -fno-rtti"
export CXXFLAGS="-O3 -DNDEBUG -fPIC -fvisibility=hidden -fno-exceptions -fno-rtti"

cd "$LIBTORRENT_BUILD"

# CMake 配置
cmake "$LIBTORRENT_SRC" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN/share/cmake/android/android.toolchain.cmake" \
    -DANDROID_NDK="$ANDROID_NDK_HOME" \
    -DANDROID_ABI="$ABI" \
    -DANDROID_PLATFORM=android-$API_LEVEL \
    -DANDROID_STL=c++_static \
    -DCMAKE_CXX_STANDARD=17 \
    -DCMAKE_CXX_STANDARD_REQUIRED=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DBUILD_STATIC_LIBS=ON \
    -DCMAKE_PREFIX_PATH="$BOOST_INSTALL;$OPENSSL_INSTALL" \
    -Ddeprecated_functions=OFF \
    -Dlogging=OFF \
    -Dstats=OFF \
    -Ddebug=OFF \
    -Dasserts=OFF \
    -Dmutable_storage=OFF \
    -Dmmap_disk_io=OFF \
    -Denable_big_endian=OFF \
    -Denable_asio=ON \
    -Denable_utp=ON \
    -Denable_dht=ON \
    -Denable_lsd=ON \
    -Denable_pex=ON \
    -Denable_ip_filter=OFF \
    -Denable_natpmp=ON \
    -Denable_upnp=ON \
    -Denable_encryption=ON \
    -Dencryption=ON \
    -Dshare_ratio=OFF \
    -Dannounce_entry=ON \
    -Dvector_ssl=OFF \
    -Dhttp_parser=builtin \
    -Dopenssl=builtin \
    -Dboost=builtin \
    -Dprecompiled_headers=OFF

# 编译
ninja -j$(nproc)

# 安装
ninja install

echo ""
echo "=========================================="
echo "编译完成!"
echo "=========================================="
echo ""
echo "库文件位置:"
echo "  libtorrent: $LIBTORRENT_INSTALL/lib/libtorrent.a"
echo "  Boost: $BOOST_INSTALL/lib/libboost_system.a"
echo "  OpenSSL: $OPENSSL_INSTALL/lib/libcrypto.a"
echo ""
echo "头文件位置:"
echo "  libtorrent: $LIBTORRENT_INSTALL/include"
echo "  Boost: $BOOST_INSTALL/include"
echo "  OpenSSL: $OPENSSL_INSTALL/include"
echo ""
echo "下一步: 构建 APK"
echo "  cd $PROJECT_ROOT"
echo "  ./gradlew assembleDebug"