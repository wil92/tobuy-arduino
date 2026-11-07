#ifndef SAMPLE_IMAGE_H
#define SAMPLE_IMAGE_H

#include <stdint.h>

namespace sample_image {

constexpr int32_t width = 16;
constexpr int32_t height = 16;
constexpr int32_t chunk_size = 64;

// Palette-indexed landscape: sky, sun, mountains, and grass.
constexpr char pixels[height][width + 1] = {
    "0000000000000000",
    "0000111000000000",
    "0001111100000000",
    "0001111100000000",
    "0000111000000000",
    "0000000000200000",
    "0000000002220000",
    "0000200022222000",
    "0002220222222200",
    "0022222222222220",
    "0222222222222222",
    "2222222222222222",
    "3333333333333333",
    "3333333333333333",
    "3333333333333333",
    "3333333333333333",
};

constexpr uint16_t palette[] = {
    0x7DFF, // Sky
    0xFFE0, // Sun
    0x8410, // Mountains
    0x3D84, // Grass
};

template <typename Display>
bool render(Display &display) {
    const int32_t display_width = display.width();
    const int32_t display_height = display.height();
    if (display_width <= 0 || display_height <= 0) {
        return false;
    }

    const int32_t size = display_width < display_height ? display_width : display_height;
    const int32_t left = (display_width - size) / 2;
    const int32_t top = (display_height - size) / 2;
    uint16_t buffer[chunk_size];

    display.fillScreen(0x0000);
    const bool previous_swap = display.getSwapBytes();
    display.setSwapBytes(true);
    for (int32_t y = 0; y < size; ++y) {
        const int32_t source_y = y * height / size;
        for (int32_t x = 0; x < size; x += chunk_size) {
            const int32_t count = size - x < chunk_size ? size - x : chunk_size;
            for (int32_t i = 0; i < count; ++i) {
                const int32_t source_x = (x + i) * width / size;
                buffer[i] = palette[pixels[source_y][source_x] - '0'];
            }
            display.pushImage(left + x, top + y, count, 1, buffer);
        }
    }
    display.setSwapBytes(previous_swap);
    return true;
}

} // namespace sample_image

#endif
