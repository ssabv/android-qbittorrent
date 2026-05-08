package com.qbandroid.service

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.os.Build
import com.qbandroid.core.NativeBridge

class BootReceiver : BroadcastReceiver() {

    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action == Intent.ACTION_BOOT_COMPLETED ||
            intent.action == "android.intent.action.QUICKBOOT_POWERON") {

            val prefs = context.getSharedPreferences("qbt_settings", Context.MODE_PRIVATE)
            val autoStart = prefs.getBoolean("auto_start", true)
            val startOnBoot = prefs.getBoolean("start_on_boot", false)

            if (autoStart && startOnBoot) {
                val serviceIntent = Intent(context, TorrentService::class.java)

                if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                    context.startForegroundService(serviceIntent)
                } else {
                    context.startService(serviceIntent)
                }
            }
        }
    }
}