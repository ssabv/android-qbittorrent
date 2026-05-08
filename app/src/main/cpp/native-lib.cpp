#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <cstring>
#include <android/log.h>

#include "qbittorrent/qbcore.h"

#define LOG_TAG "qBittorrent"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static std::mutex g_session_mutex;
static QBittorrentCore* g_core = nullptr;

extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_initialize(
        JNIEnv* env,
        jobject thiz,
        jstring sessionDir,
        jstring downloadPath) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* sessionDirStr = env->GetStringUTFChars(sessionDir, nullptr);
    const char* downloadPathStr = env->GetStringUTFChars(downloadPath, nullptr);

    bool result = false;
    try {
        g_core = new QBittorrentCore(std::string(sessionDirStr), std::string(downloadPathStr));
        result = g_core->initialize();
    } catch (const std::exception& e) {
        LOGE("Failed to initialize: %s", e.what());
        result = false;
    }

    env->ReleaseStringUTFChars(sessionDir, sessionDirStr);
    env->ReleaseStringUTFChars(downloadPath, downloadPathStr);

    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_qbandroid_core_NativeBridge_startSession(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (g_core) {
        g_core->start();
    }
}

JNIEXPORT void JNICALL
Java_com_qbandroid_core_NativeBridge_stopSession(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (g_core) {
        g_core->stop();
    }
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_isSessionRunning(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (g_core) {
        return g_core->isRunning() ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_addMagnet(
        JNIEnv* env,
        jobject thiz,
        jstring magnetUri) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* magnetStr = env->GetStringUTFChars(magnetUri, nullptr);
    std::string hash = "";

    if (g_core) {
        try {
            hash = g_core->addMagnet(std::string(magnetStr));
        } catch (const std::exception& e) {
            LOGE("Failed to add magnet: %s", e.what());
        }
    }

    env->ReleaseStringUTFChars(magnetUri, magnetStr);
    return env->NewStringUTF(hash.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_addTorrentFromData(
        JNIEnv* env,
        jobject thiz,
        jbyteArray data,
        jstring fileName) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* fileNameStr = env->GetStringUTFChars(fileName, nullptr);
    std::string hash = "";

    jsize len = env->GetArrayLength(data);
    jbyte* buffer = env->GetByteArrayElements(data, nullptr);

    if (g_core && buffer) {
        try {
            std::vector<char> torrentData(len);
            memcpy(torrentData.data(), buffer, len);
            hash = g_core->addTorrent(torrentData, std::string(fileNameStr));
        } catch (const std::exception& e) {
            LOGE("Failed to add torrent from data: %s", e.what());
        }
    }

    env->ReleaseByteArrayElements(data, buffer, JNI_ABORT);
    env->ReleaseStringUTFChars(fileName, fileNameStr);

    return env->NewStringUTF(hash.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_addTorrentFromFile(
        JNIEnv* env,
        jobject thiz,
        jstring filePath) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* filePathStr = env->GetStringUTFChars(filePath, nullptr);
    std::string hash = "";

    if (g_core) {
        try {
            hash = g_core->addTorrentFromFile(std::string(filePathStr));
        } catch (const std::exception& e) {
            LOGE("Failed to add torrent from file: %s", e.what());
        }
    }

    env->ReleaseStringUTFChars(filePath, filePathStr);
    return env->NewStringUTF(hash.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_removeTorrent(
        JNIEnv* env,
        jobject thiz,
        jstring hash,
        jboolean deleteFiles) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->removeTorrent(std::string(hashStr), deleteFiles == JNI_TRUE);
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_resumeTorrent(
        JNIEnv* env,
        jobject thiz,
        jstring hash) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->resumeTorrent(std::string(hashStr));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_pauseTorrent(
        JNIEnv* env,
        jobject thiz,
        jstring hash) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->pauseTorrent(std::string(hashStr));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_forceRecheck(
        JNIEnv* env,
        jobject thiz,
        jstring hash) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->forceRecheck(std::string(hashStr));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setUploadLimit(
        JNIEnv* env,
        jobject thiz,
        jstring hash,
        jint limit) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->setUploadLimit(std::string(hashStr), static_cast<int>(limit));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setDownloadLimit(
        JNIEnv* env,
        jobject thiz,
        jstring hash,
        jint limit) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->setDownloadLimit(std::string(hashStr), static_cast<int>(limit));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setGlobalUploadLimit(
        JNIEnv* env,
        jobject thiz,
        jint limit) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setGlobalUploadLimit(static_cast<int>(limit)) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setGlobalDownloadLimit(
        JNIEnv* env,
        jobject thiz,
        jint limit) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setGlobalDownloadLimit(static_cast<int>(limit)) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setMaxConnections(
        JNIEnv* env,
        jobject thiz,
        jint limit) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setMaxConnections(static_cast<int>(limit)) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setMaxPeers(
        JNIEnv* env,
        jobject thiz,
        jint limit) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setMaxPeers(static_cast<int>(limit)) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setListenPort(
        JNIEnv* env,
        jobject thiz,
        jint port) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setListenPort(static_cast<int>(port)) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setDHTEnabled(
        JNIEnv* env,
        jobject thiz,
        jboolean enabled) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setDHTEnabled(enabled == JNI_TRUE) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setPEXEnabled(
        JNIEnv* env,
        jobject thiz,
        jboolean enabled) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setPEXEnabled(enabled == JNI_TRUE) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setLSDEnabled(
        JNIEnv* env,
        jobject thiz,
        jboolean enabled) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setLSDEnabled(enabled == JNI_TRUE) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setEncryptionMode(
        JNIEnv* env,
        jobject thiz,
        jint mode) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setEncryptionMode(static_cast<int>(mode)) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setPTMode(
        JNIEnv* env,
        jobject thiz,
        jboolean enabled) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return g_core->setPTMode(enabled == JNI_TRUE) ? JNI_TRUE : JNI_FALSE;
    }
    return JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_qbandroid_core_NativeBridge_getTorrentCount(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    if (g_core) {
        return static_cast<jint>(g_core->getTorrentCount());
    }
    return 0;
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getTorrentListJson(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    std::string json = "[]";
    if (g_core) {
        json = g_core->getTorrentListJson();
    }
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getTorrentDetailsJson(
        JNIEnv* env,
        jobject thiz,
        jstring hash) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    std::string json = "{}";

    if (g_core) {
        json = g_core->getTorrentDetailsJson(std::string(hashStr));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getGlobalStatsJson(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    std::string json = "{}";
    if (g_core) {
        json = g_core->getGlobalStatsJson();
    }
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getSettingsJson(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    std::string json = "{}";
    if (g_core) {
        json = g_core->getSettingsJson();
    }
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setSettingsJson(
        JNIEnv* env,
        jobject thiz,
        jstring settingsJson) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* settingsStr = env->GetStringUTFChars(settingsJson, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->setSettingsJson(std::string(settingsStr));
    }

    env->ReleaseStringUTFChars(settingsJson, settingsStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getPeerListJson(
        JNIEnv* env,
        jobject thiz,
        jstring hash) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    std::string json = "[]";

    if (g_core) {
        json = g_core->getPeerListJson(std::string(hashStr));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getTrackerListJson(
        JNIEnv* env,
        jobject thiz,
        jstring hash) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    std::string json = "[]";

    if (g_core) {
        json = g_core->getTrackerListJson(std::string(hashStr));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getFilesListJson(
        JNIEnv* env,
        jobject thiz,
        jstring hash) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    std::string json = "[]";

    if (g_core) {
        json = g_core->getFilesListJson(std::string(hashStr));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_setFilePriority(
        JNIEnv* env,
        jobject thiz,
        jstring hash,
        jint index,
        jint priority) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = env->GetStringUTFChars(hash, nullptr);
    bool result = false;

    if (g_core) {
        result = g_core->setFilePriority(std::string(hashStr), static_cast<int>(index), static_cast<int>(priority));
    }

    env->ReleaseStringUTFChars(hash, hashStr);
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getTransferInfoJson(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    std::string json = "{}";
    if (g_core) {
        json = g_core->getTransferInfoJson();
    }
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getAllTrackerInfoJson(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    std::string json = "[]";
    if (g_core) {
        json = g_core->getAllTrackerInfoJson();
    }
    return env->NewStringUTF(json.c_str());
}

JNIEXPORT void JNICALL
Java_com_qbandroid_core_NativeBridge_saveFastResume(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (g_core) {
        g_core->saveFastResume();
    }
}

JNIEXPORT void JNICALL
Java_com_qbandroid_core_NativeBridge_loadFastResume(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (g_core) {
        g_core->loadFastResume();
    }
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getVersion(
        JNIEnv* env,
        jobject thiz) {

    return env->NewStringUTF("1.0.0");
}

JNIEXPORT jstring JNICALL
Java_com_qbandroid_core_NativeBridge_getLibtorrentVersion(
        JNIEnv* env,
        jobject thiz) {

    if (g_core) {
        return env->NewStringUTF(g_core->getLibtorrentVersion().c_str());
    }
    return env->NewStringUTF("unknown");
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_addTracker(
        JNIEnv* env,
        jobject thiz,
        jstring hash,
        jstring url) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = (hash != nullptr) ? env->GetStringUTFChars(hash, nullptr) : nullptr;
    const char* urlStr = (url != nullptr) ? env->GetStringUTFChars(url, nullptr) : nullptr;

    bool result = false;

    if (g_core && hashStr && urlStr) {
        result = g_core->addTracker(std::string(hashStr), std::string(urlStr));
    }

    if (hashStr) env->ReleaseStringUTFChars(hash, hashStr);
    if (urlStr) env->ReleaseStringUTFChars(url, urlStr);

    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_editTracker(
        JNIEnv* env,
        jobject thiz,
        jstring hash,
        jstring oldUrl,
        jstring newUrl) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = (hash != nullptr) ? env->GetStringUTFChars(hash, nullptr) : nullptr;
    const char* oldUrlStr = (oldUrl != nullptr) ? env->GetStringUTFChars(oldUrl, nullptr) : nullptr;
    const char* newUrlStr = (newUrl != nullptr) ? env->GetStringUTFChars(newUrl, nullptr) : nullptr;

    bool result = false;

    if (g_core && hashStr && oldUrlStr && newUrlStr) {
        result = g_core->editTracker(std::string(hashStr), std::string(oldUrlStr), std::string(newUrlStr));
    }

    if (hashStr) env->ReleaseStringUTFChars(hash, hashStr);
    if (oldUrlStr) env->ReleaseStringUTFChars(oldUrl, oldUrlStr);
    if (newUrlStr) env->ReleaseStringUTFChars(newUrl, newUrlStr);

    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_qbandroid_core_NativeBridge_removeTracker(
        JNIEnv* env,
        jobject thiz,
        jstring hash,
        jstring url) {

    std::lock_guard<std::mutex> lock(g_session_mutex);

    const char* hashStr = (hash != nullptr) ? env->GetStringUTFChars(hash, nullptr) : nullptr;
    const char* urlStr = (url != nullptr) ? env->GetStringUTFChars(url, nullptr) : nullptr;

    bool result = false;

    if (g_core && hashStr && urlStr) {
        result = g_core->removeTracker(std::string(hashStr), std::string(urlStr));
    }

    if (hashStr) env->ReleaseStringUTFChars(hash, hashStr);
    if (urlStr) env->ReleaseStringUTFChars(url, urlStr);

    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_qbandroid_core_NativeBridge_startAlertHandler(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (g_core) {
        g_core->startAlertHandler();
    }
}

JNIEXPORT void JNICALL
Java_com_qbandroid_core_NativeBridge_stopAlertHandler(
        JNIEnv* env,
        jobject thiz) {

    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (g_core) {
        g_core->stopAlertHandler();
    }
}

}