#include "qbcore.h"
#include "cJSON.h"

#include <libtorrent/session.hpp>
#include <libtorrent/settings.hpp>
#include <libtorrent/add_torrent_params.hpp>
#include <libtorrent/torrent_handle.hpp>
#include <libtorrent/torrent_status.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/magnet_uri.hpp>
#include <libtorrent/peer_info.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/alert_types.hpp>
#include <libtorrent/kademlia/dht_settings.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/bencode.hpp>
#include <libtorrent/lazy_entry.hpp>
#include <libtorrent/kademlia/dht_storage.hpp>

#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <algorithm>
#include <cstring>
#include <cerrno>

#if defined(__ANDROID__)
#include <android/log.h>
#define LOG_TAG "qBittorrentCore"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) printf(__VA_ARGS__)
#define LOGD(...) printf(__VA_ARGS__)
#endif

namespace lt = libtorrent;

struct QBittorrentCore::Impl {
    lt::session* session = nullptr;
    std::string sessionDir;
    std::string downloadPath;
    std::atomic<bool> isRunning{false};
    std::atomic<bool> isInitialized{false};
    std::atomic<bool> alertThreadRunning{false};
    std::thread* alertThread = nullptr;
    mutable std::shared_mutex torrentMutex;
    bool ptMode = false;

    std::string getTorrentStateString(lt::torrent_status::state_t state) {
        switch (state) {
            case lt::torrent_status::checking_files: return "checking";
            case lt::torrent_status::downloading_metadata: return "meta";
            case lt::torrent_status::downloading: return "downloading";
            case lt::torrent_status::finished: return "seeding";
            case lt::torrent_status::seeding: return "seeding";
            case lt::torrent_status::stopped: return "stopped";
            case lt::torrent_status::queued: return "queued";
            default: return "unknown";
        }
    }

    bool setupSession() {
        try {
            lt::settings_pack pack;

            pack.set_str(lt::settings_pack::intf, "0.0.0.0");
            pack.set_int(lt::settings_pack::listen_port, 6881);
            pack.set_int(lt::settings_pack::outgoing_port, 0);
            pack.set_int(lt::settings_pack::active_limit, 50);
            pack.set_int(lt::settings_pack::connections_limit, 200);
            pack.set_int(lt::settings_pack::max_peer_list_size, 1000);
            pack.set_int(lt::settings_pack::max_rejects, 50);
            pack.set_int(lt::settings_pack::recv_buffer_size, 128 * 1024);
            pack.set_int(lt::settings_pack::send_buffer_size, 128 * 1024);

            pack.set_int(lt::settings_pack::download_rate_limit, 0);
            pack.set_int(lt::settings_pack::upload_rate_limit, 0);

            pack.set_int(lt::settings_pack::dht_upload_rate_limit, 4000);
            pack.set_int(lt::settings_pack::dht_announce_interval, 60);
            pack.set_int(lt::settings_pack::dht_max_peers, 100);

            pack.set_bool(lt::settings_pack::enable_dht, true);
            pack.set_bool(lt::settings_pack::enable_lsd, true);
            pack.set_bool(lt::settings_pack::enable_upnp, true);
            pack.set_bool(lt::settings_pack::enable_natpmp, true);
            pack.set_bool(lt::settings_pack::enable_pex, true);

            pack.set_int(lt::settings_pack::enc_policy, lt::enc_policy::enabled);
            pack.set_int(lt::settings_pack::enc_level, lt::enc_level::plaintext);

            pack.set_int(lt::settings_pack::max_failcount, 3);
            pack.set_int(lt::settings_pack::min_reconnect_time, 60);
            pack.set_int(lt::settings_pack::peer_turnover, 4);
            pack.set_int(lt::settings_pack::connection_speed, 10);

            pack.set_bool(lt::settings_pack::anonymous_mode, false);
            pack.set_str(lt::settings_pack::user_agent, "qBittorrent/4.6.2");

            // 设置 qBittorrent 风格的 Peer ID
            // 格式: -qB4v2.0.9-
            // Android 版本: -qB4A1.0.0-
            // 使用固定的 peer_id 前缀保持 PT 兼容性
            lt::peer_id pid;
            memset(pid.data(), 0, 20);
            // qBittorrent v4 style: -qB4A-
            pid[0] = '-';
            pid[1] = 'q';
            pid[2] = 'B';
            pid[3] = '4';
            pid[4] = 'A';
            pid[5] = '1';
            pid[6] = '.';
            pid[7] = '0';
            pid[8] = '.';
            pid[9] = '0';
            pid[10] = '-';
            // 剩余字节用随机值或版本号填充
            for (int i = 11; i < 20; i++) {
                pid[i] = static_cast<char>(0x30 + (i - 11));
            }
            pack.set_str(lt::settings_pack::peer_fingerprint, "qBittorrent/4.6.2");

            pack.set_int(lt::settings_pack::tracker_completion_timeout, 60);
            pack.set_int(lt::settings_pack::tracker_receive_timeout, 10);
            pack.set_int(lt::settings_pack::announce_timeout, 60);

            pack.set_bool(lt::settings_pack::prioritize_partial_pieces, true);
            pack.set_int(lt::settings_pack::partial_file_layout, 1);

            lt::proxy_settings proxy;
            proxy.type = lt::proxy_settings::none;
            pack.set_proxy_settings(proxy);

            session = new lt::session(pack);

            if (session) {
                lt::dht_settings dhtSettings;
                dhtSettings.max_peers = 100;
                dhtSettings.max_torrents = 100;
                dhtSettings.max_dht_items = 100;
                session->set_dht_settings(dhtSettings);

                isInitialized = true;
                LOGI("Session initialized successfully");
                return true;
            }
        } catch (const std::exception& e) {
            LOGE("Failed to setup session: %s", e.what());
        }
        return false;
    }

