#include "flash_manager.h"

constexpr const char *FlashManager::storageKey;

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

bool FlashManager::clear() {
    return !preferences_.isKey(storageKey) || preferences_.remove(storageKey);
}
