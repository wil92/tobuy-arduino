#include <cassert>
#include <string>
#include <vector>

#include "display_manager.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

HardwareSerial Serial;

int main() {
    DisplayManager display;
    display.begin();
    assert(TFT_eSPI::drawnText().empty());
    assert(Serial.output.find("Display initialized; dimensions: 240x320") != std::string::npos);

    const std::string password(63, 'p');
    display.showSetupPassword(password.c_str());

    const std::vector<std::string> &lines = TFT_eSPI::drawnText();
    assert(lines.size() == 7);
    assert(lines[0] == "Wi-Fi setup");
    assert(lines[1] == "Network: ToBuy-Setup");
    assert(lines[2] == "Password:");
    assert(lines[3] == std::string(24, 'p'));
    assert(lines[4] == std::string(24, 'p'));
    assert(lines[5] == std::string(15, 'p'));
    assert(lines[6] == "Open http://192.168.1.1");

    std::string displayed_password = lines[3] + lines[4] + lines[5];
    assert(displayed_password == password);
    assert(Serial.output.find("Display setup screen drawing commands sent") != std::string::npos);
    assert(Serial.output.find(password) == std::string::npos);

    TFT_eSPI::clearDrawnText();
    TFT_eSPI::loadedFonts() = 0;
    Serial.output.clear();
    display.begin();
    display.showSetupPassword(password.c_str());
    assert(TFT_eSPI::drawnText().empty());
    assert(Serial.output.find("font 2 is not loaded") != std::string::npos);
    assert(Serial.output.find("cannot render setup text") != std::string::npos);

    TFT_eSPI::loadedFonts() = 1U << 2;
    TFT_eSPI::drawWidth() = 0;
    Serial.output.clear();
    display.showSetupPassword(password.c_str());
    assert(Serial.output.find("setup heading has zero width") != std::string::npos);
}
