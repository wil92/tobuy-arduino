#include <cassert>
#include <cstring>

#include "FlashManager.h"
#include "NetworkManager.h"
#include <WebServer.h>

HardwareSerial Serial;
FakeEsp ESP;
unsigned long fakeMillis = 0;
int fakePinValue = HIGH;
int configuredPin = -1;
int configuredMode = -1;
WiFiClass WiFi;
WebServer *WebServer::instance = nullptr;
Preferences *Preferences::instance = nullptr;

void setup();
void loop();

int main(int argc, char **argv) {
    assert(argc == 2);
    Preferences &storage = *Preferences::instance;
    if (std::strcmp(argv[1], "saved") == 0) {
        wifi_settings::Credentials credentials = {};
        assert(wifi_settings::assign(credentials, "saved-network", "12345678"));
        assert(storage.putBytes("wifi", &credentials, sizeof(credentials)) == sizeof(credentials));
    } else if (std::strcmp(argv[1], "invalid") == 0) {
        storage.saved.assign(2, 'x');
    }

    setup();
    assert(configuredPin == 27 && configuredMode == INPUT_PULLUP);
    if (std::strcmp(argv[1], "saved") == 0) {
        assert(WiFi.stationBegins == 1 && WiFi.hotspotBegins == 0);
        assert(WiFi.connectedSsid == "saved-network" && WiFi.connectedPassword == "12345678");
    } else if (std::strcmp(argv[1], "invalid") == 0) {
        assert(WiFi.stationBegins == 0 && WiFi.hotspotBegins == 0);
        assert(Serial.output.find("Saved Wi-Fi settings are invalid") != std::string::npos);
    } else {
        assert(WiFi.stationBegins == 0 && WiFi.hotspotBegins == 1);
        assert(WebServer::instance->started);
        WebServer::instance->request("/", HTTP_GET);
        assert(WebServer::instance->code == 200);
    }
    fakePinValue = LOW;
    loop();
    fakeMillis = 3000;
    loop();
    assert(ESP.restarted);
    assert(storage.saved.empty());
}
