#pragma once

#include "Arduino.h"
#include <vector>

constexpr int WIFI_AP_STA = 3;
constexpr int WIFI_STA = 1;
constexpr int WIFI_SCAN_RUNNING = -1;
constexpr int WIFI_SCAN_FAILED = -2;
enum wl_status_t { WL_IDLE_STATUS, WL_CONNECTED, WL_CONNECT_FAILED };

struct IPAddress {
    IPAddress(int, int, int, int) {}
};

struct WiFiClass {
    bool modeOk = true;
    bool configOk = true;
    bool apOk = true;
    int stationBegins = 0;
    int hotspotBegins = 0;
    String connectedSsid;
    String connectedPassword;
    int scanState = WIFI_SCAN_FAILED;
    std::vector<String> networks;
    void persistent(bool) {}
    bool mode(int) { return modeOk; }
    bool setAutoReconnect(bool) { return true; }
    wl_status_t begin(const char *ssid, const char *password) {
        ++stationBegins;
        connectedSsid = ssid;
        connectedPassword = password;
        return WL_IDLE_STATUS;
    }
    wl_status_t status() { return WL_IDLE_STATUS; }
    const char *localIP() { return "192.0.2.1"; }
    bool softAPConfig(IPAddress, IPAddress, IPAddress) { return configOk; }
    bool softAP(const char *, const char *) { ++hotspotBegins; return apOk; }
    const char *softAPIP() { return "192.168.1.1"; }
    int scanComplete() { return scanState; }
    void scanDelete() { scanState = WIFI_SCAN_FAILED; }
    int scanNetworks(bool) { scanState = WIFI_SCAN_RUNNING; return scanState; }
    String SSID(int index) { return networks[index]; }
};

extern WiFiClass WiFi;