    std::string torrentToJson(lt::torrent_handle handle) {
        try {
            auto status = handle.status();

            cJSON* root = cJSON_CreateObject();

            cJSON_AddStringToObject(root, "hash", status.info_hash.to_string().c_str());
            cJSON_AddStringToObject(root, "name", status.name.c_str());
            cJSON_AddNumberToObject(root, "size", static_cast<double>(status.total_wanted));
            cJSON_AddNumberToObject(root, "progress", status.progress);
            cJSON_AddStringToObject(root, "state", getTorrentStateString(status.state).c_str());
            cJSON_AddNumberToObject(root, "download_speed", static_cast<double>(status.download_rate));
            cJSON_AddNumberToObject(root, "upload_speed", static_cast<double>(status.upload_rate));
            cJSON_AddNumberToObject(root, "download_payload_rate", static_cast<double>(status.download_payload_rate));
            cJSON_AddNumberToObject(root, "upload_payload_rate", static_cast<double>(status.upload_payload_rate));
            cJSON_AddNumberToObject(root, "num_seeds", status.num_seeds);
            cJSON_AddNumberToObject(root, "num_leechers", status.num_leechers);
            cJSON_AddNumberToObject(root, "seeds_connected", status.seeds_connected);
            cJSON_AddNumberToObject(root, "peers_connected", status.peers_connected);
            cJSON_AddNumberToObject(root, "uploaded", static_cast<double>(status.total_upload));
            cJSON_AddNumberToObject(root, "downloaded", static_cast<double>(status.total_download));
            cJSON_AddNumberToObject(root, "ratio", status.share_ratio);
            cJSON_AddNumberToObject(root, "active_time", status.active_time);
            cJSON_AddNumberToObject(root, "seed_time", status.seed_time);
            cJSON_AddNumberToObject(root, "download_time", status.downloading_time);
            cJSON_AddStringToObject(root, "save_path", status.save_path.c_str());

            auto ti = handle.torrent_file();
            if (ti) {
                cJSON_AddNumberToObject(root, "piece_count", ti->num_pieces());
                cJSON_AddNumberToObject(root, "piece_size", ti->piece_length());
                cJSON_AddNumberToObject(root, "num_files", ti->num_files());
                cJSON_AddNumberToObject(root, "total_size", static_cast<double>(ti->total_size()));
            }

            cJSON_AddBoolToObject(root, "is_seeding", status.is_seeding);
            cJSON_AddBoolToObject(root, "is_finished", status.is_finished);
            cJSON_AddBoolToObject(root, "has_metadata", handle.has_metadata());
            cJSON_AddNumberToObject(root, "num_pieces", status.num_pieces);
            cJSON_AddNumberToObject(root, "num_verified_pieces", status.num_verified_pieces);

            auto files = handle.file_progress();
            if (ti && files.size() > 0) {
                cJSON* fileArray = cJSON_CreateArray();
                for (int i = 0; i < ti->num_files() && i < 10; i++) {
                    cJSON* fileObj = cJSON_CreateObject();
                    cJSON_AddNumberToObject(fileObj, "index", i);
                    cJSON_AddStringToObject(fileObj, "name", ti->files().file_name(i).c_str());
                    cJSON_AddNumberToObject(fileObj, "size", static_cast<double>(ti->files().file_size(i)));
                    cJSON_AddNumberToObject(fileObj, "progress", ti->files().file_size(i) > 0 ? static_cast<double>(files[i]) / ti->files().file_size(i) : 0);
                    cJSON_AddItemToArray(fileArray, fileObj);
                }
                cJSON_AddItemToObject(root, "files", fileArray);
            }

            char* json = cJSON_PrintUnformatted(root);
            std::string result(json);
            cJSON_Delete(root);
            free(json);
            return result;
        } catch (const std::exception& e) {
            LOGE("Error converting torrent to JSON: %s", e.what());
            return "{}";
        }
    }
};

