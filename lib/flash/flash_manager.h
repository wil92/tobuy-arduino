#pragma once

#include <Preferences.h>
#include "server_settings.h"
#include "wifi_settings.h"

class FlashManager {
public:
    enum class LoadResult { missing, loaded, invalid };

    bool begin();
    LoadResult load(wifi_settings::Credentials &credentials);
    bool save(const wifi_settings::Credentials &credentials);
    LoadResult loadServer(server_settings::ServerUrl &server);
    bool saveServer(const server_settings::ServerUrl &server);
    bool clear();

private:
    static constexpr const char *storageKey = "wifi";
    static constexpr const char *serverKey = "server";
    Preferences preferences_;
};
