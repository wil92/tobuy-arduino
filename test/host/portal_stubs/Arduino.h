#pragma once

#include <cstddef>
#include <cstring>
#include <string>

#define PROGMEM
#define LOW 0
#define HIGH 1
#define INPUT_PULLUP 2

class String : public std::string {
public:
    using std::string::string;
    String() = default;
    String(const std::string &value) : std::string(value) {}
    using std::string::operator+=;
};

struct HardwareSerial {
    std::string output;
    void begin(unsigned long) {}
    void print(const char *text) { output += text; }
    void print(int value) { output += std::to_string(value); }
    void println(const char *text) { output += std::string(text) + "\n"; }
    void println(int value) { output += std::to_string(value) + "\n"; }
};

struct FakeEsp {
    bool restarted = false;
    void restart() { restarted = true; }
};

extern HardwareSerial Serial;
extern FakeEsp ESP;
extern unsigned long fakeMillis;
extern int fakePinValue;
extern int configuredPin;
extern int configuredMode;
inline unsigned long millis() { return fakeMillis; }
inline void pinMode(int pin, int mode) { configuredPin = pin; configuredMode = mode; }
inline int digitalRead(int) { return fakePinValue; }
