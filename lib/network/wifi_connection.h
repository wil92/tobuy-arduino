#pragma once

#include <WiFi.h>

template <typename Wifi, typename Logger>
class WifiConnection {
public:
    WifiConnection(Wifi &wifi, Logger &logger) : wifi_(wifi), logger_(logger) {}

    void begin(const char *ssid, const char *password) {
        if (ssid == nullptr || ssid[0] == '\0') {
            logger_.println("Wi-Fi disabled: network SSID is missing");
            return;
        }

        wifi_.persistent(false);
        if (!wifi_.mode(WIFI_STA)) {
            logger_.println("Wi-Fi failed: could not enable station mode");
            return;
        }
        if (!wifi_.setAutoReconnect(true)) {
            logger_.println("Wi-Fi failed: could not enable automatic reconnection");
            return;
        }

        logger_.println("Wi-Fi connecting");
        if (wifi_.begin(ssid, password) == WL_CONNECT_FAILED) {
            logger_.println("Wi-Fi failed: could not start connection");
            return;
        }
        active_ = true;
    }

    void update() {
        if (!active_) return;

        const wl_status_t status = wifi_.status();
        if (static_cast<int>(status) == last_status_) return;
        last_status_ = static_cast<int>(status);

        if (status == WL_CONNECTED) {
            logger_.print("Wi-Fi connected; IP address: ");
            logger_.println(wifi_.localIP());
        } else {
            logger_.print("Wi-Fi status: ");
            logger_.println(last_status_);
        }
    }

private:
    Wifi &wifi_;
    Logger &logger_;
    bool active_ = false;
    int last_status_ = -1;
};
