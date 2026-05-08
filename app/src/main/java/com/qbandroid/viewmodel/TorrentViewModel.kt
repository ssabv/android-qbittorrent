package com.qbandroid.viewmodel

import androidx.lifecycle.ViewModel
import androidx.lifecycle.viewModelScope
import com.qbandroid.core.NativeBridge
import com.qbandroid.model.GlobalStats
import com.qbandroid.model.PeerInfo
import com.qbandroid.model.Settings
import com.qbandroid.model.TorrentInfo
import com.qbandroid.model.TorrentState
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import org.json.JSONArray
import org.json.JSONObject

class TorrentViewModel : ViewModel() {

    private val _torrents = MutableStateFlow<List<TorrentInfo>>(emptyList())
    val torrents: StateFlow<List<TorrentInfo>> = _torrents.asStateFlow()

    private val _stats = MutableStateFlow(GlobalStats(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0))
    val stats: StateFlow<GlobalStats> = _stats.asStateFlow()

    private val _settings = MutableStateFlow(Settings(
        downloadLimit = 0,
        uploadLimit = 0,
        maxConnections = 200,
        maxPeers = 500,
        listenPort = 6881,
        dhtEnabled = true,
        lsdEnabled = true,
        upnpEnabled = true,
        natpmpEnabled = true,
        encryption = 0,
        ptMode = false,
        autoStart = true,
        wakelockEnabled = true,
        downloadPath = ""
    ))
    val settings: StateFlow<Settings> = _settings.asStateFlow()

    private val _selectedTorrent = MutableStateFlow<TorrentInfo?>(null)
    val selectedTorrent: StateFlow<TorrentInfo?> = _selectedTorrent.asStateFlow()

    private val _isLoading = MutableStateFlow(false)
    val isLoading: StateFlow<Boolean> = _isLoading.asStateFlow()

    private val _error = MutableStateFlow<String?>(null)
    val error: StateFlow<String?> = _error.asStateFlow()

    private val _isSessionRunning = MutableStateFlow(false)
    val isSessionRunning: StateFlow<Boolean> = _isSessionRunning.asStateFlow()

    private var isInitialized = false

    init {
        startPeriodicRefresh()
    }

    private fun startPeriodicRefresh() {
        viewModelScope.launch(Dispatchers.IO) {
            while (isActive) {
                try {
                    if (_isSessionRunning.value) {
                        refreshTorrents()
                        refreshStats()
                    }
                } catch (e: Exception) {
                    e.printStackTrace()
                }
                delay(2000)
            }
        }
    }

