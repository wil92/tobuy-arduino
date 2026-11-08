#pragma once

#include <TFT_eSPI.h>

class DisplayManager {
public:
    void begin();
    void showSetupPassword(const char *password);
    void showError(const char *message);
    TFT_eSPI &screen() { return display_; }

private:
    TFT_eSPI display_;
};
