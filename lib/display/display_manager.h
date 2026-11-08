#pragma once

#include <TFT_eSPI.h>

class DisplayManager {
public:
    void begin();
    void showSetupPassword(const char *password);

private:
    TFT_eSPI display_;
};
