package com.qbandroid.ui.screens

import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.text.KeyboardOptions
import androidx.compose.foundation.verticalScroll
import androidx.compose.material3.Button
import androidx.compose.material3.Card
import androidx.compose.material3.CardDefaults
import androidx.compose.material3.HorizontalDivider
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.OutlinedTextField
import androidx.compose.material3.Switch
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.collectAsState
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.text.input.KeyboardType
import androidx.compose.ui.unit.dp
import androidx.lifecycle.viewmodel.compose.viewModel
import com.qbandroid.viewmodel.TorrentViewModel

@Composable
fun SettingsScreen(
    viewModel: TorrentViewModel = viewModel()
) {
    val settings by viewModel.settings.collectAsState()

    var downloadLimitText by remember(settings.downloadLimit) { mutableStateOf(if (settings.downloadLimit > 0) settings.downloadLimit.toString() else "") }
    var uploadLimitText by remember(settings.uploadLimit) { mutableStateOf(if (settings.uploadLimit > 0) settings.uploadLimit.toString() else "") }
    var maxConnectionsText by remember(settings.maxConnections) { mutableStateOf(settings.maxConnections.toString()) }
    var maxPeersText by remember(settings.maxPeers) { mutableStateOf(settings.maxPeers.toString()) }
    var listenPortText by remember(settings.listenPort) { mutableStateOf(settings.listenPort.toString()) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .padding(16.dp)
            .verticalScroll(rememberScrollState())
    ) {
        Text(
            text = "Settings",
            style = MaterialTheme.typography.headlineMedium
        )

        Spacer(modifier = Modifier.height(16.dp))

        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Speed Limits",
                    style = MaterialTheme.typography.titleMedium
                )

                Spacer(modifier = Modifier.height(12.dp))

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    OutlinedTextField(
                        value = downloadLimitText,
                        onValueChange = {
                            downloadLimitText = it
                            it.toIntOrNull()?.let { value -> viewModel.setDownloadLimit(value) }
                        },
                        label = { Text("DL (KB/s)") },
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                        modifier = Modifier.weight(1f),
                        singleLine = true
                    )

                    OutlinedTextField(
                        value = uploadLimitText,
                        onValueChange = {
                            uploadLimitText = it
                            it.toIntOrNull()?.let { value -> viewModel.setUploadLimit(value) }
                        },
                        label = { Text("UL (KB/s)") },
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                        modifier = Modifier.weight(1f),
                        singleLine = true
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Connection Limits",
                    style = MaterialTheme.typography.titleMedium
                )

                Spacer(modifier = Modifier.height(12.dp))

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.spacedBy(8.dp)
                ) {
                    OutlinedTextField(
                        value = maxConnectionsText,
                        onValueChange = {
                            maxConnectionsText = it
                            it.toIntOrNull()?.let { value -> viewModel.setMaxConnections(value) }
                        },
                        label = { Text("Connections") },
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                        modifier = Modifier.weight(1f),
                        singleLine = true
                    )

                    OutlinedTextField(
                        value = maxPeersText,
                        onValueChange = {
                            maxPeersText = it
                            it.toIntOrNull()?.let { value -> viewModel.setMaxPeers(value) }
                        },
                        label = { Text("Max Peers") },
                        keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                        modifier = Modifier.weight(1f),
                        singleLine = true
                    )
                }

                Spacer(modifier = Modifier.height(8.dp))

                OutlinedTextField(
                    value = listenPortText,
                    onValueChange = {
                        listenPortText = it
                        it.toIntOrNull()?.let { value -> viewModel.setListenPort(value) }
                    },
                    label = { Text("Listen Port") },
                    keyboardOptions = KeyboardOptions(keyboardType = KeyboardType.Number),
                    modifier = Modifier.fillMaxWidth(),
                    singleLine = true
                )
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "Network Options",
                    style = MaterialTheme.typography.titleMedium
                )

                Spacer(modifier = Modifier.height(8.dp))

                SwitchSetting(
                    title = "DHT",
                    description = "Distributed Hash Table (disable for PT)",
                    checked = settings.dhtEnabled,
                    onCheckedChange = { viewModel.setDHTEnabled(it) }
                )

                HorizontalDivider()

                SwitchSetting(
                    title = "PEX",
                    description = "Peer Exchange (disable for PT)",
                    checked = settings.ptMode,
                    onCheckedChange = { viewModel.setPTMode(it) }
                )

                HorizontalDivider()

                SwitchSetting(
                    title = "LSD",
                    description = "Local Service Discovery",
                    checked = settings.lsdEnabled,
                    onCheckedChange = { viewModel.setLSDEnabled(it) }
                )
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Card(
            modifier = Modifier.fillMaxWidth(),
            elevation = CardDefaults.cardElevation(defaultElevation = 2.dp)
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "PT Mode",
                    style = MaterialTheme.typography.titleMedium
                )

                Spacer(modifier = Modifier.height(8.dp))

                Text(
                    text = "PT Mode automatically disables DHT, PEX, and LSD for private trackers. Use this when downloading from PT sites.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )

                Spacer(modifier = Modifier.height(8.dp))

                Row(
                    modifier = Modifier.fillMaxWidth(),
                    horizontalArrangement = Arrangement.SpaceBetween,
                    verticalAlignment = Alignment.CenterVertically
                ) {
                    Text(
                        text = "Enable PT Mode",
                        style = MaterialTheme.typography.bodyLarge
                    )
                    Switch(
                        checked = settings.ptMode,
                        onCheckedChange = { viewModel.setPTMode(it) }
                    )
                }
            }
        }

        Spacer(modifier = Modifier.height(16.dp))

        Card(
            modifier = Modifier.fillMaxWidth(),
            colors = CardDefaults.cardColors(
                containerColor = MaterialTheme.colorScheme.surfaceVariant
            )
        ) {
            Column(
                modifier = Modifier.padding(16.dp)
            ) {
                Text(
                    text = "About",
                    style = MaterialTheme.typography.titleMedium
                )

                Spacer(modifier = Modifier.height(8.dp))

                Text(
                    text = "qBittorrent Android v${viewModel.getVersion()}",
                    style = MaterialTheme.typography.bodyMedium
                )

                Text(
                    text = "libtorrent ${viewModel.getLibtorrentVersion()}",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant
                )
            }
        }

        Spacer(modifier = Modifier.height(32.dp))
    }
}

@Composable
fun SwitchSetting(
    title: String,
    description: String,
    checked: Boolean,
    onCheckedChange: (Boolean) -> Unit
) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .padding(vertical = 8.dp),
        horizontalArrangement = Arrangement.SpaceBetween,
        verticalAlignment = Alignment.CenterVertically
    ) {
        Column(modifier = Modifier.weight(1f)) {
            Text(
                text = title,
                style = MaterialTheme.typography.bodyLarge
            )
            Text(
                text = description,
                style = MaterialTheme.typography.bodySmall,
                color = MaterialTheme.colorScheme.onSurfaceVariant
            )
        }
        Switch(
            checked = checked,
            onCheckedChange = onCheckedChange
        )
    }
}