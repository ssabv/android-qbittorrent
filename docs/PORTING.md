# qBittorrent Android Core 移植说明

## 核心架构

```
┌─────────────────────────────────────────────────────────────┐
│                    Android Application                       │
├─────────────────────────────────────────────────────────────┤
│  Kotlin + Jetpack Compose UI                                │
│  ViewModel + StateFlow                                      │
├─────────────────────────────────────────────────────────────┤
│                    JNI Bridge Layer                          │
│  NativeBridge.kt ↔ native-lib.cpp ↔ qbcore.cpp             │
├─────────────────────────────────────────────────────────────┤
│                 qBittorrent Core Bridge                     │
│  qbcore.cpp - 保留核心 BT/PT 逻辑                            │
│  - session 管理                                              │
│  - torrent handle 操作                                      │
│  - peer/tracker 管理                                        │
│  - fastresume                                               │
├─────────────────────────────────────────────────────────────┤
│                  libtorrent-rasterbar                        │
│  Android 交叉编译的静态库                                    │
│  - DHT/PEX/LSD                                              │
│  - uTP/TCP                                                   │
│  - encryption                                                │
│  - PT 协议支持                                               │
└─────────────────────────────────────────────────────────────┘
```

## PT 兼容性策略

### 1. 保留的功能

| 模块 | 状态 | 说明 |
|------|------|------|
| Peer ID | ✅ 保留 | 保持 qBittorrent 标识 |
| Announce | ✅ 保留 | 支持 HTTP/HTTPS tracker |
| Passkey | ✅ 保留 | 支持 private tracker 认证 |
| Ratio | ✅ 保留 | 上传/下载比率统计 |
| Upload stats | ✅ 保留 | 上传量、速度统计 |
| Fastresume | ✅ 保留 | 种子状态持久化 |

### 2. PT 模式配置

```cpp
void QBittorrentCore::setPTMode(bool enabled) {
    if (enabled) {
        // PT 模式：禁用 DHT/PEX/LSD
        settings_pack.set_bool(enable_dht, false);
        settings_pack.set_bool(enable_pex, false);
        settings_pack.set_bool(enable_lsd, false);

        // 限制连接数
        settings_pack.set_int(connections_limit, 100);
        settings_pack.set_int(max_peer_list_size, 100);

        // 加密策略
        settings_pack.set_int(enc_policy, enc_policy::forced);
    } else {
        // 普通 BT 模式
        settings_pack.set_bool(enable_dht, true);
        settings_pack.set_bool(enable_pex, true);
        settings_pack.set_bool(enable_lsd, true);
    }
    session->apply_settings(settings_pack);
}
```

### 3. Peer ID 格式

```cpp
// 保持原始 qBittorrent Peer ID
// -qB4v2.0.9-
// Android 版本: -qB4A1.0.0-

settings_pack.set_str(user_agent, "qBittorrent/4.6.2");
```

### 4. Tracker 兼容性

```cpp
// 支持的 tracker 类型
- HTTP/HTTPS (主流 PT 站)
- UDP (公共 tracker)
- DHT (PT 模式下禁用)
- PEX (PT 模式下禁用)

// Tracker 认证
add_torrent_params.set_username(username);
add_torrent_params.set_password(password);
```

## 做种功能

### 1. 保持做种配置

```cpp
bool QBittorrentCore::resumeTorrent(const std::string& hash) {
    torrent_handle handle = session->find_torrent(hash);
    // 清除 paused 标志，保持做种
    handle.set_flags(torrent_flags::none, torrent_flags::paused);
    return true;
}
```

### 2. Fast Resume

```cpp
void QBittorrentCore::saveFastResume() {
    entry state;
    session->save_state(state);
    // 保存到 Android 应用目录
    std::ofstream out(appDir + "/fastresume.data");
    lt::bencode(ostream_iterator<char>(out), state);
}

void QBittorrentCore::loadFastResume() {
    std::ifstream in(appDir + "/fastresume.data");
    entry state;
    lt::lazy_bdecode(istream_iterator<char>(in), ..., state);
    session->load_state(state);
}
```

