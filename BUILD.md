# qBittorrent Android - Build Guide

## 概述

本项目是一个完整的 qBittorrent Android 移植版本，实现了真正的 BT/PT 下载引擎。

## 系统要求

- Android SDK 35 (Android 15)
- NDK r27
- CMake 3.22+
- JDK 17
- Android Studio Arctic Fox 或更高版本
- Minimum SDK: Android 10 (API 29)
- Target SDK: Android 15 (API 35)
- ABI: arm64-v8a

## 项目结构

```
android-qbittorrent/
├── app/
│   └── src/main/
│       ├── cpp/                    # Native C++ 代码
│       │   ├── native-lib.cpp     # JNI 入口
│       │   ├── CMakeLists.txt     # CMake 构建配置
│       │   └── qbittorrent/
│       │       ├── qbcore.cpp      # libtorrent 封装
│       │       ├── qbcore.h
│       │       ├── cJSON.c         # JSON 解析
│       │       └── cJSON.h
│       ├── java/                  # Kotlin 代码
│       │   └── com/qbandroid/
│       │       ├── MainActivity.kt
│       │       ├── core/NativeBridge.kt
│       │       ├── service/TorrentService.kt
│       │       ├── viewmodel/TorrentViewModel.kt
│       │       └── ui/screens/...
│       └── res/                   # 资源文件
├── third_party/                   # 第三方库 (编译后)
├── scripts/                      # 构建脚本
├── build.gradle.kts              # Gradle 构建配置
└── local.properties              # 本地 SDK 配置
```

## 快速构建

### 1. 配置本地环境

编辑 `local.properties` (根据你的环境调整):

```properties
sdk.dir=/opt/android-sdk
ndk.dir=/opt/android-ndk/ndk/27.0.12077973
cmake.dir=/opt/android-sdk/cmake/3.22.1
```

### 2. 编译 Debug APK

```bash
cd android-qbittorrent

# 生成 Gradle Wrapper (如果需要)
# gradle wrapper --gradle-version 8.4

# 构建 Debug APK
./scripts/build.sh debug

# 或者使用 gradle
./gradlew assembleDebug
```

### 3. 编译 Release APK

```bash
./gradlew assembleRelease

# 输出位置:
# app/build/outputs/apk/release/app-arm64-v8a-release.apk
```

## 完整构建流程

### 1. 预编译 libtorrent (可选)

如果需要使用预编译的 libtorrent:

```bash
cd scripts
chmod +x build_all.sh
./build_all.sh
```

这将下载并编译:
- OpenSSL 3.2.1
- Boost 1.85.0
- libtorrent 2.0.9

### 2. Android Studio 导入

1. 打开 Android Studio
2. File → Open → 选择 `android-qbittorrent` 目录
3. 等待 Gradle 同步完成
4. Build → Build APK

### 3. 命令行构建

```bash
# Debug
./gradlew assembleDebug

# Release
./gradlew assembleRelease

# Clean
./gradlew clean

# Rebuild
./gradlew clean assembleDebug
```

## APK 输出位置

| 构建类型 | 输出路径 |
|---------|---------|
| Debug | `app/build/outputs/apk/debug/app-debug.apk` |
| Release | `app/build/outputs/apk/release/app-arm64-v8a-release.apk` |

## 功能列表

### 已实现

- ✅ 磁力链接添加 (magnet:)
- ✅ Torrent 文件添加
- ✅ 下载管理 (暂停/恢复/删除)
- ✅ 做种功能
- ✅ 速度限制 (全局/单个)
- ✅ PT 模式 (禁用 DHT/PEX/LSD)
- ✅ Tracker 管理
- ✅ Peer 列表
- ✅ 文件管理
- ✅ Fast Resume (状态持久化)
- ✅ Foreground Service 后台运行
- ✅ WakeLock 保持后台
- ✅ 网络状态监控
- ✅ 启动自动恢复
- ✅ Material You 主题

### PT 兼容性

- ✅ 保留 qBittorrent Peer ID
- ✅ 保留 tracker announce
- ✅ 支持 passkey 认证
- ✅ 比率统计
- ✅ 上传量统计
- ✅ Fastresume 持久化
- ✅ PT 模式自动禁用 DHT/PEX/LSD

## 已知问题

1. **libtorrent 需要预编译**: 如果不使用预编译库，需要运行 `build_all.sh`
2. **部分厂商后台限制**: 小米/OPPO/vivo/华为需要手动设置白名单
3. **Android Doze**: 需要用户关闭电池优化

## 调试

### 查看日志

```bash
# 过滤 qBittorrent 日志
adb logcat -s qBittorrent:V qBittorrentCore:V

# 查看所有日志
adb logcat | grep -i qbt
```

### 常见错误

1. **NDK not found**
   - 检查 `local.properties` 中的 ndk.dir 路径
   - 确保 NDK r27 已安装

2. **CMake error**
   - 确保 CMake 3.22+ 已安装
   - 检查 CMake 路径配置

3. **libtorrent 链接失败**
   - 运行 `./scripts/build_all.sh` 预编译
   - 或者将预编译的 libtorrent.a 放入 `third_party/libtorrent-install/lib/`

## Release 签名

Release 构建默认使用 debug 签名。要使用自定义签名:

1. 创建签名配置:

```kotlin
// app/build.gradle.kts
android {
    signingConfigs {
        create("release") {
            storeFile = file("keystore/my-release-key.jks")
            storePassword = "password"
            keyAlias = "alias"
            keyPassword = "password"
        }
    }

    buildTypes {
        release {
            signingConfig = signingConfigs.getByName("release")
        }
    }
}
```

2. 构建签名 APK:

```bash
./gradlew assembleRelease
```

## 技术栈

| 组件 | 版本 |
|------|------|
| Kotlin | 1.9.22 |
| Compose BOM | 2024.01.00 |
| libtorrent | 2.0.9 |
| NDK | r27 |
| Gradle | 8.4 |
| AGP | 8.2.2 |

## 许可证

GPLv2 (与 qBittorrent 相同)

## 更多信息

- qBittorrent 官网: https://www.qbittorrent.org
- libtorrent: https://github.com/arvidn/libtorrent
- Android NDK: https://developer.android.com/ndk