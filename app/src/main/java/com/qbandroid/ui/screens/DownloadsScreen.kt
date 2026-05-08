package com.qbandroid.ui.screens

import androidx.compose.animation.animateContentSize
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.filled.ArrowDownward
import androidx.compose.material.icons.filled.ArrowUpward
import androidx.compose.material.icons.filled.Delete
import androidx.compose.material.icons.filled.Info
import androidx.compose.material.icons.filled.Pause
import androidx.compose.material.icons.filled.PlayArrow
import androidx.compose.material.icons.filled.Refresh
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.CircularProgressIndicator
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.LinearProgressIndicator
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.style.TextOverflow
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.qbandroid.model.TorrentInfo
import com.qbandroid.model.TorrentState
import com.qbandroid.ui.theme.DownloadColor
import com.qbandroid.ui.theme.ErrorColor
import com.qbandroid.ui.theme.PauseColor
import com.qbandroid.ui.theme.SeedColor
import com.qbandroid.viewmodel.TorrentViewModel

@OptIn(ExperimentalMaterial3Api::class)
@Composable
fun DownloadsScreen(
    viewModel: TorrentViewModel = viewModel()
) {
    val torrents by viewModel.torrents.collectAsState()
    val stats by viewModel.stats.collectAsState()

    viewModel.initialize()

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
    ) {
        StatsCard(stats = stats)

        Spacer(modifier = Modifier.height(16.dp))

        Text(
            text = "Active Torrents (${torrents.size})",
            style = MaterialTheme.typography.titleMedium
        )

        Spacer(modifier = Modifier.height(8.dp))

        if (torrents.isEmpty()) {
            Box(
                modifier = Modifier.fillMaxSize(),
                contentAlignment = Alignment.Center
            ) {
                Text(
                    text = "No torrents\nUse the Add tab to add torrents",
                    style = MaterialTheme.typography.bodyLarge,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        } else {
            LazyColumn(
                verticalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                items(torrents, key = { it.hash }) { torrent ->
                    TorrentItem(
                        torrent = torrent,
                        onPause = { viewModel.pauseTorrent(torrent.hash) },
                        onResume = { viewModel.resumeTorrent(torrent.hash) },
                        onDelete = { viewModel.removeTorrent(torrent.hash) },
                        onRecheck = { viewModel.forceRecheck(torrent.hash) },
                        onClick = { viewModel.selectTorrent(torrent) }
                    )
                }
            }
        }
    }
}

@Composable
fun StatsCard(stats: com.qbandroid.model.GlobalStats) {
    Card(
        modifier = Modifier.fillMaxWidth(),
        colors = CardDefaults.cardColors(
            containerColor = MaterialTheme.colorScheme.primaryContainer
        )
    ) {
        Row(
            modifier = Modifier
                .fillMaxWidth()
                .padding(16.dp),
            horizontalArrangement = Arrangement.SpaceEvenly
        ) {
            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    text = "↓ ${formatSpeed(stats.downloadRate)}",
                    style = MaterialTheme.typography.titleMedium,
                    color = DownloadColor
                )
                Text(
                    text = "Download",
                    style = MaterialTheme.typography.bodySmall
                )
            }

            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    text = "↑ ${formatSpeed(stats.uploadRate)}",
                    style = MaterialTheme.typography.titleMedium,
                    color = SeedColor
                )
                Text(
                    text = "Upload",
                    style = MaterialTheme.typography.bodySmall
                )
            }

            Column(horizontalAlignment = Alignment.CenterHorizontally) {
                Text(
                    text = "${stats.numPeers}",
                    style = MaterialTheme.typography.titleMedium
                )
                Text(
                    text = "Peers",
                    style = MaterialTheme.typography.bodySmall
                )
            }
        }
    }
}

