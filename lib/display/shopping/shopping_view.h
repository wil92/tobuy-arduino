#pragma once

#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "shopping_client.h"
#include "../temperature/temperature_client.h"

class ShoppingView {
public:
    ShoppingView(TFT_eSPI &display, ShoppingClient &client);
    void begin(const char *serverUrl);
    void update();
    void render();

private:
    struct Calibration {
        uint32_t magic;
        int16_t width;
        int16_t height;
        int16_t x[3];
        int16_t y[3];
    };
    bool validCalibration() const;
    void drawCalibration();
    void drawStatus();
    void drawTemperature();
    bool mapTouch(const TS_Point &point, int &x, int &y) const;
    void onRelease(int x, int y);
    void updateLayout();
    int viewportHeight() const;
    int maxScroll() const;

    TFT_eSPI &display_;
    ShoppingClient &client_;
    TemperatureClient temperature_;
    SPIClass touchSpi_{HSPI};
    XPT2046_Touchscreen touch_{TOBUY_TOUCH_CS};
    Preferences preferences_;
    Calibration calibration_ = {};
    unsigned calibrationStep_ = 0;
    bool calibrated_ = false;
    bool touchReady_ = false;
    bool touched_ = false;
    bool calibrationPress_ = false;
    bool temperatureVisible_ = false;
    int startX_ = 0;
    int startY_ = 0;
    int lastX_ = 0;
    int lastY_ = 0;
    unsigned long touchStartedAt_ = 0;
    unsigned long lastAnimationAt_ = 0;
    uint8_t animationFrame_ = 0;
    int itemOffsets_[shopping::max_items + 1] = {};
    int scrollOffset_ = 0;
};
