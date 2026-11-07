#include <cassert>
#include <cstring>
#include <string>

#include "wifi_settings.h"

int main() {
    wifi_settings::Credentials credentials = {};
    assert(!wifi_settings::valid(credentials));
    assert(!wifi_settings::assign(credentials, nullptr, ""));
    assert(!wifi_settings::assign(credentials, "", ""));
    assert(wifi_settings::assign(credentials, "network", ""));
    assert(wifi_settings::valid(credentials));
    assert(!wifi_settings::assign(credentials, "network", "short"));
    assert(std::strcmp(credentials.ssid, "network") == 0);
    assert(wifi_settings::assign(credentials, std::string(32, 's').c_str(), "12345678"));
    assert(!wifi_settings::assign(credentials, std::string(33, 's').c_str(), "12345678"));
    assert(wifi_settings::assign(credentials, "network", std::string(63, 'p').c_str()));
    assert(wifi_settings::assign(credentials, "network", std::string(64, 'a').c_str()));
    assert(!wifi_settings::assign(credentials, "network", std::string(64, 'g').c_str()));
    assert(!wifi_settings::assign(credentials, "network", std::string(65, 'a').c_str()));
    std::memset(credentials.ssid, 'x', sizeof(credentials.ssid));
    assert(!wifi_settings::valid(credentials));
    credentials = {};
    std::memcpy(credentials.ssid, "valid", 5);
    std::memset(credentials.password, 'x', sizeof(credentials.password));
    assert(!wifi_settings::valid(credentials));

}
