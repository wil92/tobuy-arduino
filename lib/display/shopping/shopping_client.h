#pragma once

#include <Arduino.h>
#include <WebSocketsClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#include "shopping_list.h"

class ShoppingClient {
public:
    typedef void (*ChangeHandler)();
    void begin(const char *serverUrl, ChangeHandler onChange);
    void update();
    bool toggle(size_t index);
    bool removeMissing(size_t index);
    const shopping::List &list() const { return list_; }
    const char *status() const { return status_; }
    bool ready() const { return ready_; }

private:
    struct RemovalRequest {
        char id[shopping::max_id_length + 1];
        uint32_t generation;
    };
    struct RemovalResult {
        int code;
        char *body;
        size_t length;
        uint32_t generation;
    };
    static void removalWorker(void *context);
    RemovalResult sendRemoval(const RemovalRequest &request);
    void finishRemoval(const RemovalResult &result);
    bool startRemovalWorker();
    void onEvent(WStype_t type, uint8_t *payload, size_t length);
    void fetchList();
    bool acceptList(const char *data, size_t length);
    void setStatus(const char *status, bool ready);
    void notify();

    WebSocketsClient socket_;
    shopping::List list_;
    ChangeHandler onChange_ = nullptr;
    String serverUrl_;
    String host_;
    uint16_t port_ = 80;
    const char *status_ = "Connecting to Wi-Fi";
    bool configured_ = false;
    bool started_ = false;
    bool connected_ = false;
    bool ready_ = false;
    bool hasList_ = false;
    bool fetchPending_ = false;
    unsigned long connectStartedAt_ = 0;
    unsigned long lastFetch_ = 0;
    bool pending_ = false;
    bool removalPending_ = false;
    QueueHandle_t removalRequests_ = nullptr;
    QueueHandle_t removalResults_ = nullptr;
    uint32_t connectionGeneration_ = 0;
    char removalId_[shopping::max_id_length + 1] = {};
    char pendingId_[shopping::max_id_length + 1] = {};
    bool pendingValue_ = false;
    unsigned long pendingAt_ = 0;
};
