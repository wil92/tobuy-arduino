#include "shopping_list.h"

#include <cstring>
#include <memory>
#include <new>

namespace shopping {
namespace {
bool copyField(JsonVariantConst value, char *destination, size_t maximum) {
    if (!value.is<const char *>()) return false;
    JsonString text = value.as<JsonString>();
    if (text.isNull() || text.size() == 0 || text.size() > maximum ||
        std::strlen(text.c_str()) != text.size()) return false;
    std::memcpy(destination, text.c_str(), text.size() + 1);
    return true;
}
}

bool parseList(const char *json, size_t length, List &output) {
    if (!json || length == 0 || length > max_message_length) return false;
    DynamicJsonDocument document(24576);
    if (deserializeJson(document, json, length) || document.overflowed()) return false;
    JsonObjectConst root = document.as<JsonObjectConst>();
    if (root.isNull() || root["type"] != "list" || !root["revision"].is<uint32_t>() ||
        !root["periodic"].is<JsonArrayConst>() || !root["missing"].is<JsonArrayConst>()) return false;
    const JsonArrayConst periodic = root["periodic"].as<JsonArrayConst>();
    const JsonArrayConst missing = root["missing"].as<JsonArrayConst>();
    if (periodic.size() + missing.size() > max_items) return false;
    std::unique_ptr<List> candidate(new (std::nothrow) List());
    if (!candidate) return false;
    candidate->revision = root["revision"].as<uint32_t>();
    for (unsigned group = 0; group < 2; ++group) {
        JsonArrayConst items = group == 0 ? periodic : missing;
        for (JsonVariantConst value : items) {
            JsonObjectConst object = value.as<JsonObjectConst>();
            if (object.isNull()) return false;
            Item &item = candidate->items[candidate->count];
            if (!copyField(object["id"], item.id, max_id_length) ||
                !copyField(object["name"], item.name, max_name_length)) return false;
            item.periodic = group == 0;
            if (item.periodic) {
                if (!object["needed"].is<bool>()) return false;
                item.needed = object["needed"].as<bool>();
            }
            for (size_t i = 0; i < candidate->count; ++i) {
                if (std::strcmp(candidate->items[i].id, item.id) == 0) return false;
            }
            ++candidate->count;
        }
    }
    output = *candidate;
    return true;
}
}
