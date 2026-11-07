#pragma once

#include <TFT_eSPI.h>

class DisplayManager {
public:
    void begin();

private:
    TFT_eSPI display_;
};
