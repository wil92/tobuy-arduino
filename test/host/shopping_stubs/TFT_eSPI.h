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
    struct TextDraw {
        std::string value;
        int x;
        int y;
        int font;
        int size;
    };
    struct Rectangle {
        int x;
        int y;
        int width;
        int height;
    };
    std::vector<std::string> text;
    std::vector<TextDraw> textDraws;
    std::vector<Rectangle> rectangles;
    int textSize = 1;
    int32_t width() { return 240; }
    int32_t height() { return 320; }
    void fillScreen(uint16_t) { text.clear(); textDraws.clear(); rectangles.clear(); }
    void setTextColor(uint16_t, uint16_t) {}
    void setTextDatum(uint8_t) {}
    void setTextSize(uint8_t size) { textSize = size; }
    int drawString(const char *value, int x, int y, int font) {
        text.push_back(value);
        textDraws.push_back({value, x, y, font, textSize});
        return 1;
    }
    int textWidth(const char *value, int) { return std::strlen(value) * 8 * textSize; }
    int fontHeight(int) { return 16 * textSize; }
    void drawLine(int, int, int, int, uint16_t) {}
    void drawFastHLine(int, int, int, uint16_t) {}
    void fillRoundRect(int x, int y, int width, int height, int, uint16_t) {
        rectangles.push_back({x, y, width, height});
    }
};
