#pragma once

#include <cstdint>

class TFT_eSPI {
public:
    void init() {}
    int32_t width() { return 16; }
    int32_t height() { return 16; }
    void fillScreen(uint16_t) {}
    bool getSwapBytes() { return false; }
    void setSwapBytes(bool) {}
    void pushImage(int32_t, int32_t, int32_t, int32_t, uint16_t *) {}
};
