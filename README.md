# To Buy Arduino

An Arduino firmware project for a real-time shopping list display on a home
refrigerator. The planned device will connect over Wi-Fi to a server, receive
and display streamed images of the shopping list, and send touch events back to
the server. The server will then send updated images for the device to display.

## Requirements

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
- An ESP32 development board and a USB connection for uploading firmware

The PlatformIO environment is `esp32dev` and uses the Arduino framework.
It is configured for the [LCDWiki 2.8-inch ESP32-32E display](https://www.lcdwiki.com/2.8inch_ESP32-32E_Display)
(E32R28T / E32N28T). PlatformIO installs the configured platform and library
dependencies when needed.

## Sample display image

The firmware currently displays a built-in pixel-art landscape at startup. It
scales the image to fit the display, with black borders where needed, and uses
a small RGB565 buffer rather than a full-screen framebuffer. This is a local
example only: Wi-Fi, image streaming, and touch events are not implemented yet.

`platformio.ini` configures the board's ILI9341V display and enables its
active-high backlight on GPIO21. The display shares the ESP32 reset line.
For a different board, change the TFT_eSPI build flags to match its controller,
resolution, interface, GPIO wiring, and backlight before uploading. See
[TFT_eSPI's setup selection file](https://github.com/Bodmer/TFT_eSPI/blob/master/User_Setup_Select.h).
Rebuild after changing the setup. The displayed colors and orientation also
depend on the display's configuration.

## Build, test, and upload

Run these commands from the project root:

```sh
# Build the firmware
pio run -e esp32dev

# Run the host-side sample image tests (requires a C++ compiler)
g++ -std=c++11 -Wall -Wextra -Werror -Iinclude test/host/sample_image_test.cpp -o /tmp/tobuy-sample-image-test
/tmp/tobuy-sample-image-test

# Upload the firmware to the connected board
pio run -e esp32dev -t upload
```

The host test checks image colors, scaling, centering, chunking, and invalid
display dimensions using a simulated display. It cannot verify the physical
display or its wiring. If PlatformIO cannot detect the board's serial port
automatically, specify it with `--upload-port`, for example:

```sh
pio run -e esp32dev -t upload --upload-port /dev/ttyUSB0
```
