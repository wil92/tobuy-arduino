#pragma once

#include <cstddef>
#include <cstring>

namespace wifi_settings {
constexpr size_t max_ssid_length = 32;
constexpr size_t max_password_length = 64;

struct Credentials {
    char ssid[max_ssid_length + 1];
    char password[max_password_length + 1];
};

inline size_t boundedLength(const char *text, size_t capacity) {
    size_t length = 0;
    while (length < capacity && text[length] != '\0') ++length;
    return length;
}

inline bool valid(const Credentials &credentials) {
    const size_t ssidLength = boundedLength(credentials.ssid, sizeof(credentials.ssid));
    const size_t passwordLength = boundedLength(credentials.password, sizeof(credentials.password));
    if (ssidLength == 0 || ssidLength > max_ssid_length ||
        passwordLength > max_password_length) return false;
    if (passwordLength == 0) return true;
    if (passwordLength < 8) return false;
    if (passwordLength == 64) {
        for (size_t i = 0; i < passwordLength; ++i) {
            const char c = credentials.password[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F'))) return false;
        }
    }
    return true;
}

inline bool assign(Credentials &credentials, const char *ssid, const char *password) {
    if (!ssid || !password) return false;
    const size_t ssidLength = boundedLength(ssid, sizeof(credentials.ssid));
    const size_t passwordLength = boundedLength(password, sizeof(credentials.password));
    if (ssidLength > max_ssid_length || passwordLength > max_password_length) return false;
    Credentials candidate = {};
    std::memcpy(candidate.ssid, ssid, ssidLength);
    std::memcpy(candidate.password, password, passwordLength);
    if (!valid(candidate)) return false;
    credentials = candidate;
    return true;
}

}
