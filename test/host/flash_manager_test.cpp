#include <cassert>
#include <cstring>

#include "FlashManager.h"

Preferences *Preferences::instance = nullptr;

int main() {
    FlashManager flash;
    Preferences &preferences = *Preferences::instance;
    preferences.beginOk = false;
    assert(!flash.begin());
    preferences.beginOk = true;
    assert(flash.begin());
    wifi_settings::Credentials credentials = {};
    assert(flash.load(credentials) == FlashManager::LoadResult::missing);
    assert(flash.clear());
    assert(!flash.save(credentials));
    preferences.saved.assign(2, 'x');
    assert(flash.load(credentials) == FlashManager::LoadResult::invalid);
    preferences.saved.assign(sizeof(credentials), 0);
    assert(flash.load(credentials) == FlashManager::LoadResult::invalid);
    assert(wifi_settings::assign(credentials, "saved-network", "12345678"));
    preferences.fail = true;
    assert(!flash.save(credentials));
    preferences.fail = false;
    assert(flash.save(credentials));
    wifi_settings::Credentials loaded = {};
    preferences.readFails = true;
    assert(flash.load(loaded) == FlashManager::LoadResult::invalid);
    preferences.readFails = false;
    assert(flash.load(loaded) == FlashManager::LoadResult::loaded);
    assert(std::strcmp(loaded.ssid, "saved-network") == 0);
    assert(std::strcmp(loaded.password, "12345678") == 0);
    preferences.removeFails = true;
    assert(!flash.clear());
    preferences.removeFails = false;
    assert(flash.clear());
    assert(flash.load(loaded) == FlashManager::LoadResult::missing);
}
