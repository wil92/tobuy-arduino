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
    explicit BoundedStream(char *buffer) : buffer_(buffer) {}
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
    char *buffer_;
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
    RemovalResult removal;
    if (removalResults_ && xQueueReceive(removalResults_, &removal, 0) == pdTRUE) {
        finishRemoval(removal);
    }
    if (WiFi.status() != WL_CONNECTED) {
        if (started_) {
            ++connectionGeneration_;
            socket_.disconnect();
        }
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
        ++connectionGeneration_;
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
    std::unique_ptr<char[]> buffer(new (std::nothrow) char[shopping::max_message_length]);
    if (!buffer) {
        http.end();
        Serial.println("Insufficient memory for shopping list response");
        setStatus("Out of memory", false);
        return;
    }
    BoundedStream body(buffer.get());
    const int received = http.writeToStream(&body);
    http.end();
    if (received < 0 || !acceptList(body.data(), body.size())) {
        Serial.println("Invalid shopping list HTTP response");
        setStatus("Invalid server response", false);
        return;
    }
    fetchPending_ = false;
    setStatus(pending_ || removalPending_ ? "Waiting for server" : "Connected", true);
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
        const char *status = pending_ || removalPending_ ? "Waiting for server" : "Connected";
        const bool changed = std::strcmp(status_, status) != 0 || !ready_;
        setStatus(status, true);
        if (!changed) notify();
    } else {
        notify();
    }
    return true;
}

bool ShoppingClient::toggle(size_t index) {
    if (!ready_ || !connected_ || pending_ || removalPending_ ||
        index >= list_.count || !list_.items[index].periodic) return false;
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

bool ShoppingClient::startRemovalWorker() {
    if (removalRequests_) return true;
    removalRequests_ = xQueueCreate(1, sizeof(RemovalRequest));
    removalResults_ = xQueueCreate(1, sizeof(RemovalResult));
    if (!removalRequests_ || !removalResults_ ||
        xTaskCreate(removalWorker, "remove-item", 8192, this, 1, nullptr) != pdPASS) {
        Serial.println("Could not start missing-item removal task");
        if (removalRequests_) vQueueDelete(removalRequests_);
        if (removalResults_) vQueueDelete(removalResults_);
        removalRequests_ = removalResults_ = nullptr;
        return false;
    }
    return true;
}

bool ShoppingClient::removeMissing(size_t index) {
    if (!ready_ || !connected_ || pending_ || removalPending_ ||
        index >= list_.count || list_.items[index].periodic) return false;
    if (!startRemovalWorker()) {
        setStatus("Update send failed", false);
        fetchPending_ = true;
        lastFetch_ = millis() - retry_ms;
        return false;
    }
    RemovalRequest request = {};
    std::strcpy(request.id, list_.items[index].id);
    request.generation = connectionGeneration_;
    if (xQueueSend(removalRequests_, &request, 0) != pdTRUE) {
        Serial.println("Could not queue missing-item removal");
        setStatus("Update send failed", false);
        fetchPending_ = true;
        lastFetch_ = millis() - retry_ms;
        return false;
    }
    std::strcpy(removalId_, request.id);
    removalPending_ = true;
    setStatus("Waiting for server", true);
    return true;
}

void ShoppingClient::removalWorker(void *context) {
    auto *client = static_cast<ShoppingClient *>(context);
    RemovalRequest request;
    for (;;) {
        if (xQueueReceive(client->removalRequests_, &request, portMAX_DELAY) == pdTRUE) {
            RemovalResult result = client->sendRemoval(request);
            if (xQueueSend(client->removalResults_, &result, portMAX_DELAY) != pdTRUE) {
                Serial.println("Could not deliver missing-item removal result");
                delete[] result.body;
            }
        }
    }
}

ShoppingClient::RemovalResult ShoppingClient::sendRemoval(const RemovalRequest &request) {
    RemovalResult result = {};
    result.generation = request.generation;
    char encoded[shopping::max_id_length * 3 + 1];
    static const char hex[] = "0123456789ABCDEF";
    size_t offset = 0;
    for (const unsigned char *p = reinterpret_cast<const unsigned char *>(request.id); *p; ++p) {
        const unsigned char c = *p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '~') {
            encoded[offset++] = static_cast<char>(c);
        } else {
            encoded[offset++] = '%';
            encoded[offset++] = hex[c >> 4];
            encoded[offset++] = hex[c & 15];
        }
    }
    encoded[offset] = '\0';
    WiFiClient connection;
    HTTPClient http;
    http.setConnectTimeout(1500);
    http.setTimeout(15000);
    if (!http.begin(connection, serverUrl_ + "/tobuy/missing/" + encoded)) {
        Serial.println("Could not start missing-item removal");
        return result;
    }
    result.code = http.sendRequest("DELETE");
    if (result.code == HTTP_CODE_OK) {
        result.body = new (std::nothrow) char[shopping::max_message_length];
        if (result.body) {
            BoundedStream body(result.body);
            const int received = http.writeToStream(&body);
            if (received >= 0) result.length = body.size();
        } else {
            Serial.println("Insufficient memory for removal response");
        }
    }
    http.end();
    return result;
}

void ShoppingClient::finishRemoval(const RemovalResult &result) {
    std::unique_ptr<char[]> body(result.body);
    if (result.generation != connectionGeneration_ || !connected_ || WiFi.status() != WL_CONNECTED) {
        removalPending_ = false;
        Serial.println("Discarding removal response after disconnect");
        fetchPending_ = true;
        lastFetch_ = millis() - retry_ms;
        return;
    }
    if (result.code != HTTP_CODE_OK || !result.body || !result.length ||
        !acceptList(result.body, result.length)) {
        removalPending_ = false;
        Serial.printf("Missing-item removal failed: HTTP %d\n", result.code);
        setStatus(result.code == HTTP_CODE_OK ? "Invalid server response" : "Update send failed", false);
        fetchPending_ = true;
        lastFetch_ = millis();
        return;
    }
    for (size_t i = 0; i < list_.count; ++i) {
        if (std::strcmp(list_.items[i].id, removalId_) == 0) {
            removalPending_ = false;
            Serial.println("Missing-item removal not confirmed");
            setStatus("Update not confirmed", false);
            fetchPending_ = true;
            lastFetch_ = millis();
            return;
        }
    }
    removalPending_ = false;
    if (!fetchPending_) setStatus("Connected", true);
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
