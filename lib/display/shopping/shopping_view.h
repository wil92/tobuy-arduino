#pragma once

#include <Preferences.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#include "shopping_client.h"

class ShoppingView {
public:
    ShoppingView(TFT_eSPI &display, ShoppingClient &client);
    void begin();
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
    bool mapTouch(const TS_Point &point, int &x, int &y) const;
    void onRelease(int x, int y);
    size_t visibleRows() const;

    TFT_eSPI &display_;
    ShoppingClient &client_;
    SPIClass touchSpi_{HSPI};
    XPT2046_Touchscreen touch_{TOBUY_TOUCH_CS};
    Preferences preferences_;
    Calibration calibration_ = {};
    unsigned calibrationStep_ = 0;
    bool calibrated_ = false;
    bool touchReady_ = false;
    bool touched_ = false;
    bool calibrationPress_ = false;
    int startX_ = 0;
    int startY_ = 0;
    int lastX_ = 0;
    int lastY_ = 0;
    unsigned long touchStartedAt_ = 0;
    size_t firstRow_ = 0;
};
