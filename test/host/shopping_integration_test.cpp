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
int FakeHttp::deleteCode = 200;
std::string FakeHttp::deleteBody;
std::string FakeHttp::method;
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
    assert(hasText(display, "1-6/10") && socket.sent.empty());
    swipe(view, 120, 200, 180, 120);
    assert(hasText(display, "Comprar") && hasText(display, "Item5"));
    touch(view, 230, 55);
    assert(hasText(display, "Milk"));
    swipe(view, 30, 40, 120, 45);
    assert(hasText(display, "Loading temperature") && socket.sent.empty());
    FakeHttp::body = R"({"city":"MÁLAGA","country":"ESPAÑA","time":"2026-11-08T12:34","temperature":12.3,"unit":"°C"})";
    runFakeTask(0);
    view.update();
    assert(FakeHttp::url == "http://server:3000/temperature");
    assert(hasText(display, "12.3") && hasText(display, "C") && hasText(display, "MALAGA"));
    const auto city = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                   [](const TFT_eSPI::TextDraw &draw) { return draw.value == "MALAGA"; });
    const auto country = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                      [](const TFT_eSPI::TextDraw &draw) { return draw.value == "ESPANA"; });
    const auto reading = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                      [](const TFT_eSPI::TextDraw &draw) { return draw.value == "12.3"; });
    const auto clock = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                    [](const TFT_eSPI::TextDraw &draw) { return draw.value == "12:34"; });
    assert(city != display.textDraws.end() && country != display.textDraws.end() &&
           reading != display.textDraws.end() && clock != display.textDraws.end() &&
           city->size == 1 && country->size == 1 && clock->size == 4 &&
           clock->x >= 0 && clock->x + 5 * 8 * clock->size <= display.width() &&
           city->y + 16 <= country->y && country->y + 16 <= display.height() - 22 &&
           reading->y + 52 * reading->size <= clock->y && clock->y + 16 * clock->size <= city->y);
    fakeMillis += 59999;
    view.update();
    assert(hasText(display, "12:34"));
    ++fakeMillis;
    view.update();
    assert(hasText(display, "12:35") && !hasText(display, "12:34"));
    const uint16_t coolColor = display.screenColor;
    socket.event(WStype_TEXT, list(11, true));
    assert(hasText(display, "12.3") && !hasText(display, "Milk"));
    fakeMillis += 540000;
    view.update();
    FakeHttp::code = 503;
    runFakeTask(0);
    view.update();
    assert(hasText(display, "12.3") && hasText(display, "12:44") && hasText(display, "City not configured"));
    fakeMillis += 30000;
    view.update();
    FakeHttp::code = 200;
    FakeHttp::body = R"({"city":"Oslo","country":null,"time":"2026-11-08T22:17","temperature":-999,"unit":"°C"})";
    runFakeTask(0);
    view.update();
    assert(hasText(display, "-999.0") && hasText(display, "Oslo") && hasText(display, "22:17") &&
           display.screenColor != coolColor);
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
    fakeMillis += 60000;
    view.update();
    assert(hasText(display, "Milk") && !hasText(display, "22:18"));
    swipe(view, 190, 160, 80, 160);
    assert(hasText(display, "-999.0") && hasText(display, "22:18"));
    swipe(view, 80, 160, 190, 160);
    assert(hasText(display, "Milk"));
    FakeHttp::body = list(10, false);
    WiFi.currentStatus = WL_DISCONNECTED;
    client.update();
    assert(hasText(display, "Wi-Fi sin conexion"));
    auto offline = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Wi-Fi sin conexion"; });
    auto title = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                              [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Comprar"; });
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
    assert(hasText(display, "Conectando..."));
    assert(textColor(display, "Conectando...") != offlineColor);
    fakeMillis += 5000;
    client.update();
    assert(hasText(display, "Sin servidor"));
    socket.event(WStype_CONNECTED);
    assert(hasText(display, "Cargando lista"));
    client.update();
    assert(!Preferences::saved.empty() && hasText(display, "Milk") && hasText(display, "Item3"));
    assert(hasText(display, "Item5") && !hasText(display, "Item6"));
    auto milk = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                             [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Milk"; });
    auto missing = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Item1"; });
    assert(milk != display.textDraws.end() && milk->size == 1 && milk->font == 1 && milk->x == 53);
    assert(missing != display.textDraws.end() && missing->size == 1 && missing->font == 1 && missing->x == milk->x);
    display.setFreeFont(&FreeSans9pt7b);
    assert(display.fontHeight(1) == 22 && display.fontHeight(2) * 2 == 32);
    display.setTextFont(2);
    auto switchRect = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                                   [](const TFT_eSPI::Rectangle &rect) {
                                       return rect.x == 8 && rect.y == 34 && rect.width == 35 && rect.height == 40;
                                   });
    assert(switchRect != display.rectangles.end());
    const uint16_t offColor = switchRect->color;
    assert(milk->x >= switchRect->x + switchRect->width);
    assert(milk->color == milk->background && milk->background != display.screenColor);
    assert(std::count(display.text.begin(), display.text.end(), "X") == 4);
    auto removeRect = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                                   [](const TFT_eSPI::Rectangle &rect) {
                                       return rect.x == 8 && rect.y == 84 && rect.width == 35 && rect.height == 40;
                                   });
    assert(removeRect != display.rectangles.end() && removeRect->color == 0xb904);
    assert(hasText(display, "1-6/10"));
    const uint16_t connectedColor = textColor(display, "Conectado");
    auto connected = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                  [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Conectado"; });
    auto heading = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                [](const TFT_eSPI::TextDraw &draw) { return draw.value == "Comprar"; });
    assert(connected != display.textDraws.end() && heading != display.textDraws.end());
    assert(connected->y == heading->y && connected->x > heading->x &&
           connected->x + 9 * 8 <= display.width() - 8);
    const auto connectedCircles = display.circles;
    fakeMillis += 350;
    view.update();
    assert(display.circles.size() == connectedCircles.size());
    auto track = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                              [](const TFT_eSPI::Rectangle &rect) {
                                  return rect.x == 228 && rect.y == 30 && rect.width == 8 && rect.height == 266;
                              });
    auto thumb = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                              [](const TFT_eSPI::Rectangle &rect) {
                                  return rect.x == 228 && rect.y == 30 && rect.width == 8 && rect.height < 266;
                              });
    assert(track != display.rectangles.end() && thumb != display.rectangles.end());
    assert(!hasText(display, "UP") && !hasText(display, "DOWN"));
    assert(std::none_of(display.rectangles.begin(), display.rectangles.end(),
                        [](const TFT_eSPI::Rectangle &rect) { return rect.y >= 296; }));

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
    const uint16_t waitingColor = textColor(display, "Guardando");
    assert(waitingColor != connectedColor);
    fakeMillis += 350;
    view.update();
    assert(hasText(display, "Guardando") && hasText(display, "Milk"));
    assert(!client.list().items[0].needed);
    assert(!client.toggle(0));
    socket.event(WStype_TEXT, list(11, true));
    assert(client.list().items[0].needed);
    assert(hasText(display, "SI"));
    switchRect = std::find_if(display.rectangles.begin(), display.rectangles.end(),
                              [](const TFT_eSPI::Rectangle &rect) {
                                  return rect.x == 8 && rect.y == 34 && rect.width == 35 && rect.height == 40;
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
    assert(hasText(display, "Very long item") && hasText(display, "name"));
    assert(std::none_of(display.rectangles.begin(), display.rectangles.end(),
                        [](const TFT_eSPI::Rectangle &rect) { return rect.x == 228 && rect.width == 8; }));
    socket.event(WStype_TEXT, list(15, true));
    touch(view, 200, 300);
    assert(hasText(display, "Milk") && hasText(display, "1-6/10"));
    touch(view, 20, 300);
    assert(hasText(display, "Milk") && hasText(display, "1-6/10"));
    swipe(view, 120, 200, 120, 150);
    assert(!hasText(display, "Milk") && hasText(display, "2-7/10"));
    swipe(view, 120, 150, 120, 200);
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
    assert(hasText(display, "Milk") && hasText(display, "1-6/10"));
    const std::string lastCommand = socket.sent;
    touch(view, 50, 90);
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
    assert(textColor(display, "Sin servidor") != connectedColor &&
           textColor(display, "Sin servidor") != waitingColor);
    const auto unavailable = std::find_if(display.textDraws.begin(), display.textDraws.end(),
                                          [](const TFT_eSPI::TextDraw &draw) {
                                              return draw.value == "Sin servidor";
                                          });
    assert(unavailable != display.textDraws.end() && unavailable->y == 6 &&
           unavailable->x >= 85 &&
           unavailable->x + static_cast<int>(unavailable->value.size()) * 8 <= display.width() - 8);
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
        if (draw.font == 1 && draw.x == 53 && draw.value != "End") wrapped += draw.value;
    }
    assert(wrapped == longName && hasText(display, "End"));
    const std::string twoPeriodic = R"({"type":"list","revision":4,"periodic":[{"id":"long","name":")" +
                                    longName +
                                    R"(","needed":false},{"id":"next","name":"Next","needed":false}],"missing":[{"id":"a","name":"A"},{"id":"b","name":"B"},{"id":"c","name":"C"}]})";
    socket.event(WStype_TEXT, twoPeriodic);
    touch(view, 230, 55);
    assert(hasText(display, "Next"));
    socket.sendOk = true;
    touch(view, 30, 140);
    assert(socket.sent.find("\"id\":\"next\"") != std::string::npos);
    fakeMillis += 10000;
    client.update();
    FakeHttp::body = list(5, false);
    client.update();
    assert(client.ready());
    socket.event(WStype_TEXT, list(5, false));
    touch(view, 230, 55);
    socket.event(WStype_TEXT, R"({"type":"list","revision":6,"periodic":[],"missing":[]})");
    assert(hasText(display, "No items") && !hasText(display, "UP") && !hasText(display, "DOWN"));
    socket.event(WStype_TEXT, list(7, false));

    const std::string missingList = R"({"type":"list","revision":8,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[{"id":"a/b ?#%é","name":"Bread"},{"id":"other","name":"Tea"}]})";
    const std::string removedList = R"({"type":"list","revision":9,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[{"id":"other","name":"Tea"}]})";
    socket.event(WStype_TEXT, missingList);
    assert(!client.removeMissing(0));
    touch(view, 50, 90);
    assert(FakeHttp::method != "DELETE");
    touch(view, 30, 90);
    assert(hasText(display, "Guardando") && hasText(display, "Bread"));
    assert(!client.removeMissing(1) && !client.toggle(0));
    FakeHttp::deleteBody = removedList;
    runFakeTask(1);
    assert(FakeHttp::method == "DELETE" &&
           FakeHttp::url == "http://server:3000/tobuy/missing/a%2Fb%20%3F%23%25%C3%A9");
    client.update();
    assert(client.ready() && !hasText(display, "Bread") && hasText(display, "Tea"));

    socket.event(WStype_TEXT, missingList);
    assert(!hasText(display, "Bread"));
    socket.event(WStype_TEXT, R"({"type":"list","revision":10,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[{"id":"other","name":"Tea"}]})");
    touch(view, 30, 90);
    FakeHttp::deleteCode = 409;
    runFakeTask(1);
    FakeHttp::body = R"({"type":"list","revision":11,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[{"id":"other","name":"Tea"}]})";
    client.update();
    assert(!client.ready() && hasText(display, "Error al enviar") && hasText(display, "Tea"));
    fakeMillis += 5000;
    client.update();
    assert(client.ready() && hasText(display, "Tea") && FakeHttp::method == "GET");

    touch(view, 30, 90);
    FakeHttp::deleteCode = 200;
    FakeHttp::deleteBody = R"({"type":"list","revision":12,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[]})";
    socket.event(WStype_TEXT, FakeHttp::deleteBody);
    runFakeTask(1);
    client.update();
    assert(client.ready() && !hasText(display, "Tea"));

    socket.event(WStype_TEXT, R"({"type":"list","revision":13,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[{"id":"other","name":"Tea"}]})");
    touch(view, 30, 90);
    FakeHttp::deleteBody = "{invalid";
    runFakeTask(1);
    client.update();
    assert(!client.ready() && hasText(display, "Respuesta invalida") && hasText(display, "Tea"));
    FakeHttp::body = R"({"type":"list","revision":13,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[{"id":"other","name":"Tea"}]})";
    fakeMillis += 5000;
    client.update();
    assert(client.ready());

    touch(view, 30, 90);
    FakeHttp::deleteBody = FakeHttp::body;
    runFakeTask(1);
    client.update();
    assert(!client.ready() && hasText(display, "Tiempo agotado") && hasText(display, "Tea"));
    fakeMillis += 5000;
    client.update();
    assert(client.ready());

    touch(view, 30, 90);
    socket.event(WStype_DISCONNECTED);
    socket.event(WStype_CONNECTED);
    FakeHttp::body = R"({"type":"list","revision":1,"periodic":[{"id":"milk","name":"Milk","needed":false}],"missing":[{"id":"other","name":"Tea"}]})";
    client.update();
    assert(!client.removeMissing(1) && hasText(display, "Tea"));
    FakeHttp::deleteBody = R"({"type":"list","revision":99,"periodic":[],"missing":[]})";
    runFakeTask(1);
    client.update();
    assert(client.list().revision == 1 && hasText(display, "Tea") && client.ready());

    touch(view, 30, 90);
    FakeHttp::deleteCode = 404;
    runFakeTask(1);
    client.update();
    assert(!client.ready() && hasText(display, "Error al enviar"));
    fakeMillis += 5000;
    client.update();
    assert(client.ready() && hasText(display, "Tea"));

    touch(view, 30, 90);
    FakeHttp::deleteCode = 200;
    FakeHttp::deleteBody = std::string(shopping::max_message_length + 1, 'x');
    runFakeTask(1);
    client.update();
    assert(!client.ready() && hasText(display, "Respuesta invalida") && hasText(display, "Tea"));
    fakeMillis += 5000;
    client.update();
    assert(client.ready());

    WiFi.currentStatus = WL_DISCONNECTED;
    client.update();
    assert(!client.removeMissing(1) && hasText(display, "Tea"));
    WiFi.currentStatus = WL_CONNECTED;
    client.update();
    socket.event(WStype_CONNECTED);
    client.update();
    assert(client.ready());

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

    socket.event(WStype_TEXT,
                 R"({"type":"list","revision":2,"periodic":[{"id":"accent","name":"Árbol ñ ü ä ö ß","needed":true}],"missing":[{"id":"umlauts","name":"ÄÖÜ"}]})");
    assert(hasText(display, "Arbol n u a o ss"));
    assert(hasText(display, "AOU") && display.lines.size() >= 16);

    ShoppingClient secure;
    secure.begin("https://server", nullptr);
    assert(!secure.ready() && std::strcmp(secure.status(), "Server needs http URL") == 0);
}
