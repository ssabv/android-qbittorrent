#!/bin/bash

# ==========================================
# qBittorrent Android - 真机调试脚本
# ==========================================

echo "========================================"
echo "qBittorrent Android 调试工具"
echo "========================================"

# 检查 ADB
if ! command -v adb &> /dev/null; then
    echo "错误: ADB 未安装"
    echo "安装: sudo apt-get install adb"
    exit 1
fi

# 检查设备连接
DEVICE_COUNT=$(adb devices | grep -c "device$")
if [ "$DEVICE_COUNT" -eq 0 ]; then
    echo "错误: 未检测到 Android 设备"
    echo "请确保:"
    echo "  1. 手机已开启 USB 调试"
    echo "  2. 已授权此电脑调试"
    echo "  3. USB 连接正常"
    exit 1
fi

echo "已连接设备: $(adb devices | grep 'device$' | wc -l)"

# 解析命令
case "${1:-help}" in
    install)
        echo "========================================"
        echo "安装 APK"
        echo "========================================"
        APK_PATH="${2:-app/build/outputs/apk/debug/app-debug.apk}"
        if [ -f "$APK_PATH" ]; then
            adb install -r "$APK_PATH"
            echo "安装完成"
        else
            echo "错误: APK 文件不存在: $APK_PATH"
        fi
        ;;

    logcat)
        echo "========================================"
        echo "查看 qBittorrent 日志"
        echo "========================================"
        echo "按 Ctrl+C 退出"
        echo ""
        adb logcat -v time -s qBittorrent:V qBittorrentCore:V *:S
        ;;

    logcat-full)
        echo "========================================"
        echo "查看完整日志"
        echo "========================================"
        echo "按 Ctrl+C 退出"
        echo ""
        adb logcat -v time | grep -i "qbittorrent\|libtorrent"
        ;;

    logcat-native)
        echo "========================================"
        echo "查看 Native 层日志"
        echo "========================================"
        adb logcat -v time | grep -i "native\|jni\|libtorrent"
        ;;

    crash)
        echo "========================================"
        echo "查看崩溃日志"
        echo "========================================"
        adb logcat -d | grep -i "fatal\|crash\|signal\|SIGSEGV\|SIGABRT"
        ;;

    tombstone)
        echo "========================================"
        echo "查看 Native 崩溃"
        echo "========================================"
        adb shell ls -la /data/tombstones/
        echo ""
        echo "查看最近崩溃: adb shell cat /data/tombstones/tombstone_00"
        ;;

    shell)
        echo "========================================"
        echo "进入设备 Shell"
        echo "========================================"
        adb shell
        ;;

    root-shell)
        echo "========================================"
        echo "进入 Root Shell"
        echo "========================================"
        adb shell su
        ;;

    backup-log)
        echo "========================================"
        echo "导出日志"
        echo "========================================"
        LOG_FILE="qbit_log_$(date +%Y%m%d_%H%M%S).txt"
        adb logcat -d > "$LOG_FILE"
        echo "日志已保存: $LOG_FILE"
        echo "包含 qBittorrent 的行: $(grep -c qbit "$LOG_FILE" 2>/dev/null || echo 0)"
        ;;

    force-stop)
        echo "========================================"
        echo "强制停止应用"
        echo "========================================"
        adb shell am force-stop com.qbandroid
        echo "已强制停止"
        ;;

    clear-data)
        echo "========================================"
        echo "清除应用数据"
        echo "========================================"
        adb shell pm clear com.qbandroid
        echo "数据已清除"
        ;;

    restart)
        echo "========================================"
        echo "重启应用"
        echo "========================================"
        adb shell am force-stop com.qbandroid
        sleep 1
        adb shell am start -n com.qbandroid/.MainActivity
        echo "应用已重启"
        ;;

    ps)
        echo "========================================"
        echo "查看运行进程"
        echo "========================================"
        adb shell ps -A | grep -i qbit
        ;;

    netstat)
        echo "========================================"
        echo "查看网络连接"
        echo "========================================"
        adb shell netstat -an | grep -E "6881|6882|6883"
        ;;

    info)
        echo "========================================"
        echo "应用信息"
        echo "========================================"
        echo "包名: com.qbandroid"
        echo ""
        adb shell dumpsys package com.qbandroid | grep -E "version|perm|act"
        ;;

    battery)
        echo "========================================"
        echo "电池统计"
        echo "========================================"
        adb shell dumpsys batterystats --reset
        echo "电池统计已重置，使用应用后查看"
        adb shell dumpsys batterystats | grep -A 20 "com.qbandroid"
        ;;

    perf)
        echo "========================================"
        echo "性能监控"
        echo "========================================"
        echo "CPU:"
        adb shell top -n 1 | grep qbit
        echo ""
        echo "内存:"
        adb shell dumpsys meminfo com.qbandroid | grep -E "TOTAL|RSS|heap"
        ;;

    open)
        echo "========================================"
        echo "启动应用"
        echo "========================================"
        adb shell am start -n com.qbandroid/.MainActivity
        ;;

    unistall)
        echo "========================================"
        echo "卸载应用"
        echo "========================================"
        adb uninstall com.qbandroid
        ;;

    *)
        echo "用法: $0 <command> [args]"
        echo ""
        echo "调试命令:"
        echo "  install [apk]     - 安装 APK"
        echo "  logcat            - 查看日志"
        echo "  logcat-full       - 查看完整日志"
        echo "  crash             - 查看崩溃日志"
        echo "  tombstone         - 查看 Native 崩溃"
        echo "  shell             - 进入 Shell"
        echo "  backup-log        - 导出日志"
        echo "  force-stop        - 强制停止"
        echo "  clear-data        - 清除数据"
        echo "  restart           - 重启应用"
        echo "  ps                - 查看进程"
        echo "  netstat           - 查看网络连接"
        echo "  info              - 应用信息"
        echo "  battery           - 电池统计"
        echo "  perf              - 性能监控"
        echo "  open              - 启动应用"
        echo "  uninstall         - 卸载应用"
        ;;
esac