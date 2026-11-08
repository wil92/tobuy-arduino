#include <cassert>
#include <algorithm>
#include <cstring>

#include "display_manager.h"
#include "flash_manager.h"
#include "network_manager.h"
#include <TFT_eSPI.h>
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
        assert(TFT_eSPI::drawnText().empty());
    } else if (std::strcmp(argv[1], "invalid") == 0) {
        assert(WiFi.stationBegins == 0 && WiFi.hotspotBegins == 1);
        assert(WebServer::instance->started);
        assert(std::find(TFT_eSPI::drawnText().begin(), TFT_eSPI::drawnText().end(), "test-password") !=
               TFT_eSPI::drawnText().end());
        assert(Serial.output.find("Saved Wi-Fi settings are invalid") != std::string::npos);
    } else {
        assert(WiFi.stationBegins == 0 && WiFi.hotspotBegins == 1);
        assert(WebServer::instance->started);
        assert(std::find(TFT_eSPI::drawnText().begin(), TFT_eSPI::drawnText().end(), "test-password") !=
               TFT_eSPI::drawnText().end());
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
