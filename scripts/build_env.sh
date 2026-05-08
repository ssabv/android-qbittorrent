#!/bin/bash

set -e

echo "========================================"
echo "qBittorrent Android 构建环境配置"
echo "========================================"

# 检测操作系统
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$NAME
    VER=$VERSION_ID
    echo "检测到操作系统: $OS $VER"
fi

# 检查是否在 Docker 容器中
if [ -f /.dockerenv ]; then
    echo "运行在 Docker 容器中"
fi

# 检查并安装基础依赖
install_dependencies() {
    echo "========================================"
    echo "安装系统依赖..."
    echo "========================================"

    if command -v apt-get &> /dev/null; then
        sudo apt-get update
        sudo apt-get install -y \
            wget \
            curl \
            unzip \
            git \
            build-essential \
            cmake \
            ninja-build \
            python3 \
            python3-pip \
            clang \
            llvm \
            libssl-dev \
            zlib1g-dev \
            libtool \
            autoconf \
            automake \
            pkg-config
    elif command -v yum &> /dev/null; then
        sudo yum install -y \
            wget \
            curl \
            unzip \
            git \
            gcc \
            gcc-c++ \
            cmake \
            ninja-build \
            python3 \
            openssl-devel \
            zlib-devel
    elif command -v pacman &> /dev/null; then
        sudo pacman -S --noconfirm \
            wget \
            curl \
            unzip \
            git \
            base-devel \
            cmake \
            ninja \
            python \
            openssl \
            zlib
    fi

    echo "系统依赖安装完成"
}

# 安装 OpenJDK 17
install_java() {
    echo "========================================"
    echo "安装 OpenJDK 17..."
    echo "========================================"

    if command -v java &> /dev/null; then
        JAVA_VER=$(java -version 2>&1 | head -1 | cut -d'"' -f2 | cut -d'.' -f1)
        if [ "$JAVA_VER" = "17" ]; then
            echo "Java 17 已安装"
            return 0
        fi
    fi

    if command -v apt-get &> /dev/null; then
        sudo apt-get install -y openjdk-17-jdk
    elif command -v yum &> /dev/null; then
        sudo yum install -y java-17-openjdk-devel
    fi

    # 设置 JAVA_HOME
    export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64

    if [ -d "$JAVA_HOME" ]; then
        echo "JAVA_HOME=$JAVA_HOME"
        echo "export JAVA_HOME=$JAVA_HOME" >> ~/.bashrc
    else
        echo "警告: JAVA_HOME 目录不存在"
    fi
}

# 安装 Android SDK
install_android_sdk() {
    echo "========================================"
    echo "安装 Android SDK..."
    echo "========================================"

    export ANDROID_HOME=${ANDROID_HOME:-$HOME/android-sdk}
    export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973

    if [ -d "$ANDROID_HOME/platforms/android-35" ]; then
        echo "Android SDK 已安装"
        return 0
    fi

    mkdir -p "$ANDROID_HOME/cmdline-tools"
    cd /tmp

    # 下载 command line tools
    if [ ! -f commandlinetools-linux.zip ]; then
        wget -q https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip -O commandlinetools.zip
    fi

    unzip -q -o commandlinetools.zip
    rm -rf $ANDROID_HOME/cmdline-tools/latest
    mv cmdline-tools $ANDROID_HOME/cmdline-tools/latest

    # 添加到 PATH
    export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools

    # 接受协议
    yes | $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager --licenses > /dev/null 2>&1 || true

    # 安装 SDK 组件
    echo "安装 Android SDK 组件..."
    $ANDROID_HOME/cmdline-tools/latest/bin/sdkmanager \
        "platforms;android-35" \
        "build-tools;35.0.0" \
        "ndk;27.0.12077973" \
        "cmake;3.22.1"

    # 保存到 bashrc
    echo "export ANDROID_HOME=$ANDROID_HOME" >> ~/.bashrc
    echo "export ANDROID_NDK_HOME=$ANDROID_NDK_HOME" >> ~/.bashrc
    echo "export PATH=\$PATH:\$ANDROID_HOME/cmdline-tools/latest/bin:\$ANDROID_HOME/platform-tools" >> ~/.bashrc

    echo "Android SDK 安装完成: $ANDROID_HOME"
}

# 安装 Gradle
install_gradle() {
    echo "========================================"
    echo "安装 Gradle..."
    echo "========================================"

    if command -v gradle &> /dev/null; then
        GRADLE_VER=$(gradle --version 2>/dev/null | head -1 | grep -oP '\d+\.\d+' | head -1)
        if [[ $(echo "$GRADLE_VER >= 8.0" | bc -l 2>/dev/null || echo 0) -eq 1 ]]; then
            echo "Gradle 已安装: $GRADLE_VER"
            return 0
        fi
    fi

    cd /tmp
    if [ ! -f gradle-8.4-bin.zip ]; then
        wget -q https://services.gradle.org/distributions/gradle-8.4-bin.zip -O gradle-8.4-bin.zip
    fi

    unzip -q -o gradle-8.4-bin.zip
    sudo mv gradle-8.4 /opt/gradle
    sudo ln -sf /opt/gradle/bin/gradle /usr/local/bin/gradle

    echo "export GRADLE_HOME=/opt/gradle" >> ~/.bashrc
    echo "export PATH=\$PATH:\$GRADLE_HOME/bin" >> ~/.bashrc

    echo "Gradle 安装完成"
}

# 配置环境变量
configure_environment() {
    echo "========================================"
    echo "配置环境变量..."
    echo "========================================"

    # 设置基本环境变量
    export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
    export ANDROID_HOME=${ANDROID_HOME:-$HOME/android-sdk}
    export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973
    export GRADLE_HOME=/opt/gradle
    export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools:$GRADLE_HOME/bin

    # 验证环境
    echo ""
    echo "========================================"
    echo "验证环境配置"
    echo "========================================"
    echo "JAVA_HOME: $JAVA_HOME"
    java -version 2>&1 | head -1

    echo ""
    echo "ANDROID_HOME: $ANDROID_HOME"
    ls $ANDROID_HOME/platforms/ 2>/dev/null || echo "未安装"

    echo ""
    echo "ANDROID_NDK_HOME: $ANDROID_NDK_HOME"
    ls $ANDROID_NDK_HOME 2>/dev/null || echo "未安装"

    echo ""
    echo "Gradle: $(gradle --version 2>/dev/null | head -1 || echo '未安装')"

    echo ""
    echo "CMake: $(cmake --version 2>/dev/null | head -1 || echo '未安装')"

    echo ""
    echo "========================================"
    echo "环境配置完成!"
    echo "========================================"
}

# 主程序
main() {
    install_dependencies
    install_java
    install_android_sdk
    install_gradle
    configure_environment

    echo ""
    echo "========================================"
    echo "下一步操作:"
    echo "========================================"
    echo "1. 进入项目目录: cd android-qbittorrent"
    echo "2. 预编译 libtorrent: cd scripts && ./build_all.sh"
    echo "3. 构建 APK: ./gradlew assembleDebug"
    echo ""
}

# 运行主程序
main "$@"