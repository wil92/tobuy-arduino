#include <algorithm>
#include <cassert>
#include <cstring>
#include <string>
#include <vector>

#include "HTTPClient.h"
#include "shopping_view.h"

FakeSerial Serial;
FakeWiFi WiFi;
unsigned long fakeMillis = 0;
WebSocketsClient *WebSocketsClient::instance = nullptr;
int FakeHttp::code = 200;
std::string FakeHttp::body;
std::string FakeHttp::url;
std::vector<char> Preferences::saved;
bool XPT2046_Touchscreen::down = false;
TS_Point XPT2046_Touchscreen::point = {};
ShoppingView *activeView = nullptr;

static void redraw() {
    if (activeView) activeView->render();
}

static std::string list(unsigned revision, bool needed, unsigned count = 10) {
    std::string result = R"({"type":"list","revision":)" + std::to_string(revision) +
                         R"(,"periodic":[{"id":"milk","name":"Milk","needed":)" +
                         (needed ? "true" : "false") + R"(}],"missing":[)";
    for (unsigned i = 1; i < count; ++i) {
        if (i != 1) result += ',';
        result += R"({"id":"item)" + std::to_string(i) + R"(","name":"Item)" +
                  std::to_string(i) + R"("})";
    }
    return result + "]}";
}

static bool hasText(TFT_eSPI &display, const char *text) {
    return std::find(display.text.begin(), display.text.end(), text) != display.text.end();
}

static uint16_t textColor(TFT_eSPI &display, const char *text) {
    const auto draw = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                   [text](const TFT_eSPI::TextDraw &value) { return value.value == text; });
    assert(draw != display.textDraws.end());
    return draw->color;
}

static void touch(ShoppingView &view, int x, int y) {
    XPT2046_Touchscreen::point = {static_cast<int16_t>(x * 10), static_cast<int16_t>(y * 10), 100};
    XPT2046_Touchscreen::down = true;
    view.update();
    XPT2046_Touchscreen::down = false;
    view.update();
}

static void swipe(ShoppingView &view, int startX, int startY, int endX, int endY) {
    XPT2046_Touchscreen::point = {static_cast<int16_t>(startX * 10), static_cast<int16_t>(startY * 10), 100};
    XPT2046_Touchscreen::down = true;
    view.update();
    XPT2046_Touchscreen::point = {static_cast<int16_t>(endX * 10), static_cast<int16_t>(endY * 10), 100};
    view.update();
    XPT2046_Touchscreen::down = false;
    view.update();
}

