#pragma once
#include <string>
#include "Arduino.h"
#include "WiFi.h"
constexpr int HTTP_CODE_OK = 200;
struct FakeHttp {
    static int code;
    static std::string body;
    static std::string url;
};
class HTTPClient {
public:
    void setConnectTimeout(int) {}
    void setTimeout(int) {}
    bool begin(WiFiClient &, const String &url) { FakeHttp::url = url; return true; }
    int GET() { return FakeHttp::code; }
    int writeToStream(Stream *stream) {
        const auto *data = reinterpret_cast<const uint8_t *>(FakeHttp::body.data());
        return stream->write(data, FakeHttp::body.size()) == FakeHttp::body.size() ?
               static_cast<int>(FakeHttp::body.size()) : -1;
    }
    void end() {}
};
