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

static void touch(ShoppingView &view, int x, int y) {
    XPT2046_Touchscreen::point = {static_cast<int16_t>(x * 10), static_cast<int16_t>(y * 10), 100};
    XPT2046_Touchscreen::down = true;
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
    view.begin();
    assert(hasText(display, "Touch the cross"));
    touch(view, 20, 50);
    touch(view, 220, 50);
    touch(view, 20, 270);
    assert(!Preferences::saved.empty() && hasText(display, "Milk") && hasText(display, "Item3"));
    assert(!hasText(display, "Item4"));
    auto milk = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                             [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Milk"; });
    auto missing = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Item1"; });
    assert(milk != display.textDraws.end() && milk->size == 2 && milk->font == 2 && milk->x == 88);
    assert(missing != display.textDraws.end() && missing->size == 2 && missing->x == milk->x);
    assert(display.rectangles.size() == 1);
    assert(display.rectangles[0].x == 8 && display.rectangles[0].y == 58 &&
           display.rectangles[0].width == 70 && display.rectangles[0].height == 40);
    assert(milk->x >= display.rectangles[0].x + display.rectangles[0].width);
    auto down = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                             [](const TFT_eSPI::TextDraw &draw) { return draw.value == "DOWN"; });
    assert(down != display.textDraws.end() && down->size == 1);

    touch(view, 200, 65);
    assert(socket.sent.empty());
    touch(view, 7, 65);
    assert(socket.sent.empty());
    touch(view, 78, 65);
    assert(socket.sent.empty());
    touch(view, 30, 55);
    assert(socket.sent.empty());
    touch(view, 30, 98);
    assert(socket.sent.empty());

    touch(view, 30, 65);
    assert(socket.sent.find("\"id\":\"milk\"") != std::string::npos);
    assert(socket.sent.find("\"needed\":true") != std::string::npos);
    assert(!client.list().items[0].needed);
    assert(!client.toggle(0));
    socket.event(WStype_TEXT, list(11, true));
    assert(client.list().items[0].needed);
    assert(hasText(display, "ON"));
    socket.event(WStype_TEXT, list(9, false));
    assert(client.list().revision == 11 && client.list().items[0].needed);
    socket.event(WStype_TEXT, list(12, true, 3));
    assert(hasText(display, "Item2") && !hasText(display, "Item3"));
    socket.event(WStype_TEXT, list(13, true));
    assert(hasText(display, "Item3"));
    socket.event(WStype_TEXT,
                 R"({"type":"list","revision":14,"periodic":[{"id":"milk","name":"Very long item name","needed":true}],"missing":[]})");
    const auto truncated = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                        [](const TFT_eSPI::TextDraw &draw) {
                                            return draw.value.find("Very") == 0;
                                        });
    assert(truncated != display.textDraws.end() && truncated->value != "Very long item name");
    assert(truncated->x + static_cast<int>(truncated->value.size()) * 8 * truncated->size <= display.width() - 8);
    socket.event(WStype_TEXT, list(15, true));
    touch(view, 200, 300);
    assert(!hasText(display, "Milk") && hasText(display, "Item4"));
    touch(view, 20, 300);
    assert(hasText(display, "Milk"));
    const std::string lastCommand = socket.sent;
    touch(view, 30, 115);
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

    TFT_eSPI secondDisplay;
    ShoppingView secondView(secondDisplay, client);
    secondView.begin();
    assert(hasText(secondDisplay, "Milk") && !hasText(secondDisplay, "Touch the cross"));
    XPT2046_Touchscreen::point = {1000, 100, 100};
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
