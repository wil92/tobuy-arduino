#pragma once
#include <TFT_eSPI.h>
#include "shopping_client.h"
class ShoppingView {
public:
    ShoppingView(TFT_eSPI &, ShoppingClient &) {}
    int begins = 0;
    int updates = 0;
    void begin(const char *) { ++begins; }
    void update() { ++updates; }
    void setTouchFlipped(bool) {}
    void render() {}
};
