#include <Arduino.h>
#include "display_manager.h"
#include "flash_manager.h"
#include "network_manager.h"
#include "reset_button.h"
#include "setup_portal.h"
#include "shopping/shopping_client.h"
#include "shopping/shopping_view.h"

#if __has_include("setup_ap_config.h")
#include "setup_ap_config.h"
#endif

#ifndef SETUP_AP_PASSWORD
#define SETUP_AP_PASSWORD ""
#endif

#ifndef RESET_CONFIG_PIN
#define RESET_CONFIG_PIN 27
#endif

DisplayManager display;
NetworkManager network;
FlashManager flash;
ResetButton resetButton(RESET_CONFIG_PIN);
SetupPortal portal(flash, network);
ShoppingClient shoppingClient;
ShoppingView shoppingView(display.screen(), shoppingClient);
bool portalActive = false;
bool storageReady = false;
bool shoppingActive = false;

void redrawShopping() {
    shoppingView.render();
}

void checkResetButton() {
    if (!resetButton.update()) return;
    if (flash.clear()) {
        Serial.println("Wi-Fi settings cleared; restarting");
        ESP.restart();
    } else {
        Serial.println("Failed to clear Wi-Fi settings");
    }
}

void setup() {
    Serial.begin(115200);
    resetButton.begin();
    display.begin();
    if (!flash.begin()) {
        Serial.println("Failed to open Wi-Fi settings storage");
        return;
    }
    storageReady = true;
    wifi_settings::Credentials credentials = {};
    const auto result = flash.load(credentials);
    if (result == FlashManager::LoadResult::loaded) {
        network.connect(credentials.ssid, credentials.password);
        server_settings::ServerUrl server = {};
        if (flash.loadServer(server) != FlashManager::LoadResult::loaded) {
            Serial.println("Server URL missing or invalid; reset to configure");
            display.showError("Configure server URL");
        } else {
            shoppingView.begin(server.url);
            shoppingClient.begin(server.url, redrawShopping);
            shoppingActive = true;
        }
    } else {
        if (result == FlashManager::LoadResult::invalid) {
            Serial.println("Saved Wi-Fi settings are invalid; starting setup portal");
        }
        if (network.startHotspot(SETUP_AP_PASSWORD)) {
            display.showSetupPassword(SETUP_AP_PASSWORD);
            portal.begin();
            portalActive = true;
        }
    }
}

void loop() {
    if (storageReady) checkResetButton();
    if (portalActive) portal.update();
    else {
        network.update();
        if (shoppingActive) {
            shoppingClient.update();
            shoppingView.update();
        }
    }
}
