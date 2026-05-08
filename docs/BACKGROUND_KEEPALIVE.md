# Android 后台保活方案

## 概述

Android 系统对后台应用有严格限制，特别是国产定制系统 (小米/OPPO/vivo/华为)。
本指南帮助用户在各种设备上正确配置 qBittorrent Android 以保持后台运行。

---

## 1. 系统权限要求

### 必需权限

| 权限 | 作用 | 状态 |
|------|------|------|
| 存储权限 | 读写下载文件 | 必需 |
| 通知权限 | 后台服务通知 | 必需 |
| 电池优化白名单 | 防止被省电杀掉 | 必需 |
| 自启动权限 | 开机自动启动 | 推荐 |
| 悬浮窗权限 | 显示下载状态 | 可选 |

### 检查权限状态

```kotlin
// 在 SettingsScreen.kt 中检查
fun checkPermissions(context: Context): List<String> {
    val issues = mutableListOf<String>()
    val pm = context.packageManager
    val powerManager = context.getSystemService(Context.POWER_SERVICE) as PowerManager

    // 1. 检查电池优化
    if (!powerManager.isIgnoringBatteryOptimizations(context.packageName)) {
        issues.add("电池优化未关闭")
    }

    // 2. 检查通知权限 (Android 13+)
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
        val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
        if (!notificationManager.areNotificationsEnabled()) {
            issues.add("通知权限未开启")
        }
    }

    return issues
}
```

---

## 2. 厂商特定配置

### 小米 (MIUI)

**设置步骤:**

1. **省电策略**
   - 设置 → 应用设置 → 应用管理 → qBittorrent
   - 电池 Saver → 设为 "无限制"

2. **自启动**
   - 设置 → 应用设置 → 应用管理 → 自启动管理
   - 找到 qBittorrent → 开启

3. **锁屏清理**
   - 设置 → 电池和性能 → 锁屏清理
   - 找到 qBittorrent → 关闭

4. **后台弹出界面**
   - 设置 → 应用设置 → 应用管理 → qBittorrent
   - 权限管理 → 后台弹出界面 → 允许

### OPPO (ColorOS)

**设置步骤:**

1. **电池优化**
   - 设置 → 电池 → 更多设置 → 受保护的应用程序
   - 找到 qBittorrent → 打开

2. **自启动**
   - 设置 → 应用启动管理 → qBittorrent
   - 关闭 "自动管理"，手动打开所有开关

3. **省电**
   - 设置 → 电池 → 省电 → 关闭

### vivo (OriginOS)

**设置步骤:**

1. **后台耗电**
   - 设置 → 电池 → 后台耗电管理 → qBittorrent → 关闭

2. **应用启动**
   - 设置 → 应用与权限 → 应用管理 → 权限 → 自启动
   - 找到 qBittorrent → 允许

3. **省电**
   - i 管家 → 清理加速 → 右下角设置
   - 关闭 "清理后台应用"

### 华为 (HarmonyOS)

**设置步骤:**

1. **应用启动管理**
   - 设置 → 应用 → 应用启动管理
   - 找到 qBittorrent → 关闭 "自动管理"
   - 手动打开 "允许自启动"、"允许后台活动"

2. **省电**
   - 设置 → 电池 → 右上角设置
   - 关闭 "省电模式"

3. **锁屏后台应用**
   - 设置 → 电池 → 更多设置 → 关闭 "锁屏后清理"

---

## 3. 应用内检测和提示

```kotlin
// 在 MainActivity.kt 中实现

class MainActivity : ComponentActivity() {

    private var batteryOptimizationDialog by mutableStateOf(false)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // 检查电池优化
        checkBatteryOptimization()
    }

    private fun checkBatteryOptimization() {
        val powerManager = getSystemService(POWER_SERVICE) as PowerManager
        val isIgnoring = powerManager.isIgnoringBatteryOptimizations(packageName)

        if (!isIgnoring) {
            showBatteryOptimizationDialog()
        }
    }

    private fun showBatteryOptimizationDialog() {
        AlertDialog.Builder(this)
            .setTitle("电池优化提示")
            .setMessage("为了确保 qBittorrent 后台持续运行和做种，请关闭电池优化。\n\n" +
                    "步骤: 设置 → 应用 → qBittorrent → 电池 → 设为'无限制'")
            .setPositiveButton("去设置") { _, _ ->
                requestBatteryOptimizationDisable()
            }
            .setNegativeButton("稍后", null)
            .show()
    }

    private fun requestBatteryOptimizationDisable() {
        try {
            val intent = Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS).apply {
                data = Uri.parse("package:$packageName")
            }
            startActivity(intent)
        } catch (e: Exception) {
            // 某些设备可能不支持
            try {
                startActivity(Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS))
            } catch (e2: Exception) {
                // 忽略
            }
        }
    }

    // 检测厂商并显示对应提示
    private fun getDeviceManufacturer(): String {
        return Build.MANUFACTURER.lowercase()
    }

    private fun showManufacturerSpecificHint() {
        val manufacturer = getDeviceManufacturer()

        val message = when {
            manufacturer.contains("xiaomi") -> "小米设备: 请在'省电策略'中设为'无限制'"
            manufacturer.contains("oppo") -> "OPPO设备: 请在'应用启动管理'中手动开启"
            manufacturer.contains("vivo") -> "vivo设备: 请在'后台耗电管理'中关闭限制"
            manufacturer.contains("huawei") -> "华为设备: 请在'应用启动管理'中手动开启"
            else -> "请在系统设置中关闭电池优化"
        }

        // 显示提示 Snackbar
    }
}
```

