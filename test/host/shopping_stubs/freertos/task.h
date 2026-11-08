#pragma once
#include "FreeRTOS.h"
#include <stdexcept>
#include <vector>
struct FakeTask { void (*function)(void *); void *context; };
inline std::vector<FakeTask> &fakeTasks() { static std::vector<FakeTask> tasks; return tasks; }
inline int xTaskCreate(void (*function)(void *), const char *, unsigned, void *context, int, void *) {
    fakeTasks().push_back({function, context});
    return pdPASS;
}
inline void runFakeTask(size_t index) {
    try { fakeTasks().at(index).function(fakeTasks().at(index).context); }
    catch (const std::runtime_error &) {}
}