QBittorrentCore::QBittorrentCore(const std::string& sessionDir, const std::string& downloadPath)
    : m_impl(std::make_unique<Impl>())
    , m_running(false) {

    m_impl->sessionDir = sessionDir;
    m_impl->downloadPath = downloadPath;
}

QBittorrentCore::~QBittorrentCore() {
    stop();
    if (m_impl->session) {
        delete m_impl->session;
        m_impl->session = nullptr;
    }
}

bool QBittorrentCore::initialize() {
    if (!m_impl->setupSession()) {
        LOGE("Failed to setup session");
        return false;
    }

    std::string fastResumePath = m_impl->sessionDir + "/fastresume.data";
    std::ifstream in(fastResumePath, std::ios::binary);
    if (in.good()) {
        try {
            lt::entry entry;
            lt::lazy_bdecode(std::istream_iterator<char>(in), std::istream_iterator<char>(), entry);
            m_impl->session->load_state(entry);
            LOGI("Loaded fast resume data");
        } catch (const std::exception& e) {
            LOGE("Failed to load fast resume: %s", e.what());
        }
    }

    return true;
}

void QBittorrentCore::start() {
    if (m_impl->session && m_impl->isInitialized && !m_running) {
        m_impl->isRunning = true;
        m_running = true;
        LOGI("Session started");
    }
}

void QBittorrentCore::stop() {
    if (m_impl->session && m_running) {
        saveFastResume();
        m_impl->isRunning = false;
        m_running = false;
        LOGI("Session stopped");
    }
}

bool QBittorrentCore::isRunning() const {
    return m_running && m_impl->isRunning;
}

std::string QBittorrentCore::addMagnet(const std::string& magnetUri) {
    if (!m_impl->session || !m_impl->isInitialized) {
        LOGE("Session not initialized");
        return "";
    }

    try {
        lt::add_torrent_params params;
        lt::error_code ec;

        params = lt::parse_magnet_uri(magnetUri, ec);
        if (ec) {
            LOGE("Failed to parse magnet URI: %s", ec.message().c_str());
            return "";
        }

        params.save_path = m_impl->downloadPath;
        params.flags |= lt::torrent_flags::pause_on_accelerated;
        params.flags &= ~lt::torrent_flags::auto_managed;

        lt::torrent_handle handle = m_impl->session->add_torrent(params);

        std::string hash = handle.info_hash().to_string();
        LOGI("Added magnet: %s", hash.c_str());
        return hash;

    } catch (const std::exception& e) {
        LOGE("Failed to add magnet: %s", e.what());
        return "";
    }
}

std::string QBittorrentCore::addTorrent(const std::vector<char>& data, const std::string& fileName) {
    if (!m_impl->session || !m_impl->isInitialized) {
        LOGE("Session not initialized");
        return "";
    }

    try {
        lt::error_code ec;
        lt::torrent_info t(data, ec);

        if (ec) {
            LOGE("Failed to parse torrent: %s", ec.message().c_str());
            return "";
        }

        lt::add_torrent_params params;
        params.ti = std::make_shared<lt::torrent_info>(t);
        params.save_path = m_impl->downloadPath;
        params.flags &= ~lt::torrent_flags::auto_managed;

        lt::torrent_handle handle = m_impl->session->add_torrent(params, ec);

        if (ec) {
            LOGE("Failed to add torrent: %s", ec.message().c_str());
            return "";
        }

        std::string hash = handle.info_hash().to_string();
        LOGI("Added torrent: %s (%s)", hash.c_str(), fileName.c_str());
        return hash;

    } catch (const std::exception& e) {
        LOGE("Failed to add torrent: %s", e.what());
        return "";
    }
}

std::string QBittorrentCore::addTorrentFromFile(const std::string& filePath) {
    if (!m_impl->session || !m_impl->isInitialized) {
        LOGE("Session not initialized");
        return "";
    }

    try {
        lt::error_code ec;
        lt::torrent_info t(filePath, ec);

        if (ec) {
            LOGE("Failed to load torrent file: %s", ec.message().c_str());
            return "";
        }

        lt::add_torrent_params params;
        params.ti = std::make_shared<lt::torrent_info>(t);
        params.save_path = m_impl->downloadPath;
        params.flags &= ~lt::torrent_flags::auto_managed;

        lt::torrent_handle handle = m_impl->session->add_torrent(params, ec);

        if (ec) {
            LOGE("Failed to add torrent: %s", ec.message().c_str());
            return "";
        }

        std::string hash = handle.info_hash().to_string();
        LOGI("Added torrent from file: %s", hash.c_str());
        return hash;

    } catch (const std::exception& e) {
        LOGE("Failed to add torrent from file: %s", e.what());
        return "";
    }
}

