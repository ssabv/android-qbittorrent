package com.qbandroid.model

import org.json.JSONObject

data class TorrentInfo(
    val hash: String,
    val name: String,
    val size: Long,
    val progress: Double,
    val state: TorrentState,
    val downloadSpeed: Long,
    val uploadSpeed: Long,
    val downloaded: Long,
    val uploaded: Long,
    val ratio: Double,
    val numSeeds: Int,
    val numLeechers: Int,
    val seedsConnected: Int,
    val peersConnected: Int
) {
    companion object {
        fun fromJson(json: JSONObject): TorrentInfo {
            return TorrentInfo(
                hash = json.optString("hash", ""),
                name = json.optString("name", ""),
                size = json.optLong("size", 0),
                progress = json.optDouble("progress", 0.0),
                state = TorrentState.fromString(json.optString("state", "unknown")),
                downloadSpeed = json.optLong("download_speed", 0),
                uploadSpeed = json.optLong("upload_speed", 0),
                downloaded = json.optLong("downloaded", 0),
                uploaded = json.optLong("uploaded", 0),
                ratio = json.optDouble("ratio", 0.0),
                numSeeds = json.optInt("num_seeds", 0),
                numLeechers = json.optInt("num_leechers", 0),
                seedsConnected = json.optInt("seeds_connected", 0),
                peersConnected = json.optInt("peers_connected", 0)
            )
        }
    }

    fun getProgressPercent(): Int = (progress * 100).toInt()
}

enum class TorrentState(val displayName: String) {
    UNKNOWN("Unknown"),
    CHECKING("Checking"),
    DOWNLOADING_METADATA("Meta"),
    DOWNLOADING("Downloading"),
    SEEDING("Seeding"),
    PAUSED("Paused"),
    STOPPED("Stopped"),
    QUEUED("Queued");

    companion object {
        fun fromString(state: String): TorrentState {
            return when (state.lowercase()) {
                "checking" -> CHECKING
                "meta" -> DOWNLOADING_METADATA
                "downloading" -> DOWNLOADING
                "seeding" -> SEEDING
                "paused" -> PAUSED
                "stopped" -> STOPPED
                "queued" -> QUEUED
                else -> UNKNOWN
            }
        }
    }
}

data class GlobalStats(
    val downloadRate: Long,
    val uploadRate: Long,
    val totalDownload: Long,
    val totalUpload: Long,
    val numPeers: Int,
    val numSeeds: Int,
    val numLeechers: Int,
    val dhtNodes: Int,
    val torrentCount: Int,
    val pausedCount: Int,
    val errorCount: Int
) {
    companion object {
        fun fromJson(json: JSONObject): GlobalStats {
            return GlobalStats(
                downloadRate = json.optLong("download_rate", 0),
                uploadRate = json.optLong("upload_rate", 0),
                totalDownload = json.optLong("total_download", 0),
                totalUpload = json.optLong("total_upload", 0),
                numPeers = json.optInt("num_peers", 0),
                numSeeds = json.optInt("num_seeds", 0),
                numLeechers = json.optInt("num_leechers", 0),
                dhtNodes = json.optInt("dht_nodes", 0),
                torrentCount = json.optInt("torrent_count", 0),
                pausedCount = json.optInt("paused_torrent_count", 0),
                errorCount = json.optInt("error_torrent_count", 0)
            )
        }
    }
}

data class Settings(
    val downloadLimit: Int,
    val uploadLimit: Int,
    val maxConnections: Int,
    val maxPeers: Int,
    val listenPort: Int,
    val dhtEnabled: Boolean,
    val lsdEnabled: Boolean,
    val upnpEnabled: Boolean,
    val natpmpEnabled: Boolean,
    val encryption: Int,
    val ptMode: Boolean,
    val autoStart: Boolean,
    val wakelockEnabled: Boolean,
    val downloadPath: String
)

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

data class PeerInfo(
    val ip: String,
    val port: Int,
    val client: String,
    val downloadRate: Long,
    val uploadRate: Long,
    val progress: Double,
    val peerSource: String
) {
    companion object {
        fun fromJson(json: JSONObject): PeerInfo {
            return PeerInfo(
                ip = json.optString("ip", ""),
                port = json.optInt("port", 0),
                client = json.optString("client", ""),
                downloadRate = json.optLong("download_rate", 0),
                uploadRate = json.optLong("upload_rate", 0),
                progress = json.optDouble("progress", 0.0),
                peerSource = json.optString("peer_source", "")
            )
        }
    }
}