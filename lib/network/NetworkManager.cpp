#include "NetworkManager.h"

namespace {
constexpr char apName[] = "ToBuy-Setup";
}

NetworkManager::NetworkManager() : connection_(WiFi, Serial) {}

void NetworkManager::connect(const char *ssid, const char *password) {
    connection_.begin(ssid, password);
}

void NetworkManager::update() {
    connection_.update();
}

bool NetworkManager::startHotspot(const char *password) {
    if (!password || strlen(password) < 8 || strlen(password) > 63) {
        Serial.println("Setup hotspot unavailable: configure an 8-63 byte SETUP_AP_PASSWORD");
        return false;
    }

    WiFi.persistent(false);
    if (!WiFi.mode(WIFI_AP_STA) ||
        !WiFi.softAPConfig(IPAddress(192, 168, 1, 1), IPAddress(192, 168, 1, 1),
                           IPAddress(255, 255, 255, 0)) ||
        !WiFi.softAP(apName, password)) {
        Serial.println("Failed to start setup hotspot");
        return false;
    }
    Serial.print("Setup hotspot: ");
    Serial.println(apName);
    Serial.print("Setup hotspot password: ");
    Serial.println(password);
    Serial.print("Setup page: http://");
    Serial.println(WiFi.softAPIP());
    return true;
}

bool NetworkManager::startScan() {
    WiFi.scanDelete();
    return WiFi.scanNetworks(true) != WIFI_SCAN_FAILED;
}

int NetworkManager::scanComplete() {
    return WiFi.scanComplete();
}

String NetworkManager::ssid(int index) {
    return WiFi.SSID(index);
}