bool QBittorrentCore::removeTorrent(const std::string& hashStr, bool deleteFiles) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));

        if (!handle.is_valid()) {
            LOGE("Torrent not found: %s", hashStr.c_str());
            return false;
        }

        lt::remove_flags flags = deleteFiles ? lt::delete_files : lt::delete_none;
        m_impl->session->remove_torrent(handle, flags);

        LOGI("Removed torrent: %s (delete_files=%d)", hashStr.c_str(), deleteFiles);
        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to remove torrent: %s", e.what());
        return false;
    }
}

bool QBittorrentCore::resumeTorrent(const std::string& hashStr) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));

        if (!handle.is_valid()) {
            return false;
        }

        handle.set_flags(lt::torrent_flags::none, lt::torrent_flags::paused);
        handle.set_upload_limit(0);
        handle.set_download_limit(0);

        LOGI("Resumed torrent: %s", hashStr.c_str());
        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to resume torrent: %s", e.what());
        return false;
    }
}

bool QBittorrentCore::pauseTorrent(const std::string& hashStr) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));

        if (!handle.is_valid()) {
            return false;
        }

        handle.set_flags(lt::torrent_flags::paused, lt::torrent_flags::paused);

        LOGI("Paused torrent: %s", hashStr.c_str());
        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to pause torrent: %s", e.what());
        return false;
    }
}

bool QBittorrentCore::forceRecheck(const std::string& hashStr) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));

        if (!handle.is_valid()) {
            return false;
        }

        handle.force_recheck();

        LOGI("Force recheck torrent: %s", hashStr.c_str());
        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to force recheck: %s", e.what());
        return false;
    }
}

bool QBittorrentCore::setUploadLimit(const std::string& hashStr, int limit) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));

        if (!handle.is_valid()) {
            return false;
        }

        handle.set_upload_limit(limit);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setDownloadLimit(const std::string& hashStr, int limit) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));

        if (!handle.is_valid()) {
            return false;
        }

        handle.set_download_limit(limit);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setGlobalUploadLimit(int limit) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::upload_rate_limit, limit);
        m_impl->session->apply_settings(pack);
        LOGI("Set global upload limit: %d KB/s", limit / 1024);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setGlobalDownloadLimit(int limit) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::download_rate_limit, limit);
        m_impl->session->apply_settings(pack);
        LOGI("Set global download limit: %d KB/s", limit / 1024);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setMaxConnections(int limit) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::connections_limit, limit);
        m_impl->session->apply_settings(pack);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setMaxPeers(int limit) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::max_peer_list_size, limit);
        m_impl->session->apply_settings(pack);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setListenPort(int port) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::listen_port, port);
        m_impl->session->apply_settings(pack);
        LOGI("Set listen port: %d", port);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setDHTEnabled(bool enabled) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_bool(lt::settings_pack::enable_dht, enabled);
        m_impl->session->apply_settings(pack);
        LOGI("DHT enabled: %d", enabled);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setPEXEnabled(bool enabled) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_bool(lt::settings_pack::enable_pex, enabled);
        m_impl->session->apply_settings(pack);
        LOGI("PEX enabled: %d", enabled);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setLSDEnabled(bool enabled) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_bool(lt::settings_pack::enable_lsd, enabled);
        m_impl->session->apply_settings(pack);
        LOGI("LSD enabled: %d", enabled);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setEncryptionMode(int mode) {
    if (!m_impl->session) return false;

    try {
        lt::settings_pack pack;
        pack.set_int(lt::settings_pack::enc_policy, static_cast<lt::enc_policy>(mode));
        m_impl->session->apply_settings(pack);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

bool QBittorrentCore::setPTMode(bool enabled) {
    if (!m_impl->session) return false;

    m_impl->ptMode = enabled;

    try {
        lt::settings_pack pack;

        pack.set_bool(lt::settings_pack::enable_dht, !enabled);
        pack.set_bool(lt::settings_pack::enable_lsd, !enabled);
        pack.set_bool(lt::settings_pack::enable_pex, !enabled);

        if (enabled) {
            pack.set_int(lt::settings_pack::connections_limit, 100);
            pack.set_int(lt::settings_pack::max_peer_list_size, 100);
            pack.set_int(lt::settings_pack::enc_policy, lt::enc_policy::forced);
        }

        m_impl->session->apply_settings(pack);
        LOGI("PT mode: %d", enabled);
        return true;

    } catch (const std::exception& e) {
        return false;
    }
}

size_t QBittorrentCore::getTorrentCount() const {
    if (!m_impl->session) return 0;
    return m_impl->session->get_torrents().size();
}

std::string QBittorrentCore::getTorrentListJson() const {
    if (!m_impl->session) return "[]";

    try {
        auto torrents = m_impl->session->get_torrents();
        cJSON* array = cJSON_CreateArray();

        for (const auto& handle : torrents) {
            if (!handle.is_valid()) continue;

            try {
                auto status = handle.status();
                cJSON* item = cJSON_CreateObject();

                cJSON_AddStringToObject(item, "hash", status.info_hash.to_string().c_str());
                cJSON_AddStringToObject(item, "name", status.name.c_str());
                cJSON_AddNumberToObject(item, "size", static_cast<double>(status.total_wanted));
                cJSON_AddNumberToObject(item, "progress", status.progress);
                cJSON_AddStringToObject(item, "state", m_impl->getTorrentStateString(status.state).c_str());
                cJSON_AddNumberToObject(item, "download_speed", static_cast<double>(status.download_rate));
                cJSON_AddNumberToObject(item, "upload_speed", static_cast<double>(status.upload_rate));
                cJSON_AddNumberToObject(item, "downloaded", static_cast<double>(status.total_download));
                cJSON_AddNumberToObject(item, "uploaded", static_cast<double>(status.total_upload));
                cJSON_AddNumberToObject(item, "ratio", status.share_ratio);
                cJSON_AddNumberToObject(item, "num_seeds", status.num_seeds);
                cJSON_AddNumberToObject(item, "num_leechers", status.num_leechers);

                cJSON_AddItemToArray(array, item);
            } catch (...) {
                continue;
            }
        }

        char* json = cJSON_PrintUnformatted(array);
        std::string result(json);
        cJSON_Delete(array);
        free(json);
        return result;

    } catch (const std::exception& e) {
        LOGE("Failed to get torrent list: %s", e.what());
        return "[]";
    }
}

std::string QBittorrentCore::getTorrentDetailsJson(const std::string& hashStr) const {
    if (!m_impl->session) return "{}";

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));

        if (!handle.is_valid()) {
            return "{}";
        }

        return m_impl->torrentToJson(handle);

    } catch (const std::exception& e) {
        LOGE("Failed to get torrent details: %s", e.what());
        return "{}";
    }
}

