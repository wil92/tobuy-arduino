#pragma once
#include <cstring>
#include <vector>
class Preferences {
public:
    static std::vector<char> saved;
    bool begin(const char *, bool) { return true; }
    size_t getBytesLength(const char *) { return saved.size(); }
    size_t getBytes(const char *, void *output, size_t size) {
        if (size != saved.size()) return 0;
        std::memcpy(output, saved.data(), size);
        return size;
    }
    size_t putBytes(const char *, const void *value, size_t size) {
        const char *start = static_cast<const char *>(value);
        saved.assign(start, start + size);
        return size;
    }
};
