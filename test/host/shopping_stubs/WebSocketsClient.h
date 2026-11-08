#pragma once
#include <functional>
#include <string>
#include "Arduino.h"
enum WStype_t {
    WStype_DISCONNECTED, WStype_CONNECTED, WStype_TEXT, WStype_BIN,
    WStype_FRAGMENT_TEXT_START, WStype_FRAGMENT_BIN_START, WStype_FRAGMENT, WStype_FRAGMENT_FIN
};
class WebSocketsClient {
public:
    using Handler = std::function<void(WStype_t, uint8_t *, size_t)>;
    WebSocketsClient() { instance = this; }
    static WebSocketsClient *instance;
    Handler handler;
    std::string host, path, sent;
    int port = 0;
    bool sendOk = true;
    void begin(const char *h, int p, const char *u, const char *) { host = h; port = p; path = u; }
    void onEvent(Handler callback) { handler = callback; }
    void setReconnectInterval(unsigned long) {}
    void enableHeartbeat(unsigned long, unsigned long, unsigned int) {}
    void loop() {}
    void disconnect() {}
    bool sendTXT(const char *payload, size_t length) {
        sent.assign(payload, length);
        return sendOk;
    }
    void event(WStype_t type, const std::string &payload = "") {
        handler(type, reinterpret_cast<uint8_t *>(const_cast<char *>(payload.data())), payload.size());
    }
};