std::string QBittorrentCore::getGlobalStatsJson() const {
    if (!m_impl->session) return "{}";

    try {
        auto status = m_impl->session->status();
        cJSON* root = cJSON_CreateObject();

        cJSON_AddNumberToObject(root, "download_rate", static_cast<double>(status.download_rate));
        cJSON_AddNumberToObject(root, "upload_rate", static_cast<double>(status.upload_rate));
        cJSON_AddNumberToObject(root, "total_download", static_cast<double>(status.total_download));
        cJSON_AddNumberToObject(root, "total_upload", static_cast<double>(status.total_upload));
        cJSON_AddNumberToObject(root, "num_peers", status.num_peers);
        cJSON_AddNumberToObject(root, "num_seeds", status.num_seeds);
        cJSON_AddNumberToObject(root, "num_leechers", status.num_leechers);
        cJSON_AddNumberToObject(root, "dht_nodes", status.dht_nodes);
        cJSON_AddNumberToObject(root, "total_peers", status.total_peers);
        cJSON_AddNumberToObject(root, "torrent_count", status.torrent_count);
        cJSON_AddNumberToObject(root, "paused_torrent_count", status.paused_torrent_count);
        cJSON_AddNumberToObject(root, "error_torrent_count", status.error_torrent_count);

        char* json = cJSON_PrintUnformatted(root);
        std::string result(json);
        cJSON_Delete(root);
        free(json);
        return result;

    } catch (const std::exception& e) {
        LOGE("Failed to get global stats: %s", e.what());
        return "{}";
    }
}

std::string QBittorrentCore::getSettingsJson() const {
    if (!m_impl->session) return "{}";

    try {
        auto settings = m_impl->session->get_settings();
        cJSON* root = cJSON_CreateObject();

        cJSON_AddNumberToObject(root, "download_rate_limit", settings.get_int(lt::settings_pack::download_rate_limit));
        cJSON_AddNumberToObject(root, "upload_rate_limit", settings.get_int(lt::settings_pack::upload_rate_limit));
        cJSON_AddNumberToObject(root, "connections_limit", settings.get_int(lt::settings_pack::connections_limit));
        cJSON_AddNumberToObject(root, "max_peer_list_size", settings.get_int(lt::settings_pack::max_peer_list_size));
        cJSON_AddNumberToObject(root, "listen_port", settings.get_int(lt::settings_pack::listen_port));
        cJSON_AddBoolToObject(root, "enable_dht", settings.get_bool(lt::settings_pack::enable_dht));
        cJSON_AddBoolToObject(root, "enable_lsd", settings.get_bool(lt::settings_pack::enable_lsd));
        cJSON_AddBoolToObject(root, "enable_upnp", settings.get_bool(lt::settings_pack::enable_upnp));
        cJSON_AddBoolToObject(root, "enable_natpmp", settings.get_bool(lt::settings_pack::enable_natpmp));
        cJSON_AddNumberToObject(root, "enc_policy", settings.get_int(lt::settings_pack::enc_policy));

        cJSON_AddBoolToObject(root, "pt_mode", m_impl->ptMode);

        char* json = cJSON_PrintUnformatted(root);
        std::string result(json);
        cJSON_Delete(root);
        free(json);
        return result;

    } catch (const std::exception& e) {
        return "{}";
    }
}

