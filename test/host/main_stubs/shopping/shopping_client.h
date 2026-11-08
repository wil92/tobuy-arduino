#pragma once
#include <string>
class ShoppingClient {
public:
    std::string url;
    int updates = 0;
    void begin(const char *server, void (*)()) { url = server; }
    void update() { ++updates; }
};
