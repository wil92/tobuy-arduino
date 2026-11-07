#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "wifi_connection.h"

class NetworkManager {
public:
    NetworkManager();
    void connect(const char *ssid, const char *password);
    void update();
    bool startHotspot(const char *password);
    bool startScan();
    int scanComplete();
    String ssid(int index);

private:
    WifiConnection<WiFiClass, HardwareSerial> connection_;
};
