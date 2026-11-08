#include "shopping_view.h"

#include <Arduino.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
constexpr uint32_t calibration_magic = 0x544f4259;
constexpr int heading_height = 30;
constexpr int footer_height = 24;
constexpr int temperature_location_area = 144;
constexpr int row_height = 50;
constexpr int switch_x = 8;
constexpr int switch_y_offset = 4;
constexpr int switch_width = 35;
constexpr int switch_height = 40;
constexpr int item_text_x = switch_x + switch_width + 10;
constexpr int item_font = 1;
constexpr int scrollbar_width = 8;
constexpr int scrollbar_margin = 4;
constexpr uint16_t background_color = 0x0862;
constexpr uint16_t card_color = 0x1944;
constexpr uint16_t separator_color = 0x3186;
constexpr uint16_t muted_color = 0xb5b6;
constexpr uint16_t accent_color = 0x4f32;
constexpr uint16_t warning_color = 0xfce6;
constexpr uint16_t error_color = 0xfbaa;
constexpr uint16_t switch_on_color = 0x03c5;
constexpr uint16_t switch_off_color = 0x4209;
constexpr uint16_t remove_color = 0xb904;
constexpr unsigned long animation_interval_ms = 350;
constexpr int page_swipe_distance = 50;

enum class Accent : uint8_t {
    none,
    acute,
    grave,
    circumflex,
    tilde,
    diaeresis
};

struct AccentMark {
    uint8_t character;
    Accent accent;
};

struct StatusPresentation {
    const char *label;
    uint16_t color;
    bool animated;
};

char displayCharacter(uint32_t codepoint, Accent &accent) {
    accent = Accent::none;
    const int caseOffset = codepoint >= 0x00c0 && codepoint <= 0x00de ? 'A' - 'a' : 0;
    switch (codepoint) {
        case 0x00c0: case 0x00e0: accent = Accent::grave; return static_cast<char>('a' + caseOffset);
        case 0x00c1: case 0x00e1: accent = Accent::acute; return static_cast<char>('a' + caseOffset);
        case 0x00c2: case 0x00e2: accent = Accent::circumflex; return static_cast<char>('a' + caseOffset);
        case 0x00c3: case 0x00e3: accent = Accent::tilde; return static_cast<char>('a' + caseOffset);
        case 0x00c4: case 0x00e4: accent = Accent::diaeresis; return static_cast<char>('a' + caseOffset);
        case 0x00c8: case 0x00e8: accent = Accent::grave; return static_cast<char>('e' + caseOffset);
        case 0x00c9: case 0x00e9: accent = Accent::acute; return static_cast<char>('e' + caseOffset);
        case 0x00ca: case 0x00ea: accent = Accent::circumflex; return static_cast<char>('e' + caseOffset);
        case 0x00cc: case 0x00ec: accent = Accent::grave; return static_cast<char>('i' + caseOffset);
        case 0x00cd: case 0x00ed: accent = Accent::acute; return static_cast<char>('i' + caseOffset);
        case 0x00ce: case 0x00ee: accent = Accent::circumflex; return static_cast<char>('i' + caseOffset);
        case 0x00d1: case 0x00f1: accent = Accent::tilde; return static_cast<char>('n' + caseOffset);
        case 0x00d2: case 0x00f2: accent = Accent::grave; return static_cast<char>('o' + caseOffset);
        case 0x00d3: case 0x00f3: accent = Accent::acute; return static_cast<char>('o' + caseOffset);
        case 0x00d4: case 0x00f4: accent = Accent::circumflex; return static_cast<char>('o' + caseOffset);
        case 0x00d5: case 0x00f5: accent = Accent::tilde; return static_cast<char>('o' + caseOffset);
        case 0x00d6: case 0x00f6: accent = Accent::diaeresis; return static_cast<char>('o' + caseOffset);
        case 0x00d9: case 0x00f9: accent = Accent::grave; return static_cast<char>('u' + caseOffset);
        case 0x00da: case 0x00fa: accent = Accent::acute; return static_cast<char>('u' + caseOffset);
        case 0x00db: case 0x00fb: accent = Accent::circumflex; return static_cast<char>('u' + caseOffset);
        case 0x00dc: case 0x00fc: accent = Accent::diaeresis; return static_cast<char>('u' + caseOffset);
        case 0x00dd: case 0x00fd: accent = Accent::acute; return static_cast<char>('y' + caseOffset);
        default: return '?';
    }
}

