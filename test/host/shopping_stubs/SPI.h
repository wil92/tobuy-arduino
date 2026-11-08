#pragma once
#define HSPI 2
class SPIClass {
public:
    explicit SPIClass(int) {}
    void begin(int, int, int, int) {}
};
