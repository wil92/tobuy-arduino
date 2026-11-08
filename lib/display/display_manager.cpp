#include "display_manager.h"

#include <Arduino.h>
#include <cstring>

#ifndef LOAD_FONT2
#error "The setup screen requires TFT_eSPI font 2 (LOAD_FONT2)"
#endif

void DisplayManager::begin() {
    display_.init();
    Serial.print("Display initialized; dimensions: ");
    Serial.print(display_.width());
    Serial.print("x");
    Serial.println(display_.height());
    if ((display_.fontsLoaded() & (1U << 2)) == 0) {
        Serial.println("Display error: font 2 is not loaded");
    }
}

void DisplayManager::showError(const char *message) {
    display_.fillScreen(TFT_BLACK);
    display_.setTextColor(TFT_WHITE, TFT_BLACK);
    display_.setTextDatum(TC_DATUM);
    display_.setTextFont(2);
    display_.drawString(message, display_.width() / 2, 20);
}

void DisplayManager::showSetupPassword(const char *password) {
    constexpr size_t password_line_length = 24;
    char password_line[password_line_length + 1];
    const int32_t center = display_.width() / 2;

    display_.fillScreen(TFT_BLACK);
    if ((display_.fontsLoaded() & (1U << 2)) == 0) {
        Serial.println("Display error: cannot render setup text without font 2");
        return;
    }
    display_.setTextColor(TFT_WHITE, TFT_BLACK);
    display_.setTextDatum(TC_DATUM);
    display_.setTextFont(2);
    if (display_.drawString("Wi-Fi setup", center, 12) == 0) {
        Serial.println("Display error: setup heading has zero width");
    }
    display_.drawString("Network: ToBuy-Setup", center, 38);
    display_.drawString("Password:", center, 66);

    const size_t password_length = strlen(password);
    size_t line_number = 0;
    for (size_t offset = 0; offset < password_length; offset += password_line_length) {
        size_t line_length = password_length - offset;
        if (line_length > password_line_length) line_length = password_line_length;
        memcpy(password_line, password + offset, line_length);
        password_line[line_length] = '\0';
        display_.drawString(password_line, center, 94 + line_number * 24);
        ++line_number;
    }

    display_.drawString("Open http://192.168.1.1", center, 190);
    Serial.println("Display setup screen drawing commands sent");
}
