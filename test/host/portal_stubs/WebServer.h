#pragma once

#include "Arduino.h"
#include <functional>
#include <map>
#include <utility>

enum HTTPMethod { HTTP_GET, HTTP_POST };

class WebServer {
public:
    explicit WebServer(int) { instance = this; }
    static WebServer *instance;
    std::map<std::pair<std::string, HTTPMethod>, std::function<void()>> routes;
    std::map<std::string, String> args;
    std::function<void()> notFound;
    int code = 0;
    std::string contentType;
    std::string body;
    bool started = false;

    void on(const char *path, HTTPMethod method, std::function<void()> handler) {
        routes[{path, method}] = handler;
    }
    void onNotFound(std::function<void()> handler) { notFound = handler; }
    void begin() { started = true; }
    void handleClient() {}
    void sendHeader(const char *, const char *) {}
    void send(int status, const char *type, const String &value) {
        code = status; contentType = type; body = value;
    }
    void send_P(int status, const char *type, const char *value) { send(status, type, value); }
    bool hasArg(const char *name) { return args.count(name) != 0; }
    String arg(const char *name) { return args.at(name); }
    void request(const char *path, HTTPMethod method) {
        const auto route = routes.find({path, method});
        if (route == routes.end()) notFound();
        else route->second();
    }
};
