#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#define TFT_BLACK 0
#define TFT_WHITE 1
#define TFT_RED 2
#define TFT_GREEN 3
#define TFT_DARKGREY 4
#define TFT_YELLOW 5
#define TL_DATUM 0
class TFT_eSPI {
public:
    std::vector<std::string> text;
    int32_t width() { return 240; }
    int32_t height() { return 320; }
    void fillScreen(uint16_t) { text.clear(); }
    void setTextColor(uint16_t, uint16_t) {}
    void setTextDatum(uint8_t) {}
    int drawString(const char *value, int, int, int) { text.push_back(value); return 1; }
    int textWidth(const char *value, int) { return std::strlen(value) * 8; }
    void drawLine(int, int, int, int, uint16_t) {}
    void drawFastHLine(int, int, int, uint16_t) {}
    void fillRoundRect(int, int, int, int, int, uint16_t) {}
};
