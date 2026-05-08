#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$PROJECT_ROOT"

echo "======================================"
echo "qBittorrent Android Release Build"
echo "======================================"

if [ ! -f "gradlew" ]; then
    echo "Downloading Gradle wrapper..."
    curl -L -o gradlew https://raw.githubusercontent.com/gradle/gradle/master/gradlew
    chmod +x gradlew
fi

export ANDROID_SDK_ROOT=/opt/android-sdk
export ANDROID_NDK_HOME=/opt/android-ndk/ndk/27.0.12077973

if [ "$1" == "debug" ]; then
    echo "Building debug APK..."
    ./gradlew assembleDebug --no-daemon
    echo "======================================"
    echo "Debug APK generated!"
    echo "Output: app/build/outputs/apk/debug/app-debug.apk"
elif [ "$1" == "release" ]; then
    echo "Building release APK..."
    ./gradlew assembleRelease --no-daemon
    echo "======================================"
    echo "Release APK generated!"
    echo "Output: app/build/outputs/apk/release/"
else
    echo "Usage: ./release.sh [debug|release]"
    echo ""
    echo "Examples:"
    echo "  ./release.sh debug    - Build debug APK"
    echo "  ./release.sh release - Build release APK"
fi

echo "======================================"