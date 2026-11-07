#include <cassert>
#include <cstring>
#include <string>

#include "setup_portal.h"

HardwareSerial Serial;
FakeEsp ESP;
unsigned long fakeMillis = 0;
WiFiClass WiFi;
WebServer *WebServer::instance = nullptr;
Preferences *Preferences::instance = nullptr;

int main() {
    FlashManager flash;
    Preferences &preferences = *Preferences::instance;
    NetworkManager network;
    SetupPortal portal(flash, network);
    WebServer &server = *WebServer::instance;
    assert(!network.startHotspot(""));
    WiFi.apOk = false;
    assert(!network.startHotspot("password"));
    WiFi.apOk = true;
    assert(network.startHotspot("password"));
    assert(flash.begin());
    portal.begin();
    assert(server.started);
    assert(Serial.output.find("Setup hotspot password: password") != std::string::npos);

    server.request("/", HTTP_GET);
    assert(server.code == 200 && server.contentType.find("text/html") == 0);
    assert(server.body.find("Wi-Fi configuration") != std::string::npos);
    server.request("/scan", HTTP_POST);
    assert(server.code == 202);
    server.request("/refresh-ssid", HTTP_GET);
    assert(server.code == 202);
    WiFi.networks = {String("home"), String("q\"\\\n")};
    WiFi.scanState = 2;
    server.request("/refresh-ssid", HTTP_GET);
    assert(server.code == 200 && server.body == "[\"home\",\"q\\\"\\\\\\u000a\"]");
    WiFi.scanState = WIFI_SCAN_FAILED;
    server.request("/refresh-ssid", HTTP_GET);
    assert(server.code == 500);

    server.request("/", HTTP_POST);
    assert(server.code == 400 && preferences.saved.empty());
    server.args["ssid"] = "network";
    server.args["password"] = "short";
    server.request("/", HTTP_POST);
    assert(server.code == 400 && preferences.saved.empty());
    server.args["password"] = "12345678";
    preferences.fail = true;
    server.request("/", HTTP_POST);
    assert(server.code == 500 && preferences.saved.empty());
    preferences.fail = false;
    server.request("/", HTTP_POST);
    assert(server.code == 200 && preferences.saved.size() == sizeof(wifi_settings::Credentials));
    wifi_settings::Credentials saved = {};
    std::memcpy(&saved, preferences.saved.data(), sizeof(saved));
    assert(std::strcmp(saved.ssid, "network") == 0);
    assert(std::strcmp(saved.password, "12345678") == 0);
    server.request("/", HTTP_POST);
    assert(server.code == 409);
    fakeMillis = 999;
    portal.update();
    assert(!ESP.restarted);
    fakeMillis = 1000;
    portal.update();
    assert(ESP.restarted);
}
