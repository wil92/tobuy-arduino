#include "display_manager.h"

#include <Arduino.h>
#include "sample_image.h"

void DisplayManager::begin() {
    display_.init();
    if (!sample_image::render(display_)) {
        Serial.println("Cannot draw the sample image: invalid display dimensions");
    }
}
