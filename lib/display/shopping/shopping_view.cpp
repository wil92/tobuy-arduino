#include "shopping_view.h"

#include <Arduino.h>
#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace {
constexpr uint32_t calibration_magic = 0x544f4259;
constexpr int heading_height = 54;
constexpr int footer_height = 40;
constexpr int row_height = 32;
}

ShoppingView::ShoppingView(TFT_eSPI &display, ShoppingClient &client)
    : display_(display), client_(client) {}

void ShoppingView::begin() {
    touchSpi_.begin(TOBUY_TOUCH_SCK, TOBUY_TOUCH_MISO, TOBUY_TOUCH_MOSI, TOBUY_TOUCH_CS);
    if (!touch_.begin(touchSpi_)) {
        Serial.println("Touchscreen initialization failed");
        display_.fillScreen(TFT_BLACK);
        display_.drawString("Touchscreen error", 10, 10, 2);
        return;
    }
    touchReady_ = true;
    if (!preferences_.begin("touch", false)) {
        Serial.println("Could not open touch calibration storage");
    } else if (preferences_.getBytesLength("calibration") == sizeof(calibration_) &&
               preferences_.getBytes("calibration", &calibration_, sizeof(calibration_)) == sizeof(calibration_)) {
        calibrated_ = validCalibration();
    }
    render();
}

bool ShoppingView::validCalibration() const {
    if (calibration_.magic != calibration_magic || calibration_.width != display_.width() ||
        calibration_.height != display_.height()) return false;
    for (unsigned i = 0; i < 3; ++i) {
        if (calibration_.x[i] < 0 || calibration_.x[i] > 4095 ||
            calibration_.y[i] < 0 || calibration_.y[i] > 4095) return false;
    }
    const int32_t dx1 = calibration_.x[1] - calibration_.x[0];
    const int32_t dy1 = calibration_.y[1] - calibration_.y[0];
    const int32_t dx2 = calibration_.x[2] - calibration_.x[0];
    const int32_t dy2 = calibration_.y[2] - calibration_.y[0];
    return std::abs(dx1 * dy2 - dx2 * dy1) > 10000;
}

void ShoppingView::drawCalibration() {
    display_.fillScreen(TFT_BLACK);
    display_.setTextColor(TFT_WHITE, TFT_BLACK);
    display_.setTextDatum(TL_DATUM);
    display_.drawString("Touch the cross", 10, 5, 2);
    display_.drawString(client_.status(), 10, 25, 2);
    const int x = calibrationStep_ == 1 ? display_.width() - 20 : 20;
    const int y = calibrationStep_ == 2 ? display_.height() - 50 : 50;
    display_.drawLine(x - 10, y, x + 10, y, TFT_YELLOW);
    display_.drawLine(x, y - 10, x, y + 10, TFT_YELLOW);
}

size_t ShoppingView::visibleRows() const {
    const int available = display_.height() - heading_height - footer_height;
    return available > 0 ? static_cast<size_t>(available / row_height) : 0;
}

void ShoppingView::render() {
    if (!touchReady_) {
        display_.fillScreen(TFT_BLACK);
        display_.setTextColor(TFT_WHITE, TFT_BLACK);
        display_.setTextDatum(TL_DATUM);
        display_.drawString("Touchscreen error", 10, 10, 2);
        return;
    }
    if (!calibrated_) {
        drawCalibration();
        return;
    }
    const shopping::List &list = client_.list();
    const size_t rows = visibleRows();
    const size_t maximum = list.count > rows ? list.count - rows : 0;
    if (firstRow_ > maximum) firstRow_ = maximum;
    display_.fillScreen(TFT_BLACK);
    display_.setTextDatum(TL_DATUM);
    display_.setTextColor(TFT_WHITE, TFT_BLACK);
    display_.drawString("To buy", 8, 4, 2);
    display_.setTextColor(client_.ready() ? TFT_GREEN : TFT_RED, TFT_BLACK);
    display_.drawString(client_.status(), 8, 27, 2);
    display_.setTextColor(TFT_WHITE, TFT_BLACK);
    if (list.count == 0 && client_.ready()) display_.drawString("No items", 8, heading_height + 8, 2);
    for (size_t row = 0; row < rows && firstRow_ + row < list.count; ++row) {
        const shopping::Item &item = list.items[firstRow_ + row];
        const int y = heading_height + static_cast<int>(row) * row_height;
        char name[shopping::max_name_length + 1];
        std::strcpy(name, item.name);
        while (name[0] && display_.textWidth(name, 2) > display_.width() - 80) {
            name[std::strlen(name) - 1] = '\0';
        }
        display_.drawString(name, 8, y + 6, 2);
        if (item.periodic) {
            const uint16_t color = item.needed ? TFT_GREEN : TFT_DARKGREY;
            display_.fillRoundRect(display_.width() - 64, y + 3, 56, 25, 8, color);
            display_.setTextColor(TFT_WHITE, color);
            display_.drawString(item.needed ? "ON" : "OFF", display_.width() - 56, y + 7, 2);
            display_.setTextColor(TFT_WHITE, TFT_BLACK);
        }
        display_.drawFastHLine(0, y + row_height - 1, display_.width(), TFT_DARKGREY);
    }
    const int footer = display_.height() - footer_height;
    display_.drawString(firstRow_ ? "UP" : "-", 20, footer + 10, 2);
    display_.drawString(firstRow_ < maximum ? "DOWN" : "-", display_.width() - 80, footer + 10, 2);
}

