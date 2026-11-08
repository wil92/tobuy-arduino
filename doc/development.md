# Development

The ESP32 Arduino framework supplies WiFi, WebServer, HTTPClient and
Preferences. PlatformIO installs the display, touchscreen, JSON and WebSocket
libraries declared in `platformio.ini`.

Firmware components live under `lib/`: `network` manages Wi-Fi and scanning,
`flash` stores settings, `portal` serves configuration requests, `display`
renders the setup screen and contains `display/shopping` for the list and touchscreen
and `display/temperature` for the temperature client, and
`components` contains the reset button. `src/main.cpp` coordinates their startup
and update loops. See the [coding rules](coding-rules.md) for project conventions.

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
g++ -std=c++11 -Wall -Wextra -Werror -Ilib/flash test/host/server_settings_test.cpp -o /tmp/tobuy-server-settings-test
/tmp/tobuy-server-settings-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/portal_stubs -Ilib/flash test/host/flash_manager_test.cpp lib/flash/flash_manager.cpp -o /tmp/tobuy-flash-test
/tmp/tobuy-flash-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/portal_stubs -Ilib/components test/host/reset_button_test.cpp -o /tmp/tobuy-reset-test
/tmp/tobuy-reset-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/portal_stubs -Ilib/portal -Ilib/network -Ilib/flash test/host/setup_portal_test.cpp lib/portal/setup_portal.cpp lib/network/network_manager.cpp lib/flash/flash_manager.cpp -o /tmp/tobuy-setup-portal-test
/tmp/tobuy-setup-portal-test
g++ -std=c++11 -Wall -Wextra -Werror '-DSETUP_AP_PASSWORD="test-password"' -Itest/host/main_stubs -Itest/host/portal_stubs -Ilib/portal -Ilib/network -Ilib/flash -Ilib/display -Ilib/components test/host/main_integration_test.cpp src/main.cpp lib/portal/setup_portal.cpp lib/network/network_manager.cpp lib/flash/flash_manager.cpp lib/display/display_manager.cpp -o /tmp/tobuy-main-test
/tmp/tobuy-main-test missing && /tmp/tobuy-main-test saved && /tmp/tobuy-main-test invalid && /tmp/tobuy-main-test configured
g++ -std=c++11 -Wall -Wextra -Werror -I.pio/libdeps/esp32dev/ArduinoJson/src -Ilib/display/shopping test/host/shopping_list_test.cpp lib/display/shopping/shopping_list.cpp -o /tmp/tobuy-shopping-list-test
/tmp/tobuy-shopping-list-test
g++ -std=c++11 -Wall -Wextra -Werror -DTOBUY_TOUCH_SCK=25 -DTOBUY_TOUCH_MISO=39 -DTOBUY_TOUCH_MOSI=32 -DTOBUY_TOUCH_CS=33 -Itest/host/shopping_stubs -I.pio/libdeps/esp32dev/ArduinoJson/src -Ilib/display/shopping test/host/shopping_integration_test.cpp lib/display/shopping/shopping_client.cpp lib/display/shopping/shopping_list.cpp lib/display/shopping/shopping_view.cpp lib/display/temperature/temperature_client.cpp -o /tmp/tobuy-shopping-test
/tmp/tobuy-shopping-test
g++ -std=c++11 -Wall -Wextra -Werror -Itest/host/shopping_stubs -I.pio/libdeps/esp32dev/ArduinoJson/src -Ilib/display/temperature test/host/temperature_test.cpp lib/display/temperature/temperature_client.cpp -o /tmp/tobuy-temperature-test
/tmp/tobuy-temperature-test
python3 -m unittest discover -s test/host -p 'test_*.py'
# Exercise the frontend behavior (requires Node.js)
node test/host/frontend_test.js

# Upload the firmware to the connected board
pio run -e esp32dev -t upload
```

The host tests check the setup, list and temperature protocols, simulated server synchronization,
touch navigation, and generated page. They cannot verify the physical
display, touchscreen, button wiring, or Wi-Fi connectivity. If PlatformIO cannot
detect the board's serial port automatically, specify it with `--upload-port`,
for example:

```sh
pio run -e esp32dev -t upload --upload-port /dev/ttyUSB0
```