bool QBittorrentCore::setSettingsJson(const std::string& settingsJson) {
    if (!m_impl->session) return false;

    cJSON* root = cJSON_Parse(settingsJson.c_str());
    if (!root) return false;

    try {
        lt::settings_pack pack;

        cJSON* item = cJSON_GetObjectItem(root, "download_rate_limit");
        if (item && item->type == cJSON_Number) {
            pack.set_int(lt::settings_pack::download_rate_limit, item->valueint);
        }

        item = cJSON_GetObjectItem(root, "upload_rate_limit");
        if (item && item->type == cJSON_Number) {
            pack.set_int(lt::settings_pack::upload_rate_limit, item->valueint);
        }

        item = cJSON_GetObjectItem(root, "connections_limit");
        if (item && item->type == cJSON_Number) {
            pack.set_int(lt::settings_pack::connections_limit, item->valueint);
        }

        item = cJSON_GetObjectItem(root, "max_peer_list_size");
        if (item && item->type == cJSON_Number) {
            pack.set_int(lt::settings_pack::max_peer_list_size, item->valueint);
        }

        item = cJSON_GetObjectItem(root, "listen_port");
        if (item && item->type == cJSON_Number) {
            pack.set_int(lt::settings_pack::listen_port, item->valueint);
        }

        item = cJSON_GetObjectItem(root, "enable_dht");
        if (item && (item->type == cJSON_True || item->type == cJSON_False)) {
            pack.set_bool(lt::settings_pack::enable_dht, item->type == cJSON_True);
        }

        item = cJSON_GetObjectItem(root, "enable_lsd");
        if (item && (item->type == cJSON_True || item->type == cJSON_False)) {
            pack.set_bool(lt::settings_pack::enable_lsd, item->type == cJSON_True);
        }

        item = cJSON_GetObjectItem(root, "pt_mode");
        if (item && (item->type == cJSON_True || item->type == cJSON_False)) {
            m_impl->ptMode = (item->type == cJSON_True);
            cJSON_Delete(root);
            return setPTMode(m_impl->ptMode);
        }

        cJSON_Delete(root);
        m_impl->session->apply_settings(pack);
        return true;

    } catch (const std::exception& e) {
        cJSON_Delete(root);
        return false;
    }
}

std::string QBittorrentCore::getPeerListJson(const std::string& hashStr) const {
    if (!m_impl->session) return "[]";

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));
        if (!handle.is_valid()) return "[]";

        auto peers = handle.get_peer_info();
        cJSON* array = cJSON_CreateArray();

        for (const auto& peer : peers) {
            cJSON* item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "ip", peer.ip.address().to_string().c_str());
            cJSON_AddNumberToObject(item, "port", peer.ip.port());
            cJSON_AddStringToObject(item, "client", peer.client.c_str());
            cJSON_AddNumberToObject(item, "download_rate", static_cast<double>(peer.download_rate));
            cJSON_AddNumberToObject(item, "upload_rate", static_cast<double>(peer.upload_rate));
            cJSON_AddNumberToObject(item, "progress", peer.progress);
            cJSON_AddStringToObject(item, "peer_source", peer.source.to_string().c_str());
            cJSON_AddItemToArray(array, item);
        }

        char* json = cJSON_PrintUnformatted(array);
        std::string result(json);
        cJSON_Delete(array);
        free(json);
        return result;

    } catch (const std::exception& e) {
        return "[]";
    }
}

std::string QBittorrentCore::getTrackerListJson(const std::string& hashStr) const {
    if (!m_impl->session) return "[]";

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));
        if (!handle.is_valid()) return "[]";

        auto trackers = handle.trackers();
        cJSON* array = cJSON_CreateArray();

        for (const auto& tracker : trackers) {
            cJSON* item = cJSON_CreateObject();
            cJSON_AddStringToObject(item, "url", tracker.url.c_str());
            cJSON_AddNumberToObject(item, "tier", tracker.tier);
            cJSON_AddNumberToObject(item, "max_failures", tracker.max_failures);
            cJSON_AddNumberToObject(item, "fail_count", tracker.fail_count);
            cJSON_AddNumberToObject(item, "success_count", tracker.success_count);
            cJSON_AddNumberToObject(item, "last_error", tracker.last_error);
            cJSON_AddStringToObject(item, "status", tracker.status.c_str());
            cJSON_AddItemToArray(array, item);
        }

        char* json = cJSON_PrintUnformatted(array);
        std::string result(json);
        cJSON_Delete(array);
        free(json);
        return result;

    } catch (const std::exception& e) {
        return "[]";
    }
}

