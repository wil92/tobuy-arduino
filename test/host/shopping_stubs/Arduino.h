#pragma once
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>

class String : public std::string {
public:
    using std::string::string;
    String() = default;
    String(const std::string &value) : std::string(value) {}
    String substring(size_t start) const { return substr(start); }
    String substring(size_t start, size_t end) const { return substr(start, end - start); }
    int indexOf(char c) const { size_t pos = find(c); return pos == npos ? -1 : static_cast<int>(pos); }
    long toInt() const { return std::stol(*this); }
    bool isEmpty() const { return empty(); }
};
class Stream {
public:
    virtual ~Stream() = default;
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t *, size_t) = 0;
    virtual int available() = 0;
    virtual int read() = 0;
    virtual int peek() = 0;
    virtual void flush() = 0;
};
struct FakeSerial {
    void println(const char *) {}
    template <typename... Args> void printf(const char *, Args...) {}
};
extern FakeSerial Serial;
extern unsigned long fakeMillis;
inline unsigned long millis() { return fakeMillis; }
