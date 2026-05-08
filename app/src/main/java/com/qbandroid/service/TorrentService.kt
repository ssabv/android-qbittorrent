package com.qbandroid.service

import android.app.Notification
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.SharedPreferences
import android.net.ConnectivityManager
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.os.Binder
import android.os.Build
import android.os.IBinder
import android.os.PowerManager
import androidx.core.app.NotificationCompat
import com.qbandroid.MainActivity
import com.qbandroid.QBittorrentApp
import com.qbandroid.R
import com.qbandroid.core.NativeBridge
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch

class TorrentService : Service() {

    private val binder = LocalBinder()
    private val serviceScope = CoroutineScope(SupervisorJob() + Dispatchers.IO)

    private var wakeLock: PowerManager.WakeLock? = null
    private var networkCallback: ConnectivityManager.NetworkCallback? = null

    private var updateJob: Job? = null
    private var lastTorrentCount = 0

    private lateinit var prefs: SharedPreferences

    private var sessionInitialized = false
    private var isNetworkAvailable = false

    var onSessionStarted: (() -> Unit)? = null

    inner class LocalBinder : Binder() {
        fun getService(): TorrentService = this@TorrentService
    }

    override fun onCreate() {
        super.onCreate()
        prefs = getSharedPreferences("qbt_settings", Context.MODE_PRIVATE)

        if (prefs.getBoolean(KEY_WAKELOCK_ENABLED, true)) {
            acquireWakeLock()
        }

        registerNetworkCallback()

        if (isNetworkAvailable) {
            initializeSession()
        }
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_PAUSE_ALL -> {
                pauseAllTorrents()
            }
            ACTION_RESUME_ALL -> {
                resumeAllTorrents()
            }
        }

