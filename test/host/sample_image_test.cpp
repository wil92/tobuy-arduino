#include <cassert>
#include <cstdint>
#include <vector>

#include "sample_image.h"

struct FakeDisplay {
    int32_t w;
    int32_t h;
    bool swap = false;
    int calls = 0;
    std::vector<uint16_t> framebuffer;

    FakeDisplay(int32_t width, int32_t height)
        : w(width), h(height), framebuffer(width > 0 && height > 0 ? width * height : 0, 0xffff) {}

    int32_t width() const { return w; }
    int32_t height() const { return h; }
    bool getSwapBytes() const { return swap; }
    void setSwapBytes(bool value) { swap = value; }
    void fillScreen(uint16_t color) {
        for (auto &pixel : framebuffer) pixel = color;
    }
    void pushImage(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t *data) {
        assert(swap);
        assert(x >= 0 && y >= 0 && x + width <= w && y + height <= h);
        assert(width > 0 && width <= sample_image::chunk_size && height == 1);
        ++calls;
        for (int32_t i = 0; i < width; ++i) framebuffer[y * w + x + i] = data[i];
    }
    uint16_t at(int32_t x, int32_t y) const { return framebuffer[y * w + x]; }
};

int main() {
    for (const auto &row : sample_image::pixels) {
        for (int x = 0; x < sample_image::width; ++x) {
            assert(row[x] >= '0' && row[x] <= '3');
        }
        assert(row[sample_image::width] == '\0');
    }

    FakeDisplay native(16, 16);
    assert(sample_image::render(native));
    assert(!native.swap);
    assert(native.calls == 16);
    assert(native.at(0, 0) == 0x7dff);
    assert(native.at(5, 2) == 0xffe0);
    assert(native.at(10, 7) == 0x8410);
    assert(native.at(0, 15) == 0x3d84);

    FakeDisplay landscape(160, 128);
    landscape.swap = true;
    assert(sample_image::render(landscape));
    assert(landscape.swap);
    assert(landscape.calls == 256);
    assert(landscape.at(15, 64) == 0);
    assert(landscape.at(16, 0) == 0x7dff);
    assert(landscape.at(56, 16) == 0xffe0);
    assert(landscape.at(143, 127) == 0x3d84);
    assert(landscape.at(144, 64) == 0);

    FakeDisplay portrait(3, 5);
    assert(sample_image::render(portrait));
    assert(portrait.at(0, 0) == 0);
    assert(portrait.at(0, 1) == 0x7dff);
    assert(portrait.at(0, 4) == 0);

    FakeDisplay invalid(0, 16);
    assert(!sample_image::render(invalid));
    assert(invalid.calls == 0);
}
