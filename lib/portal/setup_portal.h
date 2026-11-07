#pragma once

#include <WebServer.h>

#include "flash_manager.h"
#include "network_manager.h"

class SetupPortal {
public:
    SetupPortal(FlashManager &flash, NetworkManager &network);
    void begin();
    void update();

private:
    void startScan();
    void sendNetworks();
    void saveSettings();

    FlashManager &flash_;
    NetworkManager &network_;
    WebServer server_{80};
    bool restartPending_ = false;
    unsigned long restartAt_ = 0;
};
