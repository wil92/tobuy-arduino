#pragma once

#include <ArduinoJson.h>
#include <cstddef>
#include <cstdint>

namespace shopping {
constexpr size_t max_items = 64;
constexpr size_t max_id_length = 32;
constexpr size_t max_name_length = 64;
constexpr size_t max_message_length = 8192;

struct Item {
    char id[max_id_length + 1];
    char name[max_name_length + 1];
    bool periodic;
    bool needed;
};

struct List {
    uint32_t revision = 0;
    size_t count = 0;
    Item items[max_items] = {};
};

bool parseList(const char *json, size_t length, List &output);
}
