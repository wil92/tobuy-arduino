#include <Arduino.h>
#include <TFT_eSPI.h>

#include "sample_image.h"

TFT_eSPI display;

void setup() {
    Serial.begin(115200);
    display.init();

    if (!sample_image::render(display)) {
        Serial.println("Cannot draw the sample image: invalid display dimensions");
    }
}

void loop() {
}
