package com.qbandroid.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.qbandroid.model.TorrentInfo
import com.qbandroid.model.TrackerInfo
import com.qbandroid.viewmodel.TorrentViewModel
import org.json.JSONArray
import org.json.JSONObject

@Composable
fun TrackersScreen(
    viewModel: TorrentViewModel = viewModel()
) {
    val torrents by viewModel.torrents.collectAsState()

    val trackersMap = remember(torrents) {
        torrents.associate { torrent ->
            torrent.hash to getTrackersForTorrent(viewModel, torrent.hash)
        }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        Text(
            text = "Trackers",
            style = MaterialTheme.typography.headlineMedium
        )

        Spacer(modifier = Modifier.height(8.dp))

        Text(
            text = "${torrents.size} torrents",
            style = MaterialTheme.typography.bodyMedium,
            color = MaterialTheme.colorScheme.onSurfaceVariant
        )

        Spacer(modifier = Modifier.height(16.dp))

        if (torrents.isEmpty()) {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = "No trackers\nAdd torrents to see their trackers",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        } else {
            LazyColumn(
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                items(torrents, key = { it.hash }) { torrent ->
                    TrackerCard(
                        torrent = torrent,
                        trackers = trackersMap[torrent.hash] ?: emptyList()
                    )
                }
            }
        }
    }
}

private fun getTrackersForTorrent(viewModel: TorrentViewModel, hash: String): List<TrackerInfo> {
    return try {
        val json = viewModel.getTrackerList(hash)
        val array = JSONArray(json)
        val list = mutableListOf<TrackerInfo>()

        for (i in 0 until array.length()) {
            val obj = array.getJSONObject(i)
            list.add(
                TrackerInfo(
                    hash = obj.optString("hash", ""),
                    name = obj.optString("name", ""),
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

@Composable
fun TrackerCard(
    torrent: TorrentInfo,
    trackers: List<TrackerInfo>
) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(
            modifier = Modifier.padding(12.dp)
        ) {
            Text(
                text = torrent.name,
                style = MaterialTheme.typography.titleMedium,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )

            Spacer(modifier = Modifier.height(8.dp))

            if (trackers.isEmpty()) {
                Text(
                    text = "No trackers",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            } else {
                trackers.forEach { tracker ->
                    TrackerRow(tracker = tracker)
                    Spacer(modifier = Modifier.height(4.dp))
                }
            }
        }
    }
}

@Composable
fun TrackerRow(tracker: TrackerInfo) {
    val statusColor = when {
        tracker.status.contains("Working") || tracker.status.contains("OK") ->
            MaterialTheme.colorScheme.primary
        tracker.status.contains("Error") || tracker.lastError != 0 ->
            MaterialTheme.colorScheme.error
        else -> MaterialTheme.colorScheme.onSurfaceVariant
    }

    Row(
        modifier = Modifier.fillMaxWidth(),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = extractDomain(tracker.url),
                style = MaterialTheme.typography.bodySmall,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
            Text(
                text = tracker.url,
                style = MaterialTheme.typography.labelSmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant,
                maxLines = 1,
                overflow = TextOverflow.Ellipsis
            )
        }

        Text(
            text = tracker.status,
            style = MaterialTheme.typography.labelSmall,
            color = statusColor
        )
    }
}

private fun extractDomain(url: String): String {
    return try {
        val domain = url
            .removePrefix("http://")
            .removePrefix("https://")
            .removePrefix("udp://")
            .substringBefore("/")
        domain
    } catch (e: Exception) {
        url
    }
}