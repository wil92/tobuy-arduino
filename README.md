# To Buy Arduino

ESP32 firmware for a real-time shopping list display. The device fetches the
list over HTTP, receives updates over WebSocket, and lets users change periodic
items on the touchscreen.

The `esp32dev` PlatformIO environment uses Arduino and targets the
[LCDWiki 2.8-inch ESP32-32E display](https://www.lcdwiki.com/2.8inch_ESP32-32E_Display)
(E32R28T / E32N28T). The touchscreen features require the E32R28T.

## Documentation

- [Device setup and operation](doc/device-setup.md): Wi-Fi, display, touch
  calibration, and troubleshooting.
- [Server protocol](doc/server-protocol.md): HTTP and WebSocket messages and
  synchronization requirements.
- [Development](doc/development.md): project layout, frontend generation,
  builds, tests, and firmware uploads.
- [Coding rules](doc/coding-rules.md): project conventions for contributors.
