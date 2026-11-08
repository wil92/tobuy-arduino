#include "flash_manager.h"

constexpr const char *FlashManager::storageKey;
constexpr const char *FlashManager::serverKey;

bool FlashManager::begin() {
    return preferences_.begin("tobuy", false);
}

FlashManager::LoadResult FlashManager::load(wifi_settings::Credentials &credentials) {
    if (!preferences_.isKey(storageKey)) return LoadResult::missing;
    wifi_settings::Credentials candidate = {};
    if (preferences_.getBytesLength(storageKey) != sizeof(candidate) ||
        preferences_.getBytes(storageKey, &candidate, sizeof(candidate)) != sizeof(candidate) ||
        !wifi_settings::valid(candidate)) return LoadResult::invalid;
    credentials = candidate;
    return LoadResult::loaded;
}

bool FlashManager::save(const wifi_settings::Credentials &credentials) {
    return wifi_settings::valid(credentials) &&
           preferences_.putBytes(storageKey, &credentials, sizeof(credentials)) == sizeof(credentials);
}

FlashManager::LoadResult FlashManager::loadServer(server_settings::ServerUrl &server) {
    if (!preferences_.isKey(serverKey)) return LoadResult::missing;
    server_settings::ServerUrl candidate = {};
    if (preferences_.getBytesLength(serverKey) != sizeof(candidate) ||
        preferences_.getBytes(serverKey, &candidate, sizeof(candidate)) != sizeof(candidate) ||
        !server_settings::valid(candidate)) return LoadResult::invalid;
    server = candidate;
    return LoadResult::loaded;
}

bool FlashManager::saveServer(const server_settings::ServerUrl &server) {
    return server_settings::valid(server) &&
           preferences_.putBytes(serverKey, &server, sizeof(server)) == sizeof(server);
}

bool FlashManager::clear() {
    const bool wifiCleared = !preferences_.isKey(storageKey) || preferences_.remove(storageKey);
    const bool serverCleared = !preferences_.isKey(serverKey) || preferences_.remove(serverKey);
    return wifiCleared && serverCleared;
}
