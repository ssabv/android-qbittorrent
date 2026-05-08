#pragma once

#include <string>
#include <vector>
#include <memory>
#include <atomic>

class QBittorrentCore {
public:
    QBittorrentCore(const std::string& sessionDir, const std::string& downloadPath);
    ~QBittorrentCore();

    bool initialize();
    void start();
    void stop();
    bool isRunning() const;

    std::string addMagnet(const std::string& magnetUri);
    std::string addTorrent(const std::vector<char>& data, const std::string& fileName);
    std::string addTorrentFromFile(const std::string& filePath);

    bool removeTorrent(const std::string& hash, bool deleteFiles);
    bool resumeTorrent(const std::string& hash);
    bool pauseTorrent(const std::string& hash);
    bool forceRecheck(const std::string& hash);

    bool setUploadLimit(const std::string& hash, int limit);
    bool setDownloadLimit(const std::string& hash, int limit);
    bool setGlobalUploadLimit(int limit);
    bool setGlobalDownloadLimit(int limit);
    bool setMaxConnections(int limit);
    bool setMaxPeers(int limit);
    bool setListenPort(int port);
    bool setDHTEnabled(bool enabled);
    bool setPEXEnabled(bool enabled);
    bool setLSDEnabled(bool enabled);
    bool setEncryptionMode(int mode);
    bool setPTMode(bool enabled);

    size_t getTorrentCount() const;
    std::string getTorrentListJson() const;
    std::string getTorrentDetailsJson(const std::string& hash) const;
    std::string getGlobalStatsJson() const;
    std::string getSettingsJson() const;
    bool setSettingsJson(const std::string& settingsJson);

    std::string getPeerListJson(const std::string& hash) const;
    std::string getTrackerListJson(const std::string& hash) const;
    std::string getFilesListJson(const std::string& hash) const;
    bool setFilePriority(const std::string& hash, int index, int priority);

    std::string getTransferInfoJson() const;
    std::string getAllTrackerInfoJson() const;

    bool addTracker(const std::string& hash, const std::string& url);
    bool editTracker(const std::string& hash, const std::string& oldUrl, const std::string& newUrl);
    bool removeTracker(const std::string& hash, const std::string& url);

    void saveFastResume();
    void loadFastResume();

    void startAlertHandler();
    void stopAlertHandler();

    std::string getLibtorrentVersion() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::atomic<bool> m_running;
};