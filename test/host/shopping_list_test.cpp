#include <cassert>
#include <cstring>
#include <string>

#include "shopping_list.h"

int main() {
    shopping::List list;
    const std::string valid = R"({"type":"list","revision":42,"periodic":[{"id":"milk","name":"Milk","needed":true}],"missing":[{"id":"bread","name":"Bread"}]})";
    assert(shopping::parseList(valid.data(), valid.size(), list));
    assert(list.revision == 42 && list.count == 2);
    assert(list.items[0].periodic && list.items[0].needed);
    assert(std::strcmp(list.items[0].id, "milk") == 0);
    assert(!list.items[1].periodic && std::strcmp(list.items[1].name, "Bread") == 0);

    const char *invalid[] = {
        "not json",
        R"({"type":"other","revision":1,"periodic":[],"missing":[]})",
        R"({"type":"list","revision":-1,"periodic":[],"missing":[]})",
        R"({"type":"list","revision":1,"periodic":null,"missing":[]})",
        R"({"type":"list","revision":1,"periodic":[{"id":"a","name":"A","needed":1}],"missing":[]})",
        R"({"type":"list","revision":1,"periodic":[{"id":"a","name":"A","needed":true}],"missing":[{"id":"a","name":"B"}]})",
        R"({"type":"list","revision":1,"periodic":[],"missing":[{"id":"a\u0000b","name":"A"}]})",
        R"({"type":"list","revision":1,"periodic":[],"missing":[{"id":"","name":"A"}]})"
    };
    for (const char *json : invalid) {
        assert(!shopping::parseList(json, std::strlen(json), list));
        assert(list.revision == 42 && list.count == 2);
    }
    std::string oversized = R"({"type":"list","revision":2,"periodic":[],"missing":[)";
    for (size_t i = 0; i <= shopping::max_items; ++i) {
        if (i) oversized += ',';
        oversized += R"({"id":")" + std::to_string(i) + R"(","name":"Item"})";
    }
    oversized += "]}";
    assert(!shopping::parseList(oversized.data(), oversized.size(), list));
    const size_t lastItem = oversized.rfind(",{\"id\":");
    const std::string maximum = oversized.substr(0, lastItem) + "]}";
    assert(shopping::parseList(maximum.data(), maximum.size(), list));
    assert(list.count == shopping::max_items);
    std::string longName = R"({"type":"list","revision":2,"periodic":[],"missing":[{"id":"a","name":")" +
                           std::string(shopping::max_name_length + 1, 'a') + R"("}]})";
    assert(!shopping::parseList(longName.data(), longName.size(), list));
    const char *empty = R"({"type":"list","revision":0,"periodic":[],"missing":[]})";
    assert(shopping::parseList(empty, std::strlen(empty), list));
    assert(list.count == 0 && list.revision == 0);
    assert(!shopping::parseList(nullptr, 1, list));
    assert(!shopping::parseList(empty, shopping::max_message_length + 1, list));
}