bool continuationByte(uint8_t byte) {
    return (byte & 0xc0) == 0x80;
}

size_t decodeCodepoint(const char *text, uint32_t &codepoint) {
    const uint8_t first = static_cast<uint8_t>(text[0]);
    codepoint = first;
    if (first < 0x80) return 1;
    if (first >= 0xc2 && first <= 0xdf && text[1] && continuationByte(text[1])) {
        codepoint = ((first & 0x1f) << 6) | (static_cast<uint8_t>(text[1]) & 0x3f);
        return 2;
    }
    if (first >= 0xe0 && first <= 0xef && text[1] && text[2] &&
        continuationByte(text[1]) && continuationByte(text[2])) {
        const uint8_t second = static_cast<uint8_t>(text[1]);
        if ((first != 0xe0 || second >= 0xa0) && (first != 0xed || second < 0xa0)) {
            codepoint = ((first & 0x0f) << 12) | ((second & 0x3f) << 6) |
                        (static_cast<uint8_t>(text[2]) & 0x3f);
            return 3;
        }
    }
    if (first >= 0xf0 && first <= 0xf4 && text[1] && text[2] && text[3] &&
        continuationByte(text[1]) && continuationByte(text[2]) && continuationByte(text[3])) {
        const uint8_t second = static_cast<uint8_t>(text[1]);
        if ((first != 0xf0 || second >= 0x90) && (first != 0xf4 || second < 0x90)) {
            codepoint = ((first & 0x07) << 18) | ((second & 0x3f) << 12) |
                        ((static_cast<uint8_t>(text[2]) & 0x3f) << 6) |
                        (static_cast<uint8_t>(text[3]) & 0x3f);
            return 4;
        }
    }
    codepoint = '?';
    return 1;
}

bool toDisplayText(const char *text, char *output, size_t outputSize,
                   AccentMark *marks, size_t markCapacity, size_t &markCount) {
    size_t outputLength = 0;
    markCount = 0;
    while (*text) {
        uint32_t codepoint;
        const size_t consumed = decodeCodepoint(text, codepoint);
        text += consumed;
        if (codepoint >= 0x20 && codepoint < 0x7f) {
            if (outputLength + 1 >= outputSize) return false;
            output[outputLength++] = static_cast<char>(codepoint);
            continue;
        }
        if (codepoint == 0x00df || codepoint == 0x1e9e) {
            if (outputLength + 2 >= outputSize) return false;
            output[outputLength++] = codepoint == 0x00df ? 's' : 'S';
            output[outputLength++] = codepoint == 0x00df ? 's' : 'S';
            continue;
        }
        Accent accent;
        const char character = displayCharacter(codepoint, accent);
        if (outputLength + 1 >= outputSize) return false;
        if (accent != Accent::none && marks) {
            if (markCount >= markCapacity) return false;
            marks[markCount++] = {static_cast<uint8_t>(outputLength), accent};
        }
        output[outputLength++] = character;
    }
    output[outputLength] = '\0';
    return true;
}

void drawAccent(TFT_eSPI &display, const AccentMark &mark, int x, int y, int scale, uint16_t color) {
    switch (mark.accent) {
        case Accent::acute:
            display.drawLine(x - scale, y + 4 * scale, x + scale, y + scale, color);
            break;
        case Accent::grave:
            display.drawLine(x - scale, y + scale, x + scale, y + 4 * scale, color);
            break;
        case Accent::circumflex:
            display.drawLine(x - 3 * scale, y + 4 * scale, x, y + scale, color);
            display.drawLine(x, y + scale, x + 3 * scale, y + 4 * scale, color);
            break;
        case Accent::tilde:
            display.drawLine(x - 3 * scale, y + 2 * scale, x - scale, y + scale, color);
            display.drawLine(x - scale, y + scale, x + scale, y + 2 * scale, color);
            display.drawLine(x + scale, y + 2 * scale, x + 3 * scale, y + scale, color);
            break;
        case Accent::diaeresis:
            display.drawLine(x - 3 * scale, y + scale, x - 2 * scale, y + scale, color);
            display.drawLine(x + 2 * scale, y + scale, x + 3 * scale, y + scale, color);
            break;
        case Accent::none:
            break;
    }
}

