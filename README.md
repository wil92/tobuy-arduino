# To Buy Arduino

An Arduino firmware project for a real-time shopping list display on a home
refrigerator. The planned device will connect over Wi-Fi to a server, receive
and display streamed images of the shopping list, and send touch events back to
the server. The server will then send updated images for the device to display.

## Requirements

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
- An ESP32 development board and a USB connection for uploading firmware

The PlatformIO environment is `esp32dev` and uses the Arduino framework.
PlatformIO installs the configured platform and library dependencies when needed.

## Build, test, and upload

Run these commands from the project root:

```sh
# Build the firmware
pio run -e esp32dev

# Build, upload, and run tests on the connected board
pio test -e esp32dev

# Upload the firmware to the connected board
pio run -e esp32dev -t upload
```

There are currently no project tests. If PlatformIO cannot detect the board's
serial port automatically, specify it with `--upload-port`, for example:

```sh
pio run -e esp32dev -t upload --upload-port /dev/ttyUSB0
```