    fun initialize() {
        if (isInitialized) return

        viewModelScope.launch(Dispatchers.IO) {
            try {
                _isSessionRunning.value = NativeBridge.isSessionRunning()
                if (_isSessionRunning.value) {
                    refreshSettings()
                    isInitialized = true
                }
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun onSessionStarted() {
        _isSessionRunning.value = true
        if (!isInitialized) {
            isInitialized = true
            refreshSettings()
        }
    }

    fun refreshTorrents() {
        try {
            val json = NativeBridge.getTorrentListJson()
            if (json.isBlank() || json == "[]") {
                _torrents.value = emptyList()
                return
            }

            val array = JSONArray(json)
            val list = mutableListOf<TorrentInfo>()

            for (i in 0 until array.length()) {
                try {
                    val obj = array.getJSONObject(i)
                    list.add(parseTorrentFromJson(obj))
                } catch (e: Exception) {
                    continue
                }
            }

            _torrents.value = list
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    private fun parseTorrentFromJson(obj: JSONObject): TorrentInfo {
        return TorrentInfo(
            hash = obj.optString("hash", ""),
            name = obj.optString("name", "Unknown"),
            size = obj.optLong("size", 0),
            progress = obj.optDouble("progress", 0.0),
            state = TorrentState.fromString(obj.optString("state", "unknown")),
            downloadSpeed = obj.optLong("download_speed", 0),
            uploadSpeed = obj.optLong("upload_speed", 0),
            downloaded = obj.optLong("downloaded", 0),
            uploaded = obj.optLong("uploaded", 0),
            ratio = obj.optDouble("ratio", 0.0),
            numSeeds = obj.optInt("num_seeds", 0),
            numLeechers = obj.optInt("num_leechers", 0),
            seedsConnected = obj.optInt("seeds_connected", 0),
            peersConnected = obj.optInt("peers_connected", 0)
        )
    }

    private fun refreshStats() {
        try {
            val json = NativeBridge.getGlobalStatsJson()
            if (json.isBlank() || json == "{}") {
                return
            }

            val obj = JSONObject(json)
            _stats.value = GlobalStats(
                downloadRate = obj.optLong("download_rate", 0),
                uploadRate = obj.optLong("upload_rate", 0),
                totalDownload = obj.optLong("total_download", 0),
                totalUpload = obj.optLong("total_upload", 0),
                numPeers = obj.optInt("num_peers", 0),
                numSeeds = obj.optInt("num_seeds", 0),
                numLeechers = obj.optInt("num_leechers", 0),
                dhtNodes = obj.optInt("dht_nodes", 0),
                torrentCount = obj.optInt("torrent_count", 0),
                pausedCount = obj.optInt("paused_torrent_count", 0),
                errorCount = obj.optInt("error_torrent_count", 0)
            )
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun refreshSettings() {
        try {
            val json = NativeBridge.getSettingsJson()
            if (json.isBlank() || json == "{}") {
                return
            }

            val obj = JSONObject(json)

            _settings.value = Settings(
                downloadLimit = obj.optInt("download_rate_limit", 0),
                uploadLimit = obj.optInt("upload_rate_limit", 0),
                maxConnections = obj.optInt("connections_limit", 200),
                maxPeers = obj.optInt("max_peer_list_size", 500),
                listenPort = obj.optInt("listen_port", 6881),
                dhtEnabled = obj.optBoolean("enable_dht", true),
                lsdEnabled = obj.optBoolean("enable_lsd", true),
                upnpEnabled = obj.optBoolean("enable_upnp", true),
                natpmpEnabled = obj.optBoolean("enable_natpmp", true),
                encryption = obj.optInt("enc_policy", 0),
                ptMode = obj.optBoolean("pt_mode", false),
                autoStart = true,
                wakelockEnabled = true,
                downloadPath = ""
            )
        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    fun addMagnet(magnetUri: String) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                _isLoading.value = true
                val hash = NativeBridge.addMagnet(magnetUri)
                if (hash.isNotBlank()) {
                    delay(500)
                    refreshTorrents()
                } else {
                    _error.value = "Failed to add magnet link"
                }
            } catch (e: Exception) {
                _error.value = e.message
            } finally {
                _isLoading.value = false
            }
        }
    }

    fun addTorrentFile(filePath: String) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                _isLoading.value = true
                val hash = NativeBridge.addTorrentFromFile(filePath)
                if (hash.isNotBlank()) {
                    delay(500)
                    refreshTorrents()
                }
            } catch (e: Exception) {
                _error.value = e.message
            } finally {
                _isLoading.value = false
            }
        }
    }

    fun addTorrentData(data: ByteArray, fileName: String) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                _isLoading.value = true
                val hash = NativeBridge.addTorrentFromData(data, fileName)
                if (hash.isNotBlank()) {
                    delay(500)
                    refreshTorrents()
                }
            } catch (e: Exception) {
                _error.value = e.message
            } finally {
                _isLoading.value = false
            }
        }
    }

