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
    bool begin(const char *, bool) { return beginOk; }
    bool isKey(const char *) { return !saved.empty(); }
    size_t getBytesLength(const char *) { return saved.size(); }
    size_t getBytes(const char *, void *output, size_t length) {
        if (readFails || length != saved.size()) return 0;
        std::memcpy(output, saved.data(), length);
        return length;
    }
    bool remove(const char *) {
        if (removeFails) return false;
        saved.clear();
        return true;
    }
    size_t putBytes(const char *, const void *value, size_t length) {
        if (fail) return 0;
        const char *data = static_cast<const char *>(value);
        saved.assign(data, data + length);
        return length;
    }
};
