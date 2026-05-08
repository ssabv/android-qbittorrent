#include "../qbcore.h"

struct QBittorrentCore::Impl {
};

QBittorrentCore::QBittorrentCore(const std::string& sessionDir, const std::string& downloadPath)
    : pImpl(std::make_unique<Impl>()) {
}

QBittorrentCore::~QBittorrentCore() = default;

bool QBittorrentCore::initialize() { return true; }
void QBittorrentCore::start() {}
void QBittorrentCore::stop() {}
bool QBittorrentCore::isRunning() const { return false; }

std::string QBittorrentCore::addMagnet(const std::string& magnetUri) { return ""; }
std::string QBittorrentCore::addTorrent(const std::vector<char>& data, const std::string& fileName) { return ""; }
std::string QBittorrentCore::addTorrentFromFile(const std::string& filePath) { return ""; }
bool QBittorrentCore::removeTorrent(const std::string& hash, bool deleteFiles) { return false; }

bool QBittorrentCore::resumeTorrent(const std::string& hash) { return false; }
bool QBittorrentCore::pauseTorrent(const std::string& hash) { return false; }
bool QBittorrentCore::forceRecheck(const std::string& hash) { return false; }

bool QBittorrentCore::setUploadLimit(const std::string& hash, int limit) { return false; }
bool QBittorrentCore::setDownloadLimit(const std::string& hash, int limit) { return false; }
bool QBittorrentCore::setGlobalUploadLimit(int limit) { return false; }
bool QBittorrentCore::setGlobalDownloadLimit(int limit) { return false; }
bool QBittorrentCore::setMaxConnections(int limit) { return false; }
bool QBittorrentCore::setMaxPeers(int limit) { return false; }
bool QBittorrentCore::setListenPort(int port) { return false; }

bool QBittorrentCore::setDHTEnabled(bool enabled) { return false; }
bool QBittorrentCore::setPEXEnabled(bool enabled) { return false; }
bool QBittorrentCore::setLSDEnabled(bool enabled) { return false; }
bool QBittorrentCore::setEncryptionMode(int mode) { return false; }
bool QBittorrentCore::setPTMode(bool enabled) { return false; }

int QBittorrentCore::getTorrentCount() const { return 0; }
std::string QBittorrentCore::getTorrentListJson() { return "[]"; }
std::string QBittorrentCore::getTorrentDetailsJson(const std::string& hash) { return "{}"; }
std::string QBittorrentCore::getGlobalStatsJson() { return "{}"; }

std::string QBittorrentCore::getSettingsJson() { return "{}"; }
bool QBittorrentCore::setSettingsJson(const std::string& json) { return false; }

std::string QBittorrentCore::getPeerListJson(const std::string& hash) { return "[]"; }
std::string QBittorrentCore::getTrackerListJson(const std::string& hash) { return "[]"; }
std::string QBittorrentCore::getFilesListJson(const std::string& hash) { return "[]"; }
bool QBittorrentCore::setFilePriority(const std::string& hash, int index, int priority) { return false; }

std::string QBittorrentCore::getTransferInfoJson() { return "{}"; }
std::string QBittorrentCore::getAllTrackerInfoJson() { return "[]"; }

void QBittorrentCore::saveFastResume() {}
void QBittorrentCore::loadFastResume() {}

std::string QBittorrentCore::getVersion() const { return "1.0.0"; }
std::string QBittorrentCore::getLibtorrentVersion() const { return "stub"; }

bool QBittorrentCore::addTracker(const std::string& hash, const std::string& url) { return false; }
bool QBittorrentCore::editTracker(const std::string& hash, const std::string& oldUrl, const std::string& newUrl) { return false; }
bool QBittorrentCore::removeTracker(const std::string& hash, const std::string& url) { return false; }

void QBittorrentCore::startAlertHandler() {}
void QBittorrentCore::stopAlertHandler() {}