std::string QBittorrentCore::getFilesListJson(const std::string& hashStr) const {
    if (!m_impl->session) return "[]";

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));
        if (!handle.is_valid()) return "[]";

        auto ti = handle.torrent_file();
        if (!ti) return "[]";

        auto files = handle.file_progress();
        cJSON* array = cJSON_CreateArray();

        for (int i = 0; i < ti->num_files(); ++i) {
            cJSON* item = cJSON_CreateObject();
            cJSON_AddNumberToObject(item, "index", i);
            cJSON_AddStringToObject(item, "name", ti->files().file_name(i).c_str());
            cJSON_AddNumberToObject(item, "size", static_cast<double>(ti->files().file_size(i)));
            cJSON_AddNumberToObject(item, "progress", ti->files().file_size(i) > 0 ? static_cast<double>(files[i]) / ti->files().file_size(i) : 0);
            cJSON_AddNumberToObject(item, "priority", handle.file_priority(i));
            cJSON_AddItemToArray(array, item);
        }

        char* json = cJSON_PrintUnformatted(array);
        std::string result(json);
        cJSON_Delete(array);
        free(json);
        return result;

    } catch (const std::exception& e) {
        return "[]";
    }
}

bool QBittorrentCore::setFilePriority(const std::string& hashStr, int index, int priority) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));
        if (!handle.is_valid()) return false;

        return handle.set_file_priority(index, priority);
    } catch (const std::exception& e) {
        return false;
    }
}

std::string QBittorrentCore::getTransferInfoJson() const {
    return getGlobalStatsJson();
}

std::string QBittorrentCore::getAllTrackerInfoJson() const {
    if (!m_impl->session) return "[]";

    try {
        auto torrents = m_impl->session->get_torrents();
        cJSON* array = cJSON_CreateArray();

        for (const auto& handle : torrents) {
            if (!handle.is_valid()) continue;

            auto status = handle.status();
            auto trackers = handle.trackers();

            for (const auto& tracker : trackers) {
                cJSON* item = cJSON_CreateObject();
                cJSON_AddStringToObject(item, "hash", status.info_hash.to_string().c_str());
                cJSON_AddStringToObject(item, "name", status.name.c_str());
                cJSON_AddStringToObject(item, "url", tracker.url.c_str());
                cJSON_AddNumberToObject(item, "tier", tracker.tier);
                cJSON_AddStringToObject(item, "status", tracker.status.c_str());
                cJSON_AddNumberToObject(item, "last_error", tracker.last_error);
                cJSON_AddItemToArray(array, item);
            }
        }

        char* json = cJSON_PrintUnformatted(array);
        std::string result(json);
        cJSON_Delete(array);
        free(json);
        return result;

    } catch (const std::exception& e) {
        return "[]";
    }
}

void QBittorrentCore::saveFastResume() {
    if (!m_impl->session) return;

    try {
        std::string fastResumePath = m_impl->sessionDir + "/fastresume.data";
        lt::entry entry;
        m_impl->session->save_state(entry);

        std::ofstream out(fastResumePath, std::ios::binary);
        if (out) {
            lt::bencode(std::ostream_iterator<char>(out), entry);
            LOGI("Saved fast resume data");
        }
    } catch (const std::exception& e) {
        LOGE("Failed to save fast resume: %s", e.what());
    }
}

void QBittorrentCore::loadFastResume() {
    if (!m_impl->session) return;

    try {
        std::string fastResumePath = m_impl->sessionDir + "/fastresume.data";
        std::ifstream in(fastResumePath, std::ios::binary);

        if (in.good()) {
            lt::entry entry;
            lt::lazy_bdecode(std::istream_iterator<char>(in), std::istream_iterator<char>(), entry);
            m_impl->session->load_state(entry);
            LOGI("Loaded fast resume data");
        }
    } catch (const std::exception& e) {
        LOGE("Failed to load fast resume: %s", e.what());
    }
}

std::string QBittorrentCore::getLibtorrentVersion() const {
    return "2.0.9";
}

// ============================================
// Tracker 操作实现
// ============================================

bool QBittorrentCore::addTracker(const std::string& hashStr, const std::string& url) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));
        if (!handle.is_valid()) return false;

        // 添加新的 tracker URL
        std::vector<std::string> trackers;
        auto existing = handle.trackers();
        for (const auto& t : existing) {
            trackers.push_back(t.url);
        }
        trackers.push_back(url);

        // 使用 replace_trackers 来添加
        lt::replace_trackers_replace_flags flags;
        handle.replace_trackers(trackers, flags);

        LOGI("Added tracker: %s to %s", url.c_str(), hashStr.c_str());
        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to add tracker: %s", e.what());
        return false;
    }
}

