#pragma once

#include <Preferences.h>
#include "wifi_settings.h"

class FlashManager {
public:
    enum class LoadResult { missing, loaded, invalid };

    bool begin();
    LoadResult load(wifi_settings::Credentials &credentials);
    bool save(const wifi_settings::Credentials &credentials);
    bool clear();

private:
    static constexpr const char *storageKey = "wifi";
    Preferences preferences_;
};