int main() {
    ShoppingClient client;
    client.begin("http://server:3000", redraw);
    client.update();
    assert(!client.ready() && std::strcmp(client.status(), "Wi-Fi unavailable") == 0);
    WiFi.currentStatus = WL_CONNECTED;
    client.update();
    WebSocketsClient &socket = *WebSocketsClient::instance;
    assert(socket.host == "server" && socket.port == 3000 && socket.path == "/ws");
    fakeMillis += 5000;
    client.update();
    assert(!client.ready() && std::strcmp(client.status(), "Server unavailable") == 0);
    FakeHttp::body = list(10, false);
    socket.event(WStype_CONNECTED);
    client.update();
    assert(client.ready() && FakeHttp::url == "http://server:3000/tobuy");
    assert(client.list().count == 10 && !client.list().items[0].needed);

    TFT_eSPI display;
    ShoppingView view(display, client);
    activeView = &view;
    view.begin("http://server:3000");
    assert(hasText(display, "Touch the cross"));
    touch(view, 20, 50);
    touch(view, 220, 50);
    touch(view, 20, 270);

    touch(view, 120, 40);
    assert(hasText(display, "Milk") && !hasText(display, "Loading temperature"));
    swipe(view, 30, 40, 79, 40);
    assert(hasText(display, "Milk") && socket.sent.empty());
    swipe(view, 230, 55, 190, 55);
    assert(hasText(display, "1-5/10") && socket.sent.empty());
    swipe(view, 120, 200, 180, 120);
    assert(hasText(display, "TO BUY") && hasText(display, "Item5"));
    touch(view, 230, 55);
    assert(hasText(display, "Milk"));
    swipe(view, 30, 40, 120, 45);
    assert(hasText(display, "Loading temperature") && socket.sent.empty());
    FakeHttp::body = R"({"city":"London","country":"United Kingdom","temperature":12.3,"unit":"°C"})";
    runFakeTask(0);
    view.update();
    assert(FakeHttp::url == "http://server:3000/temperature");
    assert(hasText(display, "12.3") && hasText(display, "C") && hasText(display, "London"));
    const uint16_t coolColor = display.screenColor;
    socket.event(WStype_TEXT, list(11, true));
    assert(hasText(display, "12.3") && !hasText(display, "Milk"));
    fakeMillis += 600000;
    view.update();
    FakeHttp::code = 503;
    runFakeTask(0);
    view.update();
    assert(hasText(display, "12.3") && hasText(display, "City not configured"));
    fakeMillis += 30000;
    view.update();
    FakeHttp::code = 200;
    FakeHttp::body = R"({"city":"Oslo","country":null,"temperature":-999,"unit":"°C"})";
    runFakeTask(0);
    view.update();
    assert(hasText(display, "-999.0") && hasText(display, "Oslo") && display.screenColor != coolColor);
    const auto number = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                     [](const TFT_eSPI::TextDraw &draw) { return draw.value == "-999.0"; });
    const auto unit = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                   [](const TFT_eSPI::TextDraw &draw) { return draw.value == "C"; });
    assert(number != display.textDraws.end() && unit != display.textDraws.end() &&
           number->size == 1 && unit->size == number->size && number->x >= 0 &&
           unit->x + 26 * unit->size <= display.width());
    touch(view, 120, 40);
    assert(hasText(display, "-999.0"));
    swipe(view, 100, 100, 140, 180);
    assert(hasText(display, "-999.0"));
    swipe(view, 180, 160, 80, 160);
    assert(hasText(display, "Milk"));
    swipe(view, 190, 160, 80, 160);
    assert(hasText(display, "-999.0"));
    swipe(view, 80, 160, 190, 160);
    assert(hasText(display, "Milk"));
    FakeHttp::body = list(10, false);
    WiFi.currentStatus = WL_DISCONNECTED;
    client.update();
    assert(hasText(display, "Wi-Fi unavailable"));
    auto offline = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Wi-Fi unavailable"; });
    auto title = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                              [](const TFT_eSPI::TextDraw &draw) { return draw.value == "TO BUY"; });
    assert(offline != display.textDraws.end() && title != display.textDraws.end() &&
           offline->y == title->y && offline->x >= title->x + 6 * 8 &&
           offline->x + static_cast<int>(offline->value.size()) * 8 <= display.width());
    const uint16_t offlineColor = offline->color;
    auto indicator = std::find_if(display.circles.begin(), display.circles.end(),
                                  [=](const TFT_eSPI::Circle &circle) {
                                      return circle.filled && circle.color == offlineColor;
                                  });
    assert(indicator != display.circles.end());
    const int indicatorX = indicator->x;
    const int indicatorY = indicator->y;
    const size_t cardsBeforeAnimation = display.rectangles.size();
    fakeMillis += 350;
    view.update();
    indicator = std::find_if(display.circles.begin(), display.circles.end(),
                             [=](const TFT_eSPI::Circle &circle) {
                                 return circle.filled && circle.color == offlineColor;
                             });
    assert(indicator != display.circles.end() &&
           (indicator->x != indicatorX || indicator->y != indicatorY));
    assert(display.rectangles.size() == cardsBeforeAnimation && hasText(display, "Milk"));
    WiFi.currentStatus = WL_CONNECTED;
    client.update();
    assert(hasText(display, "Server connecting"));
    assert(textColor(display, "Server connecting") != offlineColor);
    fakeMillis += 5000;
    client.update();
    assert(hasText(display, "Server unavailable"));
    socket.event(WStype_CONNECTED);
    assert(hasText(display, "Loading list"));
    client.update();
    assert(!Preferences::saved.empty() && hasText(display, "Milk") && hasText(display, "Item3"));
    assert(hasText(display, "Item4") && !hasText(display, "Item5"));
    auto milk = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                             [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Milk"; });
    auto missing = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Item1"; });
    assert(milk != display.textDraws.end() && milk->size == 1 && milk->font == 1 && milk->x == 88);
    assert(missing != display.textDraws.end() && missing->size == 1 && missing->font == 1 && missing->x == milk->x);
    display.setFreeFont(&FreeSans9pt7b);
    assert(display.fontHeight(1) == 22 && display.fontHeight(2) * 2 == 32);
    display.setTextFont(2);
    auto switchRect = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                                   [](const TFT_eSPI::Rectangle &rect) {
                                       return rect.x == 8 && rect.y == 34 && rect.width == 70 && rect.height == 40;
                                   });
    assert(switchRect != display.rectangles.end());
    const uint16_t offColor = switchRect->color;
    assert(milk->x >= switchRect->x + switchRect->width);
    assert(milk->color == milk->background && milk->background != display.screenColor);
    assert(std::count_if(display.circles.begin(), display.circles.end(),
                         [](const TFT_eSPI::Circle &circle) { return !circle.filled; }) == 4);
    assert(hasText(display, "1-5/10"));
    const uint16_t connectedColor = textColor(display, "Connected");
    auto connected = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                  [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Connected"; });
    auto heading = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                [](const TFT_eSPI::TextDraw &draw) { return draw.value == "TO BUY"; });
    assert(connected != display.textDraws.end() && heading != display.textDraws.end());
    assert(connected->y == heading->y && connected->x > heading->x &&
           connected->x + 9 * 8 <= display.width() - 8);
    const auto connectedCircles = display.circles;
    fakeMillis += 350;
    view.update();
    assert(display.circles.size() == connectedCircles.size());
    auto track = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                              [](const TFT_eSPI::Rectangle &rect) {
                                  return rect.x == 228 && rect.y == 30 && rect.width == 8 && rect.height == 250;
                              });
    auto thumb = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                              [](const TFT_eSPI::Rectangle &rect) {
                                  return rect.x == 228 && rect.y == 30 && rect.width == 8 && rect.height < 250;
                              });
    assert(track != display.rectangles.end() && thumb != display.rectangles.end());
    auto down = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                             [](const TFT_eSPI::TextDraw &draw) { return draw.value == "DOWN"; });
    assert(down != display.textDraws.end() && down->size == 1);

    touch(view, 200, 40);
    assert(socket.sent.empty());
    touch(view, 7, 40);
    assert(socket.sent.empty());
    touch(view, 78, 40);
    assert(socket.sent.empty());
    touch(view, 30, 32);
    assert(socket.sent.empty());
    touch(view, 30, 74);
    assert(socket.sent.empty());

    touch(view, 30, 40);
    assert(socket.sent.find("\"id\":\"milk\"") != std::string::npos);
    assert(socket.sent.find("\"needed\":true") != std::string::npos);
    const uint16_t waitingColor = textColor(display, "Saving");
    assert(waitingColor != connectedColor);
    fakeMillis += 350;
    view.update();
    assert(hasText(display, "Saving") && hasText(display, "Milk"));
    assert(!client.list().items[0].needed);
    assert(!client.toggle(0));
    socket.event(WStype_TEXT, list(11, true));
    assert(client.list().items[0].needed);
    assert(hasText(display, "ON"));
    switchRect = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                              [](const TFT_eSPI::Rectangle &rect) {
                                  return rect.x == 8 && rect.y == 34 && rect.width == 70 && rect.height == 40;
                              });
    assert(switchRect != display.rectangles.end() && switchRect->color != offColor);
    socket.event(WStype_TEXT, list(9, false));
    assert(client.list().revision == 11 && client.list().items[0].needed);
    socket.event(WStype_TEXT, list(12, true, 3));
    assert(hasText(display, "Item2") && !hasText(display, "Item3"));
    socket.event(WStype_TEXT, list(13, true));
    assert(hasText(display, "Item3"));
    socket.event(WStype_TEXT,
                 R"({"type":"list","revision":14,"periodic":[{"id":"milk","name":"Very long item name","needed":true}],"missing":[]})");
    assert(hasText(display, "Very long") && hasText(display, "item name"));
    assert(std::none_of(display.rectangles.begin(), display.rectangles.end(),
                        [](const TFT_eSPI::Rectangle &rect) { return rect.x == 228 && rect.width == 8; }));
    socket.event(WStype_TEXT, list(15, true));
    touch(view, 200, 300);
    assert(!hasText(display, "Milk") && hasText(display, "Item4") && hasText(display, "2-6/10"));
    touch(view, 20, 300);
    assert(hasText(display, "Milk"));
    XPT2046_Touchscreen::point = {2300, 650, 100};
    XPT2046_Touchscreen::down = true;
    view.update();
    XPT2046_Touchscreen::point = {2300, 2700, 100};
    view.update();
    XPT2046_Touchscreen::down = false;
    view.update();
    assert(hasText(display, "Item9") && !hasText(display, "Milk"));
    touch(view, 230, 55);
    assert(hasText(display, "Milk") && hasText(display, "1-5/10"));
    const std::string lastCommand = socket.sent;
    touch(view, 30, 90);
    assert(socket.sent == lastCommand);
    XPT2046_Touchscreen::point = {1200, 2400, 100};
    XPT2046_Touchscreen::down = true;
    view.update();
    XPT2046_Touchscreen::point = {1200, 1000, 100};
    view.update();
    XPT2046_Touchscreen::down = false;
    view.update();
    assert(hasText(display, "Item5") && !hasText(display, "Milk"));

    socket.event(WStype_TEXT, "{invalid");
    assert(!client.ready());
    FakeHttp::body = list(16, true);
    client.update();
    assert(client.ready() && client.list().revision == 16);
    socket.event(WStype_DISCONNECTED);
    assert(!client.ready() && std::strcmp(client.status(), "Server unavailable") == 0);
    assert(textColor(display, "Server unavailable") != connectedColor &&
           textColor(display, "Server unavailable") != waitingColor);
    const auto unavailable = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                          [](const TFT_eSPI::TextDraw &draw) {
                                              return draw.value == "Server unavailable";
                                          });
    assert(unavailable != display.textDraws.end() && unavailable->y == 6 &&
           unavailable->x >= 85 && unavailable->x + 18 * 8 <= display.width() - 8);
    socket.event(WStype_CONNECTED);
    FakeHttp::code = 500;
    client.update();
    assert(!client.ready());
    FakeHttp::code = 200;
    FakeHttp::body = list(1, false);
    fakeMillis += 5000;
    client.update();
    assert(client.ready() && client.list().revision == 1);
    assert(client.toggle(0));
    fakeMillis += 10000;
    client.update();
    assert(!client.ready() && std::strcmp(client.status(), "Update not confirmed") == 0);
    client.update();
    assert(client.ready() && !client.list().items[0].needed);

    socket.sendOk = false;
    assert(!client.toggle(0));
    assert(!client.ready() && std::strcmp(client.status(), "Update send failed") == 0);
    client.update();
    assert(client.ready());
    socket.event(WStype_TEXT, "{invalid");
    FakeHttp::body = std::string(shopping::max_message_length + 1, 'x');
    client.update();
    assert(!client.ready() && std::strcmp(client.status(), "Invalid server response") == 0);
    FakeHttp::body = list(2, false);
    fakeMillis += 5000;
    client.update();
    assert(client.ready() && client.list().revision == 2);

    const std::string longName = "abcdefgh12345678ABCDEFGH87654321ijklmnop11223344IJKLMNOP55667788";
    assert(longName.size() == shopping::max_name_length);
    const std::string longList = R"({"type":"list","revision":3,"periodic":[{"id":"long","name":")" +
                                 longName + R"(","needed":false}],"missing":[{"id":"last","name":"End"}]})";
    socket.event(WStype_TEXT, longList);
    std::string wrapped;
    for (const auto &draw : display.textDraws) {
        if (draw.font == 1 && draw.x == 88 && draw.value != "End") wrapped += draw.value;
    }
    assert(wrapped == longName && hasText(display, "End"));
    const std::string twoPeriodic = R"({"type":"list","revision":4,"periodic":[{"id":"long","name":")" +
                                    longName +
                                    R"(","needed":false},{"id":"next","name":"Next","needed":false}],"missing":[{"id":"a","name":"A"},{"id":"b","name":"B"},{"id":"c","name":"C"}]})";
    socket.event(WStype_TEXT, twoPeriodic);
    touch(view, 230, 55);
    assert(hasText(display, "Next"));
    socket.sendOk = true;
    touch(view, 30, 180);
    assert(socket.sent.find("\"id\":\"next\"") != std::string::npos);
    socket.event(WStype_TEXT, list(5, false));
    touch(view, 230, 55);
    socket.event(WStype_TEXT, R"({"type":"list","revision":6,"periodic":[],"missing":[]})");
    assert(hasText(display, "No items") && hasText(display, "UP") && hasText(display, "DOWN"));
    socket.event(WStype_TEXT, list(7, false));

    TFT_eSPI secondDisplay;
    ShoppingView secondView(secondDisplay, client);
    secondView.begin("http://server:3000");
    assert(hasText(secondDisplay, "Milk") && !hasText(secondDisplay, "Touch the cross"));
    XPT2046_Touchscreen::point = {300, 100, 100};
    XPT2046_Touchscreen::down = true;
    secondView.update();
    fakeMillis += 3000;
    secondView.update();
    assert(hasText(secondDisplay, "Touch the cross"));
    XPT2046_Touchscreen::down = false;
    secondView.update();

    ShoppingClient secure;
    secure.begin("https://server", nullptr);
    assert(!secure.ready() && std::strcmp(secure.status(), "Server needs http URL") == 0);
}
