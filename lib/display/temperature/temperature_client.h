#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

namespace temperature {
constexpr size_t max_body_length = 1024;
constexpr size_t max_place_length = 96;

struct Reading {
    float celsius = 0;
    uint16_t minuteOfDay = 0;
    char city[max_place_length + 1] = {};
    char country[max_place_length + 1] = {};
};

bool parse(const char *data, size_t length, Reading &reading);
uint16_t background(float celsius);
}

class TemperatureClient {
public:
    void begin(const char *serverUrl);
    bool update(bool visible);
    bool hasReading() const { return hasReading_; }
    const temperature::Reading &reading() const { return reading_; }
    void formatTime(char (&out)[6]) const;
    const char *status() const { return status_; }

private:
    struct Result {
        temperature::Reading reading;
        const char *error;
    };
    static void worker(void *context);
    Result fetch();

    String url_;
    QueueHandle_t requests_ = nullptr;
    QueueHandle_t results_ = nullptr;
    temperature::Reading reading_;
    const char *status_ = "Loading temperature";
    bool ready_ = false;
    bool pending_ = false;
    bool hasReading_ = false;
    bool attempted_ = false;
    unsigned long lastAttempt_ = 0;
    unsigned long interval_ = 0;
    uint32_t clockTick_ = 0;
};
