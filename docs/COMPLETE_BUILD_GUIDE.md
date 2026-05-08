# qBittorrent Android - 完整构建与部署指南

## 目录结构

```
android-qbittorrent/
├── Dockerfile                      # Docker 构建环境
├── docker-compose.yml              # Docker Compose 配置
├── README.md                       # 项目说明
├── BUILD.md                        # 构建指南
├── COMPLETE_BUILD_GUIDE.md         # 完整构建指南
│
├── local.properties                # 本地 SDK 配置
├── gradle.properties               # Gradle 配置
├── settings.gradle                 # Gradle 设置
├── build.gradle                    # 根项目配置
│
├── gradle/
│   └── wrapper/
│       └── gradle-wrapper.properties
│
├── app/
│   ├── build.gradle                # App 模块配置
│   ├── proguard-rules.pro          # ProGuard 配置
│   └── src/main/
│       ├── AndroidManifest.xml
│       ├── cpp/
│       │   ├── CMakeLists.txt       # C++ 构建配置
│       │   ├── native-lib.cpp      # JNI 入口
│       │   └── qbittorrent/
│       │       ├── qbcore.h         # Core 头文件
│       │       ├── qbcore.cpp       # Core 实现
│       │       ├── cJSON.h          # JSON 解析头
│       │       └── cJSON.c          # JSON 解析实现
│       ├── java/
│       │   └── com/qbandroid/
│       │       ├── MainActivity.kt
│       │       ├── QBittorrentApp.kt
│       │       ├── core/
│       │       │   └── NativeBridge.kt
│       │       ├── model/
│       │       │   └── Models.kt
│       │       ├── viewmodel/
│       │       │   └── TorrentViewModel.kt
│       │       ├── service/
│       │       │   ├── TorrentService.kt
│       │       │   └── BootReceiver.kt
│       │       └── ui/
│       │           ├── theme/
│       │           │   ├── Theme.kt
│       │           │   └── Typography.kt
│       │           └── screens/
│       │               ├── MainScreen.kt
│       │               ├── DownloadsScreen.kt
│       │               ├── AddTorrentScreen.kt
│       │               ├── TrackersScreen.kt
│       │               └── SettingsScreen.kt
│       └── res/
│           ├── values/
│           │   ├── strings.xml
│           │   ├── colors.xml
│           │   └── themes.xml
│           └── drawable/
│
├── scripts/
│   ├── build_env.sh                # 环境配置脚本
│   ├── build_all.sh                # 完整编译脚本 (Boost+OpenSSL+libtorrent)
│   ├── build_boost.sh             # Boost 编译脚本
│   ├── build_openssl.sh           # OpenSSL 编译脚本
│   ├── build_apk.sh               # APK 构建脚本
│   └── debug.sh                    # 调试脚本
│
└── docs/
    ├── PORTING.md                   # 移植说明
    └── BACKGROUND_KEEPALIVE.md      # 后台保活方案
```

---

## 构建顺序

### 步骤 1: 环境配置 (仅首次)

```bash
# 方式1: 一键配置 (推荐)
chmod +x scripts/build_env.sh
./scripts/build_env.sh

# 方式2: Docker
docker-compose build
docker-compose run --rm qbittorrent-builder

# 方式3: 手动
# 1. 安装 OpenJDK 17
# 2. 安装 Android SDK + NDK r27 + CMake
# 3. 配置 local.properties
```

### 步骤 2: 预编译 libtorrent

```bash
cd scripts
chmod +x build_all.sh
./build_all.sh
```

**耗时**: 约 30-60 分钟 (取决于 CPU)

**输出**:
- `third_party/libtorrent-install/lib/libtorrent.a`
- `third_party/boost-install/lib/`
- `third_party/openssl-install/lib/`

### 步骤 3: 构建 APK

```bash
# Debug APK
chmod +x scripts/build_apk.sh
./scripts/build_apk.sh debug

# Release APK
./scripts/build_apk.sh release

# 清理后重新构建
./scripts/build_apk.sh clean
./scripts/build_apk.sh debug
```

### 步骤 4: 安装测试

```bash
# 安装 APK
adb install app/build/outputs/apk/debug/app-debug.apk

# 查看日志
./scripts/debug.sh logcat

# 重启应用
./scripts/debug.sh restart
```

---

## 常见编译错误

### 1. CMake 找不到 NDK

```
CMake Error: Could not find NDK
```

**解决方案**:
```bash
# 检查 local.properties
cat local.properties
# 应该包含:
# ndk.dir=/path/to/ndk/27.0.12077973
```

### 2. libtorrent 链接失败

```
undefined reference to `lt::session::add_torrent'
```

**解决方案**:
```bash
# 重新运行编译脚本
cd scripts
./build_all.sh
```

### 3. Java 版本不对

```
error: invalid source release: 17
```

**解决方案**:
```bash
# 检查 Java 版本
java -version
# 应该是 17

# 设置 JAVA_HOME
export JAVA_HOME=/usr/lib/jvm/java-17-openjdk-amd64
```

### 4. Gradle 下载超时

```
Read timed out
```

**解决方案**:
```bash
# 使用国内镜像
# 在 gradle.properties 中添加:
org.gradle.parallel=true
org.gradle.daemon=true
org.gradle.jvmargs=-Xmx4096m