int supportedTextWidth(TFT_eSPI &display, const char *text, int font) {
    char converted[temperature::max_place_length + 1];
    size_t markCount;
    if (!toDisplayText(text, converted, sizeof(converted), nullptr, 0, markCount)) {
        return display.textWidth("?", font);
    }
    return display.textWidth(converted, font);
}

void drawSupportedText(TFT_eSPI &display, const char *text, int x, int y, int font, int scale = 1,
                       uint16_t accentColor = TFT_WHITE) {
    char converted[temperature::max_place_length + 1];
    AccentMark marks[temperature::max_place_length];
    size_t markCount;
    if (!toDisplayText(text, converted, sizeof(converted), marks, temperature::max_place_length, markCount)) {
        Serial.println("Display text exceeds supported character buffer");
        return;
    }
    display.drawString(converted, x, y, font);
    for (size_t i = 0; i < markCount; ++i) {
        const size_t character = marks[i].character;
        const char saved = converted[character];
        converted[character] = '\0';
        const int prefixWidth = display.textWidth(converted, font);
        converted[character] = saved;
        char glyph[] = {saved, '\0'};
        const int glyphWidth = display.textWidth(glyph, font);
        drawAccent(display, marks[i], x + prefixWidth + glyphWidth / 2, y, scale, accentColor);
    }
}

StatusPresentation statusPresentation(const char *status) {
    if (std::strcmp(status, "Connected") == 0) return {"Conectado", accent_color, false};
    if (std::strcmp(status, "Connecting to Wi-Fi") == 0) return {"Conectando Wi-Fi", warning_color, true};
    if (std::strcmp(status, "Wi-Fi unavailable") == 0) return {"Wi-Fi sin conexion", error_color, true};
    if (std::strcmp(status, "Connecting to server") == 0) return {"Conectando...", warning_color, true};
    if (std::strcmp(status, "Server unavailable") == 0) return {"Sin servidor", error_color, true};
    if (std::strcmp(status, "Loading list") == 0) return {"Cargando lista", warning_color, true};
    if (std::strcmp(status, "Waiting for server") == 0) return {"Guardando", warning_color, true};
    if (std::strcmp(status, "Update not confirmed") == 0) return {"Tiempo agotado", error_color, false};
    if (std::strcmp(status, "Update send failed") == 0) return {"Error al enviar", error_color, false};
    if (std::strcmp(status, "Invalid server message") == 0) return {"Mensaje invalido", error_color, false};
    if (std::strcmp(status, "Invalid server response") == 0) return {"Respuesta invalida", error_color, false};
    if (std::strcmp(status, "Server needs http URL") == 0) return {"Requiere HTTP", error_color, false};
    if (std::strcmp(status, "Invalid server URL") == 0) return {"URL invalida", error_color, false};
    if (std::strcmp(status, "Out of memory") == 0) return {"Sin memoria", error_color, false};
    return {"Error del servidor", error_color, false};
}

bool nextLine(TFT_eSPI &display, const char *name, size_t &offset, int availableWidth, char *line) {
    while (name[offset] == ' ') ++offset;
    if (!name[offset]) return false;
    const size_t start = offset;
    size_t end = start;
    size_t lastSpace = start;
    while (name[end] && name[end] != '\n') {
        size_t next = end + 1;
        while (name[next] && (static_cast<unsigned char>(name[next]) & 0xc0) == 0x80) ++next;
        const size_t length = next - start;
        std::memcpy(line, name + start, length);
        line[length] = '\0';
        if (supportedTextWidth(display, line, item_font) > availableWidth && end > start) break;
        if (name[end] == ' ') lastSpace = end;
        end = next;
    }
    if (name[end] && name[end] != '\n' && lastSpace > start) end = lastSpace;
    size_t length = end - start;
    while (length && name[start + length - 1] == ' ') --length;
    std::memcpy(line, name + start, length);
    line[length] = '\0';
    offset = end;
    if (name[offset] == '\n') ++offset;
    return true;
}
}