### 3. 种子状态持久化

```json
// fastresume.data 结构
{
  "peers": [...],
  "trackers": [...],
  "file_progress": [...],
  "file_priority": [...],
  "save_path": "/storage/.../downloads",
  "added_time": 1234567890,
  "completed_time": 1234567890,
  "download_rate_limit": 0,
  "upload_rate_limit": 0
}
```

## Android 后台优化

### 1. Foreground Service

```kotlin
class TorrentService : Service() {
    override fun onCreate() {
        startForeground(NOTIFICATION_ID, createNotification())
        acquireWakeLock()
    }

    private fun createNotification(): Notification {
        return Notification.Builder(this, CHANNEL_ID)
            .setContentTitle("qBittorrent")
            .setContentText("Running - ${activeTorrents} torrents")
            .setSmallIcon(R.drawable.ic_notification)
            .setOngoing(true)
            .build()
    }
}
```

### 2. WakeLock

```kotlin
private fun acquireWakeLock() {
    val powerManager = getSystemService(POWER_SERVICE) as PowerManager
    wakeLock = powerManager.newWakeLock(
        PowerManager.PARTIAL_WAKE_LOCK,
        "qBittorrent::DownloadWakeLock"
    ).apply {
        acquire(MAX_VALUE.toLong(), "qBittorrent::Download")
    }
}
```

### 3. 自动恢复

```kotlin
class BootReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action == Intent.ACTION_BOOT_COMPLETED) {
            val prefs = context.getSharedPreferences("settings", MODE_PRIVATE)
            if (prefs.getBoolean("auto_start", true)) {
                context.startService(Intent(context, TorrentService::class.java))
            }
        }
    }
}
```

### 4. 电池优化检测

```kotlin
fun checkBatteryOptimization(context: Context): Boolean {
    val powerManager = context.getSystemService(POWER_SERVICE) as PowerManager
    return powerManager.isIgnoringBatteryOptimizations(context.packageName)
}
```

## 文件路径配置

### Android 11+ 推荐路径

```kotlin
val downloadDir = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
    Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS)
} else {
    File(Environment.getExternalStorageDirectory(), "Downloads")
}

val sessionDir = File(context.filesDir, "qbt_session")
```

### 权限处理

```kotlin
// Android 13+ 需要 READ_MEDIA_* 权限
if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
    requestPermissions(arrayOf(
        Manifest.permission.READ_MEDIA_VIDEO,
        Manifest.permission.READ_MEDIA_AUDIO,
        Manifest.permission.READ_MEDIA_IMAGES
    ), REQUEST_MEDIA_PERMISSION)
} else {
    requestPermissions(arrayOf(
        Manifest.permission.READ_EXTERNAL_STORAGE,
        Manifest.permission.WRITE_EXTERNAL_STORAGE
    ), REQUEST_STORAGE_PERMISSION)
}
```

## 网络优化

### 1. 连接配置

```cpp
settings_pack.set_int(connections_limit, 200);      // 全局最大连接
settings_pack.set_int(max_peer_list_size, 1000);    // 最大 peer 列表
settings_pack.set_int(active_limit, 50);            // 活跃种子数

settings_pack.set_int(recv_buffer_size, 128 * 1024);
settings_pack.set_int(send_buffer_size, 128 * 1024);
```

### 2. PT 限速

```cpp
// PT 模式下自动限速，避免被ban
if (ptMode) {
    settings_pack.set_int(download_rate_limit, 10 * 1024 * 1024); // 10MB/s
    settings_pack.set_int(upload_rate_limit, 5 * 1024 * 1024);    // 5MB/s
}
```

### 3. 加密策略

```cpp
settings_pack.set_int(enc_policy, enc_policy::enabled);
settings_pack.set_int(enc_level, enc_level::plaintext);
```

## 版本信息

- **qBittorrent**: 4.6.2
- **libtorrent**: 2.0.9
- **Android SDK**: 35 (Android 15)
- **Minimum SDK**: 29 (Android 10)
- **ABI**: arm64-v8a