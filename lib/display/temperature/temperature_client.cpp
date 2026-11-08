#include "temperature_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace {
constexpr unsigned long refresh_ms = 10UL * 60 * 1000;
constexpr unsigned long retry_ms = 30UL * 1000;

class BoundedBody : public Stream {
public:
    size_t write(uint8_t value) override {
        if (length == sizeof(data)) return 0;
        data[length++] = static_cast<char>(value);
        return 1;
    }
    size_t write(const uint8_t *bytes, size_t size) override {
        if (size > sizeof(data) - length) return 0;
        std::memcpy(data + length, bytes, size);
        length += size;
        return size;
    }
    int available() override { return 0; }
    int read() override { return -1; }
    int peek() override { return -1; }
    void flush() override {}
    char data[temperature::max_body_length] = {};
    size_t length = 0;
};

bool copyPlace(JsonVariantConst value, char *out, bool required) {
    if (value.isNull() && !required) {
        out[0] = '\0';
        return true;
    }
    if (!value.is<const char *>()) return false;
    JsonString text = value.as<JsonString>();
    if ((required && text.size() == 0) || text.size() > temperature::max_place_length ||
        std::strlen(text.c_str()) != text.size()) return false;
    std::memcpy(out, text.c_str(), text.size() + 1);
    return true;
}

bool parseTime(JsonVariantConst value, uint16_t &minuteOfDay) {
    if (!value.is<const char *>()) return false;
    JsonString text = value.as<JsonString>();
    const char *s = text.c_str();
    if (text.size() != 16 || std::strlen(s) != 16 ||
        s[4] != '-' || s[7] != '-' || s[10] != 'T' || s[13] != ':') return false;
    for (size_t i = 0; i < 16; ++i) {
        if (i == 4 || i == 7 || i == 10 || i == 13) continue;
        if (s[i] < '0' || s[i] > '9') return false;
    }
    const unsigned year = (s[0] - '0') * 1000 + (s[1] - '0') * 100 + (s[2] - '0') * 10 + s[3] - '0';
    const unsigned month = (s[5] - '0') * 10 + s[6] - '0';
    const unsigned day = (s[8] - '0') * 10 + s[9] - '0';
    const unsigned hour = (s[11] - '0') * 10 + s[12] - '0';
    const unsigned minute = (s[14] - '0') * 10 + s[15] - '0';
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (!year || month < 1 || month > 12 || hour > 23 || minute > 59) return false;
    const bool leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    if (!day || day > days[month - 1] + (month == 2 && leap ? 1U : 0U)) return false;
    minuteOfDay = static_cast<uint16_t>(hour * 60 + minute);
    return true;
}
}

bool temperature::parse(const char *data, size_t length, Reading &reading) {
    if (!data || !length || length > max_body_length) return false;
    StaticJsonDocument<2048> doc;
    if (deserializeJson(doc, data, length) || doc.overflowed()) return false;
    JsonObjectConst root = doc.as<JsonObjectConst>();
    if (root.isNull() || !root["temperature"].is<float>() || root["unit"] != "\xc2\xb0" "C") return false;
    Reading candidate;
    candidate.celsius = root["temperature"].as<float>();
    if (!std::isfinite(candidate.celsius) || candidate.celsius < -999.0f || candidate.celsius > 999.0f ||
        !parseTime(root["time"], candidate.minuteOfDay) ||
        !copyPlace(root["city"], candidate.city, true) ||
        !copyPlace(root["country"], candidate.country, false)) return false;
    reading = candidate;
    return true;
}

uint16_t temperature::background(float celsius) {
    if (celsius < 0) return 0x11b2;
    if (celsius < 15) return 0x0472;
    if (celsius < 25) return 0x3448;
    if (celsius < 30) return 0xa420;
    return 0x9904;
}

