#pragma once

#include <cstddef>
#include <cstring>
#include <vector>

class Preferences {
public:
    Preferences() { instance = this; }
    static Preferences *instance;
    bool beginOk = true;
    bool fail = false;
    bool readFails = false;
    bool removeFails = false;
    std::vector<char> saved;
    std::vector<char> server;
    std::vector<char> &slot(const char *key) { return std::strcmp(key, "server") == 0 ? server : saved; }
    bool begin(const char *, bool) { return beginOk; }
    bool isKey(const char *key) { return !slot(key).empty(); }
    size_t getBytesLength(const char *key) { return slot(key).size(); }
    size_t getBytes(const char *key, void *output, size_t length) {
        std::vector<char> &data = slot(key);
        if (readFails || length != data.size()) return 0;
        std::memcpy(output, data.data(), length);
        return length;
    }
    bool remove(const char *key) {
        if (removeFails) return false;
        slot(key).clear();
        return true;
    }
    size_t putBytes(const char *key, const void *value, size_t length) {
        if (fail) return 0;
        const char *data = static_cast<const char *>(value);
        slot(key).assign(data, data + length);
        return length;
    }
};
