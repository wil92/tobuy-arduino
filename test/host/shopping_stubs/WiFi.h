#pragma once
enum { WL_CONNECTED = 3, WL_DISCONNECTED = 6 };
struct WiFiClient {};
struct FakeWiFi {
    int currentStatus = WL_DISCONNECTED;
    int status() const { return currentStatus; }
};
extern FakeWiFi WiFi;
