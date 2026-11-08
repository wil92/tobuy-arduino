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
int FakeHttp::deleteCode = 200;
std::string FakeHttp::deleteBody;
std::string FakeHttp::method;

int main() {
    temperature::Reading reading;
    const std::string valid = R"({"city":"London","country":"United Kingdom","time":"2026-11-08T23:59","temperature":12.3,"unit":"°C"})";
    assert(temperature::parse(valid.data(), valid.size(), reading));
    assert(reading.celsius > 12.29f && reading.celsius < 12.31f);
    assert(std::strcmp(reading.city, "London") == 0 && std::strcmp(reading.country, "United Kingdom") == 0);
    assert(reading.minuteOfDay == 1439);
    const std::string noCountry = R"({"city":"Oslo","country":null,"time":"2024-02-29T00:01","temperature":-2,"unit":"°C"})";
    assert(temperature::parse(noCountry.data(), noCountry.size(), reading));
    assert(reading.celsius == -2 && reading.country[0] == '\0');
    const std::string invalid[] = {
        "{}", R"({"city":"x","time":"2026-11-08T12:00","temperature":true,"unit":"°C"})",
        R"({"city":"x","time":"2026-11-08T12:00","temperature":"12","unit":"°C"})",
        R"({"city":"x","time":"2026-11-08T12:00","temperature":12,"unit":"F"})",
        R"({"city":"","time":"2026-11-08T12:00","temperature":12,"unit":"°C"})",
        R"({"city":"x","time":"2026-11-08T12:00","temperature":1000,"unit":"°C"})",
        R"({"city":"x\u0000y","time":"2026-11-08T12:00","temperature":12,"unit":"°C"})",
        R"({"city":"x","temperature":12,"unit":"°C"})",
        R"({"city":"x","temperature":12,"unit":"°C","time":null})",
        R"({"city":"x","temperature":12,"unit":"°C","time":"2026-02-29T12:00"})",
        R"({"city":"x","temperature":12,"unit":"°C","time":"1900-02-29T12:00"})",
        R"({"city":"x","temperature":12,"unit":"°C","time":"2026-13-01T12:00"})",
        R"({"city":"x","temperature":12,"unit":"°C","time":"2026-11-08T24:00"})",
        R"({"city":"x","temperature":12,"unit":"°C","time":"2026-11-08T12:60"})",
        R"({"city":"x","temperature":12,"unit":"°C","time":"2026-11-08T12:00Z"})",
        R"({"city":"x","temperature":12,"unit":"°C","time":"2026-11-08T12:00\u0000"})",
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
    char time[6];
    client.formatTime(time);
    assert(std::strcmp(time, "23:59") == 0);
    fakeMillis += 59999;
    assert(!client.update(true));
    ++fakeMillis;
    assert(client.update(true));
    client.formatTime(time);
    assert(std::strcmp(time, "00:00") == 0);
    fakeMillis += 120000;
    assert(!client.update(false));
    client.formatTime(time);
    assert(std::strcmp(time, "00:02") == 0);
    WiFi.currentStatus = WL_DISCONNECTED;
    assert(client.update(true));
    assert(client.hasReading() && std::strcmp(client.status(), "Wi-Fi unavailable") == 0);
    fakeMillis += 60000;
    assert(client.update(true));
    client.formatTime(time);
    assert(std::strcmp(time, "00:03") == 0);
    WiFi.currentStatus = WL_CONNECTED;
    assert(client.update(true));
    runFakeTask(0);
    assert(client.update(true) && std::strcmp(client.status(), "") == 0);
    client.formatTime(time);
    assert(std::strcmp(time, "23:59") == 0);
    fakeMillis += 599999;
    assert(client.update(true));
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

    TemperatureClient wrap;
    wrap.begin("http://server:3000");
    fakeMillis = 0xfffffff0UL;
    FakeHttp::code = 200;
    FakeHttp::body = valid;
    assert(wrap.update(true));
    runFakeTask(1);
    assert(wrap.update(true));
    fakeMillis += 60000;
    assert(wrap.update(true));
    wrap.formatTime(time);
    assert(std::strcmp(time, "00:00") == 0);
}
