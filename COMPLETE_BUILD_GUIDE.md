# qBittorrent Android - 完整构建指南

## 环境要求

### 必需软件
| 软件 | 版本 | 说明 |
|------|------|------|
| JDK | 17 | Android 开发必须 JDK 17 |
| Android SDK | 35 | Android 15 (API 35) |
| Android NDK | r27 | Native 开发必须 |
| CMake | 3.22+ | C++ 构建 |
| Gradle | 8.4 | 项目构建工具 |
| Android Studio | 2023+ | 推荐使用 |

### 安装步骤

#### 1. 安装 JDK 17

```bash
# Ubuntu/Debian
sudo apt install openjdk-17-jdk

# 或下载 Oracle JDK 17
# https://www.oracle.com/java/technologies/downloads/#java17

# 设置 JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export PATH=$JAVA_HOME/bin:$PATH
```

#### 2. 安装 Android SDK

```bash
# 方法一: 使用 Android Studio
# 下载: https://developer.android.com/studio
# 安装后打开 SDK Manager 下载以下组件:
# - Android SDK Platform 35
# - Android SDK Build-Tools 35
# - CMake 3.22.1

# 方法二: 命令行安装
mkdir -p ~/android-sdk/cmdline-tools
cd ~/android-sdk/cmdline-tools
wget https://dl.google.com/android/repository/commandlinetools-linux-11076708_latest.zip
unzip commandlinetools-linux-11076708_latest.zip
mv cmdline-tools latest

# 安装 SDK 组件
export ANDROID_HOME=~/android-sdk
export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin
yes | sdkmanager --licenses
sdkmanager "platforms;android-35" "build-tools;35.0.0" "cmake;3.22.1" "ndk;27.0.12077973"
```

#### 3. 配置环境变量

```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
export ANDROID_HOME=~/android-sdk
export ANDROID_NDK_HOME=$ANDROID_HOME/ndk/27.0.12077973
export PATH=$PATH:$ANDROID_HOME/cmdline-tools/latest/bin:$ANDROID_HOME/platform-tools
```

#### 4. 验证环境

```bash
java -version
# 输出: openjdk version "17.0.x"

echo $ANDROID_HOME
# 输出: /home/xxx/android-sdk

ls $ANDROID_NDK_HOME
# 应该看到 ndk-bundle 目录

cmake --version
# 输出: cmake version 3.22.x
```

---

## 项目构建

### 方式一: Android Studio

1. **打开项目**
   ```
   打开 Android Studio → File → Open → 选择 android-qbittorrent 目录
   ```

2. **同步 Gradle**
   - 等待 Gradle 同步完成 (首次可能需要 5-10 分钟)
   - 如果有错误，根据提示修复

3. **构建 Debug APK**
   ```
   Build → Build Bundle(s) / APK(s) → Build APK(s)
   ```

4. **输出位置**
   ```
   app/build/outputs/apk/debug/app-debug.apk
   ```

### 方式二: 命令行

```bash
cd android-qbittorrent

# 首次设置 (如果 gradlew 不存在)
gradle wrapper --gradle-version 8.4

# 构建 Debug APK
./gradlew assembleDebug

# 构建 Release APK
./gradlew assembleRelease

# 构建前清理
./gradlew clean

# 查看构建任务
./gradlew tasks --group=build
```

### 方式三: 使用构建脚本

```bash
# Debug 构建
./scripts/build.sh debug

# Release 构建
./scripts/build.sh release

# 清理并重新构建
./scripts/build.sh clean && ./scripts/build.sh debug
```

---

## 预编译 libtorrent (可选但推荐)

如果需要使用优化的 libtorrent 库:

```bash
cd scripts

# 给脚本执行权限
chmod +x build_all.sh

# 运行完整构建 (这会下载并编译 Boost + OpenSSL + libtorrent)
./build_all.sh
```

编译完成后:
- libtorrent: `third_party/libtorrent-install/lib/libtorrent.a`
- Boost: `third_party/boost-install/lib/`
- OpenSSL: `third_party/openssl-install/lib/`

---

## 常见问题

### 1. NDK not found

```bash
# 检查 local.properties 配置
cat local.properties
# 应该包含:
# sdk.dir=/path/to/android-sdk
# ndk.dir=/path/to/android-ndk/ndk/27.0.12077973
# cmake.dir=/path/to/android-sdk/cmake/3.22.1
```

### 2. CMake 版本过低

```bash
# 安装新版 CMake
sdkmanager "cmake;3.22.1"
```

### 3. libtorrent 链接失败

```bash
# 如果没有预编译 libtorrent，CMake 会尝试使用系统库
# 如果失败，需要运行 build_all.sh 预编译
```

### 4. Gradle 下载慢

```bash
# 使用国内镜像
# 编辑 gradle.properties
org.gradle.daemon=true
org.gradle.parallel=true
org.gradle.caching=true
org.gradle.jvmargs=-Xmx4096m
```

---

## APK 输出

| 类型 | 路径 | 大小 |
|------|------|------|
| Debug | `app/build/outputs/apk/debug/app-debug.apk` | ~30MB |
| Release | `app/build/outputs/apk/release/app-arm64-v8a-release.apk` | ~15MB |

---

## 安装测试

```bash
# 安装到设备
adb install app/build/outputs/apk/debug/app-debug.apk

# 查看日志
adb logcat -s qBittorrent:V qBittorrentCore:V

# 推送测试 magnet
adb shell am start -d "magnet:?xt=urn:btih:test" com.qbandroid
```

---

## 完整依赖列表

```
android-qbittorrent/
├── app/
│   └── src/main/
│       ├── cpp/              # C++ Native
│       │   ├── qbittorrent/ # libtorrent 封装
│       │   └── native-lib.cpp
│       └── java/            # Kotlin
│           └── com/qbandroid/
├── third_party/             # 第三方库 (可选)
├── scripts/                 # 构建脚本
└── gradle/                  # Gradle Wrapper
```

---

## 下一步

APK 构建成功后:
1. 安装到 Android 设备
2. 授予存储/通知权限
3. 添加 magnet 链接测试下载
4. 开启 PT 模式测试私有 tracker

## 技术支持

- qBittorrent: https://www.qbittorrent.org
- libtorrent: https://github.com/arvidn/libtorrent
- Android NDK: https://developer.android.com/ndk
- Android Studio: https://developer.android.com/studio