@Composable
fun TorrentItem(
    torrent: TorrentInfo,
    onPause: () -> Unit,
    onResume: () -> Unit,
    onDelete: () -> Unit,
    onRecheck: () -> Unit,
    onClick: () -> Unit
) {
    val stateColor = when (torrent.state) {
        TorrentState.DOWNLOADING -> DownloadColor
        TorrentState.SEEDING -> SeedColor
        TorrentState.PAUSED -> PauseColor
        TorrentState.CHECKING -> MaterialTheme.colorScheme.primary
        TorrentState.ERROR -> ErrorColor
        else -> MaterialTheme.colorScheme.onSurface
    }

    Card(
        modifier = Modifier
            .fillMaxWidth()
            .animateContentSize()
            .clickable(onClick = onClick),
        elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
    ) {
        Column(
            modifier = Modifier.padding(12.dp)
        ) {
            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Text(
                    text = torrent.name,
                    style = MaterialTheme.typography.bodyLarge,
                    maxLines = 1,
                    overflow = TextOverflow.Ellipsis,
                    modifier = Modifier.weight(1f)
                )

                Text(
                    text = torrent.state.displayName,
                    style = MaterialTheme.typography.labelSmall,
                    color = stateColor
                )
            }

            Spacer(modifier = Modifier.height(8.dp))

            LinearProgressIndicator(
                progress = { (torrent.progress).toFloat() },
                modifier = Modifier.fillMaxWidth(),
                color = stateColor,
                trackColor = MaterialTheme.colorScheme.surfaceVariant
            )

            Spacer(modifier = Modifier.height(8.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Column {
                    Text(
                        text = "Size: ${formatSize(torrent.size)}",
                        style = MaterialTheme.typography.bodySmall
                    )
                    Text(
                        text = "Progress: ${torrent.getProgressPercent()}%",
                        style = MaterialTheme.typography.bodySmall
                    )
                }

                Column(horizontalAlignment = Alignment.End) {
                    Text(
                        text = "↓ ${formatSpeed(torrent.downloadSpeed)}",
                        style = MaterialTheme.typography.bodySmall,
                        color = DownloadColor
                    )
                    Text(
                        text = "↑ ${formatSpeed(torrent.uploadSpeed)}",
                        style = MaterialTheme.typography.bodySmall,
                        color = SeedColor
                    )
                }
            }

            Spacer(modifier = Modifier.height(8.dp))

            Row(
                modifier = Modifier.fillMaxWidth(),
                horizontalArrangement = Arrangement.End
            ) {
                IconButton(onClick = onClick, modifier = Modifier.size(32.dp)) {
                    Icon(Icons.Default.Info, contentDescription = "Details", modifier = Modifier.size(20.dp))
                }

                if (torrent.state == TorrentState.DOWNLOADING || torrent.state == TorrentState.SEEDING) {
                    IconButton(onClick = onPause, modifier = Modifier.size(32.dp)) {
                        Icon(Icons.Default.Pause, contentDescription = "Pause", modifier = Modifier.size(20.dp))
                    }
                } else if (torrent.state == TorrentState.PAUSED) {
                    IconButton(onClick = onResume, modifier = Modifier.size(32.dp)) {
                        Icon(Icons.Default.PlayArrow, contentDescription = "Resume", modifier = Modifier.size(20.dp))
                    }
                }

                IconButton(onClick = onRecheck, modifier = Modifier.size(32.dp)) {
                    Icon(Icons.Default.Refresh, contentDescription = "Recheck", modifier = Modifier.size(20.dp))
                }

                IconButton(onClick = onDelete, modifier = Modifier.size(32.dp)) {
                    Icon(Icons.Default.Delete, contentDescription = "Delete", tint = ErrorColor, modifier = Modifier.size(20.dp))
                }
            }
        }
    }
}

fun formatSpeed(bytes: Long): String {
    return when {
        bytes < 1024 -> "${bytes}B/s"
        bytes < 1024 * 1024 -> "${bytes / 1024}KB/s"
        bytes < 1024 * 1024 * 1024 -> "${bytes / (1024 * 1024)}MB/s"
        else -> "${bytes / (1024 * 1024 * 1024)}GB/s"
    }
}

fun formatSize(bytes: Long): String {
    return when {
        bytes < 1024 -> "${bytes}B"
        bytes < 1024 * 1024 -> "${bytes / 1024}KB"
        bytes < 1024 * 1024 * 1024 -> "${bytes / (1024 * 1024)}MB"
        else -> "${bytes / (1024 * 1024 * 1024)}GB"
    }
}