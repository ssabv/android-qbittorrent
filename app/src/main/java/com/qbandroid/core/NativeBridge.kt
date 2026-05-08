package com.qbandroid.core

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

object NativeBridge {

    init {
        System.loadLibrary("qbcore")
    }

    external fun initialize(sessionDir: String, downloadPath: String): Boolean

    external fun startSession()

    external fun stopSession()

    external fun isSessionRunning(): Boolean

    external fun addMagnet(magnetUri: String): String

    external fun addTorrentFromData(data: ByteArray, fileName: String): String

    external fun addTorrentFromFile(filePath: String): String

    external fun removeTorrent(hash: String, deleteFiles: Boolean): Boolean

    external fun resumeTorrent(hash: String): Boolean

    external fun pauseTorrent(hash: String): Boolean

    external fun forceRecheck(hash: String): Boolean

    external fun setUploadLimit(hash: String, limit: Int): Boolean

    external fun setDownloadLimit(hash: String, limit: Int): Boolean

    external fun setGlobalUploadLimit(limit: Int): Boolean

    external fun setGlobalDownloadLimit(limit: Int): Boolean

    external fun setMaxConnections(limit: Int): Boolean

    external fun setMaxPeers(limit: Int): Boolean

    external fun setListenPort(port: Int): Boolean

    external fun setDHTEnabled(enabled: Boolean): Boolean

    external fun setPEXEnabled(enabled: Boolean): Boolean

    external fun setLSDEnabled(enabled: Boolean): Boolean

    external fun setEncryptionMode(mode: Int): Boolean

    external fun setPTMode(enabled: Boolean): Boolean

    external fun getTorrentCount(): Int

    external fun getTorrentListJson(): String

    external fun getTorrentDetailsJson(hash: String): String

    external fun getGlobalStatsJson(): String

    external fun getSettingsJson(): String

    external fun setSettingsJson(settingsJson: String): Boolean

    external fun getPeerListJson(hash: String): String

external fun getTrackerListJson(hash: String): String

    external fun getFilesListJson(hash: String): String

    external fun setFilePriority(hash: String, index: Int, priority: Int): Boolean

    external fun getTransferInfoJson(): String

    external fun getAllTrackerInfoJson(): String

    external fun addTracker(hash: String, url: String): Boolean

    external fun editTracker(hash: String, oldUrl: String, newUrl: String): Boolean

    external fun removeTracker(hash: String, url: String): Boolean

    external fun saveFastResume()

    external fun loadFastResume()

    external fun startAlertHandler()

    external fun stopAlertHandler()

    external fun loadFastResume()

    external fun startAlertHandler()

    external fun stopAlertHandler()

    external fun getVersion(): String

    external fun getLibtorrentVersion(): String

    suspend fun addMagnetAsync(magnetUri: String): String = withContext(Dispatchers.IO) {
        addMagnet(magnetUri)
    }

    suspend fun addTorrentFromDataAsync(data: ByteArray, fileName: String): String = withContext(Dispatchers.IO) {
        addTorrentFromData(data, fileName)
    }

    suspend fun addTorrentFromFileAsync(filePath: String): String = withContext(Dispatchers.IO) {
        addTorrentFromFile(filePath)
    }

    suspend fun removeTorrentAsync(hash: String, deleteFiles: Boolean): Boolean = withContext(Dispatchers.IO) {
        removeTorrent(hash, deleteFiles)
    }

    suspend fun getTorrentListJsonAsync(): String = withContext(Dispatchers.IO) {
        getTorrentListJson()
    }

    suspend fun getGlobalStatsJsonAsync(): String = withContext(Dispatchers.IO) {
        getGlobalStatsJson()
    }
}