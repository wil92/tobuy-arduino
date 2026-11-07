#pragma once

#include <Arduino.h>

class ResetButton {
public:
    explicit ResetButton(int pin) : pin_(pin) {}
    void begin() { pinMode(pin_, INPUT_PULLUP); }
    bool update() {
        if (digitalRead(pin_) == LOW) {
            if (!pressed_) {
                pressed_ = true;
                pressedAt_ = millis();
            } else if (!handled_ && static_cast<unsigned long>(millis() - pressedAt_) >= 3000) {
                handled_ = true;
                return true;
            }
        } else {
            pressed_ = false;
            handled_ = false;
        }
        return false;
    }

private:
    int pin_;
    bool pressed_ = false;
    bool handled_ = false;
    unsigned long pressedAt_ = 0;
};
