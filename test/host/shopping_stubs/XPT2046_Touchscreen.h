#pragma once
#include "SPI.h"
struct TS_Point { int16_t x, y, z; };
class XPT2046_Touchscreen {
public:
    explicit XPT2046_Touchscreen(int) {}
    static bool down;
    static TS_Point point;
    bool begin(SPIClass &) { return true; }
    bool touched() { return down; }
    TS_Point getPoint() { return point; }
};