    fun removeTorrent(hash: String, deleteFiles: Boolean = false) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                val result = NativeBridge.removeTorrent(hash, deleteFiles)
                if (result) {
                    delay(300)
                    refreshTorrents()
                }
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun resumeTorrent(hash: String) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.resumeTorrent(hash)
                delay(300)
                refreshTorrents()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun pauseTorrent(hash: String) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.pauseTorrent(hash)
                delay(300)
                refreshTorrents()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun forceRecheck(hash: String) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.forceRecheck(hash)
                delay(500)
                refreshTorrents()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setDownloadLimit(limit: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setGlobalDownloadLimit(limit)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setUploadLimit(limit: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setGlobalUploadLimit(limit)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setMaxConnections(limit: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setMaxConnections(limit)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setMaxPeers(limit: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setMaxPeers(limit)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setListenPort(port: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setListenPort(port)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setDHTEnabled(enabled: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setDHTEnabled(enabled)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setPEXEnabled(enabled: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setPEXEnabled(enabled)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setLSDEnabled(enabled: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setLSDEnabled(enabled)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setPTMode(enabled: Boolean) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setPTMode(enabled)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun setEncryptionMode(mode: Int) {
        viewModelScope.launch(Dispatchers.IO) {
            try {
                NativeBridge.setEncryptionMode(mode)
                refreshSettings()
            } catch (e: Exception) {
                _error.value = e.message
            }
        }
    }

    fun selectTorrent(torrent: TorrentInfo?) {
        _selectedTorrent.value = torrent
    }

    fun getTorrentDetails(hash: String): String {
        return try {
            NativeBridge.getTorrentDetailsJson(hash)
        } catch (e: Exception) {
            "{}"
        }
    }

    fun getTrackerList(hash: String): List<TrackerInfo> {
        return try {
            val json = NativeBridge.getTrackerListJson(hash)
            val array = JSONArray(json)
            val list = mutableListOf<TrackerInfo>()
            for (i in 0 until array.length()) {
                val obj = array.getJSONObject(i)
                list.add(
                    TrackerInfo(
                        hash = obj.optString("hash", ""),
                        name = "",
                        url = obj.optString("url", ""),
                        tier = obj.optInt("tier", 0),
                        status = obj.optString("status", ""),
                        lastError = obj.optInt("last_error", 0)
                    )
                )
            }
            list
        } catch (e: Exception) {
            emptyList()
        }
    }

    fun getFilesList(hash: String): List<FileInfo> {
        return try {
            val json = NativeBridge.getFilesListJson(hash)
            val array = JSONArray(json)
            val list = mutableListOf<FileInfo>()
            for (i in 0 until array.length()) {
                val obj = array.getJSONObject(i)
                list.add(
                    FileInfo(
                        index = obj.optInt("index", 0),
                        name = obj.optString("name", ""),
                        size = obj.optLong("size", 0),
                        progress = obj.optDouble("progress", 0.0),
                        priority = obj.optInt("priority", 0),
                        available = obj.optLong("available", 0)
                    )
                )
            }
            list
        } catch (e: Exception) {
            emptyList()
        }
    }

    fun getPeerList(hash: String): List<PeerInfo> {
        return try {
            val json = NativeBridge.getPeerListJson(hash)
            val array = JSONArray(json)
            val list = mutableListOf<PeerInfo>()
            for (i in 0 until array.length()) {
                val obj = array.getJSONObject(i)
                list.add(
                    PeerInfo(
                        ip = obj.optString("ip", ""),
                        port = obj.optInt("port", 0),
                        client = obj.optString("client", ""),
                        downloadRate = obj.optLong("download_rate", 0),
                        uploadRate = obj.optLong("upload_rate", 0),
                        progress = obj.optDouble("progress", 0.0),
                        peerSource = obj.optString("peer_source", "")
                    )
                )
            }
            list
        } catch (e: Exception) {
            emptyList()
        }
    }

    fun getVersion(): String {
        return try {
            NativeBridge.getVersion()
        } catch (e: Exception) {
            "unknown"
        }
    }

    fun getLibtorrentVersion(): String {
        return try {
            NativeBridge.getLibtorrentVersion()
        } catch (e: Exception) {
            "unknown"
        }
    }

    fun clearError() {
        _error.value = null
    }

    fun getDownloadingTorrents(): List<TorrentInfo> {
        return _torrents.value.filter { it.state == TorrentState.DOWNLOADING }
    }

    fun getSeedingTorrents(): List<TorrentInfo> {
        return _torrents.value.filter { it.state == TorrentState.SEEDING }
    }

    fun getPausedTorrents(): List<TorrentInfo> {
        return _torrents.value.filter { it.state == TorrentState.PAUSED }
    }
}

data class TrackerInfo(
    val hash: String,
    val name: String,
    val url: String,
    val tier: Int,
    val status: String,
    val lastError: Int
)

data class FileInfo(
    val index: Int,
    val name: String,
    val size: Long,
    val progress: Double,
    val priority: Int,
    val available: Long
)