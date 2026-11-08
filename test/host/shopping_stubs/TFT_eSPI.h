#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <Fonts/GFXFF/FreeSans9pt7b.h>
#include <Fonts/GFXFF/FreeSansBold24pt7b.h>
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
        uint16_t color;
        uint16_t background;
    };
    struct Rectangle {
        int x;
        int y;
        int width;
        int height;
        uint16_t color;
    };
    struct Circle { int x; int y; int radius; uint16_t color; bool filled; };
    std::vector<std::string> text;
    std::vector<TextDraw> textDraws;
    std::vector<Rectangle> rectangles;
    std::vector<Circle> circles;
    int textSize = 1;
    bool freeFont = false;
    bool largeFont = false;
    uint16_t foreground = TFT_WHITE;
    uint16_t background = TFT_BLACK;
    uint16_t screenColor = TFT_BLACK;
    int viewportTop = 0;
    int viewportBottom = 320;
    int32_t width() { return 240; }
    int32_t height() { return 320; }
    void fillScreen(uint16_t color) {
        screenColor = color;
        text.clear(); textDraws.clear(); rectangles.clear(); circles.clear();
    }
    void setTextColor(uint16_t color, uint16_t back) { foreground = color; background = back; }
    void setTextDatum(uint8_t) {}
    void setTextSize(uint8_t size) { textSize = size; }
    void setFreeFont(const GFXfont *font) { freeFont = true; largeFont = font->size == 24; }
    void setTextFont(uint8_t) { freeFont = false; largeFont = false; }
    int drawString(const char *value, int x, int y, int font) {
        if (y >= viewportBottom || y + fontHeight(font) <= viewportTop) return 0;
        text.push_back(value);
        textDraws.push_back({value, x, y, font, textSize, foreground, background});
        return 1;
    }
    int textWidth(const char *value, int font) { return std::strlen(value) * (font == 1 && freeFont ? (largeFont ? 26 : 10) : 8) * textSize; }
    int fontHeight(int font) { return (font == 1 && freeFont ? (largeFont ? 52 : 22) : 16) * textSize; }
    void setViewport(int, int y, int, int height, bool) { viewportTop = y; viewportBottom = y + height; }
    void resetViewport() { viewportTop = 0; viewportBottom = height(); }
    void drawLine(int, int, int, int, uint16_t) {}
    void drawFastHLine(int, int, int, uint16_t) {}
    void fillRect(int x, int y, int width, int height, uint16_t) {
        auto overlaps = [=](int left, int top, int right, int bottom) {
            return left < x + width && right > x && top < y + height && bottom > y;
        };
        for (auto it = textDraws.begin(); it != textDraws.end();) {
            const int charWidth = it->font == 1 ? 10 : 8;
            const int lineHeight = it->font == 1 ? 22 : 16;
            if (overlaps(it->x, it->y, it->x + static_cast<int>(it->value.size()) * charWidth * it->size,
                         it->y + lineHeight * it->size)) {
                for (auto textIt = text.begin(); textIt != text.end(); ++textIt) {
                    if (*textIt == it->value) { text.erase(textIt); break; }
                }
                it = textDraws.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = circles.begin(); it != circles.end();) {
            if (overlaps(it->x - it->radius, it->y - it->radius,
                         it->x + it->radius + 1, it->y + it->radius + 1)) it = circles.erase(it);
            else ++it;
        }
    }
    void fillRoundRect(int x, int y, int width, int height, int, uint16_t color) {
        rectangles.push_back({x, y, width, height, color});
    }
    void fillCircle(int x, int y, int radius, uint16_t color) { circles.push_back({x, y, radius, color, true}); }
    void drawCircle(int x, int y, int radius, uint16_t color) { circles.push_back({x, y, radius, color, false}); }
};