bool QBittorrentCore::editTracker(const std::string& hashStr, const std::string& oldUrl, const std::string& newUrl) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));
        if (!handle.is_valid()) return false;

        // 获取当前 tracker 列表并替换
        std::vector<std::string> trackers;
        auto existing = handle.trackers();
        for (const auto& t : existing) {
            if (t.url == oldUrl) {
                trackers.push_back(newUrl);
            } else {
                trackers.push_back(t.url);
            }
        }

        handle.replace_trackers(trackers, lt::replace_trackers_replace_flags());

        LOGI("Edited tracker: %s -> %s", oldUrl.c_str(), newUrl.c_str());
        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to edit tracker: %s", e.what());
        return false;
    }
}

bool QBittorrentCore::removeTracker(const std::string& hashStr, const std::string& url) {
    if (!m_impl->session) return false;

    try {
        lt::torrent_handle handle = m_impl->session->find_torrent(lt::sha1_hash(hashStr));
        if (!handle.is_valid()) return false;

        // 移除指定的 tracker
        std::vector<std::string> trackers;
        auto existing = handle.trackers();
        for (const auto& t : existing) {
            if (t.url != url) {
                trackers.push_back(t.url);
            }
        }

        handle.replace_trackers(trackers, lt::replace_trackers_replace_flags());

        LOGI("Removed tracker: %s from %s", url.c_str(), hashStr.c_str());
        return true;

    } catch (const std::exception& e) {
        LOGE("Failed to remove tracker: %s", e.what());
        return false;
    }
}

// ============================================
// Alert 处理线程
// ============================================

void QBittorrentCore::startAlertHandler() {
    if (!m_impl->session) return;

    if (m_impl->alertThreadRunning) return;

    m_impl->alertThreadRunning = true;

    m_impl->alertThread = new std::thread([this]() {
        LOGI("Alert handler thread started");

        while (m_impl->alertThreadRunning && m_impl->session) {
            try {
                std::vector<lt::alert*> alerts;
                m_impl->session->pop_alerts(&alerts);

                for (lt::alert* alert : alerts) {
                    if (!alert) continue;

                    std::string type = alert->what();

                    // state_update_alert - torrent 状态更新
                    if (type == "state_update_alert") {
                        auto* su = dynamic_cast<lt::state_update_alert*>(alert);
                        if (su) {
                            for (const auto& status : su->status) {
                                // LOGD("Torrent %s state: %d", status.name.c_str(), status.state);
                            }
                        }
                    }
                    // torrent_finished_alert - 下载完成
                    else if (type == "torrent_finished_alert") {
                        auto* tf = dynamic_cast<lt::torrent_finished_alert*>(alert);
                        if (tf) {
                            LOGI("Torrent finished: %s", tf->torrent_name().c_str());
                            // 自动开始做种
                        }
                    }
                    // tracker_error_alert - tracker 错误
                    else if (type == "tracker_error_alert") {
                        auto* te = dynamic_cast<lt::tracker_error_alert*>(alert);
                        if (te) {
                            LOGE("Tracker error: %s - %s", te->tracker_url().c_str(), te->msg().c_str());
                        }
                    }
                    // tracker_reply_alert - tracker 回复
                    else if (type == "tracker_reply_alert") {
                        auto* tr = dynamic_cast<lt::tracker_reply_alert*>(alert);
                        if (tr) {
                            LOGI("Tracker reply: %s - %d peers", tr->tracker_url().c_str(), tr->num_peers());
                        }
                    }
                    // save_resume_data_alert - 保存 resume 数据
                    else if (type == "save_resume_data_alert") {
                        auto* sd = dynamic_cast<lt::save_resume_data_alert*>(alert);
                        if (sd) {
                            LOGI("Save resume data: %s", sd->torrent_name().c_str());
                        }
                    }
                    // peer_disconnected_alert
                    else if (type == "peer_disconnected_alert") {
                        // LOGD("Peer disconnected");
                    }
                    // peer_connect_alert
                    else if (type == "peer_connect_alert") {
                        // LOGD("Peer connected");
                    }
                    // dht_announce_alert
                    else if (type == "dht_announce_alert") {
                        auto* da = dynamic_cast<lt::dht_announce_alert*>(alert);
                        if (da) {
                            LOGI("DHT announce: %s:%d", da->addr().to_string().c_str(), da->port());
                        }
                    }

                    // 释放 alert 内存
                    delete alert;
                }

            } catch (const std::exception& e) {
                LOGE("Alert handler error: %s", e.what());
            }

            // 休眠一小段时间避免 CPU 满载
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        LOGI("Alert handler thread stopped");
    });
}

void QBittorrentCore::stopAlertHandler() {
    if (!m_impl->alertThreadRunning) return;

    m_impl->alertThreadRunning = false;

    if (m_impl->alertThread && m_impl->alertThread->joinable()) {
        m_impl->alertThread->join();
        delete m_impl->alertThread;
        m_impl->alertThread = nullptr;
    }
}