---

## 4. 自动化检测代码

```kotlin
// 检测工具类
object BackgroundKeepAliveHelper {

    fun checkAll(context: Context): BackgroundCheckResult {
        val pm = context.packageManager
        val powerManager = context.getSystemService(Context.POWER_SERVICE) as PowerManager

        val issues = mutableListOf<String>()
        val recommendations = mutableListOf<String>()

        // 1. 电池优化
        if (!powerManager.isIgnoringBatteryOptimizations(context.packageName)) {
            issues.add("电池优化未关闭")
            recommendations.add("设置 → 电池优化 → 关闭 qBittorrent 优化")
        }

        // 2. 通知权限 (Android 13+)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            val notificationManager = context.getSystemService(Context.NOTIFICATION_SERVICE) as NotificationManager
            if (!notificationManager.areNotificationsEnabled()) {
                issues.add("通知权限未开启")
                recommendations.add("设置 → 通知 → 允许 qBittorrent 通知")
            }
        }

        // 3. 厂商特定检测
        val manufacturer = Build.MANUFACTURER.lowercase()

        when {
            manufacturer.contains("xiaomi") -> {
                if (!isMiuiAutoStartEnabled(context)) {
                    recommendations.add("MIUI: 设置 → 应用自启动 → 开启 qBittorrent")
                }
            }
            manufacturer.contains("huawei") -> {
                if (!isHuaweiProtected(context)) {
                    recommendations.add("EMUI: 设置 → 电池 → 受保护应用 → 开启 qBittorrent")
                }
            }
        }

        return BackgroundCheckResult(
            isHealthy = issues.isEmpty(),
            issues = issues,
            recommendations = recommendations
        )
    }

    private fun isMiuiAutoStartEnabled(context: Context): Boolean {
        return try {
            val pm = context.packageManager
            pm.getApplicationInfoEnabledSetting("com.qbandroid")
            true
        } catch (e: Exception) {
            false
        }
    }

    private fun isHuaweiProtected(context: Context): Boolean {
        return try {
            val intent = Intent().apply {
                component = ComponentName("com.huawei.systemmanager",
                    "com.huawei.systemmanager.optimize.process.ProtectActivity")
            }
            context.packageManager.resolveActivity(intent, 0) != null
        } catch (e: Exception) {
            false
        }
    }
}

data class BackgroundCheckResult(
    val isHealthy: Boolean,
    val issues: List<String>,
    val recommendations: List<String>
)
```

---

## 5. 用户提示页面 UI

```kotlin
@Composable
fun BatteryOptimizationScreen() {
    val context = LocalContext.current

    Column(
        modifier = Modifier.padding(16.dp)
    ) {
        Text(
            text = "后台运行设置",
            style = MaterialTheme.typography.headlineSmall
        )

        Spacer(modifier = Modifier.height(16.dp))

        val result = BackgroundKeepAliveHelper.checkAll(context)

        if (!result.isHealthy) {
            Card(
                colors = CardDefaults.cardColors(
                    containerColor = MaterialTheme.colorScheme.errorContainer
                )
            ) {
                Column(modifier = Modifier.padding(16.dp)) {
                    Text(
                        text = "检测到问题",
                        color = MaterialTheme.colorScheme.error
                    )
                    result.issues.forEach {
                        Text(text = "• $it", modifier = Modifier.padding(start = 8.dp))
                    }
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Text(
            text = "推荐设置",
            style = MaterialTheme.typography.titleMedium
        )

        result.recommendations.forEach { rec ->
            Card(
                modifier = Modifier.padding(vertical = 4.dp)
            ) {
                Text(
                    text = rec,
                    modifier = Modifier.padding(12.dp),
                    style = MaterialTheme.typography.bodyMedium
                )
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Button(onClick = {
            val intent = Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS).apply {
                data = Uri.parse("package:${context.packageName}")
            }
            context.startActivity(intent)
        }) {
            Text("打开电池优化设置")
        }
    }
}
```

---

## 6. 总结

| 厂商 | 关键设置 | 难度 |
|------|----------|------|
| 小米 | 省电策略 → 无限制 | 中等 |
| OPPO | 应用启动管理 | 困难 |
| vivo | 后台耗电管理 | 中等 |
| 华为 | 应用启动管理 | 困难 |
| Samsung | 电池优化 | 简单 |
| 原生 Android | 电池优化 | 简单 |

**最佳实践**: 
1. 首次启动时自动检测并提示用户
2. 在设置页面提供一键优化入口
3. 提供各厂商详细图文教程链接