#pragma once
#include "FreeRTOS.h"
#include <cstring>
#include <stdexcept>
#include <vector>

struct FakeQueue {
    size_t size;
    std::vector<uint8_t> data;
};
using QueueHandle_t = FakeQueue *;
inline QueueHandle_t xQueueCreate(unsigned, size_t size) { return new FakeQueue{size, {}}; }
inline void vQueueDelete(QueueHandle_t queue) { delete queue; }
inline int xQueueSend(QueueHandle_t queue, const void *value, unsigned long) {
    if (!queue->data.empty()) return 0;
    const auto *bytes = static_cast<const uint8_t *>(value);
    queue->data.assign(bytes, bytes + queue->size);
    return pdTRUE;
}
inline int xQueueReceive(QueueHandle_t queue, void *value, unsigned long timeout) {
    if (queue->data.empty()) {
        if (timeout == portMAX_DELAY) throw std::runtime_error("worker waiting");
        return 0;
    }
    std::memcpy(value, queue->data.data(), queue->size);
    queue->data.clear();
    return pdTRUE;
}
