#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "======================================"
echo "qBittorrent Android Build Script"
echo "======================================"

export ANDROID_SDK_ROOT="${ANDROID_SDK_ROOT:-/opt/android-sdk}"
export ANDROID_NDK_HOME="${ANDROID_NDK_HOME:-/opt/android-ndk/ndk/27.0.12077973}"
export PATH="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin:$PATH"

if [ ! -d "$ANDROID_NDK_HOME" ]; then
    echo "Error: NDK not found at $ANDROID_NDK_HOME"
    echo "Please set ANDROID_NDK_HOME environment variable"
    exit 1
fi

if [ ! -d "$ANDROID_SDK_ROOT" ]; then
    echo "Error: Android SDK not found at $ANDROID_SDK_ROOT"
    echo "Please set ANDROID_SDK_ROOT environment variable"
    exit 1
fi

echo "Building qBittorrent Android..."

cd "$PROJECT_ROOT"

if [ "$1" == "clean" ]; then
    echo "Cleaning build..."
    rm -rf app/build
    rm -rf .gradle
    rm -rf third_party
    echo "Clean completed!"
    exit 0
fi

echo "Building APK..."

if [ -f "./gradlew" ]; then
    chmod +x ./gradlew

    if [ "$1" == "debug" ]; then
        ./gradlew assembleDebug --no-daemon --stacktrace
    elif [ "$1" == "release" ]; then
        ./gradlew assembleRelease --no-daemon --stacktrace
    else
        ./gradlew assembleDebug --no-daemon --stacktrace
    fi
else
    echo "Error: gradlew not found"
    echo "Please generate Gradle wrapper first"
    echo "Run: gradle wrapper --gradle-version 8.4"
    exit 1
fi

if [ -d "app/build/outputs/apk" ]; then
    echo "======================================"
    echo "Build completed!"
    echo "======================================"

    if [ -f "app/build/outputs/apk/debug/app-debug.apk" ]; then
        echo "Debug APK: app/build/outputs/apk/debug/app-debug.apk"
    fi

    if [ -f "app/build/outputs/apk/release/app-arm64-v8a-release.apk" ]; then
        echo "Release APK: app/build/outputs/apk/release/app-arm64-v8a-release.apk"
    fi
else
    echo "Error: APK not found in expected location"
    exit 1
fi