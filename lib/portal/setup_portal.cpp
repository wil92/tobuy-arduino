#include "setup_portal.h"

#include "views/setup_page.h"

namespace {
void appendJsonString(String &result, const String &value) {
    const char hex[] = "0123456789abcdef";
    result += '"';
    for (size_t i = 0; i < value.length(); ++i) {
        const unsigned char c = static_cast<unsigned char>(value[i]);
        if (c == '"' || c == '\\') {
            result += '\\';
            result += static_cast<char>(c);
        } else if (c < 0x20) {
            result += "\\u00";
            result += hex[c >> 4];
            result += hex[c & 0x0f];
        } else {
            result += static_cast<char>(c);
        }
    }
    result += '"';
}
}

SetupPortal::SetupPortal(FlashManager &flash, NetworkManager &network)
    : flash_(flash), network_(network) {}

void SetupPortal::begin() {
    server_.on("/", HTTP_GET, [this]() {
        server_.sendHeader("Cache-Control", "no-store");
        server_.send_P(200, "text/html; charset=utf-8", setup_page);
    });
    server_.on("/scan", HTTP_POST, [this]() { startScan(); });
    server_.on("/refresh-ssid", HTTP_GET, [this]() { sendNetworks(); });
    server_.on("/", HTTP_POST, [this]() { saveSettings(); });
    server_.onNotFound([this]() { server_.send(404, "text/plain", "Not found"); });
    server_.begin();
}

void SetupPortal::update() {
    server_.handleClient();
    if (restartPending_ && static_cast<unsigned long>(millis() - restartAt_) >= 1000) {
        ESP.restart();
    }
}

void SetupPortal::startScan() {
    if (network_.scanComplete() == WIFI_SCAN_RUNNING) {
        server_.send(202, "text/plain", "Scanning");
        return;
    }
    if (!network_.startScan()) {
        Serial.println("Wi-Fi scan failed to start");
        server_.send(500, "text/plain", "Could not start Wi-Fi scan");
        return;
    }
    server_.send(202, "text/plain", "Scanning");
}

void SetupPortal::sendNetworks() {
    const int count = network_.scanComplete();
    server_.sendHeader("Cache-Control", "no-store");
    if (count == WIFI_SCAN_RUNNING) {
        server_.send(202, "text/plain", "Scanning");
        return;
    }
    if (count < 0) {
        Serial.println("Wi-Fi scan failed");
        server_.send(500, "text/plain", "Wi-Fi scan failed; try again");
        return;
    }
    String result = "[";
    for (int i = 0; i < count; ++i) {
        if (i) result += ',';
        appendJsonString(result, network_.ssid(i));
    }
    result += ']';
    server_.send(200, "application/json; charset=utf-8", result);
}

void SetupPortal::saveSettings() {
    if (restartPending_) {
        server_.send(409, "text/plain", "Device is restarting");
        return;
    }
    if (!server_.hasArg("ssid") || !server_.hasArg("password") || !server_.hasArg("server")) {
        server_.send(400, "text/plain", "SSID, password, and server URL are required");
        return;
    }
    const String ssid = server_.arg("ssid");
    const String password = server_.arg("password");
    wifi_settings::Credentials credentials = {};
    if (ssid.length() != strlen(ssid.c_str()) || password.length() != strlen(password.c_str()) ||
        !wifi_settings::assign(credentials, ssid.c_str(), password.c_str())) {
        server_.send(400, "text/plain", "Invalid SSID or password length");
        return;
    }
    const String serverUrl = server_.arg("server");
    server_settings::ServerUrl server = {};
    if (serverUrl.length() != strlen(serverUrl.c_str()) ||
        !server_settings::assign(server, serverUrl.c_str())) {
        server_.send(400, "text/plain", "Invalid server URL");
        return;
    }
    // Wi-Fi settings are saved last because their presence ends setup mode.
    if (!flash_.saveServer(server)) {
        Serial.println("Failed to save server URL");
        server_.send(500, "text/plain", "Could not save settings");
        return;
    }
    if (!flash_.save(credentials)) {
        Serial.println("Failed to save Wi-Fi settings");
        server_.send(500, "text/plain", "Could not save settings");
        return;
    }
    server_.send(200, "text/plain", "Settings saved. The device is restarting.");
    restartAt_ = millis();
    restartPending_ = true;
}
