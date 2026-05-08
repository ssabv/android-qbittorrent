#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "=========================================="
echo "qBittorrent Android - APK 构建脚本"
echo "=========================================="

# 环境检测
if [ -z "$JAVA_HOME" ]; then
    export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
fi

if [ -z "$ANDROID_HOME" ]; then
    export ANDROID_HOME=${HOME}/android-sdk
fi

if [ -z "$ANDROID_NDK_HOME" ]; then
    export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973
fi

echo "JAVA_HOME: $JAVA_HOME"
echo "ANDROID_HOME: $ANDROID_HOME"
echo "ANDROID_NDK_HOME: $ANDROID_NDK_HOME"

cd "$PROJECT_ROOT"

# 检查 Gradle
if [ ! -f "./gradlew" ]; then
    echo "生成 Gradle wrapper..."
    gradle wrapper --gradle-version 8.4
fi

chmod +x ./gradlew

# 检查 SDK
if [ ! -d "$ANDROID_HOME/platforms/android-35" ]; then
    echo "错误: Android SDK 未安装"
    echo "请运行: scripts/build_env.sh"
    exit 1
fi

# 检查 NDK
if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "错误: Android NDK 未安装"
    echo "请运行: scripts/build_env.sh"
    exit 1
fi

# 解析参数
BUILD_TYPE="${1:-debug}"

case "$BUILD_TYPE" in
    debug)
        echo ""
        echo "=========================================="
        echo "构建 Debug APK"
        echo "=========================================="
        TASK="assembleDebug"
        ;;
    release)
        echo ""
        echo "=========================================="
        echo "构建 Release APK"
        echo "=========================================="
        TASK="assembleRelease"
        ;;
    clean)
        echo ""
        echo "=========================================="
        echo "清理构建"
        echo "=========================================="
        ./gradlew clean
        echo "清理完成"
        exit 0
        ;;
    all)
        echo ""
        echo "=========================================="
        echo "构建 Debug + Release APK"
        echo "=========================================="
        ./gradlew clean assembleDebug assembleRelease
        ;;
    *)
        echo "用法: $0 [debug|release|clean|all]"
        echo "  debug   - 构建 Debug APK (默认)"
        echo "  release - 构建 Release APK"
        echo "  clean   - 清理构建"
        echo "  all     - 构建 Debug + Release"
        exit 1
        ;;
esac

# 执行构建
echo ""
echo "开始构建..."
echo ""

./gradlew $TASK --no-daemon --stacktrace

# 检查输出
if [ -d "app/build/outputs/apk" ]; then
    echo ""
    echo "=========================================="
    echo "构建完成!"
    echo "=========================================="
    echo ""

    echo "输出文件:"
    if [ -f "app/build/outputs/apk/debug/app-debug.apk" ]; then
        ls -lh app/build/outputs/apk/debug/app-debug.apk
        echo "Debug: app/build/outputs/apk/debug/app-debug.apk"
    fi

    if [ -f "app/build/outputs/apk/release/app-arm64-v8a-release.apk" ]; then
        ls -lh app/build/outputs/apk/release/app-arm64-v8a-release.apk
        echo "Release: app/build/outputs/apk/release/app-arm64-v8a-release.apk"
    fi

    if [ -f "app/build/outputs/apk/release/app-universal-release.apk" ]; then
        ls -lh app/build/outputs/apk/release/app-universal-release.apk
        echo "Universal: app/build/outputs/apk/release/app-universal-release.apk"
    fi
else
    echo ""
    echo "错误: APK 输出目录未找到"
    exit 1
fi