void TemperatureClient::begin(const char *serverUrl) {
    if (!serverUrl || std::strncmp(serverUrl, "http://", 7) != 0) {
        Serial.println("Temperature requires an HTTP server URL");
        status_ = "HTTP server required";
        return;
    }
    url_ = String(serverUrl) + "/temperature";
    requests_ = xQueueCreate(1, sizeof(uint8_t));
    results_ = xQueueCreate(1, sizeof(Result));
    if (!requests_ || !results_) {
        Serial.println("Could not allocate temperature queues");
        if (requests_) vQueueDelete(requests_);
        if (results_) vQueueDelete(results_);
        requests_ = results_ = nullptr;
        status_ = "Temperature unavailable";
        return;
    }
    if (xTaskCreate(worker, "temperature", 8192, this, 1, nullptr) != pdPASS) {
        Serial.println("Could not start temperature task");
        vQueueDelete(requests_);
        vQueueDelete(results_);
        requests_ = results_ = nullptr;
        status_ = "Temperature unavailable";
        return;
    }
    ready_ = true;
}

void TemperatureClient::formatTime(char (&out)[6]) const {
    const unsigned minute = reading_.minuteOfDay % 1440U;
    std::snprintf(out, sizeof(out), "%02u:%02u", minute / 60, minute % 60);
}

bool TemperatureClient::update(bool visible) {
    bool changed = false;
    const uint32_t now = static_cast<uint32_t>(millis());
    if (hasReading_) {
        const uint32_t elapsedMinutes = (now - clockTick_) / 60000U;
        if (elapsedMinutes) {
            reading_.minuteOfDay = static_cast<uint16_t>((reading_.minuteOfDay + elapsedMinutes) % 1440U);
            clockTick_ += elapsedMinutes * 60000U;
            changed = visible;
        }
    }
    Result result;
    if (ready_ && xQueueReceive(results_, &result, 0) == pdTRUE) {
        pending_ = false;
        if (result.error) {
            status_ = result.error;
            interval_ = retry_ms;
        } else {
            reading_ = result.reading;
            hasReading_ = true;
            clockTick_ = now;
            status_ = "";
            interval_ = refresh_ms;
        }
        lastAttempt_ = millis();
        attempted_ = true;
        changed = true;
    }
    if (ready_ && visible && WiFi.status() != WL_CONNECTED) {
        if (std::strcmp(status_, "Wi-Fi unavailable") != 0) {
            status_ = "Wi-Fi unavailable";
            changed = true;
        }
        attempted_ = false;
        return changed;
    }
    if (ready_ && visible && !pending_ &&
        (!attempted_ || static_cast<unsigned long>(millis() - lastAttempt_) >= interval_)) {
        uint8_t request = 1;
        if (xQueueSend(requests_, &request, 0) == pdTRUE) {
            pending_ = true;
            status_ = "Loading temperature";
            changed = true;
        } else {
            Serial.println("Could not queue temperature request");
            status_ = "Temperature unavailable";
            attempted_ = true;
            interval_ = retry_ms;
            lastAttempt_ = millis();
            changed = true;
        }
    }
    return changed;
}

void TemperatureClient::worker(void *context) {
    auto *client = static_cast<TemperatureClient *>(context);
    uint8_t request;
    for (;;) {
        if (xQueueReceive(client->requests_, &request, portMAX_DELAY) == pdTRUE) {
            const Result result = client->fetch();
            if (xQueueSend(client->results_, &result, portMAX_DELAY) != pdTRUE)
                Serial.println("Could not deliver temperature result");
        }
    }
}

TemperatureClient::Result TemperatureClient::fetch() {
    Result result = {};
    if (WiFi.status() != WL_CONNECTED) {
        result.error = "Wi-Fi unavailable";
        return result;
    }
    WiFiClient connection;
    HTTPClient http;
    http.setConnectTimeout(1500);
    http.setTimeout(12000);
    if (!http.begin(connection, url_)) {
        Serial.println("Could not start temperature request");
        result.error = "Temperature unavailable";
        return result;
    }
    const int code = http.GET();
    if (code != HTTP_CODE_OK) {
        Serial.printf("Temperature HTTP error: %d\n", code);
        http.end();
        result.error = code == 503 ? "City not configured" :
                       code == 404 ? "City not found" : "Temperature unavailable";
        return result;
    }
    BoundedBody body;
    const int received = http.writeToStream(&body);
    http.end();
    if (received < 0 || !temperature::parse(body.data, body.length, result.reading)) {
        Serial.println("Invalid temperature response");
        result.error = "Invalid temperature";
    }
    return result;
}