bool ShoppingView::mapTouch(const TS_Point &point, int &x, int &y) const {
    if (!validCalibration()) return false;
    const int32_t dx1 = calibration_.x[1] - calibration_.x[0];
    const int32_t dy1 = calibration_.y[1] - calibration_.y[0];
    const int32_t dx2 = calibration_.x[2] - calibration_.x[0];
    const int32_t dy2 = calibration_.y[2] - calibration_.y[0];
    const int64_t determinant = static_cast<int64_t>(dx1) * dy2 - static_cast<int64_t>(dx2) * dy1;
    const int64_t dx = point.x - calibration_.x[0];
    const int64_t dy = point.y - calibration_.y[0];
    x = 20 + static_cast<int>((dx * dy2 - dy * dx2) * (display_.width() - 40) / determinant);
    y = 50 + static_cast<int>((dy * dx1 - dx * dy1) * (display_.height() - 100) / determinant);
    x = std::max(0, std::min(display_.width() - 1, x));
    y = std::max(0, std::min(display_.height() - 1, y));
    return true;
}

void ShoppingView::update() {
    if (!touchReady_) return;
    const bool down = touch_.touched();
    if (down) {
        const TS_Point point = touch_.getPoint();
        if (!calibrated_) {
            if (!touched_) {
                calibrationPress_ = true;
                calibration_.x[calibrationStep_] = point.x;
                calibration_.y[calibrationStep_] = point.y;
                ++calibrationStep_;
                if (calibrationStep_ == 3) {
                    calibration_.magic = calibration_magic;
                    calibration_.width = display_.width();
                    calibration_.height = display_.height();
                    calibrated_ = validCalibration();
                    if (!calibrated_) {
                        Serial.println("Touch calibration invalid; retrying");
                        calibrationStep_ = 0;
                    } else if (preferences_.putBytes("calibration", &calibration_, sizeof(calibration_)) != sizeof(calibration_)) {
                        Serial.println("Could not save touch calibration");
                    }
                }
                render();
            }
        } else {
            int x, y;
            if (mapTouch(point, x, y)) {
                if (!touched_) {
                    startX_ = x;
                    startY_ = y;
                    touchStartedAt_ = millis();
                }
                lastX_ = x;
                lastY_ = y;
                if (startY_ < 25 && y < 25 &&
                    static_cast<unsigned long>(millis() - touchStartedAt_) >= 3000) {
                    calibrated_ = false;
                    calibrationStep_ = 0;
                    calibrationPress_ = true;
                    render();
                }
            }
        }
    } else if (touched_ && calibrated_ && !calibrationPress_) {
        onRelease(lastX_, lastY_);
    }
    if (!down) calibrationPress_ = false;
    touched_ = down;
}

void ShoppingView::onRelease(int x, int y) {
    const size_t count = client_.list().count;
    const size_t rows = visibleRows();
    if (rows == 0) return;
    const size_t maximum = count > rows ? count - rows : 0;
    const int delta = y - startY_;
    if (std::abs(delta) > 25) {
        const size_t amount = std::max(1, std::abs(delta) / row_height);
        firstRow_ = delta < 0 ? std::min(maximum, firstRow_ + amount) :
                               (amount > firstRow_ ? 0 : firstRow_ - amount);
        render();
    } else if (startY_ >= display_.height() - footer_height && y >= display_.height() - footer_height) {
        if (x < display_.width() / 2 && firstRow_ > 0) --firstRow_;
        else if (x >= display_.width() / 2 && firstRow_ < maximum) ++firstRow_;
        render();
    } else if (startY_ >= heading_height && startY_ < display_.height() - footer_height &&
               std::abs(y - startY_) < 15 && startX_ >= display_.width() - 70 && x >= display_.width() - 70) {
        const size_t row = static_cast<size_t>((startY_ - heading_height) / row_height);
        if (row < rows && static_cast<size_t>((y - heading_height) / row_height) == row) {
            client_.toggle(firstRow_ + row);
        }
    }
}
