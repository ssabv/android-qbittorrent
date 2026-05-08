# qBittorrent Android

一个将 qBittorrent 核心移植到 Android 的原生应用。

## 功能特性

- ✅ 磁力链接支持
- ✅ Torrent 文件支持
- ✅ 做种功能
- ✅ PT 模式 (禁用 DHT/PEX/LSD)
- ✅ 前后台下载/做种
- ✅ Foreground Service 常驻
- ✅ WakeLock 保持后台运行
- ✅ 启动自动恢复
- ✅ WebUI (可选)
- ✅ Material You 主题

## 技术栈

| 模块 | 技术 |
|------|------|
| UI | Kotlin + Jetpack Compose + Material 3 |
| Core | qBittorrent-nox Core |
| BT Engine | libtorrent-rasterbar 2.0.9 |
| Native Bridge | JNI + CMake |
| Background | Foreground Service |
| Min SDK | Android 10 (API 29) |
| Target SDK | Android 15 (API 35) |
| ABI | arm64-v8a |

## 目录结构

```
android-qbittorrent/
├── app/
│   ├── src/main/
│   │   ├── java/com/qbandroid/
│   │   │   ├── MainActivity.kt
│   │   │   ├── QBittorrentApp.kt
│   │   │   ├── core/
│   │   │   │   └── NativeBridge.kt
│   │   │   ├── model/
│   │   │   │   └── Models.kt
│   │   │   ├── viewmodel/
│   │   │   │   └── TorrentViewModel.kt
│   │   │   ├── service/
│   │   │   │   ├── TorrentService.kt
│   │   │   │   └── BootReceiver.kt
│   │   │   └── ui/
│   │   │       ├── theme/
│   │   │       │   ├── Theme.kt
│   │   │       │   └── Typography.kt
│   │   │       └── screens/
│   │   │           ├── MainScreen.kt
│   │   │           ├── DownloadsScreen.kt
│   │   │           ├── AddTorrentScreen.kt
│   │   │           ├── TrackersScreen.kt
│   │   │           └── SettingsScreen.kt
│   │   ├── cpp/
│   │   │   ├── CMakeLists.txt
│   │   │   ├── native-lib.cpp
│   │   │   └── qbittorrent/
│   │   │       ├── qbcore.h
│   │   │       └── qbcore.cpp
│   │   ├── res/
│   │   │   ├── values/
│   │   │   │   ├── strings.xml
│   │   │   │   ├── colors.xml
│   │   │   │   └── themes.xml
│   │   │   └── drawable/
│   │   ├── AndroidManifest.xml
│   │   └── build.gradle.kts
│   └── proguard-rules.pro
├── scripts/
│   ├── build_libtorrent.sh
│   └── release.sh
├── docs/
│   └── PORTING.md
├── gradle/
│   └── wrapper/
│       └── gradle-wrapper.properties
├── build.gradle.kts
├── settings.gradle.kts
├── gradle.properties
├── local.properties
└── README.md
```

## 编译步骤

### 1. 环境要求

- Android Studio Arctic Fox 或更高版本
- NDK r27
- CMake 3.22+
- JDK 17
- Android SDK 35

### 2. 配置 local.properties

```properties
sdk.dir=/path/to/android/sdk
ndk.dir=/path/to/android-ndk/ndk/27.0.12077973
cmake.dir=/path/to/android-sdk/cmake/3.22.1
```

### 3. 编译 libtorrent (可选，预编译库)

```bash
cd scripts
chmod +x build_libtorrent.sh
./build_libtorrent.sh
```

### 4. 在 Android Studio 中导入

1. 打开 Android Studio
2. 选择 "Open"
3. 选择 `android-qbittorrent` 目录
4. 等待 Gradle 同步完成
5. 构建项目

### 5. 命令行编译

```bash
# Debug APK
./gradlew assembleDebug

# Release APK
./gradlew assembleRelease
```

### 6. APK 输出位置

- Debug: `app/build/outputs/apk/debug/app-debug.apk`
- Release: `app/build/outputs/apk/release/app-arm64-v8a-release.apk`

## PT 模式配置

PT 模式会自动：
- 禁用 DHT
- 禁用 PEX (Peer Exchange)
- 禁用 LSD (Local Service Discovery)

适用于 private trackers (PT 站)。

## 权限说明

```xml
<uses-permission android:name="android.permission.INTERNET" />
<uses-permission android:name="android.permission.FOREGROUND_SERVICE" />
<uses-permission android:name="android.permission.POST_NOTIFICATIONS" />
<uses-permission android:name="android.permission.WAKE_LOCK" />
<uses-permission android:name="android.permission.REQUEST_IGNORE_BATTERY_OPTIMIZATIONS" />
```

## 后台运行注意事项

- 建议用户手动关闭电池优化
- 部分厂商 (小米/OPPO/vivo/华为) 需要手动设置白名单
- 长期做种建议使用 NAS 或 VPS

## 已知限制

1. Android 后台限制较多，需要用户手动配置
2. 部分 PT 站可能需要额外配置
3. WebUI 功能需要额外实现

## 版本信息

- **应用版本**: 1.0.0
- **qBittorrent Core**: 4.6.2
- **libtorrent**: 2.0.9

## 许可证

GPLv2 (与 qBittorrent 相同)