        startForeground(QBittorrentApp.NOTIFICATION_ID, createNotification())
        return START_STICKY
    }

    override fun onBind(intent: Intent?): IBinder {
        return binder
    }

    override fun onDestroy() {
        super.onDestroy()
        saveStateAndStop()

        wakeLock?.let {
            if (it.isHeld) {
                it.release()
            }
        }

        networkCallback?.let {
            try {
                val cm = getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager
                cm.unregisterNetworkCallback(it)
            } catch (e: Exception) {
                // ignore
            }
        }

        updateJob?.cancel()
    }

    private fun initializeSession() {
        if (sessionInitialized) return

        val sessionDir = filesDir.absolutePath + "/session"
        val downloadDir = getDownloadPath()

        java.io.File(sessionDir).mkdirs()
        java.io.File(downloadDir).mkdirs()

        try {
            val result = NativeBridge.initialize(sessionDir, downloadDir)
            if (result) {
                // 先加载 fast resume
                NativeBridge.loadFastResume()

                // 启动 session
                NativeBridge.startSession()

                // 启动 alert 处理线程 (重要!)
                NativeBridge.startAlertHandler()

                sessionInitialized = true

                // 启动定期更新
                startPeriodicUpdate()

                // 启动网络监控
                registerNetworkCallback()

                // 更新通知
                updateNotification()

                // 回调
                onSessionStarted?.invoke()

                LOGI("Session initialized with alert handler")
            }
        } catch (e: Exception) {
            LOGE("Failed to initialize session: ${e.message}")
            e.printStackTrace()
        }
    }

    private fun startDownloading() {
        initializeSession()
    }

    private fun pauseAllTorrents() {
        if (!sessionInitialized) return

        try {
            val json = NativeBridge.getTorrentListJson()
            val array = org.json.JSONArray(json)
            for (i in 0 until array.length()) {
                val obj = array.getJSONObject(i)
                val hash = obj.optString("hash", "")
                val state = obj.optString("state", "")
                if (state == "downloading" || state == "seeding") {
                    NativeBridge.pauseTorrent(hash)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun resumeAllTorrents() {
        if (!sessionInitialized) return

        try {
            val json = NativeBridge.getTorrentListJson()
            val array = org.json.JSONArray(json)
            for (i in 0 until array.length()) {
                val obj = array.getJSONObject(i)
                val hash = obj.optString("hash", "")
                val state = obj.optString("state", "")
                if (state == "paused") {
                    NativeBridge.resumeTorrent(hash)
                }
            }
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun saveStateAndStop() {
        if (sessionInitialized) {
            try {
                // 先停止 alert handler
                NativeBridge.stopAlertHandler()

                // 保存 fast resume
                NativeBridge.saveFastResume()

                // 停止 session
                NativeBridge.stopSession()

                LOGI("Session stopped, alert handler stopped")
            } catch (e: Exception) {
                e.printStackTrace()
            }
            sessionInitialized = false
        }
    }

    private fun acquireWakeLock() {
        val powerManager = getSystemService(POWER_SERVICE) as PowerManager
        wakeLock = powerManager.newWakeLock(
            PowerManager.PARTIAL_WAKE_LOCK,
            "qBittorrent::TorrentServiceWakeLock"
        ).apply {
            setReferenceCounted(false)
            acquire(10 * 60 * 60 * 1000L)
        }
    }

    private fun registerNetworkCallback() {
        val cm = getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager

        val request = NetworkRequest.Builder()
            .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
            .build()

        networkCallback = object : ConnectivityManager.NetworkCallback() {
            override fun onAvailable(network: Network) {
                isNetworkAvailable = true
                LOGI("Network available")

                if (!sessionInitialized) {
                    // 首次连接，启动 session
                    startDownloading()
                } else {
                    // 网络恢复，重新初始化 session
                    LOGI("Network restored, re-initializing session")
                    try {
                        // 重新加载 fast resume 并恢复
                        val sessionDir = filesDir.absolutePath + "/session"
                        val downloadDir = getDownloadPath()
                        NativeBridge.initialize(sessionDir, downloadDir)
                        NativeBridge.loadFastResume()
                        NativeBridge.startSession()
                        NativeBridge.startAlertHandler()
                        sessionInitialized = true
                        LOGI("Session restored after network recovery")
                    } catch (e: Exception) {
                        LOGE("Failed to restore session: ${e.message}")
                    }
                }
            }

            override fun onLost(network: Network) {
                isNetworkAvailable = false
                LOGI("Network lost")
            }

            override fun onUnavailable() {
                isNetworkAvailable = false
                LOGI("Network unavailable")
            }

            override fun onCapabilitiesChanged(
                network: Network,
                capabilities: NetworkCapabilities
            ) {
                val hasInternet = capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                val validated = capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)
                LOGI("Network capabilities changed - Internet: $hasInternet, Validated: $validated")

                if (hasInternet && validated && !sessionInitialized) {
                    startDownloading()
                }
            }
        }

        try {
            cm.registerNetworkCallback(request, networkCallback!!)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun LOGI(msg: String) {
        android.util.Log.i("TorrentService", msg)
    }

    private fun LOGE(msg: String) {
        android.util.Log.e("TorrentService", msg)
    }

    private fun isNetworkAvailable(): Boolean {
        val cm = getSystemService(CONNECTIVITY_SERVICE) as ConnectivityManager
        val network = cm.activeNetwork ?: return false
        val capabilities = cm.getNetworkCapabilities(network) ?: return false

        return capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET) &&
                capabilities.hasCapability(NetworkCapabilities.NET_CAPABILITY_VALIDATED)
    }

    private fun getDownloadPath(): String {
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            getExternalFilesDir(null)?.absolutePath + "/Downloads"
        } else {
            java.io.File(
                android.os.Environment.getExternalStorageDirectory(),
                "Download/qBittorrent"
            ).absolutePath
        }
    }

    private fun startPeriodicUpdate() {
        updateJob = serviceScope.launch {
            while (isActive) {
                try {
                    updateNotification()
                } catch (e: Exception) {
                    e.printStackTrace()
                }
                delay(3000)
            }
        }
    }

    private fun updateNotification() {
        if (!sessionInitialized) return

        try {
            val currentCount = NativeBridge.getTorrentCount()
            val statsJson = NativeBridge.getTransferInfoJson()

            lastTorrentCount = currentCount

            val notification = createNotificationWithStats(currentCount, statsJson)
            val nm = getSystemService(NOTIFICATION_SERVICE) as NotificationManager
            nm.notify(QBittorrentApp.NOTIFICATION_ID, notification)
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun createNotification(): Notification {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val pauseIntent = Intent(this, TorrentService::class.java).apply {
            action = ACTION_PAUSE_ALL
        }
        val pausePendingIntent = PendingIntent.getService(
            this, 1, pauseIntent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, QBittorrentApp.NOTIFICATION_CHANNEL_ID)
            .setContentTitle(getString(R.string.notification_title))
            .setContentText("Service running")
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentIntent(pendingIntent)
            .addAction(android.R.drawable.ic_media_pause, "Pause", pausePendingIntent)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .build()
    }

    private fun createNotificationWithStats(torrentCount: Int, statsJson: String): Notification {
        val intent = Intent(this, MainActivity::class.java)
        val pendingIntent = PendingIntent.getActivity(
            this, 0, intent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val downloadSpeed = parseDownloadSpeed(statsJson)
        val uploadSpeed = parseUploadSpeed(statsJson)

        val statusText = if (torrentCount > 0) {
            "↓ ${formatSpeed(downloadSpeed)} ↑ ${formatSpeed(uploadSpeed)} | $torrentCount torrents"
        } else {
            "No active torrents"
        }

        return NotificationCompat.Builder(this, QBittorrentApp.NOTIFICATION_CHANNEL_ID)
            .setContentTitle(getString(R.string.notification_title))
            .setContentText(statusText)
            .setSmallIcon(android.R.drawable.stat_sys_download)
            .setContentIntent(pendingIntent)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .setCategory(NotificationCompat.CATEGORY_SERVICE)
            .build()
    }

    private fun parseDownloadSpeed(json: String): Long {
        return try {
            val regex = "\"download_rate\":\\s*([0-9]+)".toRegex()
            val match = regex.find(json)
            match?.groupValues?.get(1)?.toLongOrNull() ?: 0L
        } catch (e: Exception) {
            0L
        }
    }

    private fun parseUploadSpeed(json: String): Long {
        return try {
            val regex = "\"upload_rate\":\\s*([0-9]+)".toRegex()
            val match = regex.find(json)
            match?.groupValues?.get(1)?.toLongOrNull() ?: 0L
        } catch (e: Exception) {
            0L
        }
    }

    private fun formatSpeed(bytesPerSec: Long): String {
        return when {
            bytesPerSec < 1024 -> "${bytesPerSec}B/s"
            bytesPerSec < 1024 * 1024 -> "${bytesPerSec / 1024}KB/s"
            else -> "${bytesPerSec / (1024 * 1024)}MB/s"
        }
    }

    fun getServiceState(): Boolean = sessionInitialized

    fun restartService() {
        saveStateAndStop()
        initializeSession()
    }

    fun refreshTorrents() {
        if (sessionInitialized) {
            try {
                NativeBridge.getTorrentListJson()
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }

    companion object {
        private const val KEY_WAKELOCK_ENABLED = "wakelock_enabled"
        const val ACTION_PAUSE_ALL = "com.qbandroid.PAUSE_ALL"
        const val ACTION_RESUME_ALL = "com.qbandroid.RESUME_ALL"
    }
}