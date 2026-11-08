#include <cassert>
#include <cstring>
#include <string>

#include "server_settings.h"

int main() {
    server_settings::ServerUrl server = {};
    assert(!server_settings::valid(server));
    assert(!server_settings::assign(server, nullptr));
    for (const char *good : {"http://192.168.2.3:3000", "http://tobuy.net", "https://tobuy.net:443/", "http://a"}) {
        assert(server_settings::assign(server, good));
        assert(server_settings::valid(server));
    }
    assert(std::strcmp(server.url, "http://a") == 0);
    assert(server_settings::assign(server, "http://tobuy.net/"));
    assert(std::strcmp(server.url, "http://tobuy.net") == 0);
    for (const char *bad : {"", "tobuy.net", "ftp://x", "http://", "http:///", "http://:80", "http://host:",
                            "http://host:0", "http://host:65536", "http://host:123456", "http://host/path",
                            "http://ho st", "http://-host", "http://host.", "http://host:80a"}) {
        assert(!server_settings::assign(server, bad));
    }
    assert(std::strcmp(server.url, "http://tobuy.net") == 0);
    assert(!server_settings::assign(server, ("http://" + std::string(121, 'a')).c_str()));
    assert(server_settings::assign(server, ("http://" + std::string(120, 'a')).c_str()));
    std::memset(server.url, 'h', sizeof(server.url));
    assert(!server_settings::valid(server));
}