# 或者手动下载 gradle 8.4 到 ~/.gradle/wrapper/dists/
```

### 5. C++ 编译错误

```
fatal error: 'libtorrent/session.hpp' file not found
```

**解决方案**:
```bash
# 检查 CMake include 路径
cat app/src/main/cpp/CMakeLists.txt
# 确保 include_directories 指向正确的 libtorrent 头文件目录
```

### 6. 内存不足

```
Out of memory error
```

**解决方案**:
```bash
# 增加 Gradle 内存
# 在 gradle.properties:
org.gradle.jvmargs=-Xmx8192m -XX:+HeapDumpOnOutOfMemoryError
```

---

## Android 15 注意事项

### 1. 存储权限变化

Android 15 进一步限制了外部存储访问:

```kotlin
// Android 15 需要使用更精确的媒体权限
if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    requestPermissions(arrayOf(
        Manifest.permission.READ_MEDIA_VIDEO,
        Manifest.permission.READ_MEDIA_AUDIO,
        Manifest.permission.READ_MEDIA_IMAGES
    ), REQUEST_MEDIA_CODE)
} else {
    requestPermissions(arrayOf(
        Manifest.permission.READ_EXTERNAL_STORAGE
    ), REQUEST_STORAGE_CODE)
}
```

### 2. 前台服务类型

```xml
<!-- Android 15 需要指定 foregroundServiceType -->
<service
    android:name=".service.TorrentService"
    android:foregroundServiceType="dataSync"
    android:exported="false" />
```

### 3. 通知权限

```kotlin
// Android 15 需要动态请求通知权限
if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
        != PackageManager.PERMISSION_GRANTED) {
        requestPermission(Manifest.permission.POST_NOTIFICATIONS)
    }
}
```

### 4. 包可见性

Android 15 默认隐藏应用:

```xml
<!-- 如果需要查询其他应用 -->
<queries>
    <intent>
        <action android:name="android.intent.action.VIEW" />
        <data android:scheme="magnet" />
    </intent>
</queries>
```

---

## PT 做种稳定性建议

### 1. 网络优化

```kotlin
// 在 settings_pack 中配置
settings.set_int(connections_limit, 150)       // 限制连接数
settings.set_int(max_peer_list_size, 500)       // 限制 peer 列表
settings.set_int(download_rate_limit, 0)       // 不限速 (根据需求)
settings.set_int(upload_rate_limit, 0)         // 不限速
```

### 2. PT 模式配置

```kotlin
fun setPTMode(enabled: Boolean) {
    if (enabled) {
        // 禁用 DHT/PEX/LSD
        enable_dht = false
        enable_pex = false
        enable_lsd = false

        // 加密 (某些 PT 站必需)
        enc_policy = forced

        // 限速保护
        download_rate_limit = 10 * 1024 * 1024  // 10MB/s
        upload_rate_limit = 5 * 1024 * 1024      // 5MB/s
    }
}
```

### 3. Tracker 配置

```kotlin
// 添加备用 tracker
val backupTrackers = listOf(
    "https://tracker.example.com/announce",
    "udp://backup.example.com:6969/announce"
)

// 在 add_torrent_params 中设置
add_torrent_params.trackers = backupTrackers
```

### 4. 做种保持

```kotlin
// 确保 torrent 不会被自动暂停
torrent_handle.handle.set_flags(
    torrent_flags::none,
    torrent_flags::auto_managed  // 移除自动管理
)

// 定期保存 fast resume
fun periodicSaveResume() {
    session.save_state()
    // 保存到文件
}
```

### 5. 健康检查

```kotlin
// 监控 torrent 状态
data class TorrentHealth(
    val hash: String,
    val name: String,
    val state: String,
    val numPeers: Int,
    val numSeeds: Int,
    val downloadRate: Long,
    val uploadRate: Long,
    val ratio: Double,
    val errors: List<String>
)

// 检测问题 torrent
fun checkProblemTorrents(): List<TorrentHealth> {
    // 1. 检查长时间无速度
    // 2. 检查 tracker 错误
    // 3. 检查 peer 连接数
    // 4. 检查做种比率
}
```

---

## 最终总结

### 构建命令汇总

```bash
# 完整构建流程
cd android-qbittorrent

# 1. 配置环境
./scripts/build_env.sh

# 2. 编译 libtorrent (30-60分钟)
cd scripts
./build_all.sh

# 3. 构建 APK (5-10分钟)
cd ..
./scripts/build_apk.sh debug

# 4. 安装测试
adb install app/build/outputs/apk/debug/app-debug.apk

# 5. 查看日志
./scripts/debug.sh logcat
```

### APK 输出

| 类型 | 路径 | 大小 |
|------|------|------|
| Debug | `app/build/outputs/apk/debug/app-debug.apk` | ~30MB |
| Release | `app/build/outputs/apk/release/app-arm64-v8a-release.apk` | ~15MB |

### 功能验证

- ✅ Magnet 链接添加
- ✅ Torrent 文件添加
- ✅ 下载/做种
- ✅ PT 模式
- ✅ 后台运行
- ✅ Fast Resume

### 注意事项

1. **网络限制**: 确保编译机器网络畅通
2. **磁盘空间**: 需要至少 10GB
3. **编译时间**: 首次编译约 30-60 分钟
4. **PT 稳定性**: 建议使用 NAS 做长期种