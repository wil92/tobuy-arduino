#pragma once

#include <cstdint>
#include <string>
#include <vector>

#define TFT_BLACK 0x0000
#define TFT_WHITE 0xffff
#define TC_DATUM 1
#define LOAD_FONT2 1

class TFT_eSPI {
public:
    void init() {}
    int32_t width() { return 240; }
    int32_t height() { return 320; }
    uint16_t fontsLoaded() { return loadedFonts(); }
    void fillScreen(uint16_t) {}
    void setTextColor(uint16_t, uint16_t) {}
    void setTextDatum(uint8_t) {}
    void setTextFont(uint8_t) {}
    int16_t drawString(const char *text, int32_t, int32_t) {
        drawnText().push_back(text);
        return drawWidth();
    }

    static uint16_t &loadedFonts() {
        static uint16_t fonts = 1U << 2;
        return fonts;
    }

    static int16_t &drawWidth() {
        static int16_t width = 1;
        return width;
    }

    static std::vector<std::string> &drawnText() {
        static std::vector<std::string> lines;
        return lines;
    }

    static void clearDrawnText() { drawnText().clear(); }
};
