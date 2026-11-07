#include <cassert>
#include <climits>

#include "ResetButton.h"

unsigned long fakeMillis = 0;
int fakePinValue = HIGH;
int configuredPin = -1;
int configuredMode = -1;

int main() {
    ResetButton button(27);
    button.begin();
    assert(configuredPin == 27 && configuredMode == INPUT_PULLUP);
    assert(!button.update());
    fakePinValue = LOW;
    assert(!button.update());
    fakeMillis = 2999;
    assert(!button.update());
    fakeMillis = 3000;
    assert(button.update());
    assert(!button.update());
    fakePinValue = HIGH;
    assert(!button.update());
    fakePinValue = LOW;
    fakeMillis = ULONG_MAX - 1000;
    assert(!button.update());
    fakeMillis = 1999;
    assert(button.update());
    fakePinValue = HIGH;
    assert(!button.update());
    fakePinValue = LOW;
    assert(!button.update());
    fakeMillis += 1000;
    fakePinValue = HIGH;
    assert(!button.update());
    fakePinValue = LOW;
    assert(!button.update());
}
