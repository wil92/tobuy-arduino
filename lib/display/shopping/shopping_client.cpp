#include "shopping_client.h"

#include <HTTPClient.h>
#include <WiFi.h>
#include <cstring>
#include <memory>
#include <new>

namespace {
constexpr unsigned long retry_ms = 5000;
constexpr unsigned long response_timeout_ms = 10000;

class BoundedStream : public Stream {
public:
    size_t write(uint8_t value) override {
        if (length_ == shopping::max_message_length) return 0;
        buffer_[length_++] = static_cast<char>(value);
        return 1;
    }
    size_t write(const uint8_t *data, size_t size) override {
        if (size > shopping::max_message_length - length_) return 0;
        std::memcpy(buffer_ + length_, data, size);
        length_ += size;
        return size;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    const char *data() const { return buffer_; }
    size_t size() const { return length_; }
private:
    char buffer_[shopping::max_message_length] = {};
    size_t length_ = 0;
};
}

void ShoppingClient::begin(const char *serverUrl, ChangeHandler onChange) {
    onChange_ = onChange;
    if (!serverUrl || std::strncmp(serverUrl, "http://", 7) != 0) {
        setStatus("Server needs http URL", false);
        return;
    }
    serverUrl_ = serverUrl;
    String authority = serverUrl_.substring(7);
    const int colon = authority.indexOf(':');
    host_ = colon < 0 ? authority : authority.substring(0, colon);
    if (colon >= 0) port_ = static_cast<uint16_t>(authority.substring(colon + 1).toInt());
    if (host_.isEmpty() || port_ == 0) {
        setStatus("Invalid server URL", false);
        return;
    }
    configured_ = true;
    notify();
}

void ShoppingClient::update() {
    if (!configured_) return;
    if (WiFi.status() != WL_CONNECTED) {
        if (started_) socket_.disconnect();
        started_ = connected_ = hasList_ = pending_ = false;
        setStatus("Wi-Fi unavailable", false);
        return;
    }
    if (!started_) {
        started_ = true;
        connectStartedAt_ = millis();
        setStatus("Connecting to server", false);
        socket_.begin(host_.c_str(), port_, "/ws", "");
        socket_.onEvent([this](WStype_t type, uint8_t *payload, size_t length) {
            onEvent(type, payload, length);
        });
        socket_.setReconnectInterval(retry_ms);
        socket_.enableHeartbeat(15000, 3000, 2);
    }
    socket_.loop();
    if (!connected_ && static_cast<unsigned long>(millis() - connectStartedAt_) >= retry_ms) {
        setStatus("Server unavailable", false);
    }
    if (connected_ && fetchPending_ &&
        static_cast<unsigned long>(millis() - lastFetch_) >= retry_ms) {
        lastFetch_ = millis();
        fetchList();
    }
    if (pending_ && static_cast<unsigned long>(millis() - pendingAt_) >= response_timeout_ms) {
        pending_ = false;
        setStatus("Update not confirmed", false);
        fetchPending_ = true;
        lastFetch_ = millis() - retry_ms;
    }
}

void ShoppingClient::onEvent(WStype_t type, uint8_t *payload, size_t length) {
    if (type == WStype_CONNECTED) {
        connected_ = true;
        hasList_ = false;
        pending_ = false;
        fetchPending_ = true;
        lastFetch_ = millis() - retry_ms;
        setStatus("Loading list", false);
    } else if (type == WStype_DISCONNECTED) {
        connected_ = false;
        pending_ = false;
        setStatus("Server unavailable", false);
    } else if (type == WStype_TEXT) {
        if (!acceptList(reinterpret_cast<const char *>(payload), length)) {
            Serial.println("Invalid shopping list WebSocket message");
            setStatus("Invalid server message", false);
            fetchPending_ = true;
            lastFetch_ = millis() - retry_ms;
        }
    } else if (type == WStype_BIN || type == WStype_FRAGMENT_TEXT_START ||
               type == WStype_FRAGMENT_BIN_START || type == WStype_FRAGMENT ||
               type == WStype_FRAGMENT_FIN) {
        Serial.println("Unsupported shopping list WebSocket frame");
        setStatus("Invalid server message", false);
        fetchPending_ = true;
    }
}

void ShoppingClient::fetchList() {
    WiFiClient connection;
    HTTPClient http;
    http.setConnectTimeout(1500);
    http.setTimeout(1500);
    if (!http.begin(connection, serverUrl_ + "/tobuy")) {
        Serial.println("Could not start shopping list request");
        setStatus("Server unavailable", false);
        return;
    }
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("Shopping list HTTP error: %d\n", code);
        http.end();
        setStatus("Server unavailable", false);
        return;
    }
    std::unique_ptr<BoundedStream> body(new (std::nothrow) BoundedStream());
    if (!body) {
        http.end();
        Serial.println("Insufficient memory for shopping list response");
        setStatus("Out of memory", false);
        return;
    }
    const int received = http.writeToStream(body.get());
    http.end();
    if (received < 0 || !acceptList(body->data(), body->size())) {
        Serial.println("Invalid shopping list HTTP response");
        setStatus("Invalid server response", false);
        return;
    }
    fetchPending_ = false;
    setStatus(pending_ ? "Waiting for server" : "Connected", true);
}

bool ShoppingClient::acceptList(const char *data, size_t length) {
    std::unique_ptr<shopping::List> candidate(new (std::nothrow) shopping::List());
    if (!candidate) {
        Serial.println("Insufficient memory for shopping list");
        return false;
    }
    if (!shopping::parseList(data, length, *candidate)) return false;
    if (hasList_ && candidate->revision < list_.revision) return true;
    list_ = *candidate;
    hasList_ = true;
    if (pending_) {
        for (size_t i = 0; i < list_.count; ++i) {
            const shopping::Item &item = list_.items[i];
            if (item.periodic && std::strcmp(item.id, pendingId_) == 0 &&
                item.needed == pendingValue_) {
                pending_ = false;
                break;
            }
        }
    }
    if (!fetchPending_) {
        const char *status = pending_ ? "Waiting for server" : "Connected";
        const bool changed = std::strcmp(status_, status) != 0 || !ready_;
        setStatus(status, true);
        if (!changed) notify();
    } else {
        notify();
    }
    return true;
}

bool ShoppingClient::toggle(size_t index) {
    if (!ready_ || !connected_ || pending_ || index >= list_.count || !list_.items[index].periodic) return false;
    const shopping::Item &item = list_.items[index];
    StaticJsonDocument<256> message;
    message["type"] = "set_periodic";
    message["id"] = item.id;
    message["needed"] = !item.needed;
    char output[256];
    const size_t length = serializeJson(message, output, sizeof(output));
    if (!length || !socket_.sendTXT(output, length)) {
        Serial.println("Failed to send periodic item update");
        setStatus("Update send failed", false);
        fetchPending_ = true;
        lastFetch_ = millis() - retry_ms;
        return false;
    }
    std::strcpy(pendingId_, item.id);
    pendingValue_ = !item.needed;
    pendingAt_ = millis();
    pending_ = true;
    setStatus("Waiting for server", true);
    return true;
}

void ShoppingClient::setStatus(const char *status, bool ready) {
    if (std::strcmp(status_, status) == 0 && ready_ == ready) return;
    status_ = status;
    ready_ = ready;
    notify();
}

void ShoppingClient::notify() {
    if (onChange_) onChange_();
}
