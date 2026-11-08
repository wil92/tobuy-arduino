# Device setup and operation

## Requirements

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
- An ESP32 development board and a USB connection for uploading firmware

The PlatformIO environment is `esp32dev` and uses the Arduino framework.
It is configured for the [LCDWiki 2.8-inch ESP32-32E display](https://www.lcdwiki.com/2.8inch_ESP32-32E_Display)
(E32R28T / E32N28T). PlatformIO installs the configured platform and library
dependencies when needed. See [development](development.md) for build and upload
commands.

## Display

While the device is in Wi-Fi setup mode, the display shows the setup hotspot
name, its password, and the setup page address. In normal mode, the shopping
list has a **Comprar** heading. It displays periodic items with SI/NO switches
and non-periodic missing items with red X buttons. SI means the periodic item is
needed. Controls appear to the left of the item names. Tap an X to mark a
missing item completed in Google Keep; this
does not permanently delete it. Periodic items cannot be removed this way.
The display waits for the server's updated list rather than changing it locally.
Swipe vertically or drag the right-hand scrollbar to scroll. Long
item names wrap over multiple lines. The top-right status indicator shows
connection and update messages in Spanish; an animated indicator denotes
connection attempts or a pending update. The last list may remain visible but controls are disabled
while disconnected or another update is pending. The footer shows the visible
item range and total.

Swipe horizontally in either direction to switch between the shopping list
and the current temperature in Celsius, with the server's city and country.
The background is blue below 0 °C, cyan from 0 to below
15 °C, green from 15 to below 25 °C, orange from 25 to below 30 °C, and red
at 30 °C and above. Temperatures are shown to one decimal place; colors use
the unrounded reading. The last reading remains visible with an error message
if an update fails. Configure `WEATHER_CITY` on the server as described in
its [temperature documentation](https://github.com/wil92/tobuy-server/blob/master/doc/temperature.md).

On the first normal-mode startup, tap each cross to calibrate the resistive
touchscreen. Calibration is saved separately from Wi-Fi settings. To recalibrate,
hold the "To buy" heading for three seconds and tap the three crosses again.
The configuration targets the E32R28T's XPT2046 on a separate SPI bus:
SCK GPIO25, MISO GPIO39, MOSI GPIO32 and CS GPIO33. These pins are configured
with `TOBUY_TOUCH_*` in `platformio.ini`; an E32N28T has no touchscreen.
Physical calibration and touch accuracy depend on the device.

`platformio.ini` configures the board's ILI9341V display and enables its
active-high backlight on GPIO21. The display shares the ESP32 reset line.
For a different board, change the TFT_eSPI build flags to match its controller,
resolution, interface, GPIO wiring, and backlight before uploading. See
[TFT_eSPI's setup selection file](https://github.com/Bodmer/TFT_eSPI/blob/master/User_Setup_Select.h).
Rebuild after changing the setup. The displayed colors also depend on the
display's configuration.

To flip the screen top/down (180 degrees), copy `include/display_setup.example.h`
to `include/display_setup.h` and set `DISPLAY_FLIP_VERTICAL` to `1`. This
optional, ignored file defaults to `0`, the original orientation. The touch
coordinates are flipped with the display, and an existing calibration stays
valid.

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

The setup portal also asks for the tobuy server URL, such as
`http://192.168.2.3:3000` or `http://tobuy.net` (host or IP, optional port;
no path). It is stored in flash under the `server` key, next to the Wi-Fi
settings, and is cleared by the reset button. The portal also accepts
`https` URLs, but the shopping client currently supports **HTTP/WS only** and
displays an error for HTTPS rather than connecting without certificate
verification. Existing installations without a server URL must reset and
complete setup again. See the [server protocol](server-protocol.md) for
server requirements and security limitations.

The previous optional `include/wifi_credentials.h` is no longer used for
network configuration.
