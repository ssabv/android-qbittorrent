#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

NDK_VERSION="r27"
ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-/opt/android-ndk/ndk/27.0.12077973}"
ANDROID_SDK_HOME="${ANDROID_SDK_HOME:-/opt/android-sdk}"
TOOLCHAIN_VERSION="17.0.0"

LIBTORRENT_VERSION="2.0.9"
LIBTORRENT_SRC="$PROJECT_ROOT/third_party/libtorrent"
LIBTORRENT_BUILD="$PROJECT_ROOT/third_party/libtorrent-build"
LIBTORRENT_INSTALL="$PROJECT_ROOT/third_party/libtorrent-install"

ABI="arm64-v8a"
API_LEVEL=29
ARCH="arm64"

echo "======================================"
echo "libtorrent Android Build Script"
echo "======================================"
echo "NDK: $ANDROID_NDK_HOME"
echo "ABI: $ABI"
echo "API: $API_LEVEL"
echo "Version: $LIBTORRENT_VERSION"
echo "======================================"

if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "Error: NDK not found at $ANDROID_NDK_HOME"
    echo "Please set ANDROID_NDK_HOME or install NDK $NDK_VERSION"
    exit 1
fi

if [ ! -d "$LIBTORRENT_SRC" ]; then
    echo "Cloning libtorrent..."
    git clone --depth 1 --branch "v$LIBTORRENT_VERSION" \
        https://github.com/arvidn/libtorrent.git \
        "$LIBTORRENT_SRC"
fi

mkdir -p "$LIBTORRENT_BUILD"
mkdir -p "$LIBTORRENT_INSTALL"

TOOLCHAIN="$LIBTORRENT_BUILD/toolchain"
rm -rf "$TOOLCHAIN"

echo "Setting up Android toolchain..."
"$ANDROID_NDK_HOME/build/tools/make-standalone-toolchain.sh" \
    --platform=android-$API_LEVEL \
    --arch=$ARCH \
    --install-dir="$TOOLCHAIN" \
    --toolchain=llvm \
    --stl=libc++

export CC="$TOOLCHAIN/bin/clang"
export CXX="$TOOLCHAIN/bin/clang++"
export AR="$TOOLCHAIN/bin/llvm-ar"
export RANLIB="$TOOLCHAIN/bin/llvm-ranlib"
export STRIP="$TOOLCHAIN/bin/llvm-strip"

export CFLAGS="-O3 -DNDEBUG -fPIC -fvisibility=hidden -fno-exceptions -fno-rtti"
export CXXFLAGS="-O3 -DNDEBUG -fPIC -fvisibility=hidden -fno-exceptions -fno-rtti"

export LDFLAGS="-fPIC -fvisibility=hidden"

cd "$LIBTORRENT_BUILD"

if [ ! -f "CMakeLists.txt" ]; then
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
        -Ddeprecated_functions=OFF \
        -Dlogging=OFF \
        -Dstats=OFF \
        -Ddebug=OFF \
        -Dasserts=OFF \
        -Dmutable_storage=OFF \
        -Dmmap_disk_io=OFF \
        -Duse_interfaces=OFF \
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
        -Dopenssl=OFF \
        -Dboost=OFF \
        -Dprecompiled_headers=OFF
fi

echo "Building libtorrent..."
ninja -j$(nproc)

echo "Installing libtorrent..."
ninja install

echo "======================================"
echo "Build completed successfully!"
echo "======================================"
echo "Static library: $LIBTORRENT_INSTALL/lib/libtorrent.a"
echo "Headers: $LIBTORRENT_INSTALL/include"
echo "======================================"