ShoppingView::ShoppingView(TFT_eSPI &display, ShoppingClient &client)
    : display_(display), client_(client) {}

void ShoppingView::begin(const char *serverUrl) {
    temperature_.begin(serverUrl);
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

void ShoppingView::drawTemperature() {
    const uint16_t color = temperature_.hasReading()
                               ? temperature::background(temperature_.reading().celsius)
                               : background_color;
    display_.fillScreen(color);
    display_.setTextDatum(TL_DATUM);
    if (temperature_.hasReading()) {
        char number[16];
        std::snprintf(number, sizeof(number), "%.1f", temperature_.reading().celsius);
        display_.setFreeFont(&FreeSansBold24pt7b);
        int size = 3;
        int numberWidth, unitWidth, degreeWidth;
        for (;;) {
            display_.setTextSize(size);
            numberWidth = display_.textWidth(number, 1);
            unitWidth = display_.textWidth("C", 1);
            degreeWidth = 9 * size;
            if (size == 1 || (numberWidth + degreeWidth + unitWidth <= display_.width() - 12 &&
                              display_.fontHeight(1) <= display_.height() - heading_height - temperature_location_area)) break;
            --size;
        }
        const int left = (display_.width() - numberWidth - degreeWidth - unitWidth) / 2;
        const int y = heading_height + (display_.height() - heading_height - temperature_location_area - display_.fontHeight(1)) / 2;
        display_.setTextColor(TFT_WHITE, color);
        display_.drawString(number, left, y, 1);
        display_.drawCircle(left + numberWidth + 4 * size, y + 5 * size, 3 * size, TFT_WHITE);
        display_.drawString("C", left + numberWidth + degreeWidth, y, 1);
        display_.setTextFont(2);
        display_.setTextSize(4);
        const int locationTop = display_.height() - temperature_location_area;
        char time[6];
        temperature_.formatTime(time);
        display_.drawString(time, (display_.width() - display_.textWidth(time, 2)) / 2, locationTop, 2);
        const int cityTop = locationTop + display_.fontHeight(2) + 8;
        display_.setTextSize(1);
        const int locationLineHeight = display_.fontHeight(2);
        display_.setViewport(6, cityTop, display_.width() - 12, locationLineHeight * 2, false);
        drawSupportedText(display_, temperature_.reading().city, 6, cityTop, 2, 1);
        drawSupportedText(display_, temperature_.reading().country, 6, cityTop + locationLineHeight, 2, 1);
        display_.resetViewport();
    }
    display_.setTextSize(1);
    display_.setTextFont(2);
    display_.setTextColor(TFT_WHITE, color);
    display_.drawString(temperature_.status(), 8, display_.height() - 22, 2);
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

int ShoppingView::viewportHeight() const {
    const int available = display_.height() - heading_height - footer_height;
    return std::max(0, available);
}

int ShoppingView::maxScroll() const {
    return std::max(0, itemOffsets_[client_.list().count] - viewportHeight());
}

void ShoppingView::updateLayout() {
    const shopping::List &list = client_.list();
    const int textWidth = std::max(1, display_.width() - item_text_x - scrollbar_width - 2 * scrollbar_margin);
    display_.setTextSize(1);
    display_.setFreeFont(&FreeSans9pt7b);
    const int lineHeight = display_.fontHeight(item_font);
    itemOffsets_[0] = 0;
    for (size_t i = 0; i < list.count; ++i) {
        size_t offset = 0;
        int lines = 0;
        char line[shopping::max_name_length + 1];
        while (nextLine(display_, list.items[i].name, offset, textWidth, line)) ++lines;
        itemOffsets_[i + 1] = itemOffsets_[i] + std::max(row_height, lines * lineHeight + 10);
    }
    scrollOffset_ = std::min(scrollOffset_, maxScroll());
    display_.setTextFont(2);
}

void ShoppingView::drawStatus() {
    const StatusPresentation status = statusPresentation(client_.status());
    display_.setTextSize(1);
    display_.setTextFont(2);
    display_.setTextDatum(TL_DATUM);
    const int left = 73;
    const int right = display_.width() - 8;
    const int iconWidth = 12;
    display_.fillRect(left, 0, display_.width() - left, heading_height - 2, background_color);
    display_.setViewport(left + iconWidth, 0, right - left - iconWidth, heading_height - 2, false);
    display_.setTextColor(status.color, background_color);
    const int x = std::max(left + iconWidth, right - display_.textWidth(status.label, 2));
    display_.drawString(status.label, x, 6, 2);
    display_.resetViewport();
    const int centerX = x - 9;
    const int centerY = 14;
    if (status.animated) {
        static const int8_t dx[] = {0, 4, 0, -4};
        static const int8_t dy[] = {-4, 0, 4, 0};
        for (unsigned i = 0; i < 4; ++i) {
            display_.fillCircle(centerX + dx[i], centerY + dy[i], 2,
                                i == animationFrame_ ? status.color : separator_color);
        }
    } else {
        display_.fillCircle(centerX, centerY, 3, status.color);
    }
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
    if (temperatureVisible_) {
        drawTemperature();
        return;
    }
    const shopping::List &list = client_.list();
    updateLayout();
    const int viewport = viewportHeight();
    const int maximum = maxScroll();
    display_.fillScreen(background_color);
    display_.setTextDatum(TL_DATUM);
    display_.setTextSize(1);
    display_.setTextColor(TFT_WHITE, background_color);
    display_.drawString("Comprar", 8, 6, 2);
    drawStatus();
    display_.drawFastHLine(8, heading_height - 2, display_.width() - 16, separator_color);
    display_.setTextColor(muted_color, background_color);
    if (list.count == 0 && client_.ready()) display_.drawString("No items", 12, heading_height + 14, 2);
    size_t firstVisible = list.count;
    size_t lastVisible = 0;
    display_.setViewport(0, heading_height, display_.width(), viewport, false);
    for (size_t i = 0; i < list.count; ++i) {
        const int y = heading_height + itemOffsets_[i] - scrollOffset_;
        const int height = itemOffsets_[i + 1] - itemOffsets_[i];
        if (itemOffsets_[i + 1] <= scrollOffset_ || itemOffsets_[i] >= scrollOffset_ + viewport) continue;
        if (firstVisible == list.count) firstVisible = i;
        lastVisible = i + 1;
        const shopping::Item &item = list.items[i];
        display_.fillRoundRect(4, y + 1, display_.width() - scrollbar_width - 3 * scrollbar_margin, height - 3, 5, card_color);
        display_.setFreeFont(&FreeSans9pt7b);
        display_.setTextColor(TFT_WHITE, TFT_WHITE);
        const int lineHeight = display_.fontHeight(item_font);
        size_t offset = 0;
        int lineY = y + (height == row_height ? (row_height - lineHeight) / 2 : 5);
        char line[shopping::max_name_length + 1];
        const int textWidth = std::max(1, display_.width() - item_text_x - scrollbar_width - 2 * scrollbar_margin);
        while (nextLine(display_, item.name, offset, textWidth, line)) {
            drawSupportedText(display_, line, item_text_x, lineY, item_font);
            lineY += lineHeight;
        }
        display_.setTextFont(2);
        const uint16_t color = item.periodic ? (item.needed ? switch_on_color : switch_off_color) : remove_color;
        const char *label = item.periodic ? (item.needed ? "SI" : "NO") : "X";
        display_.fillRoundRect(switch_x, y + switch_y_offset, switch_width, switch_height, 8, color);
        display_.setTextColor(TFT_WHITE, color);
        display_.drawString(label, switch_x + (switch_width - display_.textWidth(label, 2)) / 2,
                            y + switch_y_offset + (switch_height - display_.fontHeight(2)) / 2, 2);
    }
    display_.resetViewport();
    if (maximum > 0 && viewport > 0) {
        const int trackX = display_.width() - scrollbar_width - scrollbar_margin;
        const int total = itemOffsets_[list.count];
        const int thumbHeight = std::max(16, viewport * viewport / total);
        const int thumbY = heading_height + (viewport - thumbHeight) * scrollOffset_ / maximum;
        display_.fillRoundRect(trackX, heading_height, scrollbar_width, viewport, 4, separator_color);
        display_.fillRoundRect(trackX, thumbY, scrollbar_width, thumbHeight, 4, muted_color);
    }
    const int footer = display_.height() - footer_height;
    char position[24];
    if (list.count && firstVisible < list.count) {
        std::snprintf(position, sizeof(position), "%u-%u/%u",
                      static_cast<unsigned>(firstVisible + 1),
                      static_cast<unsigned>(lastVisible),
                      static_cast<unsigned>(list.count));
        display_.setTextColor(muted_color, background_color);
        display_.drawString(position, (display_.width() - display_.textWidth(position, 2)) / 2,
                            footer + 4, 2);
    }
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
    if (temperature_.update(calibrated_ && temperatureVisible_) && calibrated_ && temperatureVisible_) render();
    if (calibrated_ && !temperatureVisible_ && statusPresentation(client_.status()).animated &&
        static_cast<unsigned long>(millis() - lastAnimationAt_) >= animation_interval_ms) {
        lastAnimationAt_ = millis();
        animationFrame_ = (animationFrame_ + 1) % 4;
        drawStatus();
    }
    const bool down = touch_.touched();
    if (down) {
        TS_Point point = touch_.getPoint();
        if (touchFlipped_) {
            // Normalize raw readings to the default orientation so calibration stays valid.
            point.x = 4095 - point.x;
            point.y = 4095 - point.y;
        }
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
                if (!temperatureVisible_ && startX_ < 73 && x < 73 && startY_ < 25 && y < 25 &&
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
    const int horizontal = std::abs(x - startX_);
    const int vertical = std::abs(y - startY_);
    if (horizontal >= page_swipe_distance && horizontal > vertical) {
        temperatureVisible_ = !temperatureVisible_;
        temperature_.update(temperatureVisible_);
        render();
        return;
    }
    if (temperatureVisible_) return;
    if (horizontal >= 15 && vertical <= 25) return;
    const size_t count = client_.list().count;
    const int viewport = viewportHeight();
    if (viewport == 0) return;
    const int maximum = maxScroll();
    const int delta = y - startY_;
    if (maximum > 0 && startX_ >= display_.width() - scrollbar_width - 3 * scrollbar_margin &&
        startY_ >= heading_height && startY_ < heading_height + viewport) {
        const int thumbHeight = std::max(16, viewport * viewport / itemOffsets_[count]);
        const int travel = viewport - thumbHeight;
        if (travel > 0) {
            const int thumbTop = (travel * scrollOffset_) / maximum;
            if (startY_ - heading_height >= thumbTop && startY_ - heading_height < thumbTop + thumbHeight) {
                scrollOffset_ = std::max(0, std::min(maximum, scrollOffset_ + delta * maximum / travel));
            } else {
                scrollOffset_ = std::max(0, std::min(maximum,
                    (y - heading_height - thumbHeight / 2) * maximum / travel));
            }
        }
        render();
    } else if (std::abs(delta) > 25) {
        scrollOffset_ = std::max(0, std::min(maximum, scrollOffset_ - delta));
        render();
    } else if (startY_ >= heading_height && startY_ < display_.height() - footer_height &&
               std::abs(y - startY_) < 15 && horizontal < 15 &&
               startX_ >= switch_x && startX_ < switch_x + switch_width &&
               x >= switch_x && x < switch_x + switch_width) {
        for (size_t i = 0; i < count; ++i) {
            const int switchTop = heading_height + itemOffsets_[i] - scrollOffset_ + switch_y_offset;
            if (startY_ >= switchTop && startY_ < switchTop + switch_height &&
                y >= switchTop && y < switchTop + switch_height &&
                y >= heading_height && y < heading_height + viewport) {
                if (client_.list().items[i].periodic) client_.toggle(i);
                else client_.removeMissing(i);
                break;
            }
        }
    }
}
