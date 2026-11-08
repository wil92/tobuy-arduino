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

## Display

While the device is in Wi-Fi setup mode, the display shows the setup hotspot
name, its password, and the setup page address. Shopping-list rendering and
touch events are not implemented yet.

`platformio.ini` configures the board's ILI9341V display and enables its
active-high backlight on GPIO21. The display shares the ESP32 reset line.
For a different board, change the TFT_eSPI build flags to match its controller,
resolution, interface, GPIO wiring, and backlight before uploading. See
[TFT_eSPI's setup selection file](https://github.com/Bodmer/TFT_eSPI/blob/master/User_Setup_Select.h).
Rebuild after changing the setup. The displayed colors and orientation also
depend on the display's configuration.

The setup screen requires TFT_eSPI font 2 (`LOAD_FONT2`). On startup, serial
output reports the display dimensions and any missing-font or zero-width text
errors. A drawing-commands-sent message does not verify the display hardware;
if the screen stays blank, check the board model and display wiring.

## Wi-Fi setup

1. Copy `include/setup_ap_config.example.h` to `include/setup_ap_config.h`
   and set `SETUP_AP_PASSWORD` to a unique 8-63 byte password. This ignored
   file is compiled into the firmware and must not be committed. The hotspot
   will not start if the password is missing or invalid.
2. Build and upload the firmware. When no valid Wi-Fi settings are saved, the
   device starts the `ToBuy-Setup` hotspot and displays its password and
   address. The password and address are also printed to the serial monitor at
   115200 baud.
3. Join that hotspot and open **http://192.168.1.1/**. Select a scanned
   network or enter an SSID manually, enter its password (leave it empty for an
   open network), and select **Configure device**. The device saves the settings
   and restarts. SSIDs may contain up to 32 bytes; passwords must be empty,
   8-63 bytes, or a 64-digit hexadecimal key.

The saved network settings are stored in ESP32 nonvolatile storage and persist
across reboots. They are not encrypted by this firmware; anyone with access to
the device's flash may be able to read them. The setup page uses unencrypted
HTTP on the password-protected hotspot, so configure it only in a trusted
environment. The hotspot password is also printed on serial as requested.

On subsequent boots the device connects in station mode without blocking the
display and enables the Wi-Fi library's automatic reconnection. Connection
status and the IP address are logged to serial. Invalid saved settings also
start the setup portal so they can be replaced. To change or clear settings,
connect a momentary button between GPIO27 and GND and hold it for three
seconds; the internal pull-up is enabled. GPIO27 is assigned to the optional
SPI peripheral chip select on the documented board, but is unused by this
firmware. Choose an input-capable pin with a pull-up and change
`RESET_CONFIG_PIN` in the `platformio.ini` build flags if needed.
The device restarts into setup mode after a successful reset. If a network
password is incorrect or the network is unavailable, use this button to
reconfigure it. Do not connect a button until the board's wiring is confirmed.

The previous optional `include/wifi_credentials.h` is no longer used for
network configuration. The ESP32 Arduino framework supplies WiFi, WebServer,
and Preferences; no additional library dependencies are required. The firmware
does not yet connect to a shopping-list server.

Firmware components live under `lib/`: `network` manages Wi-Fi and scanning,
`flash` stores settings, `portal` serves configuration requests, `display`
renders the setup screen, and `components` contains the reset button.
`src/main.cpp` coordinates their startup and update loops.

## Configuration frontend

Edit `frontend/setup.html`, then run `python3 build_views.py` to generate
`lib/portal/views/setup_page.h`. PlatformIO also regenerates it before a build.
Run `python3 build_views.py --check` to verify that the generated header matches
the source; do not edit the header directly. The generator uses Python 3's
standard library only.

## Build, test, and upload

Run these commands from the project root:

```sh
# Build the firmware
pio run -e esp32dev

# Run the host-side tests (requires a C++ compiler)
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/portal_stubs -Ilib/display test/host/display_manager_test.cpp lib/display/display_manager.cpp -o /tmp/tobuy-display-test
/tmp/tobuy-display-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/stubs -Ilib/network test/host/wifi_connection_test.cpp -o /tmp/tobuy-wifi-connection-test
/tmp/tobuy-wifi-connection-test
g++ -std=c++11 -Wall -Wextra -Werror -Ilib/flash test/host/wifi_settings_test.cpp -o /tmp/tobuy-wifi-settings-test
/tmp/tobuy-wifi-settings-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/portal_stubs -Ilib/flash test/host/flash_manager_test.cpp lib/flash/flash_manager.cpp -o /tmp/tobuy-flash-test
/tmp/tobuy-flash-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/portal_stubs -Ilib/components test/host/reset_button_test.cpp -o /tmp/tobuy-reset-test
/tmp/tobuy-reset-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/portal_stubs -Ilib/portal -Ilib/network -Ilib/flash test/host/setup_portal_test.cpp lib/portal/setup_portal.cpp lib/network/network_manager.cpp lib/flash/flash_manager.cpp -o /tmp/tobuy-setup-portal-test
/tmp/tobuy-setup-portal-test
g++ -std=c++11 -Wall -Wextra -Werror '-DSETUP_AP_PASSWORD="test-password"' -Itest/host/portal_stubs -Ilib/portal -Ilib/network -Ilib/flash -Ilib/display -Ilib/components test/host/main_integration_test.cpp src/main.cpp lib/portal/setup_portal.cpp lib/network/network_manager.cpp lib/flash/flash_manager.cpp lib/display/display_manager.cpp -o /tmp/tobuy-main-test
/tmp/tobuy-main-test missing && /tmp/tobuy-main-test saved && /tmp/tobuy-main-test invalid
python3 -m unittest discover -s test/host -p 'test_*.py'
# Exercise the frontend behavior (requires Node.js)
node test/host/frontend_test.js

# Upload the firmware to the connected board
pio run -e esp32dev -t upload
```

The host tests check setup-screen rendering, Wi-Fi settings, portal endpoints, and
the generated page with simulated hardware. They cannot verify the physical
display, button wiring, or Wi-Fi connectivity. If PlatformIO cannot detect the board's serial port
automatically, specify it with `--upload-port`, for example:

```sh
pio run -e esp32dev -t upload --upload-port /dev/ttyUSB0
```
