#include <cassert>
#include <cstring>
#include <string>

#include "HTTPClient.h"
#include "temperature_client.h"

FakeSerial Serial;
FakeWiFi WiFi;
unsigned long fakeMillis = 0;
int FakeHttp::code = 200;
std::string FakeHttp::body;
std::string FakeHttp::url;

int main() {
    temperature::Reading reading;
    const std::string valid = R"({"city":"London","country":"United Kingdom","temperature":12.3,"unit":"°C"})";
    assert(temperature::parse(valid.data(), valid.size(), reading));
    assert(reading.celsius > 12.29f && reading.celsius < 12.31f);
    assert(std::strcmp(reading.city, "London") == 0 && std::strcmp(reading.country, "United Kingdom") == 0);
    const std::string noCountry = R"({"city":"Oslo","country":null,"temperature":-2,"unit":"°C"})";
    assert(temperature::parse(noCountry.data(), noCountry.size(), reading));
    assert(reading.celsius == -2 && reading.country[0] == '\0');
    const std::string invalid[] = {
        "{}", R"({"city":"x","temperature":true,"unit":"°C"})",
        R"({"city":"x","temperature":"12","unit":"°C"})",
        R"({"city":"x","temperature":12,"unit":"F"})",
        R"({"city":"","temperature":12,"unit":"°C"})",
        R"({"city":"x","temperature":1000,"unit":"°C"})",
        R"({"city":"x\u0000y","temperature":12,"unit":"°C"})",
        std::string(temperature::max_body_length + 1, 'x')
    };
    for (const auto &body : invalid) {
        assert(!temperature::parse(body.data(), body.size(), reading));
        assert(reading.celsius == -2);
    }
    assert(temperature::background(-0.1f) != temperature::background(0));
    assert(temperature::background(14.9f) != temperature::background(15));
    assert(temperature::background(24.9f) != temperature::background(25));
    assert(temperature::background(29.9f) != temperature::background(30));

    TemperatureClient client;
    client.begin("http://server:3000");
    assert(!client.update(false));
    assert(client.update(true));
    assert(std::strcmp(client.status(), "Wi-Fi unavailable") == 0 && !client.hasReading());
    WiFi.currentStatus = WL_CONNECTED;
    assert(!client.update(false));
    assert(client.update(true));
    FakeHttp::body = valid;
    runFakeTask(0);
    assert(client.update(true));
    assert(client.hasReading() && std::strcmp(client.status(), "") == 0);
    assert(FakeHttp::url == "http://server:3000/temperature");
    WiFi.currentStatus = WL_DISCONNECTED;
    assert(client.update(true));
    assert(client.hasReading() && std::strcmp(client.status(), "Wi-Fi unavailable") == 0);
    WiFi.currentStatus = WL_CONNECTED;
    assert(client.update(true));
    runFakeTask(0);
    assert(client.update(true) && std::strcmp(client.status(), "") == 0);
    fakeMillis += 599999;
    assert(!client.update(true));
    ++fakeMillis;
    assert(client.update(true));
    FakeHttp::body = std::string(temperature::max_body_length + 1, 'x');
    runFakeTask(0);
    assert(client.update(true));
    assert(std::strcmp(client.status(), "Invalid temperature") == 0 && client.hasReading());
    fakeMillis += 30000;
    assert(client.update(true));
    FakeHttp::code = 404;
    runFakeTask(0);
    assert(client.update(true));
    assert(std::strcmp(client.status(), "City not found") == 0);
    TemperatureClient secure;
    secure.begin("https://server");
    assert(std::strcmp(secure.status(), "HTTP server required") == 0 && !secure.update(true));
}
