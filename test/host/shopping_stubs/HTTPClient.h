#pragma once
#include <string>
#include "Arduino.h"
#include "WiFi.h"
constexpr int HTTP_CODE_OK = 200;
struct FakeHttp {
    static int code;
    static std::string body;
    static std::string url;
    static int deleteCode;
    static std::string deleteBody;
    static std::string method;
};
class HTTPClient {
public:
    void setConnectTimeout(int) {}
    void setTimeout(int) {}
    bool begin(WiFiClient &, const String &url) { FakeHttp::url = url; return true; }
    int GET() { FakeHttp::method = "GET"; return FakeHttp::code; }
    int sendRequest(const char *method) { FakeHttp::method = method; return FakeHttp::deleteCode; }
    int writeToStream(Stream *stream) {
        const std::string &body = FakeHttp::method == "DELETE" ? FakeHttp::deleteBody : FakeHttp::body;
        const auto *data = reinterpret_cast<const uint8_t *>(body.data());
        return stream->write(data, body.size()) == body.size() ? static_cast<int>(body.size()) : -1;
    }
    void end() {}
};
