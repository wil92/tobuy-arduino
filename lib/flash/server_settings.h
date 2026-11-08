#pragma once

#include <cstddef>
#include <cstring>

namespace server_settings {
constexpr size_t max_url_length = 127;

struct ServerUrl {
    char url[max_url_length + 1];
};

inline bool isHostChar(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           c == '.' || c == '-';
}

// Accepts http(s)://host[:port] with an optional trailing slash.
inline bool validText(const char *text, size_t length) {
    size_t pos = 0;
    if (length > 7 && std::strncmp(text, "http://", 7) == 0) pos = 7;
    else if (length > 8 && std::strncmp(text, "https://", 8) == 0) pos = 8;
    else return false;
    if (text[length - 1] == '/') --length;
    const size_t hostStart = pos;
    while (pos < length && isHostChar(text[pos])) ++pos;
    if (pos == hostStart || text[hostStart] == '.' || text[hostStart] == '-' ||
        text[pos - 1] == '.' || text[pos - 1] == '-') return false;
    if (pos == length) return true;
    if (text[pos] != ':' || pos + 1 == length || length - pos - 1 > 5) return false;
    unsigned long port = 0;
    for (++pos; pos < length; ++pos) {
        if (text[pos] < '0' || text[pos] > '9') return false;
        port = port * 10 + static_cast<unsigned long>(text[pos] - '0');
    }
    return port >= 1 && port <= 65535;
}

inline bool valid(const ServerUrl &server) {
    const size_t length = std::strlen(server.url) <= max_url_length ? std::strlen(server.url) : 0;
    return length > 0 && validText(server.url, length);
}

inline bool assign(ServerUrl &server, const char *url) {
    if (!url) return false;
    size_t length = 0;
    while (length <= max_url_length && url[length] != '\0') ++length;
    if (length > max_url_length) return false;
    if (!validText(url, length)) return false;
    if (url[length - 1] == '/') --length;
    ServerUrl candidate = {};
    std::memcpy(candidate.url, url, length);
    server = candidate;
    return true;
}
}
