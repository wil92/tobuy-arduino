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
    assert(!Preferences::saved.empty() && hasText(display, "Milk") && hasText(display, "Item6"));
    assert(!hasText(display, "Item7"));

    touch(view, 200, 65);
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
    assert(hasText(display, "Item2") && !hasText(display, "Item6"));
    socket.event(WStype_TEXT, list(13, true));
    assert(hasText(display, "Item6"));
    touch(view, 200, 300);
    assert(!hasText(display, "Milk") && hasText(display, "Item7"));
    touch(view, 20, 300);
    assert(hasText(display, "Milk"));
    const std::string lastCommand = socket.sent;
    touch(view, 200, 100);
    assert(socket.sent == lastCommand);
    XPT2046_Touchscreen::point = {1200, 2400, 100};
    XPT2046_Touchscreen::down = true;
    view.update();
    XPT2046_Touchscreen::point = {1200, 1000, 100};
    view.update();
    XPT2046_Touchscreen::down = false;
    view.update();
    assert(hasText(display, "Item9") && !hasText(display, "Milk"));

    socket.event(WStype_TEXT, "{invalid");
    assert(!client.ready());
    FakeHttp::body = list(14, true);
    client.update();
    assert(client.ready() && client.list().revision == 14);
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
